#include "openminecraft/vm/pixeltower/stdlib/om_stdlib_object.hpp"
#include <any>
#include <cstdint>
#include <typeindex>

namespace openminecraft::vm::pixeltower::stdlib
{
namespace java::lang
{
Object ::Object()
{
    data = new uint8_t[0];
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
uint64_t Object::fieldOffset(std::string name)
{
    return 0;
}
uint64_t Object::globalFieldOffset(std::string name)
{
    return 0;
}
uint8_t *Object::staticData()
{
    return data;
}
void Object::invoke(std::string name, std::stack<std::any, std::list<std::any>> &stk)
{
    if (name == "<init>")
    {
        stk.pop();
        stk.pop();
    }
    else if (name == "equals")
    {
        auto arg1 = stk.top();
        stk.pop();
        auto arg2 = stk.top();
        stk.pop();
        stk.pop();

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

        stk.push(result);
    }
}
} // namespace java::lang
} // namespace openminecraft::vm::pixeltower::stdlib