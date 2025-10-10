#include "flight_camera.hpp"

#include <algorithm>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <SDL3/SDL_keycode.h>

FlightCamera::FlightCamera(const glm::vec3 p_Pos, const glm::vec3 p_Dir, const glm::vec3 p_Up, const float p_Fov, const float p_Near, const float p_Far)
    : PerspectiveCamera(p_Pos, p_Dir, p_Up, p_Fov, p_Near, p_Far)
{
    setMouseCaptured(true);
}

void FlightCamera::mouseMoved(const float p_RelX, const float p_RelY)
{
    if (!m_IsMouseCaptured) return;
	m_Yaw += p_RelX * m_MouseSensitivity;
    m_Pitch += p_RelY * m_MouseSensitivity;

    m_Pitch = std::min(m_Pitch, 89.0f);
    m_Pitch = std::max(m_Pitch, -89.0f);
    if (m_Yaw > 360.0f)
		m_Yaw -= 360.0f;
	if (m_Yaw < -360.0f)
		m_Yaw += 360.0f;

    glm::vec3 l_NewFront;
    l_NewFront.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    l_NewFront.y = sin(glm::radians(m_Pitch));
    l_NewFront.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    setDir(l_NewFront);
}

void FlightCamera::keyPressed(const uint32_t p_Key)
{
    if (!m_IsMouseCaptured)
    {
        m_WPressed = false;
        m_SPressed = false;
        m_APressed = false;
        m_DPressed = false;
        m_SpacePressed = false;
        m_ShiftPressed = false;
        return;
    }
	switch (p_Key)
	{
	case SDLK_W:
		m_WPressed = true;
		break;
	case SDLK_S:
		m_SPressed = true;
		break;
	case SDLK_A:
		m_APressed = true;
		break;
	case SDLK_D:
		m_DPressed = true;
		break;
	case SDLK_SPACE:
		m_SpacePressed = true;
		break;
	case SDLK_LSHIFT:
		m_ShiftPressed = true;
		break;
	default:
		break;
	}
}

void FlightCamera::keyReleased(const uint32_t p_Key)
{
	switch (p_Key)
	{
	case SDLK_W:
		m_WPressed = false;
		break;
	case SDLK_S:
		m_SPressed = false;
		break;
	case SDLK_A:
		m_APressed = false;
		break;
	case SDLK_D:
		m_DPressed = false;
		break;
	case SDLK_SPACE:
		m_SpacePressed = false;
		break;
	case SDLK_LSHIFT:
		m_ShiftPressed = false;
		break;
	default:
		break;
	}
}

void FlightCamera::updateEvents(const float p_Delta)
{
	glm::vec3 l_MoveDir(0.0f);
	if (m_WPressed)
	{
		l_MoveDir += m_Front;
	}
	if (m_SPressed)
	{
		l_MoveDir -= m_Front;
	}
	if (m_APressed)
	{
		l_MoveDir -= m_Right;
	}
	if (m_DPressed)
	{
		l_MoveDir += m_Right;
	}
	if (m_SpacePressed)
	{
		l_MoveDir -= glm::vec3(0.0f, 1.0f, 0.0f);
	}
	if (m_ShiftPressed)
	{
		l_MoveDir += glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (l_MoveDir.x != 0 || l_MoveDir.y != 0 || l_MoveDir.z != 0)
	{
		move(glm::normalize(l_MoveDir) * (m_MovingSpeed * p_Delta));
	}
}

void FlightCamera::mouseScrolled(const float p_Y)
{
    // Change moving speed
    m_MovingSpeed = (1.f + p_Y * 0.05f) * m_MovingSpeed;
    m_MovingSpeed = std::clamp(m_MovingSpeed, 0.5f, 300.0f);
}