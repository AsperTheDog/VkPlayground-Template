#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	Camera(glm::vec3 p_Pos, glm::vec3 p_Dir, glm::vec3 p_Up);
    virtual ~Camera() = default;

	void move(glm::vec3 p_Dir);
	void lookAt(glm::vec3 p_Target);

	void setPosition(glm::vec3 p_Pos);
	void setDir(glm::vec3 p_Dir);
    
    void setScreenSize(uint32_t p_Width, uint32_t p_Height);

	[[nodiscard]] glm::vec3 getPosition() const;
	[[nodiscard]] glm::vec3 getDir() const;

	glm::mat4& getViewMatrix();
	glm::mat4& getInvViewMatrix();
    glm::mat4& getProjMatrix();
    glm::mat4& getVPMatrix();
    glm::mat4& getInvProjMatrix();
    glm::mat4& getInvVPMatrix();

	virtual void mouseMoved(int32_t p_RelX, int32_t p_RelY) {}
	virtual void keyPressed(uint32_t p_Key) {}
	virtual void keyReleased(uint32_t p_Key) {}
    virtual void mouseButtonPressed(uint32_t p_Button) {}
    virtual void mouseButtonReleased(uint32_t p_Button) {}
	virtual void updateEvents(float p_Delta) {}
    virtual void mouseScrolled(int32_t p_Y) {}

    void setMouseCaptured(bool p_Captured);

    virtual void setViewDirty();
    virtual void setProjDirty();

protected:
    void recalculateViewMatrix();
    virtual void recalculateProjMatrix();

	void calculateRightVector();

	glm::vec3 m_Position;
	glm::vec3 m_Front;
	glm::vec3 m_Right;
    glm::vec3 m_Up;
    glm::vec2 m_ScreenSize{1000.f, 1000.f};
	float m_AspectRatio = 1.f;

	bool m_ViewDirty = true;
	glm::mat4 m_ViewMatrix{};
	bool m_InvViewDirty = true;
	glm::mat4 m_InvViewMatrix{};
    bool m_IsMouseCaptured = true;

    bool m_ProjDirty = true;
	glm::mat4 m_ProjMatrix{};
	bool m_InvProjDirty = true;
	glm::mat4 m_InvProjMatrix{};

    bool m_VPMatrixDirty = true;
	glm::mat4 m_VPMatrix{};
    bool m_InvVPMatrixDirty = true;
    glm::mat4 m_InvVPMatrix{};
};

