#ifndef OM_TEXT_INPUT_CONTROLLER_HPP
#define OM_TEXT_INPUT_CONTROLLER_HPP

#include <atomic>
#include <cmath>
#include <cstdint>

namespace openminecraft::renderer::common
{
// Decoupled bridge between the UI (which knows when a text field is focused)
// and the window (the only place allowed to call SDL text-input functions).
class OMTextInputController
{
  public:
    static auto instance() -> OMTextInputController &
    {
        static OMTextInputController controller;
        return controller;
    }

    // Any text-editing control (single or multi line) currently owns the keyboard.
    void setTextFocus(bool value)
    {
        textFocused.store(value, std::memory_order_relaxed);
    }
    auto isTextFocus() const -> bool
    {
        return textFocused.load(std::memory_order_relaxed);
    }

    // UI side (event thread). Safe to call from any thread.
    void setActive(bool value)
    {
        active.store(value, std::memory_order_relaxed);
    }
    void setArea(float x, float y, float width, float height)
    {
        // Publishing the same rectangle every frame made the window call
        // SDL_SetTextInputArea continuously, which dismisses the IME candidate
        // window on some platforms. Only bump the version on a real change.
        if (std::abs(x - areaX.load(std::memory_order_relaxed)) < 0.01f &&
            std::abs(y - areaY.load(std::memory_order_relaxed)) < 0.01f &&
            std::abs(width - areaWidth.load(std::memory_order_relaxed)) < 0.01f &&
            std::abs(height - areaHeight.load(std::memory_order_relaxed)) < 0.01f)
        {
            return;
        }

        areaX.store(x, std::memory_order_relaxed);
        areaY.store(y, std::memory_order_relaxed);
        areaWidth.store(width, std::memory_order_relaxed);
        areaHeight.store(height, std::memory_order_relaxed);
        areaVersion.fetch_add(1, std::memory_order_relaxed);
    }

    // Window side (main thread).
    auto isActive() const -> bool
    {
        return active.load(std::memory_order_relaxed);
    }
    auto takeArea(float &x, float &y, float &width, float &height) -> bool
    {
        const auto version = areaVersion.load(std::memory_order_relaxed);

        if (version == appliedAreaVersion)
        {
            return false;
        }

        appliedAreaVersion = version;
        x = areaX.load(std::memory_order_relaxed);
        y = areaY.load(std::memory_order_relaxed);
        width = areaWidth.load(std::memory_order_relaxed);
        height = areaHeight.load(std::memory_order_relaxed);
        return true;
    }

  private:
    std::atomic<bool> active{false};
    std::atomic<bool> textFocused{false};
    std::atomic<float> areaX{0.0f};
    std::atomic<float> areaY{0.0f};
    std::atomic<float> areaWidth{1.0f};
    std::atomic<float> areaHeight{1.0f};
    std::atomic<uint64_t> areaVersion{0};
    uint64_t appliedAreaVersion = 0;
};
} // namespace openminecraft::renderer::common

#endif
