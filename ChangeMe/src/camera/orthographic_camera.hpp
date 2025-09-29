#pragma once
#include "camera.hpp"

class OrthographicCamera  : public Camera
{
public:
    OrthographicCamera(glm::vec3 p_Pos, glm::vec3 p_Dir, glm::vec3 p_Up, glm::vec2 p_XBounds, glm::vec2 p_YBounds, float p_Near = 0.1f, float p_Far = 1000.0f);

    void setBounds(glm::vec2 p_XBounds, glm::vec2 p_YBounds);
    void setNearPlane(float p_Near);
    void setFarPlane(float p_Far);

    void setProjectionData(glm::vec2 p_XBounds, glm::vec2 p_YBounds, float p_Near, float p_Far);

    [[nodiscard]] glm::vec2 getXBounds() const { return m_XBounds; }
    [[nodiscard]] glm::vec2 getYBounds() const { return m_YBounds; }
    [[nodiscard]] float getNearPlane() const { return m_Near; }
    [[nodiscard]] float getFarPlane() const { return m_Far; }

protected:
    void recalculateProjMatrix() override;

	float m_Near;
	float m_Far;

    glm::vec2 m_XBounds;
    glm::vec2 m_YBounds;
};

