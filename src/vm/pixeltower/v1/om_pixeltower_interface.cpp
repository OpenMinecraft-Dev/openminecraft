#include "openminecraft/vm/pixeltower/v1/om_pixeltower_interface.hpp"

namespace openminecraft::vm::pixeltower::v1
{
OMPixelTowerInterface::OMPixelTowerInterface(v0::OMPixelTower *t) : tower(t)
{
}

v0::OMField *OMPixelTowerInterface::findField(v0::OMKlass *klass, std::string name, std::string desc)
{
    for (auto &f : klass->fields)
    {
        if (f.name == name && f.desc == desc)
        {
            return &f;
        }
    }

    return nullptr;
}
} // namespace openminecraft::vm::pixeltower::v1