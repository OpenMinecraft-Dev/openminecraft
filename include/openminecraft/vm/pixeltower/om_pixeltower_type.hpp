#ifndef OM_PIXELTOWER_TYPE_HPP
#define OM_PIXELTOWER_TYPE_HPP

#include <any>
#include <cstdint>
#include <list>
#include <stack>
#include <string>
namespace openminecraft::vm::pixeltower
{
class OMNativeObjectType
{
  public:
    virtual uint64_t length() = 0;
    virtual std::string name() = 0;

    virtual void invoke(std::string name, std::stack<std::any, std::list<std::any>> &stack) = 0;
};
}; // namespace openminecraft::vm::pixeltower

#endif