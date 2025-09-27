#include "perspective_camera.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

PerspectiveCamera::PerspectiveCamera(glm::vec3 p_Pos, glm::vec3 p_Dir, glm::vec3 p_Up, float p_Fov, float p_Near, float p_Far)
    : Camera(p_Pos, p_Dir, p_Up), m_Fov(p_Fov), m_Near(p_Near), m_Far(p_Far)
{

}

void PerspectiveCamera::setProjectionData(const float p_Fov, const float p_Near, const float p_Far)
{
    m_Fov = p_Fov;
    m_Near = p_Near;
    m_Far = p_Far;
    setProjDirty();
}

void PerspectiveCamera::recalculateProjMatrix()
{
    m_ProjMatrix = glm::perspective(glm::radians(m_Fov), 16.0f/9, m_Near, m_Far);
	m_ProjDirty = false;
}
