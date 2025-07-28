#ifndef OM_PIXELTOWER_DEBUGGER_HPP
#define OM_PIXELTOWER_DEBUGGER_HPP

#include "fmt/color.h"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
namespace openminecraft::vm::pixeltower::v1
{
class OMDebugger
{
  public:
    OMDebugger() : logger("OMDebugger", this)
    {
    }
    ~OMDebugger() = default;

    void debugStack()
    {
        logger.debug("format examples: ");
        logger.debug(fmt::format("{}", fmt::styled("normal data", fmt::fg(fmt::color::white))));
        logger.debug(fmt::format("{}", fmt::styled("frame metadata", fmt::fg(fmt::color::yellow_green))));
        logger.debug(fmt::format("{}", fmt::styled("free / dirty data", fmt::fg(fmt::color::gray))));

        void *currentData = (uint8_t *)v0::currentThread.stackPointer - 6 * sizeof(void *);
        auto style = fmt::fg(fmt::color::white);
        while (true)
        {
            std::string desc = "";

            if (currentData <= v0::currentThread.stackPointer)
            {
                style = fmt::fg(fmt::color::gray);
                if (currentData == v0::currentThread.stackPointer)
                {
                    desc = "<== Stack pointer (dirty data begins)";
                }
            }
            else
            {
                style = fmt::fg(fmt::color::antique_white);

                auto frame = v0::currentThread.currentFrame;
                while (frame != nullptr)
                {
                    if (frame == currentData)
                    {
                        desc = fmt::format("<== Frame base of {}.{}{}", frame->method->klass->name, frame->method->name,
                                           frame->method->desc);
                    }
                    if ((size_t)currentData - (size_t)frame < sizeof(v0::OMFrame))
                    {
                        style = fmt::fg(fmt::color::yellow_green);
                    }
                    frame = frame->prev;
                }
            }

            logger.debug("{}: {} {}", currentData,
                         fmt::styled(fmt::format("{:#018x}", (size_t)*static_cast<void **>(currentData)), style), desc);

            currentData = (uint8_t *)currentData + sizeof(void *);
            if (currentData == v0::currentThread.stack)
            {
                break;
            }
        }

        logger.debug("*** STACK ENDS ***");
    }

  private:
    log::OMLogger logger;
};
}; // namespace openminecraft::vm::pixeltower::v1

#endif