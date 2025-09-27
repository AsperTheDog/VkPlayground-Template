#include "ortho_controller_camera.hpp"

#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <SDL2/SDL_mouse.h>

OrthoControllerCamera::OrthoControllerCamera(const glm::vec3 p_Pos, const glm::vec3 p_Dir, const glm::vec3 p_Up, const glm::vec2 p_XBounds, const glm::vec2 p_YBounds, const float p_Near, const float p_Far)
    : OrthographicCamera(p_Pos, p_Dir, p_Up, p_XBounds, p_YBounds, p_Near, p_Far)
{
    recalculateUnitsPerPixel();
}

void OrthoControllerCamera::mouseMoved(const int32_t p_RelX, const int32_t p_RelY)
{
    if (!m_LeftMousePressed) return;

    const float l_Dx = static_cast<float>(p_RelX) * m_UnitsPerPixel.x;
    const float l_Dy = static_cast<float>(p_RelY) * m_UnitsPerPixel.y;

    const glm::vec3 l_Delta = (-m_Right * l_Dx + m_Up * l_Dy) * (m_PanningSpeed * 0.01f);

    move(l_Delta);
}

void OrthoControllerCamera::mouseButtonPressed(const uint32_t p_Button)
{
    if (p_Button == SDL_BUTTON_LEFT)
        m_LeftMousePressed = true;
}

void OrthoControllerCamera::mouseButtonReleased(const uint32_t p_Button)
{
    if (p_Button == SDL_BUTTON_LEFT)
        m_LeftMousePressed = false;
}

void OrthoControllerCamera::mouseScrolled(const int32_t p_Y)
{
    m_Zoom -= static_cast<float>(p_Y);
    m_Zoom = std::max(m_Zoom, 0.1f);
    recalculateUnitsPerPixel();
    setProjDirty();
}

void OrthoControllerCamera::recalculateUnitsPerPixel()
{
    const float l_InvZoom = 1.f / m_Zoom;
    glm::vec2 l_ViewSize;
    l_ViewSize.x = (m_XBounds.y - m_XBounds.x) * l_InvZoom;
    l_ViewSize.y = (m_YBounds.y - m_YBounds.x) * l_InvZoom;
    m_UnitsPerPixel.x = l_ViewSize.x / m_ScreenSize.x;
    m_UnitsPerPixel.y = l_ViewSize.y / m_ScreenSize.y;
}

void OrthoControllerCamera::recalculateProjMatrix()
{
    const float l_XCenter = 0.5f * (m_XBounds.x + m_XBounds.y);
    const float l_YCenter = 0.5f * (m_YBounds.x + m_YBounds.y);
    const float l_InvZoom = 1.f / m_Zoom;
    const float l_XHalfSize = 0.5f * (m_XBounds.y - m_XBounds.x) * l_InvZoom;
    const float l_YHalfSize = 0.5f * (m_YBounds.y - m_YBounds.x) * l_InvZoom;
    m_ProjMatrix = glm::ortho(l_XCenter - l_XHalfSize, l_XCenter + l_XHalfSize, l_YCenter - l_YHalfSize, l_YCenter + l_YHalfSize, m_Near, m_Far);
    m_ProjDirty = false;
}
