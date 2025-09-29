#include "arcball_camera.hpp"

#include <algorithm>
#include <SDL2/SDL_mouse.h>

ArcballCamera::ArcballCamera(const glm::vec3 p_Target, const float p_DistanceToTarget, const float p_Fov, const float p_Near, const float p_Far)
    : PerspectiveCamera(glm::vec3{}, glm::vec3{0.0f, 0.0f, 1.0f}, glm::vec3{0.0f, 1.0f, 0.0f}, p_Fov, p_Near, p_Far)
{
    m_DistanceToTarget = p_DistanceToTarget;
    setTarget(p_Target);
}

void ArcballCamera::setTarget(const glm::vec3 p_Target)
{
    m_Target = p_Target;
    setPosition(m_Target - m_Front * m_DistanceToTarget);
}

void ArcballCamera::mouseMoved(const int32_t p_RelX, const int32_t p_RelY)
{
    if (m_LeftMousePressed)
    {
        const float l_Dx = static_cast<float>(p_RelX) * m_RotatingSpeed;
        const float l_Dy = static_cast<float>(p_RelY) * m_RotatingSpeed;

        m_Yaw += l_Dx;
        m_Pitch += l_Dy;

        constexpr float LIMIT = glm::radians(89.0f);
        m_Pitch = glm::clamp(m_Pitch, -LIMIT, LIMIT);

        const float l_X = m_DistanceToTarget * cosf(m_Pitch) * cosf(m_Yaw);
        const float l_Y = m_DistanceToTarget * sinf(m_Pitch);
        const float l_Z = m_DistanceToTarget * cosf(m_Pitch) * sinf(m_Yaw);
        
        setPosition(m_Target + glm::vec3(l_X, -l_Y, l_Z));
        setDir(glm::normalize(m_Target - m_Position));
    }
    else if (m_RightMousePressed)
    {
        const float l_Dx = static_cast<float>(p_RelX);
        const float l_Dy = static_cast<float>(p_RelY);

        const glm::vec3 l_Delta = (m_Right * -l_Dx + m_Up * -l_Dy) * (m_MovingSpeed * 0.01f);

        setTarget(m_Target + l_Delta);
    }
}

void ArcballCamera::mouseButtonPressed(const uint32_t p_Button)
{
    if (p_Button == SDL_BUTTON_LEFT)
        m_LeftMousePressed = true;
    else if (p_Button == SDL_BUTTON_RIGHT)
        m_RightMousePressed = true;
}

void ArcballCamera::mouseButtonReleased(const uint32_t p_Button)
{
    if (p_Button == SDL_BUTTON_LEFT)
        m_LeftMousePressed = false;
    else if (p_Button == SDL_BUTTON_RIGHT)
        m_RightMousePressed = false;
}

void ArcballCamera::mouseScrolled(const int32_t p_Y)
{
    m_DistanceToTarget -= static_cast<float>(p_Y);
    m_DistanceToTarget = std::max(m_DistanceToTarget, 1.0f);
    setPosition(m_Target - m_Front * m_DistanceToTarget);
}
