#ifndef OM_PIXELTOWER_LINKER_HPP
#define OM_PIXELTOWER_LINKER_HPP

#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include <any>
#include <list>
#include <stack>
namespace openminecraft::vm::pixeltower
{
class OMLinker
{
  public:
    OMLinker(OMClassLoader &loader);
    ~OMLinker();

    uint64_t fieldOffset(std::string clazz, std::string name, bool isStatic);
    void callMethod(std::any interpreter, std::string clazz, std::string func, std::string desc, bool isStatic,
                    std::stack<std::any, std::list<std::any>> &stk);
    uint8_t *staticData(std::string clazz);

  private:
    OMClassLoader &loader;
};
} // namespace openminecraft::vm::pixeltower

#endif