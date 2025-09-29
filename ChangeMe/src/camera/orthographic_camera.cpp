#include "orthographic_camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>

OrthographicCamera::OrthographicCamera(glm::vec3 p_Pos, glm::vec3 p_Dir, glm::vec3 p_Up, glm::vec2 p_XBounds, glm::vec2 p_YBounds, float p_Near, float p_Far)
    : Camera(p_Pos, p_Dir, p_Up), m_Near(p_Near), m_Far(p_Far), m_XBounds(p_XBounds), m_YBounds(p_YBounds)
{
}

void OrthographicCamera::setBounds(const glm::vec2 p_XBounds, const glm::vec2 p_YBounds)
{
    m_XBounds = p_XBounds;
    m_YBounds = p_YBounds;
    setProjDirty();
}

void OrthographicCamera::setNearPlane(const float p_Near)
{
    m_Near = p_Near;
    setProjDirty();
}

void OrthographicCamera::setFarPlane(const float p_Far)
{
    m_Far = p_Far;
    setProjDirty();
}

void OrthographicCamera::setProjectionData(const glm::vec2 p_XBounds, const glm::vec2 p_YBounds, const float p_Near, const float p_Far)
{
    m_XBounds = p_XBounds;
    m_YBounds = p_YBounds;
    m_Near = p_Near;
    m_Far = p_Far;
    setProjDirty();
}

void OrthographicCamera::recalculateProjMatrix()
{
    m_ProjMatrix = glm::ortho(m_XBounds.x, m_XBounds.y, m_YBounds.x, m_YBounds.y, m_Near, m_Far);
    m_ProjDirty = false;
}
