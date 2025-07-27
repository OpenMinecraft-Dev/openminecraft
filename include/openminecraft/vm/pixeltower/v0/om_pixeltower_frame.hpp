#ifndef OM_PIXELTOWER_FRAME_HPP
#define OM_PIXELTOWER_FRAME_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
namespace openminecraft::vm::pixeltower::v0
{
struct OMFrame
{
    OMMethod *method;
    OMFrame *prev;
    void *returnAddr;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif
