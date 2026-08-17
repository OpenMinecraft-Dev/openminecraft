#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace openminecraft::renderer::common::basics
{
OMCamera::OMCamera(OMRenderer *renderer, glm::vec3 location, float yaw, float pitch)
    : renderer(renderer), position(location), yaw(yaw), pitch(pitch), logger("OMCamera", this)
{
}

auto OMCamera::fetchViewMat() -> glm::mat4
{
    glm::vec3 front;
    front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front.y = std::sin(glm::radians(pitch));
    front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front = glm::normalize(front);
    return glm::lookAt(glm::vec3(0.0), front, {0.0f, 1.0f, 0.0f});
}

auto OMCamera::fetchProjMat() -> glm::mat4
{
    auto extent = renderer->getExtent();
    return glm::perspective(glm::radians(70.0f), extent.x / extent.y, 1000.0f, 0.05f);
}

void OMCamera::modYaw(float d)
{
    this->yaw += d;

    if (yaw < 0.0f)
        yaw += 360.0f;
    yaw = std::fmod(yaw + 180.0f, 360.0f);
    yaw -= 180.0f;
}

void OMCamera::modPitch(float d)
{
    this->pitch += d;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
}

void OMCamera::moveCamera(OMCameraMovement mv, float d)
{

    glm::vec3 front;
    front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front.y = 0;
    // front.y = std::sin(glm::radians(pitch));
    front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front = glm::normalize(front);

    switch (mv)
    {
    case Forward:
        position += front * d;
        break;
    case Back:
        position -= front * d;
        break;
    case Left:
        position -= glm::normalize(glm::cross(front, {0.0f, 1.0f, 0.0f})) * d;
        break;
    case Right:
        position += glm::normalize(glm::cross(front, {0.0f, 1.0f, 0.0f})) * d;
        break;
    case Up:
        position += glm::vec3{0.0f, 1.0f, 0.0f} * d;
        break;
    case Down:
        position -= glm::vec3{0.0f, 1.0f, 0.0f} * d;
        break;
    default:
        break;
    }
}

} // namespace openminecraft::renderer::common::basics
