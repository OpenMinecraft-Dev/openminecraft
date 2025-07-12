#ifndef OM_PIXELTOWER_LINKER_HPP
#define OM_PIXELTOWER_LINKER_HPP

#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include <any>
#include <functional>
#include <list>
#include <memory>
#include <stack>
namespace openminecraft::vm::pixeltower
{
class OMLinker
{
  public:
    OMLinker(OMClassLoader &loader);
    ~OMLinker();

    void callMethod(std::any interpreter, std::string clazz, std::string func, std::string desc, bool isStatic,
                    std::stack<std::any, std::list<std::any>> &stk);

  private:
    OMClassLoader &loader;
};
} // namespace openminecraft::vm::pixeltower

#endif