#ifndef OM_STDLIB_OBJECT_HPP

#define OM_STDLIB_OBJECT_HPP

#include "openminecraft/vm/pixeltower/om_pixeltower_type.hpp"
#include <any>
#include <vector>
namespace openminecraft::vm::pixeltower::stdlib
{
namespace java::lang
{
class Object : public pixeltower::OMNativeObjectType
{
  public:
    ~Object();
    Object();

    uint64_t length() override;
    std::string name() override;
    void invoke(std::string name, std::stack<std::any, std::list<std::any>> &stack) override;
};
} // namespace java::lang
}; // namespace openminecraft::vm::pixeltower::stdlib

#endif