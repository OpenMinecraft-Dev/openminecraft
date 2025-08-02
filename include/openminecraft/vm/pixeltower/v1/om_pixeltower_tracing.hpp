#ifndef OM_PIXELTOWER_TRACING_HPP
#define OM_PIXELTOWER_TRACING_HPP
#include <string>
namespace openminecraft::vm::pixeltower::v1::tracing
{
void installHandler();

struct OMTracingFrame
{
    void *location;
    std::string name;
};
} // namespace openminecraft::vm::pixeltower::v1::tracing

#endif