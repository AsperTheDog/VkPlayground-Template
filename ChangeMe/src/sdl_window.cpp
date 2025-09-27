#include "sdl_window.hpp"

#include <iostream>
#include <stdexcept>
#include <SDL2/SDL.h>

#include "backends/imgui_impl_sdl2.h"
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
    m_SDLHandle = SDL_CreateWindow(p_Name.data(), p_Top, p_Left, p_Width, p_Height, p_Flags | SDL_WINDOW_VULKAN);

    m_KeyPressed.connect([this](const uint32_t p_Key)
        {
            if (p_Key == SDLK_q)
                toggleMouseCapture();
        });
}

void SDLWindow::initImgui() const
{
    ImGui_ImplSDL2_InitForVulkan(m_SDLHandle);
}

bool SDLWindow::shouldClose() const
{
    return SDL_QuitRequested();
}

uint32_t SDLWindow::getRequiredVulkanExtensionCount() const
{
    uint32_t extensionCount;
    SDL_Vulkan_GetInstanceExtensions(m_SDLHandle, &extensionCount, nullptr);
    return extensionCount;
}

void SDLWindow::getRequiredVulkanExtensions(const char* p_Container[]) const
{
    uint32_t extensionCount = getRequiredVulkanExtensionCount();
    SDL_Vulkan_GetInstanceExtensions(m_SDLHandle, &extensionCount, p_Container);
}

SDLWindow::WindowSize SDLWindow::getSize() const
{
    Sint32 width, height;
    SDL_GetWindowSize(m_SDLHandle, &width, &height);
    return { width, height };
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
        ImGui_ImplSDL2_ProcessEvent(&l_Event);
        switch (l_Event.type)
        {
        case SDL_WINDOWEVENT:
            if (l_Event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED && l_Event.window.data1 > 0 && l_Event.window.data2 > 0)
            {
                m_ResizeSignal.emit(WindowSize {l_Event.window.data1, l_Event.window.data2}.toExtent2D());
                m_Minimized = false;
            }
            else if (l_Event.window.event == SDL_WINDOWEVENT_MINIMIZED)
                m_Minimized = true;
            else if (l_Event.window.event == SDL_WINDOWEVENT_RESTORED && m_Minimized)
                m_Minimized = false;
            break;
        case SDL_MOUSEMOTION:
            m_MouseMoved.emit(l_Event.motion.xrel, l_Event.motion.yrel);
            break;
        case SDL_KEYDOWN:
            m_KeyPressed.emit(l_Event.key.keysym.sym);
            break;
        case SDL_MOUSEBUTTONDOWN:
            m_MouseButtonPressed.emit(l_Event.button.button);
            break;
        case SDL_MOUSEBUTTONUP:
            m_MouseButtonReleased.emit(l_Event.button.button);
            break;
        case SDL_MOUSEWHEEL:
            m_MouseScrolled.emit(l_Event.wheel.y);
            break;
        case SDL_KEYUP:
            m_KeyReleased.emit(l_Event.key.keysym.sym);
            break;
        }
    }
    const uint64_t l_Now = SDL_GetTicks64();
    m_Delta = (static_cast<float>(l_Now) - m_PrevDelta) * 0.001f;
    m_PrevDelta = static_cast<float>(l_Now);
    m_EventsProcessed.emit(m_Delta);
}

void SDLWindow::toggleMouseCapture()
{
    m_MouseCaptured = !m_MouseCaptured;
    if (m_MouseCaptured)
        SDL_SetRelativeMouseMode(SDL_TRUE);
    else
        SDL_SetRelativeMouseMode(SDL_FALSE);
    m_MouseCaptureChanged.emit(m_MouseCaptured);
}

void SDLWindow::createSurface(const VkInstance p_Instance)
{
    if (m_Surface != nullptr)
        throw std::runtime_error("Surface already created");

    if (SDL_Vulkan_CreateSurface(m_SDLHandle, p_Instance, &m_Surface) == SDL_FALSE)
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
    ImGui_ImplSDL2_Shutdown();
}

void SDLWindow::frameImgui() const
{
    ImGui_ImplSDL2_NewFrame();
}

Signal<VkExtent2D>& SDLWindow::getResizedSignal()
{
    return m_ResizeSignal;
}

Signal<int32_t, int32_t>& SDLWindow::getMouseMovedSignal()
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

Signal<int32_t>& SDLWindow::getMouseScrolledSignal()
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
