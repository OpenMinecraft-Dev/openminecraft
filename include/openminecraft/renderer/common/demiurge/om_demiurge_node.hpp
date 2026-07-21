#ifndef OM_DEMIURGE_NODE_HPP
#define OM_DEMIURGE_NODE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

namespace openminecraft::renderer::common::demiurge
{
class OMDemiurgeNode
{
  public:
    struct Style
    {
        OMDemiurgeSize width{OMDemiurgeSize::fill()};
        OMDemiurgeSize height{OMDemiurgeSize::fill()};
        float minWidth = 0;
        float maxWidth = std::numeric_limits<float>::max();
        float minHeight = 0;
        float maxHeight = std::numeric_limits<float>::max();

        OMDemiurgeEdgeInsets margin = {}, padding = {}, border = {};
        OMDemiurgePosition position = Relative;
        OMDemiurgeAlignment alignment = TopLeft;

        auto fixed() -> bool
        {
            return width.unit == OMDemiurgeSize::Pixel && height.unit == OMDemiurgeSize::Pixel;
        }
    } style;

    OMDemiurgeNode();
    ~OMDemiurgeNode();

    void mount(std::shared_ptr<OMDemiurgeNode> child)
    {
        child->parent = this;
        children.push_back(child);
    }

    void umount(std::shared_ptr<OMDemiurgeNode> child)
    {
        auto f = std::find(children.begin(), children.end(), child);
        if (f != children.end())
        {
            child->parent = nullptr;
            children.erase(f);
        }
    }

    void layout();

  private:
    // YGNodeRef yogaNode;
    OMDemiurgeNode *parent = nullptr;
    std::vector<std::shared_ptr<OMDemiurgeNode>> children;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
