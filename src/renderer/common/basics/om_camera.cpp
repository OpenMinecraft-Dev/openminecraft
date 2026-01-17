#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace openminecraft::renderer::common::basics
{
OMCamera::OMCamera(OMRenderer *renderer, glm::vec3 location) : renderer(renderer), location(location)
{
}

glm::mat4 OMCamera::fetchViewMat()
{
    glm::vec3 front;
    front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front.y = std::sin(glm::radians(pitch));
    front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front = glm::normalize(front);
    return glm::lookAt(location, location + front, {0.0f, 1.0f, 0.0f});
}
} // namespace openminecraft::renderer::common::basics
