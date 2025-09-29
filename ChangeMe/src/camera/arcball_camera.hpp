#pragma once

#include "perspective_camera.hpp"

class ArcballCamera final : public PerspectiveCamera
{
public:
    ArcballCamera(glm::vec3 p_Target, float p_DistanceToTarget, float p_Fov = 70.0f, float p_Near = 0.1f, float p_Far = 1000.0f);

    void setTarget(glm::vec3 p_Target);

	void mouseMoved(int32_t p_RelX, int32_t p_RelY) override;
    void mouseButtonPressed(uint32_t p_Button) override;
    void mouseButtonReleased(uint32_t p_Button) override;
    void mouseScrolled(int32_t p_Y) override;

private:
    float m_MovingSpeed = 1.f;
	float m_RotatingSpeed = 0.01f;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_DistanceToTarget = 10.0f;

    glm::vec3 m_Target{};

    bool m_LeftMousePressed = false;
    bool m_RightMousePressed = false;
};

