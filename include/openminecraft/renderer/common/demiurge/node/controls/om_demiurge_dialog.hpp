#ifndef OM_DEMIURGE_DIALOG_HPP
#define OM_DEMIURGE_DIALOG_HPP

#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Minimal message dialog: a full-screen modal overlay with a centered panel,
// one multi-line message and a single close button.
class OMDemiurgeDialog
{
  public:
    explicit OMDemiurgeDialog(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeDialog();

    void show(const std::shared_ptr<OMDemiurgeNode> &root, const std::string &message,
              const std::string &closeLabel = "Close");
    void close();
    auto isOpen() const -> bool
    {
        return openRequested || closing;
    }

    void setOnClose(std::function<void()> handler)
    {
        onClose = std::move(handler);
    }

  private:
    auto updateAnimation() -> void;

    geom::OMFontSet *fontset = nullptr;
    std::shared_ptr<bool> alive = std::make_shared<bool>(true);
    std::weak_ptr<OMDemiurgeNode> root;
    std::shared_ptr<OMDemiurgeNode> layerNode;
    std::shared_ptr<OMDemiurgeNode> panelNode;
    std::shared_ptr<OMDemiurgeNode> messageNode;
    std::shared_ptr<OMDemiurgeNode> closeButton;
    std::function<void()> onClose = []() {};

    animation::OMAnimationValue<float> openAmount{0.0f};
    bool openRequested = false;
    bool closing = false;
    bool cleanupScheduled = false;
    uint64_t generation = 0;
    std::string currentMessage;
    std::string currentCloseLabel;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
