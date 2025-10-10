#include "sdl_window.hpp"

#include <iostream>
#include <stdexcept>
#include <SDL3/SDL.h>

#include "backends/imgui_impl_sdl3.h"
#include "vulkan_context.hpp"

VkExtent2D SDLWindow::WindowSize::toExtent2D() const
{
    return { width, height };
}

SDLWindow::WindowSize::WindowSize(const uint32_t p_Width, const uint32_t p_Height)
    : width(p_Width), height(p_Height)
{

}

SDLWindow::WindowSize::WindowSize(const Sint32 p_Width, const Sint32 p_Height)
    : width(static_cast<uint32_t>(p_Width)), height(static_cast<uint32_t>(p_Height))
{

}

SDLWindow::SDLWindow(const std::string_view p_Name, const int p_Width, const int p_Height, const int p_Top, const int p_Left, const uint32_t p_Flags)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    m_SDLHandle = SDL_CreateWindow(p_Name.data(), p_Width, p_Height, p_Flags | SDL_WINDOW_VULKAN);

    m_KeyPressed.connect([this](const uint32_t p_Key)
        {
            if (p_Key == SDLK_Q)
                toggleMouseCapture();
        });
}

void SDLWindow::initImgui() const
{
    ImGui_ImplSDL3_InitForVulkan(m_SDLHandle);
}

bool SDLWindow::shouldClose() const
{
    return m_ShouldClose;
}

uint32_t SDLWindow::getRequiredVulkanExtensionCount() const
{
    uint32_t l_ExtensionCount;
    if (!SDL_Vulkan_GetInstanceExtensions(&l_ExtensionCount))
        throw std::runtime_error("Failed to get required Vulkan extension count from SDL");
    return l_ExtensionCount;
}

void SDLWindow::getRequiredVulkanExtensions(const char* p_Container[]) const
{
    uint32_t l_ExtensionCount;
    const char* const* l_Exts = SDL_Vulkan_GetInstanceExtensions(&l_ExtensionCount);
    if (!l_Exts)
        throw std::runtime_error("Failed to get required Vulkan extensions from SDL");
    for (Uint32 i = 0; i < l_ExtensionCount; ++i) {
        p_Container[i] = l_Exts[i];
    }
}

SDLWindow::WindowSize SDLWindow::getSize() const
{
    Sint32 l_Width, l_Height;
    SDL_GetWindowSize(m_SDLHandle, &l_Width, &l_Height);
    return { l_Width, l_Height };
}

bool SDLWindow::isMinimized() const
{
    return m_Minimized;
}

void SDLWindow::pollEvents()
{
    SDL_Event l_Event;
    while (SDL_PollEvent(&l_Event))
    {
        ImGui_ImplSDL3_ProcessEvent(&l_Event);
        switch (l_Event.type)
        {
        case SDL_EVENT_QUIT:
            m_ShouldClose = true;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            if (l_Event.window.data1 > 0 && l_Event.window.data2 > 0) 
            {
                m_ResizeSignal.emit(WindowSize{l_Event.window.data1, l_Event.window.data2}.toExtent2D());
                m_Minimized = false;
            }
            break;
        case SDL_EVENT_WINDOW_MINIMIZED:
            m_Minimized = true;
            break;
        case SDL_EVENT_WINDOW_RESTORED:
            m_Minimized = false;
            break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                int32_t l_PxW = 0, l_PxH = 0;
                SDL_GetWindowSizeInPixels(m_SDLHandle, &l_PxW, &l_PxH);
                if (l_PxW > 0 && l_PxH > 0)
                    m_PixelResizeSignal.emit(WindowSize{l_PxW, l_PxH}.toExtent2D());
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            m_MouseMoved.emit(l_Event.motion.xrel, l_Event.motion.yrel);
            break;
        case SDL_EVENT_KEY_DOWN:
            m_KeyPressed.emit(l_Event.key.key);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            m_MouseButtonPressed.emit(l_Event.button.button);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            m_MouseButtonReleased.emit(l_Event.button.button);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            m_MouseScrolled.emit(l_Event.wheel.y);
            break;
        case SDL_EVENT_KEY_UP:
            m_KeyReleased.emit(l_Event.key.key);
            break;
        }
    }
    const uint64_t l_Now = SDL_GetTicks();
    m_Delta = (static_cast<float>(l_Now) - m_PrevDelta) * 0.001f;
    m_PrevDelta = static_cast<float>(l_Now);
    m_EventsProcessed.emit(m_Delta);
}

void SDLWindow::toggleMouseCapture()
{
    m_MouseCaptured = !m_MouseCaptured;
    if (m_MouseCaptured)
        SDL_SetWindowRelativeMouseMode(m_SDLHandle, true);
    else
        SDL_SetWindowRelativeMouseMode(m_SDLHandle, false);
    m_MouseCaptureChanged.emit(m_MouseCaptured);
}

void SDLWindow::createSurface(const VkInstance p_Instance)
{
    if (m_Surface != nullptr)
        throw std::runtime_error("Surface already created");

    if (!SDL_Vulkan_CreateSurface(m_SDLHandle, p_Instance, nullptr, &m_Surface))
        throw std::runtime_error("failed to create SDLHandle surface!");
}

SDL_Window* SDLWindow::operator*() const
{
    return m_SDLHandle;
}

VkSurfaceKHR SDLWindow::getSurface() const
{
    return m_Surface;
}

void SDLWindow::free()
{
    if (m_Surface != nullptr)
    {
        vkDestroySurfaceKHR(VulkanContext::s_VkHandle, m_Surface, nullptr);
        m_Surface = nullptr;
    }

    SDL_DestroyWindow(m_SDLHandle);
    SDL_Quit();
    m_SDLHandle = nullptr;
}

void SDLWindow::shutdownImgui() const
{
    ImGui_ImplSDL3_Shutdown();
}

void SDLWindow::frameImgui() const
{
    ImGui_ImplSDL3_NewFrame();
}

Signal<VkExtent2D>& SDLWindow::getResizedSignal()
{
    return m_ResizeSignal;
}

Signal<VkExtent2D>& SDLWindow::getPixelResizedSignal()
{
    return m_PixelResizeSignal;
}

Signal<float, float>& SDLWindow::getMouseMovedSignal()
{
    return m_MouseMoved;
}

Signal<uint32_t>& SDLWindow::getKeyPressedSignal()
{
    return m_KeyPressed;
}

Signal<uint32_t>& SDLWindow::getKeyReleasedSignal()
{
    return m_KeyReleased;
}

Signal<uint32_t>& SDLWindow::getMouseButtonPressedSignal()
{
    return m_MouseButtonPressed;
}

Signal<uint32_t>& SDLWindow::getMouseButtonReleasedSignal()
{
    return m_MouseButtonReleased;
}

Signal<float>& SDLWindow::getMouseScrolledSignal()
{
    return m_MouseScrolled;
}

Signal<float>& SDLWindow::getEventsProcessedSignal()
{
    return m_EventsProcessed;
}

Signal<bool>& SDLWindow::getMouseCaptureChangedSignal()
{
    return m_MouseCaptureChanged;
}
