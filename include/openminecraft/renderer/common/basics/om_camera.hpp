#ifndef OM_CAMERA_HPP
#define OM_CAMERA_HPP

#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <glm/glm.hpp>

namespace openminecraft::renderer::common::basics
{
class OMCamera
{
  public:
    OMCamera(OMRenderer *renderer, glm::vec3 location);
    ~OMCamera();
    glm::mat4 fetchViewMat();

  private:
    const OMRenderer *renderer;

    glm::vec3 location;
    float yaw = 0.0f;
    float pitch = 0.0f;
};
} // namespace openminecraft::renderer::common::basics

#endif
