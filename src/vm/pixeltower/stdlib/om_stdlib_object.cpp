#include "openminecraft/vm/pixeltower/stdlib/om_stdlib_object.hpp"
#include <any>
#include <typeindex>

namespace openminecraft::vm::pixeltower::stdlib
{
namespace java::lang
{
Object ::Object()
{
}
Object::~Object()
{
}
uint64_t Object::length()
{
    return 0;
}
std::string Object::name()
{
    return "java/lang/Object";
}
void Object::invoke(std::string name, std::stack<std::any> &stk, std::vector<std::any> &args)
{
    if (name == "<init>")
    {
    }
    else if (name == "equals")
    {
        auto fm = stk.top();
        stk.pop();
        auto arg1 = args[0];
        auto arg2 = args[1];

        int result;

        if (std::type_index(arg1.type()) != std::type_index(typeid(void *)) ||
            std::type_index(arg2.type()) != std::type_index(typeid(void *)))
        {
            result = 0;
        }
        else
        {
            result = std::any_cast<void *>(arg2) == std::any_cast<void *>(arg1) ? 1 : 0;
        }

        stk.push(fm);
        stk.push(result);
    }
}
} // namespace java::lang
} // namespace openminecraft::vm::pixeltower::stdlib