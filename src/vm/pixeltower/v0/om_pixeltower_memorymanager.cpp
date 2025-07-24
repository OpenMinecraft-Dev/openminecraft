#include "openminecraft/vm/pixeltower/om_pixeltower_memorymanager.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include <any>
#include <memory>
#include <stack>
#include <typeindex>

#define HEAP_SIZE 1024 * 128

namespace openminecraft::vm::pixeltower
{
std::string ARRAY_TYPE = "array";
OMMemoryManager::OMMemoryManager(std::any tower, std::shared_ptr<OMClassLoader> cld)
    : logger("pixeltower/OMMemoryManager", this), cld(cld), tower(tower)
{
}
OMMemoryManager::~OMMemoryManager()
{
}

void *OMMemoryManager::fetchInternal(int size)
{
    auto p = mem::allocator::tracedMallocVMData(size);
    memset(p, 0, size);
    blockStatus[p] = false;
    return p;
}

void OMMemoryManager::gc()
{
    if (blockStatus.size() < 16384) {
        return;
    }
    logger.debug("serial gc begin");
    auto interpr = std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(tower);
    for (auto p : interpr->stack)
    {
        std::stack<std::any> stkb(p.second);

        while (!stkb.empty())
        {
            auto i = std::type_index(stkb.top().type());
            if (i == std::type_index(typeid(void *)))
            {
                searchFromInstance(std::any_cast<void *>(stkb.top()));
            }
            else if (i == std::type_index(typeid(std::shared_ptr<runtime::OMFrameMetadata>)))
            {
                auto fr = std::any_cast<std::shared_ptr<runtime::OMFrameMetadata>>(stkb.top());
                for (auto r : fr->local)
                {
                    if (std::type_index(r.type()) == std::type_index(typeid(void *)))
                    {
                        searchFromInstance(std::any_cast<void *>(r));
                    }
                }
            }
            stkb.pop();
        }
    }

    for (auto l : cld->classes)
    {
        searchFromStatic(l.second);
    }

    for (auto p = blockStatus.begin(); p != blockStatus.end();)
    {
        if (p->second)
        {
            p->second = false;
            ++p;
        }
        else
        {
            deallocate(p->first);
            p = blockStatus.erase(p);
        }
    }
}

void *OMMemoryManager::allocate(std::shared_ptr<OMClass> cls)
{
    auto result = fetchInternal(sizeof(void *) + cls->objectLength);
    auto clsp = (OMClass **)result;
    *clsp = cls.get();
    return result;
}
void *OMMemoryManager::allocateArray(std::shared_ptr<OMClass> cls, int *lengths, int dim)
{
    if (dim == 1)
    {
        auto result = fetchInternal(sizeof(OMArrayHeader) + (sizeof(void *) * *lengths));
        auto arr = (OMArrayHeader *)result;
        arr->classifierPointer = &ARRAY_TYPE;
        arr->length = *lengths;
        arr->classPointer = cls.get();
        arr->type = Reference;
        arr->dim = 1;
        return result;
    }

    auto result = fetchInternal(sizeof(OMArrayHeader) + (sizeof(void *) * *lengths));
    auto arr = (OMArrayHeader *)result;
    arr->classifierPointer = &ARRAY_TYPE;
    arr->length = *lengths;
    arr->classPointer = cls.get();
    arr->type = Reference;
    arr->dim = dim;
    // offset sizeof(OMArrayHeader) bytes
    auto arrdata = ARRAY_ACCESS(result, void *);
    for (int i = 0; i < *lengths; i++)
    {
        arrdata[i] = allocateArray(cls, lengths + 1, dim - 1);
    }
    return result;
}
void *OMMemoryManager::allocateArray(OMArrayType type, int *lengths, int dim)
{
    if (dim == 1)
    {
        int objLength;
        switch (type)
        {
        case Byte:
        case Boolean:
            objLength = 1;
            break;
        case Short:
        case Char:
            objLength = 2;
            break;
        case Int:
        case Float:
        default:
            objLength = 4;
            break;
        case Long:
        case Double:
            objLength = 8;
            break;
        case Reference:
            objLength = sizeof(void *);
            break;
        }
        auto result = fetchInternal(sizeof(OMArrayHeader) + objLength * *lengths);
        auto arr = (OMArrayHeader *)result;
        arr->classifierPointer = &ARRAY_TYPE;
        arr->length = *lengths;
        arr->type = type;
        arr->dim = 1;
        return result;
    }

    auto result = fetchInternal(sizeof(OMArrayHeader) + (sizeof(void *) * *lengths));
    auto arr = (OMArrayHeader *)result;
    arr->classifierPointer = &ARRAY_TYPE;
    arr->length = *lengths;
    arr->type = type;
    arr->dim = dim;
    // offset sizeof(OMArrayHeader) bytes
    auto arrdata = ARRAY_ACCESS(result, void *);
    for (int i = 0; i < *lengths; i++)
    {
        arrdata[i] = allocateArray(type, lengths + 1, dim - 1);
    }
    return result;
}

void OMMemoryManager::searchFromInstance(void *b)
{
    if (b != nullptr)
    {
        blockStatus[b] = true;
    }
    auto arrh = (OMArrayHeader *)b;
    if (b == nullptr)
    {
        // it's abslutely TOTAL DESTRUCTION to access this pointer!
        return;
    }
    else if (arrh->classifierPointer == &ARRAY_TYPE)
    {
        // *flat* arrays, no pointers inside it
        if (arrh->type != Reference && arrh->dim == 1)
        {
            return;
        }
        auto arr = ARRAY_ACCESS(b, void *);
        for (int i = 0; i < arrh->length; i++)
        {
            searchFromInstance(arr[i]);
        }
    }
    else
    {
        auto clazz = *((OMClass **)b);
        for (auto fi : clazz->fields)
        {
            if (fi->type == OMFieldType::BytesP && (fi->accessFlag & JVM_Acc_Static) == 0)
            {
                searchFromInstance(*(void **)OBJECT_ACCESS(b, fi->offset));
            }
        }
    }
}
void OMMemoryManager::searchFromStatic(std::shared_ptr<OMClass> cls)
{
    for (auto fi : cls->fields)
    {
        if (fi->type == OMFieldType::BytesP && (fi->accessFlag & JVM_Acc_Static))
        {
            searchFromInstance(*(void **)OBJECT_ACCESS(cls->staticFieldBlock, fi->offset));
        }
    }
}

void OMMemoryManager::deallocate(void *p)
{
    mem::allocator::tracedFreeVMData(p);
}

} // namespace openminecraft::vm::pixeltower
