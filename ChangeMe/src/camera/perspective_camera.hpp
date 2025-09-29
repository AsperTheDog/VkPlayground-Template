#pragma once
#include "camera.hpp"

class PerspectiveCamera : public Camera
{
public:
    PerspectiveCamera(glm::vec3 p_Pos, glm::vec3 p_Dir, glm::vec3 p_Up, float p_Fov = 70.0f, float p_Near = 0.1f, float p_Far = 1000.0f);

	void setProjectionData(float p_Fov, float p_Near, float p_Far);

    [[nodiscard]] float getNearPlane() const { return m_Near; }
    [[nodiscard]] float getFarPlane() const { return m_Far; }

protected:
    void recalculateProjMatrix() override;

    float m_Fov;
	float m_Near;
	float m_Far;
};

