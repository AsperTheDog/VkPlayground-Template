#include "engine.hpp"

#include <array>

#include <imgui.h>
#include <iostream>
#include <thread>
#include <backends/imgui_impl_vulkan.h>

#include "vertex.hpp"
#include "vulkan_binding.hpp"
#include "vulkan_buffer.hpp"

#include "vulkan_device.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_sync.hpp"
#include "ext/vulkan_swapchain.hpp"
#include "vulkan_descriptors.hpp"
#include "camera/flight_camera.hpp"
#include "camera/ortho_controller_camera.hpp"
#include "utils/logger.hpp"

constexpr std::array<Vertex, 3> VERTICES = {
    Vertex{ {  0.0f, -0.5f, 0.0f }, { 255,   0,   0 } },
    Vertex{ {  0.5f,  0.5f, 0.0f }, {   0, 255,   0 } },
    Vertex{ { -0.5f,  0.5f, 0.0f }, {   0,   0, 255 } }
};

constexpr std::array<uint16_t, 3> INDICES = { 0, 1, 2 };

static VulkanGPU chooseCorrectGPU()
{
    std::array<VulkanGPU, 10> l_GPUs;
    VulkanContext::getGPUs(l_GPUs.data());
    for (uint32_t i = 0; i < VulkanContext::getGPUCount(); i++)
    {
        if (l_GPUs[i].getProperties().deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            return l_GPUs[i];
        }
    }

    throw std::runtime_error("No discrete GPU found");
}

