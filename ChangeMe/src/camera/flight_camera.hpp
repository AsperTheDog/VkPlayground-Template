#pragma once

#include <glm/glm.hpp>

#include "perspective_camera.hpp"

class FlightCamera final : public PerspectiveCamera
{
public:
	FlightCamera(glm::vec3 p_Pos, glm::vec3 p_Dir, glm::vec3 p_Up, float p_Fov = 70.0f, float p_Near = 0.1f, float p_Far = 1000.0f);

	void mouseMoved(int32_t p_RelX, int32_t p_RelY) override;
	void keyPressed(uint32_t p_Key) override;
	void keyReleased(uint32_t p_Key) override;
	void updateEvents(float p_Delta) override;
    void mouseScrolled(int32_t p_Y) override;

private:
    float m_MovingSpeed = 10.f;
	float m_MouseSensitivity = 0.1f;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;

	bool m_WPressed = false;
	bool m_APressed = false;
	bool m_SPressed = false;
	bool m_DPressed = false;
	bool m_SpacePressed = false;
	bool m_ShiftPressed = false;
};

