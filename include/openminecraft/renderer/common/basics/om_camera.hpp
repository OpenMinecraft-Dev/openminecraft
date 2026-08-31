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
    auto visible(OMPosition<16, int64_t, float> p, glm::mat4 &rot) -> bool
    {
        if (p.chunkx == position.chunkx && p.chunky == position.chunky && p.chunkz == position.chunkz)
        {
            return true;
        }

        auto r = p - position;
        auto l = rot * glm::vec4(r, 1.0f);
        l /= l.w;

        return !(l.x > 2 || l.x < -1.25 || l.y > 2 || l.y < -1.25 || l.z < 0.0f);
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
