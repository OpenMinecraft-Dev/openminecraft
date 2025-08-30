#include "openminecraft/vm/pixeltower/v2/om_pixeltower_gc_serial.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_oop.hpp"
#include "openminecraft/vm/pixeltower/v2/om_pixeltower_gc.hpp"

namespace openminecraft::vm::pixeltower::v2
{
OMGarbageCollectorSerial::OMGarbageCollectorSerial(v0::OMPixelTowerHeap *heap, v0::OMPixelTower *tower)
    : OMGarbageCollector(heap, tower), logger("OMGarbageCollectorSerial", this)
{
}

void OMGarbageCollectorSerial::freeObjects()
{
    auto count = 0;
    auto i = reinterpret_cast<v0::OMOOPDesc *>(heap->heapBase());
    while (i)
    {
        auto l = 0;
        if (i->klass->kind == v0::Array)
        {
            l += sizeof(v0::OMOOPArrDesc);
            auto arrd = (v0::OMOOPArrDesc *)i;
            l += i->klass->length * arrd->length;
        }
        else
        {
            l += sizeof(v0::OMOOPDesc);
            l += i->klass->length;
        }
        if (l % 8 != 0)
        {
            l += 8 - (l % 8);
        }

        // geopelia: don't try to get anything from the Mazarine End!
        auto inext = reinterpret_cast<v0::OMOOPDesc *>(heap->nextPtr(i, l));
        if (!(i->mark & v0::mreachable))
        {
            heap->deallocate(i, l);
            count++;
        }
        else
        {
            i->mark &= ~v0::mreachable;
        }
        i = inext;
    }
    logger.info("{} object freed", count);
}

void OMGarbageCollectorSerial::markSub(void *root)
{
    if (!root)
    {
        return;
    }
    auto i = reinterpret_cast<v0::OMOOPDesc *>(root);
    if (i->mark & v0::mreachable)
    {
        return;
    }
    i->mark |= v0::mreachable;
    if (i->klass->kind == v0::Array)
    {
        // gino: array with pointers, we need store these flags in OMKlass
        if (i->klass->name[1] == '[' || i->klass->name[1] == 'L')
        {
            auto arr = reinterpret_cast<v0::OMOOPArrDesc *>(i);

            if (arr->klass->heap->ptrCompEnabled())
            {
                auto arrd = arr->array<uint32_t>();
                for (int ix = 0; ix < arr->length; ix++)
                {
                    markSub(arr->klass->heap->decompressPtr(arrd[ix]));
                }
            }
            else
            {
                auto arrd = arr->array<void *>();
                for (int ix = 0; ix < arr->length; ix++)
                {
                    markSub(arrd[ix]);
                }
            }
        }
    }
    else
    {
        auto cls = i->klass;
        while (cls)
        {
            for (auto &f : cls->fields)
            {
                if ((f.desc[0] == '[' || f.desc[0] == 'L') && !(f.accessFlags & JVM_Acc_Static))
                {
                    auto ptrb = (void **)(i->data + f.offset);
                    if (heap->ptrCompEnabled())
                    {
                        markSub(heap->decompressPtr(*(uint32_t *)ptrb));
                    }
                    else
                    {
                        markSub(*ptrb);
                    }
                }
            }
            cls = cls->superClass;
        }
    }
}
void OMGarbageCollectorSerial::signUnreachable()
{
    auto i = reinterpret_cast<v0::OMOOPDesc *>(heap->heapBase());
    while (i)
    {
        if ((existsInStack(i) || static_cast<bool>(i->mark & v0::mconst)))
        {
            markSub(i);
        }
        auto l = 0;
        if (i->klass->kind == v0::Array)
        {
            l += sizeof(v0::OMOOPArrDesc);
            auto arrd = reinterpret_cast<v0::OMOOPArrDesc *>(i);
            l += i->klass->length * arrd->length;
        }
        else
        {
            l += sizeof(v0::OMOOPDesc);
            l += i->klass->length;
        }
        if (l % 8 != 0)
        {
            l += 8 - (l % 8);
        }
        i = reinterpret_cast<v0::OMOOPDesc *>(heap->nextPtr(i, l));
    }

    for (auto &i : tower->loader->classes)
    {
        for (auto &f : i->fields)
        {
            if ((f.desc[0] == '[' || f.desc[0] == 'L') && (f.accessFlags & JVM_Acc_Static))
            {
                auto ptrb = (void **)(reinterpret_cast<uint8_t *>(i->staticBlock) + f.offset);
                if (heap->ptrCompEnabled())
                {
                    markSub(heap->decompressPtr(*(uint32_t *)ptrb));
                }
                else
                {
                    markSub(*ptrb);
                }
            }
        }
    }

    heap->debug();
    freeObjects();
}

bool OMGarbageCollectorSerial::existsInStack(void *p)
{
    for (auto &th : tower->threadMap)
    {
        auto tgt = ((void **)th.second->stackPointer) - 1; // stack top
        while (tgt < th.second->stack)
        {
            if (*tgt == p)
            {
                return true;
            }
            tgt++;
        }
    }
    return false;
}
} // namespace openminecraft::vm::pixeltower::v2
