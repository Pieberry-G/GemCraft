#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace GemCraft {
namespace TinyRenderer {

    class Camera
    {
    public:
        Camera(float distance = 500.0f);

        void SetEulerAngles(float pitchAngle, float yawAngle) { m_Pitch = glm::radians(pitchAngle); m_Yaw = glm::radians(yawAngle); }

        glm::mat4 GetViewMatrix() const { return glm::lookAt(CalculatePosition(), glm::vec3(0.0f), m_Up); }
        glm::mat4 GetProjection() const { return glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip); }

        glm::vec3 CalculatePosition() const;
        glm::vec3 GetForwardDirection() const;
        glm::quat GetOrientation() const;

    public:
        float m_FOV = 45.0f, m_AspectRatio = 1.0f, m_NearClip = 0.1f, m_FarClip = 1000.0f;
        glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };
        glm::vec3 m_Up = { 0.0f, 1.0f, 0.0f };

        float m_Distance = 500.0f;
        float m_Pitch = 0.0f, m_Yaw = 0.0f;
    };

} // namespace TinyRenderer
} // namespace GemCraft