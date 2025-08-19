#include "openminecraft/vm/pixeltower/v3/om_pixeltower_classbuilder.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"

namespace openminecraft::vm::pixeltower::v3
{
OMClassBuilder::OMClassBuilder(v0::OMPixelTowerHeap *heap) : metaspace(heap)
{
}
void OMClassBuilder::nop()
{
    codes.push_back(op_nop);
}
void OMClassBuilder::load_constant()
{
    codes.push_back(op_aconst_null);
}
void OMClassBuilder::load_constant(v0::jbyte d)
{
    codes.push_back(op_bipush);
    codes.push_back(d);
}
void OMClassBuilder::load_constant(v0::jshort d)
{
    codes.push_back(op_sipush);
    // geopelia: split low and high bytes
    codes.push_back(d >> 8);
    codes.push_back(d | 0xFF);
}
} // namespace openminecraft::vm::pixeltower::v3