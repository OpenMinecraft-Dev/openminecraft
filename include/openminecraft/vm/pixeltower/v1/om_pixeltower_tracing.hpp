#ifndef OM_PIXELTOWER_TRACING_HPP
#define OM_PIXELTOWER_TRACING_HPP
#include <map>
#include <string>
namespace openminecraft::vm::pixeltower::v1::tracing
{
extern std::map<std::string, void *> registers;
void installHandler();

struct OMTracingFrame
{
    void *location;
    std::string name;
};
} // namespace openminecraft::vm::pixeltower::v1::tracing

#endif