#ifndef OM_DEMIURGE_EVENT_HPP
#define OM_DEMIURGE_EVENT_HPP

#include <cstdint>
#include <string>

namespace openminecraft::renderer::common::demiurge
{
enum class OMDemiurgeEventType
{
    MouseDown,
    MouseUp,
    MouseMove,
    MouseEnter,
    MouseExit,
    MouseWheel,
    KeyDown,
    KeyUp,
    TextInput,
};

// Unified UI event: mouse, wheel, keyboard and text input.
// Mouse coordinates are in the same screen space as the layout bounds.
struct OMDemiurgeEvent
{
    OMDemiurgeEventType type = OMDemiurgeEventType::MouseMove;

    // Mouse / wheel
    float x = 0.0f;
    float y = 0.0f;
    uint8_t button = 0;
    float wheelX = 0.0f;
    float wheelY = 0.0f;

    // Keyboard / text input
    uint32_t keycode = 0;
    uint16_t modifier = 0;
    bool repeat = false;
    std::string text;

    auto isMouse() const -> bool
    {
        switch (type)
        {
        case OMDemiurgeEventType::MouseDown:
        case OMDemiurgeEventType::MouseUp:
        case OMDemiurgeEventType::MouseMove:
        case OMDemiurgeEventType::MouseEnter:
        case OMDemiurgeEventType::MouseExit:
        case OMDemiurgeEventType::MouseWheel:
            return true;
        default:
            return false;
        }
    }

    auto isKeyboard() const -> bool
    {
        switch (type)
        {
        case OMDemiurgeEventType::KeyDown:
        case OMDemiurgeEventType::KeyUp:
        case OMDemiurgeEventType::TextInput:
            return true;
        default:
            return false;
        }
    }
};
} // namespace openminecraft::renderer::common::demiurge

#endif
