#pragma once
#include "orthographic_camera.hpp"

class OrthoControllerCamera final : public OrthographicCamera
{
public:
    OrthoControllerCamera(glm::vec3 p_Pos, glm::vec3 p_Dir, glm::vec3 p_Up, glm::vec2 p_XBounds, glm::vec2 p_YBounds, float p_Near = 0.1f, float p_Far = 1000.0f);

private:
    void mouseMoved(float p_RelX, float p_RelY) override;
    void mouseButtonPressed(uint32_t p_Button) override;
    void mouseButtonReleased(uint32_t p_Button) override;
    void mouseScrolled(float p_Y) override;

private:
    void recalculateUnitsPerPixel();
    void recalculateProjMatrix() override;

    float m_PanningSpeed = 10.f;

    float m_Zoom = 1.f;
    glm::vec2 m_UnitsPerPixel{};

    bool m_LeftMousePressed = false;
};

