#ifndef OM_DEMIURGE_NODE_HPP
#define OM_DEMIURGE_NODE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include <limits>
#include <vector>
namespace openminecraft::renderer::common::demiurge
{
class OMDemiurgeNode
{
  public:
    struct Style
    {
        OMDemiurgeSize width{OMDemiurgeSize::fit()};
        OMDemiurgeSize height{OMDemiurgeSize::fit()};
        float minWidth = 0;
        float maxWidth = std::numeric_limits<float>::max();
        float minHeight = 0;
        float maxHeight = std::numeric_limits<float>::max();

        OMDemiurgeEdgeInsets margin = {}, padding = {}, border = {};
        OMDemiurgePosition position;
    } style;

    OMDemiurgeNode();
    ~OMDemiurgeNode();

    void mount(OMDemiurgeNode *parent)
    {
        this->parent = parent;
        parent->children.push_back(this);
    }

  private:
    OMDemiurgeRect boundary;
    bool layoutDirty = true;

    OMDemiurgeNode *parent = nullptr;
    std::vector<OMDemiurgeNode *> children;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
