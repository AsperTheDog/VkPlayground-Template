#include "camera.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

Camera::Camera(const glm::vec3 p_Pos, const glm::vec3 p_Dir, glm::vec3 p_Up)
    : m_Position(p_Pos), m_Front(p_Dir), m_Up(p_Up)
{

}

void Camera::move(const glm::vec3 p_Dir)
{
	m_Position += p_Dir;
	calculateRightVector();
	setViewDirty();
}

void Camera::lookAt(const glm::vec3 p_Target)
{
	m_Front = glm::normalize(p_Target - m_Position);
	calculateRightVector();
	setViewDirty();
}

void Camera::setPosition(const glm::vec3 p_Pos)
{
	m_Position = p_Pos;
	calculateRightVector();
	setViewDirty();
}

void Camera::setDir(const glm::vec3 p_Dir)
{
	m_Front = p_Dir;
	calculateRightVector();
	setViewDirty();
}

void Camera::setScreenSize(const uint32_t p_Width, const uint32_t p_Height)
{
    m_ScreenSize = { static_cast<float>(p_Width), static_cast<float>(p_Height) };
    m_AspectRatio = static_cast<float>(p_Width) / static_cast<float>(p_Height);
    setProjDirty();
}

glm::vec3 Camera::getPosition() const
{
	return m_Position;
}

glm::vec3 Camera::getDir() const
{
	return m_Front;
}

glm::mat4& Camera::getViewMatrix()
{
	if (m_ViewDirty)
	{
		recalculateViewMatrix();
	}

	return m_ViewMatrix;
}

glm::mat4& Camera::getInvViewMatrix()
{
    if (m_InvViewDirty)
    {
        m_InvViewMatrix = glm::inverse(getViewMatrix());
        m_InvViewDirty = false;
    }
    return m_InvViewMatrix;
}

void Camera::recalculateViewMatrix()
{
    m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, glm::vec3(0, 1, 0));
	m_ViewDirty = false;
}

void Camera::recalculateProjMatrix()
{
    m_ProjMatrix = glm::mat4(1.0f);
}


glm::mat4& Camera::getProjMatrix()
{
    if (m_ProjDirty)
    {
        recalculateProjMatrix();
    }
    return m_ProjMatrix;
}

glm::mat4& Camera::getVPMatrix()
{
    if (m_VPMatrixDirty)
    {
        m_VPMatrix = getProjMatrix() * getViewMatrix();
        m_VPMatrixDirty = false;
    }
    return m_VPMatrix;
}

glm::mat4& Camera::getInvProjMatrix()
{
    if (m_InvProjDirty)
    {
        m_InvProjMatrix = glm::inverse(getProjMatrix());
        m_InvProjDirty = false;
    }
    return m_InvProjMatrix;
}

glm::mat4& Camera::getInvVPMatrix()
{
    if (m_InvVPMatrixDirty)
    {
        m_InvVPMatrix = glm::inverse(getVPMatrix());
        m_InvVPMatrixDirty = false;
    }
    return m_InvVPMatrix;
}

void Camera::calculateRightVector()
{
	m_Right = glm::normalize(glm::cross(m_Front, m_Up));
}

void Camera::setMouseCaptured(const bool p_Captured)
{
    m_IsMouseCaptured = p_Captured;
}

void Camera::setViewDirty()
{
    m_ViewDirty = true;
    m_InvViewDirty = true;
    m_VPMatrixDirty = true;
    m_InvVPMatrixDirty = true;
}

void Camera::setProjDirty()
{
    m_ProjDirty = true;
    m_InvProjDirty = true;
    m_VPMatrixDirty = true;
    m_InvVPMatrixDirty = true;
}
