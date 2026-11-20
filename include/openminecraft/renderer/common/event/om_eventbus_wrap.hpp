#ifndef OM_EVENTBUS_WRAP_HPP
#define OM_EVENTBUS_WRAP_HPP

#include <cstdint>
#include <string>
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
namespace openminecraft::renderer::common::event
{
enum OMEventType
{
    Custom,
    MouseMotion,
    MouseUp,
    MouseDown,
    MouseWheel,
    KeyDown,
    KeyUp,
    TextInput,
    TextEditing
};

struct OMEvent
{
    OMEventType type;

    union {
        struct
        {
            float x;
            float y;
            uint8_t button;
        } mousebutton;
        struct
        {
            float x;
            float y;
            uint8_t button;
        } mousemotion;
        struct
        {
            float x;
            float y;
            float wheelx;
            float wheely;
        } mousewheel;
        struct
        {
            uint32_t keycode;
            uint16_t modifier;
        } key;
        struct
        {
            uint64_t flag;
            double value;
        } custom;
    };

    // TextInput / TextEditing payload (kept outside the union: std::string is not trivial).
    std::string text;
    int selectionStart = 0;
    int selectionLength = 0;
};
using OMEventBusWrap = OMEventBus<OMEventType, OMEvent>;
} // namespace openminecraft::renderer::common::event

#endif