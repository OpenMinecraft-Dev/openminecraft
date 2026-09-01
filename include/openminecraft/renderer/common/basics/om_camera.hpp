#ifndef OM_CAMERA_HPP
#define OM_CAMERA_HPP

#include "glm/fwd.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_position.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <glm/glm.hpp>

namespace openminecraft::renderer::common::basics
{
enum OMCameraMovement
{
    Forward,
    Back,
    Left,
    Right,
    Up,
    Down
};

struct OMCameraPlane
{
    glm::vec3 normal;
    float d;
};

class OMCamera
{
  public:
    OMCamera(OMRenderer *renderer, glm::vec3 location, float yaw, float pitch);
    ~OMCamera() = default;
    auto fetchViewMat() -> glm::mat4;
    auto fetchProjMat() -> glm::mat4;
    auto getForward() const -> glm::vec3
    {
        float yawRad = glm::radians(yaw);
        float pitchRad = glm::radians(pitch);

        float cosP = cos(pitchRad);
        return {cosP * cos(yawRad), sin(pitchRad), -cosP * sin(yawRad)};
    }

    auto getRight() const -> glm::vec3
    {
        glm::vec3 forward = getForward();
        glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

        glm::vec3 right = glm::cross(forward, worldUp);
        if (glm::length(right) > 1e-6f)
        {
            return glm::normalize(right);
        }

        return {1.0f, 0.0f, 0.0f};
    }

    auto getUp() const -> glm::vec3
    {
        glm::vec3 forward = getForward();
        glm::vec3 right = getRight();

        return glm::cross(right, forward);
    }
    auto isSphereVisible(const glm::vec3 &toObj, float radius) -> bool
    {
        auto forward = getForward();
        auto right = getRight();
        auto up = getUp();

        auto extent = renderer->getExtent();
        extent = glm::max(extent, glm::vec2(1.0f, 1.0f));

        float distF = glm::dot(toObj, forward);
        if (distF < -radius)
            return false;

        float tanV = tan(glm::radians(fov) * 0.5f);
        float tanH = tanV * extent.x / extent.y;

        float distR = glm::dot(toObj, right);
        float distU = glm::dot(toObj, up);

        if (std::abs(distR) > (distF * tanH + radius))
            return false;
        if (std::abs(distU) > (distF * tanV + radius))
            return false;

        return true;
    }
    auto isVisibleByYawPitch(const std::array<glm::vec3, 8> &corners) -> bool
    {
        auto extent = renderer->getExtent();
        extent = glm::max(extent, glm::vec2(1.0f, 1.0f));

        float hFovDeg = glm::degrees(atan(tan(glm::radians(fov) * 0.5f) * extent.x / extent.y));
        float vFovDeg = glm::degrees(glm::radians(fov) * 0.5f);

        float minYaw = 180.0f, maxYaw = -180.0f;
        float minPitch = 90.0f, maxPitch = -90.0f;

        for (const auto &corner : corners)
        {

            glm::vec3 dir = corner;

            float worldYaw = atan2(dir.z, dir.x);
            float deltaYaw = glm::degrees(worldYaw) - yaw;

            while (deltaYaw > 180.0f)
                deltaYaw -= 360.0f;
            while (deltaYaw < -180.0f)
                deltaYaw += 360.0f;

            float worldPitch = atan2(dir.y, sqrt(dir.x * dir.x + dir.z * dir.z));
            float deltaPitch = glm::degrees(worldPitch) - pitch;

            minYaw = fmin(minYaw, deltaYaw);
            maxYaw = fmax(maxYaw, deltaYaw);
            minPitch = fmin(minPitch, deltaPitch);
            maxPitch = fmax(maxPitch, deltaPitch);
        }

        if (maxYaw - minYaw >= 180.0f)
            return true;

        float margin = 45.0f;

        bool yawVisible = (maxYaw >= -hFovDeg - margin) && (minYaw <= hFovDeg + margin);
        bool pitchVisible = (maxPitch >= -vFovDeg - margin) && (minPitch <= vFovDeg + margin);

        return yawVisible && pitchVisible;
    }
    void modYaw(float delta);
    void modPitch(float delta);

    void moveCamera(OMCameraMovement mv, float d);

    auto getPos() -> glm::vec3
    {
        return position.toCombinedPos();
    }
    auto getPosRaw() -> OMPosition<16, int64_t, float>
    {
        return position;
    }
    auto getYaw() -> float
    {
        return yaw;
    }
    auto getPitch() -> float
    {
        return pitch;
    }

  private:
    OMRenderer *renderer;
    log::OMLogger logger;

    OMPosition<16, int64_t, float> position;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float fov = 70.0f;
};
} // namespace openminecraft::renderer::common::basics

#endif
