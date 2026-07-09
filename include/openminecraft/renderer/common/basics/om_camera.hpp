#ifndef OM_CAMERA_HPP
#define OM_CAMERA_HPP

#include "openminecraft/log/om_log_common.hpp"
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

class OMCamera
{
  public:
    OMCamera(OMRenderer *renderer, glm::vec3 location, float yaw, float pitch);
    ~OMCamera() = default;
    auto fetchViewMat() -> glm::mat4;
    auto fetchProjMat() -> glm::mat4;
    void modYaw(float delta);
    void modPitch(float delta);

    void moveCamera(OMCameraMovement mv, float d);

  private:
    const OMRenderer *renderer;
    log::OMLogger logger;

    glm::vec3 location;
    float yaw = 0.0f;
    float pitch = 0.0f;
};
} // namespace openminecraft::renderer::common::basics

#endif
