#ifndef OM_STDLIB_SYSTEM_HPP
#define OM_STDLIB_SYSTEM_HPP

#include "openminecraft/vm/pixeltower/om_pixeltower_type.hpp"
#include <any>
#include <vector>
namespace openminecraft::vm::pixeltower::stdlib
{
namespace java::lang
{
class System : public pixeltower::OMNativeObjectType
{
  public:
    ~System();
    System();

    uint64_t length() override;
    std::string name() override;
    void invoke(std::string name, std::stack<std::any, std::list<std::any>> &stack) override;
    uint64_t fieldOffset(std::string name) override;
    uint64_t globalFieldOffset(std::string name) override;
    uint8_t *staticData() override;

  private:
    uint8_t *data;
};
} // namespace java::lang
}; // namespace openminecraft::vm::pixeltower::stdlib

#endif