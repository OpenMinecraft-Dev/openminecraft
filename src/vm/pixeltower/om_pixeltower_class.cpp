#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"

namespace openminecraft::vm::pixeltower
{
OMClass::OMClass()
{
}
OMClass::~OMClass()
{
    mem::allocator::tracedFreeVMData(staticFieldBlock);
}
void OMClass::calcFieldOffsets()
{
    if (staticFieldBlock != nullptr)
    {
        mem::allocator::tracedFreeVMData(staticFieldBlock);
    }

    uint64_t staticFieldLength = 0;
    for (auto &field : fields)
    {
        uint64_t *length = &objectLength;

        if ((field.accessFlag & JVM_Acc_Static) != 0)
        {
            length = &staticFieldLength;
        }

        field.offset = *length;

        switch (field.type)
        {
        case Byte:
        case Char:
        case Boolean:
        case Short:
        case Int:
        case Float:
            *length += 4 - (*length % 4);
            break;
        case Long:
        case Double:
            *length += 8 - (*length % 8);
            break;
            // Variable pointers
        case Reference:
        case Array:
            *length += sizeof(void *) - (*length % sizeof(void *));
            break;
        }
    }

    staticFieldBlock = mem::allocator::tracedCallocVMData(1, staticFieldLength);
}
} // namespace openminecraft::vm::pixeltower