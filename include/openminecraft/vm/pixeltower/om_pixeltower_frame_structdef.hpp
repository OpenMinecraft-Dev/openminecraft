#ifndef OM_PIXELTOWER_FRAME_STRUCTDEF_HPP
#define OM_PIXELTOWER_FRAME_STRUCTDEF_HPP

#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include <vector>
namespace openminecraft::vm::pixeltower::runtime
{
struct OMFrameMetadata
{
    std::shared_ptr<OMClass> clazz;
    OMMethodInfo &method;
    uint64_t offset;
    std::vector<std::any> local;
};
}; // namespace openminecraft::vm::pixeltower::runtime

#endif