Engine::Engine() : m_Window("Vulkan", 1920, 1080)
{
    // Vulkan Instance
    Logger::setRootContext("Engine init");

    std::vector<const char*> l_RequiredExtensions{ m_Window.getRequiredVulkanExtensionCount() };
    m_Window.getRequiredVulkanExtensions(l_RequiredExtensions.data());
#ifndef _DEBUG
    Logger::setLevels(Logger::WARN | Logger::ERR);
    VulkanContext::init(VK_API_VERSION_1_3, false, false, l_RequiredExtensions);
#else
    Logger::setLevels(Logger::ALL);
    VulkanContext::init(VK_API_VERSION_1_3, true, false, l_RequiredExtensions);
#endif

    VulkanContext::initializeTransientMemory(1LL * 1024);
    VulkanContext::initializeArenaMemory(1LL * 1024 * 1024);

    // Vulkan Surface
    m_Window.createSurface(VulkanContext::getHandle());

    // Choose Physical Device
    const VulkanGPU l_GPU = chooseCorrectGPU();

    // Select Queue Families
    const GPUQueueStructure l_QueueStructure = l_GPU.getQueueFamilies();
    QueueFamilySelector l_QueueFamilySelector(l_QueueStructure);

    const QueueFamily l_GraphicsQueueFamily = l_QueueStructure.findQueueFamily(VK_QUEUE_GRAPHICS_BIT);
    const QueueFamily l_PresentQueueFamily = l_QueueStructure.findPresentQueueFamily(m_Window.getSurface());
    const QueueFamily l_TransferQueueFamily = l_QueueStructure.findQueueFamily(VK_QUEUE_TRANSFER_BIT);

    // Select Queue Families and assign queues
    QueueFamilySelector l_Selector{ l_QueueStructure };
    l_Selector.selectQueueFamily(l_GraphicsQueueFamily, QueueFamilyTypeBits::GRAPHICS);
    l_Selector.selectQueueFamily(l_PresentQueueFamily, QueueFamilyTypeBits::PRESENT);
    m_GraphicsQueuePos = l_Selector.getOrAddQueue(l_GraphicsQueueFamily, 1.0);
    m_PresentQueuePos = l_Selector.getOrAddQueue(l_PresentQueueFamily, 1.0);
    m_TransferQueuePos = l_Selector.addQueue(l_TransferQueueFamily, 1.0);

    // Logical Device
    VulkanDeviceExtensionManager l_Extensions{};
    l_Extensions.addExtension(new VulkanSwapchainExtension(m_DeviceID));
    m_DeviceID = VulkanContext::createDevice(l_GPU, l_Selector, &l_Extensions, {});
    VulkanDevice& l_Device = VulkanContext::getDevice(m_DeviceID);

    // Swapchain
    VulkanSwapchainExtension* l_SwapchainExt = VulkanSwapchainExtension::get(m_DeviceID);
    m_SwapchainID = l_SwapchainExt->createSwapchain(m_Window.getSurface(), m_Window.getSize().toExtent2D(), { VK_FORMAT_R8G8B8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR }, VK_PRESENT_MODE_FIFO_KHR);
    VulkanSwapchain& l_Swapchain = l_SwapchainExt->getSwapchain(m_SwapchainID);

    // Command Buffers
    l_Device.configureOneTimeQueue(m_TransferQueuePos);
    l_Device.initializeCommandPool(l_GraphicsQueueFamily, 0, true);
    m_GraphicsCmdBufferID = l_Device.createCommandBuffer(l_GraphicsQueueFamily, 0, false);

    // Depth Buffer
    VulkanMemoryAllocator::MemoryPreferences l_MemPrefs {
        .preferredProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };
    m_DepthBuffer = l_Device.createAndAllocateImage(l_MemPrefs, {VK_IMAGE_TYPE_2D, VK_FORMAT_D32_SFLOAT, { l_Swapchain.getExtent().width, l_Swapchain.getExtent().height, 1 }, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 0});
    VulkanImage& l_DepthImage = l_Device.getImage(m_DepthBuffer);
    m_DepthBufferView = l_DepthImage.createImageView(VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
    
    l_Device.configureStagingBuffer(5LL * 1024 * 1024, m_TransferQueuePos);

    // Dump Data
    {
        ResourceID l_FenceID = l_Device.createFence(false);
        VulkanFence& l_Fence = l_Device.getFence(l_FenceID);

        const ResourceID l_OneTimeTransferCmdBufferID = l_Device.createCommandBuffer(l_TransferQueueFamily, 0, false);
        VulkanCommandBuffer& l_CmdBuffer = l_Device.getCommandBuffer(l_OneTimeTransferCmdBufferID, 0);
        
        // Vertex Buffer
        m_VertexBufferID = l_Device.createAndAllocateBuffer(l_MemPrefs, {sizeof(VERTICES), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, m_TransferQueuePos.familyIndex});
        
        l_CmdBuffer.beginRecording();
        l_CmdBuffer.ecmdDumpDataIntoBuffer(m_VertexBufferID, reinterpret_cast<const uint8_t*>(VERTICES.data()), sizeof(VERTICES));
        l_CmdBuffer.endRecording();
        l_CmdBuffer.submit(l_Device.getQueue(m_TransferQueuePos), {}, {}, l_FenceID);
        
        // Index Buffer
        m_IndexBufferID = l_Device.createAndAllocateBuffer(l_MemPrefs, {sizeof(INDICES), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, m_TransferQueuePos.familyIndex});
        
        l_Fence.wait();
        l_Fence.reset();

        l_CmdBuffer.reset();
        l_CmdBuffer.beginRecording();
        l_CmdBuffer.ecmdDumpDataIntoBuffer(m_IndexBufferID, reinterpret_cast<const uint8_t*>(INDICES.data()), sizeof(INDICES));
        l_CmdBuffer.endRecording();
        l_CmdBuffer.submit(l_Device.getQueue(m_TransferQueuePos), {}, {}, l_FenceID);

        l_Fence.wait();

        //Free resources
        l_Device.freeCommandBuffer(l_OneTimeTransferCmdBufferID, 0);
        l_Device.freeFence(l_Fence);
    }

    // Renderpass and pipelines
    createRenderPasses();
    createPipelines();

    // Framebuffers
    m_FramebufferIDs.resize(l_Swapchain.getImageCount());
    for (uint32_t i = 0; i < l_Swapchain.getImageCount(); i++)
    {
        VkImageView l_Color = *l_Swapchain.getImage(i).getImageView(l_Swapchain.getImageView(i));
        const std::array<VkImageView, 2> l_Attachments = { l_Color, *l_Device.getImage(m_DepthBuffer).getImageView(m_DepthBufferView) };
        m_FramebufferIDs[i] = VulkanContext::getDevice(m_DeviceID).createFramebuffer({ l_Swapchain.getExtent().width, l_Swapchain.getExtent().height, 1 }, m_RenderPassID, l_Attachments);
    }

    // Sync objects
    for (uint32_t i = 0; i < l_Swapchain.getImageCount(); i++)
    {
        m_RenderFinishedSemaphoreIDs.push_back(l_Device.createSemaphore());
    }
    m_InFlightFenceID = l_Device.createFence(true);

    m_Window.getPixelResizedSignal().connect(this, &Engine::recreateSwapchain);

    m_Camera.setScreenSize(l_Swapchain.getExtent().width, l_Swapchain.getExtent().height);

    initImgui();
    configureCamera();
}

Engine::~Engine()
{
    VulkanContext::getDevice(m_DeviceID).waitIdle();

    Logger::setRootContext("Resource cleanup");

    ImGui_ImplVulkan_Shutdown();
    m_Window.shutdownImgui();
    ImGui::DestroyContext();

    VulkanContext::freeDevice(m_DeviceID);
    m_Window.free();
    VulkanContext::free();
}

void Engine::run()
{
    VulkanDevice& l_Device = VulkanContext::getDevice(m_DeviceID);
    VulkanSwapchainExtension* l_SwapchainExt = VulkanSwapchainExtension::get(l_Device);

    VulkanFence& l_InFlightFence = l_Device.getFence(m_InFlightFenceID);

    const VulkanQueue l_GraphicsQueue = l_Device.getQueue(m_GraphicsQueuePos);
    VulkanCommandBuffer& l_GraphicsBuffer = l_Device.getCommandBuffer(m_GraphicsCmdBufferID, 0);

    while (!m_Window.shouldClose())
    {
        m_Window.pollEvents();
        if (m_Window.isMinimized())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        l_InFlightFence.wait();
        l_InFlightFence.reset();

        VulkanSwapchain& l_Swapchain = l_SwapchainExt->getSwapchain(m_SwapchainID);
        const uint32_t l_ImageIndex = l_Swapchain.acquireNextImage();
        if (l_ImageIndex == UINT32_MAX)
        {
            continue;
        }

        Engine::drawImgui();
        ImDrawData* l_ImguiDrawData = ImGui::GetDrawData();

        if (l_ImguiDrawData->DisplaySize.x <= 0.0f || l_ImguiDrawData->DisplaySize.y <= 0.0f)
        {
            continue;
        }

        // Recording
        {
            const VkExtent2D& l_Extent = l_SwapchainExt->getSwapchain(m_SwapchainID).getExtent();
          
            std::array<VkClearValue, 2> l_ClearValues;
            l_ClearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            l_ClearValues[1].depthStencil = { 1.0f, 0 };

            VkViewport l_Viewport;
            l_Viewport.x = 0.0f;
            l_Viewport.y = 0.0f;
            l_Viewport.width = static_cast<float>(l_Extent.width);
            l_Viewport.height = static_cast<float>(l_Extent.height);
            l_Viewport.minDepth = 0.0f;
            l_Viewport.maxDepth = 1.0f;

            VkRect2D l_Scissor;
            l_Scissor.offset = { 0, 0 };
            l_Scissor.extent = l_Extent;

            PushData l_PushData{};
            l_PushData.modelMatrix = glm::mat4(1.0f);
            l_PushData.viewProjMatrix = m_Camera.getVPMatrix();

            l_GraphicsBuffer.reset();
            l_GraphicsBuffer.beginRecording();

            l_GraphicsBuffer.cmdBeginRenderPass(m_RenderPassID, m_FramebufferIDs[l_ImageIndex], l_Extent, l_ClearValues);
            l_GraphicsBuffer.cmdBindVertexBuffer(m_VertexBufferID, 0);
            l_GraphicsBuffer.cmdBindIndexBuffer(m_IndexBufferID, 0, VK_INDEX_TYPE_UINT16);
            l_GraphicsBuffer.cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelineID);
            l_GraphicsBuffer.cmdSetViewport(l_Viewport);
            l_GraphicsBuffer.cmdSetScissor(l_Scissor);
            l_GraphicsBuffer.cmdPushConstant(m_GraphicsPipelineLayoutID, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushData), &l_PushData);
            l_GraphicsBuffer.cmdDrawIndexed(3, 0, 0);

            ImGui_ImplVulkan_RenderDrawData(l_ImguiDrawData, *l_GraphicsBuffer);

            l_GraphicsBuffer.cmdEndRenderPass();
            l_GraphicsBuffer.endRecording();
        }

        // Submit
        {
            const std::array<VulkanCommandBuffer::WaitSemaphoreData, 1> l_WaitSemaphores = {{{l_Swapchain.getImgSemaphore(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT}}};
            const std::array<ResourceID, 1> l_SignalSemaphores = {m_RenderFinishedSemaphoreIDs[l_ImageIndex]};
            l_GraphicsBuffer.submit(l_GraphicsQueue, l_WaitSemaphores, l_SignalSemaphores, m_InFlightFenceID);
        }

        // Present
        {
            std::array<ResourceID, 1> l_Semaphores = { {m_RenderFinishedSemaphoreIDs[l_ImageIndex]} };
            l_Swapchain.present(m_PresentQueuePos, l_Semaphores);
        }

        VulkanContext::resetTransMemory();
    }
}

void Engine::createRenderPasses()
{
    Logger::pushContext("Create RenderPass");
    VulkanRenderPassBuilder l_Builder{};
    
    VulkanSwapchainExtension* l_SwapchainExt = VulkanSwapchainExtension::get(m_DeviceID);
    const VkFormat l_Format = l_SwapchainExt->getSwapchain(m_SwapchainID).getFormat().format;

    const VkAttachmentDescription l_ColorAttachment = VulkanRenderPassBuilder::createAttachment(l_Format,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    l_Builder.addAttachment(l_ColorAttachment);
    const VkAttachmentDescription l_DepthAttachment = VulkanRenderPassBuilder::createAttachment(VK_FORMAT_D32_SFLOAT,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    l_Builder.addAttachment(l_DepthAttachment);

    const std::array<VulkanRenderPassBuilder::AttachmentReference, 2> l_ColorReferences = {{{COLOR_ATTACHMENT, 0}, {DEPTH_STENCIL_ATTACHMENT, 1}}};
    l_Builder.addSubpass(l_ColorReferences, 0);

    m_RenderPassID = VulkanContext::getDevice(m_DeviceID).createRenderPass(l_Builder, 0);
    Logger::popContext();
}

void Engine::createPipelines()
{
    VulkanDevice& l_Device = VulkanContext::getDevice(m_DeviceID);

    {
        std::array<VkPushConstantRange, 1> l_PushConstants{};
        l_PushConstants[0] = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushData) };
        m_GraphicsPipelineLayoutID = l_Device.createPipelineLayout({}, l_PushConstants);
    }
    
#ifndef _DEBUG
    VulkanShader l_Shader{0, false};
    l_Shader.enableCache("shaders/cache/shader_release.bin");
#else
    VulkanShader l_Shader{0, true};
    l_Shader.enableCache("shaders/cache/shader_debug.bin");
#endif

    l_Shader.setExpectedStages(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    l_Shader.addModule("shaders/shader.slang", "main");
    l_Shader.compile();

	const ResourceID l_VertexShader = l_Device.createShaderModule(l_Shader, VK_SHADER_STAGE_VERTEX_BIT);
	const ResourceID l_FragmentShader = l_Device.createShaderModule(l_Shader, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipelineColorBlendAttachmentState l_ColorBlendAttachment{};
	l_ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	l_ColorBlendAttachment.blendEnable = VK_FALSE;

    VulkanBinding l_Binding{0, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(Vertex)};
	l_Binding.addAttribDescription(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position));
	l_Binding.addAttribDescription(VK_FORMAT_R8G8B8_UNORM, offsetof(Vertex, color));

    std::array<VkDynamicState, 2> l_DynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VulkanPipelineBuilder l_Builder{ m_DeviceID };
    l_Builder.addVertexBinding(l_Binding, true);
	l_Builder.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
	l_Builder.setViewportState(1, 1);
	l_Builder.setRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	l_Builder.setMultisampleState(VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f);
	l_Builder.setDepthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
	l_Builder.addColorBlendAttachment(l_ColorBlendAttachment);
	l_Builder.setColorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, {0.0f, 0.0f, 0.0f, 0.0f});
	l_Builder.setDynamicState(l_DynamicStates);
    l_Builder.addShaderStage(l_VertexShader);
    l_Builder.addShaderStage(l_FragmentShader);
	m_GraphicsPipelineID = l_Device.createPipeline(l_Builder, m_GraphicsPipelineLayoutID, m_RenderPassID, 0);

    l_Device.freeShaderModule(l_VertexShader);
    l_Device.freeShaderModule(l_FragmentShader);
}

void Engine::recreateSwapchain(const VkExtent2D p_NewSize)
{
    Logger::pushContext("Recreate Swapchain");
    VulkanDevice& l_Device = VulkanContext::getDevice(m_DeviceID);
    l_Device.waitIdle();

    VulkanSwapchainExtension* l_SwapchainExtension = VulkanSwapchainExtension::get(l_Device);

    m_SwapchainID = l_SwapchainExtension->createSwapchain(m_Window.getSurface(), p_NewSize, l_SwapchainExtension->getSwapchain(m_SwapchainID).getFormat(), VK_PRESENT_MODE_FIFO_KHR, m_SwapchainID);

    VulkanSwapchain& l_Swapchain = l_SwapchainExtension->getSwapchain(m_SwapchainID);

    for (uint32_t i = 0; i < l_Swapchain.getImageCount(); ++i)
    {
        const VkImageView l_Color = *l_Swapchain.getImage(i).getImageView(l_Swapchain.getImageView(i));
        const std::array<VkImageView, 2> l_Attachments = { l_Color, *l_Device.getImage(m_DepthBuffer).getImageView(m_DepthBufferView) };
        m_FramebufferIDs[i] = VulkanContext::getDevice(m_DeviceID).createFramebuffer({ l_Swapchain.getExtent().width, l_Swapchain.getExtent().height, 1 }, m_RenderPassID, l_Attachments);
    }
    Logger::popContext();
}

void Engine::configureCamera()
{
    Camera* l_Camera = &m_Camera;
    m_Window.getKeyPressedSignal().connect(l_Camera, &Camera::keyPressed);
    m_Window.getKeyReleasedSignal().connect(l_Camera, &Camera::keyReleased);
    m_Window.getMouseMovedSignal().connect(l_Camera, &Camera::mouseMoved);
    m_Window.getMouseButtonPressedSignal().connect(l_Camera, &Camera::mouseButtonPressed);
    m_Window.getMouseButtonReleasedSignal().connect(l_Camera, &Camera::mouseButtonReleased);
    m_Window.getMouseScrolledSignal().connect(l_Camera, &Camera::mouseScrolled);
    m_Window.getEventsProcessedSignal().connect(l_Camera, &Camera::updateEvents);

    if constexpr (std::is_same_v<decltype(m_Camera), FlightCamera>)
    {
        m_Window.toggleMouseCapture();
    }
}

void Engine::initImgui() const
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    const ImGuiIO& l_IO = ImGui::GetIO(); (void)l_IO;

    ImGui::StyleColorsDark();

    VulkanDevice& l_Device = VulkanContext::getDevice(m_DeviceID);

    const std::array<VkDescriptorPoolSize, 11> l_PoolSizes =
    {{
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    }};
    const uint32_t l_ImguiPoolID = l_Device.createDescriptorPool(l_PoolSizes, 1000U * static_cast<uint32_t>(l_PoolSizes.size()), VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    VulkanSwapchainExtension* l_SwapchainExt = VulkanSwapchainExtension::get(l_Device);
    const VulkanSwapchain& l_Swapchain = l_SwapchainExt->getSwapchain(m_SwapchainID);

    m_Window.initImgui();
    ImGui_ImplVulkan_InitInfo l_InitInfo = {};
    l_InitInfo .Instance = VulkanContext::getHandle();
    l_InitInfo .PhysicalDevice = *l_Device.getGPU();
    l_InitInfo .Device = *l_Device;
    l_InitInfo .QueueFamily = m_GraphicsQueuePos.familyIndex;
    l_InitInfo .Queue = *l_Device.getQueue(m_GraphicsQueuePos);
    l_InitInfo .DescriptorPool = *l_Device.getDescriptorPool(l_ImguiPoolID);
    l_InitInfo .RenderPass = *l_Device.getRenderPass(m_RenderPassID);
    l_InitInfo .Subpass = 0;
    l_InitInfo .MinImageCount = l_Swapchain.getMinImageCount();
    l_InitInfo .ImageCount = l_Swapchain.getImageCount();
    l_InitInfo .MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&l_InitInfo );
}

void Engine::drawImgui() const
{
    ImGui_ImplVulkan_NewFrame();
    m_Window.frameImgui();
    ImGui::NewFrame();

    // ImGui here
    {
        ImGui::ShowDemoWindow();
    }

    ImGui::Render();
}
