#include "openminecraft/vm/pixeltower/om_pixeltower_memorymanager.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include <algorithm>
#include <any>
#include <cstring>
#include <memory>
#include <stack>
#include <typeindex>
#include <vector>

#define HEAP_SIZE 1024 * 128

namespace openminecraft::vm::pixeltower
{
std::string ARRAY_TYPE = "array";
OMMemoryManager::OMMemoryManager(std::any tower, std::shared_ptr<OMClassLoader> cld)
    : logger("pixeltower/OMMemoryManager", this), cld(cld), tower(tower)
{
    buffer = mem::allocator::tracedMallocVMData(HEAP_SIZE);
    blockStatus[buffer] = true;
    blockStatus[(uint8_t *)buffer + HEAP_SIZE] = false;
}
OMMemoryManager::~OMMemoryManager()
{
    mem::allocator::tracedFreeVMData(buffer);
}

void *OMMemoryManager::fetchInternal(int size)
{
alloc:
    auto siz = size + sizeof(void *) - (size % sizeof(void *));

    auto it1 = blockStatus.begin();
    for (auto it2 = blockStatus.begin();; ++it2)
    {
        ++it1;
        if (it1 == blockStatus.end())
        {
            break;
        }

        auto p = (uint8_t *)it2->first + sizeof(void *) - ((size_t)it2->first % sizeof(void *));

        if (it2->second && ((size_t)it1->first - (size_t)p) >= siz)
        {
            blockStatus[p] = false;
            blockStatus[(uint8_t *)it2->first + siz] = true;
            memset(it2->first, 0, siz);
            return p;
        }
    }

    auto inter = std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(tower);
    std::vector<void *> reachable;
    for (auto h : inter->stack)
    {
        std::stack<std::any> d(h.second);

        while (!d.empty())
        {
            if (std::type_index(d.top().type()) == std::type_index(typeid(void *)))
            {
                reachable.push_back(std::any_cast<void *>(d.top()));
            }
            else if (std::type_index(d.top().type()) ==
                     std::type_index(typeid(std::shared_ptr<runtime::OMFrameMetadata>)))
            {
                for (auto l : std::any_cast<std::shared_ptr<runtime::OMFrameMetadata>>(d.top())->local)
                {
                    if (std::type_index(l.type()) == std::type_index(typeid(void *)))
                    {
                        reachable.push_back(std::any_cast<void *>(l));
                    }
                }
            }
            d.pop();
        }
    }

    // logger.debug("{} reachable", reachable.size());

    for (auto p : blockStatus)
    {
        auto m = p.first;
        if (!p.second && m != ((uint8_t *)buffer + HEAP_SIZE) && std::count(reachable.begin(), reachable.end(), m) == 0)
        {
            deallocate(m);
        }
    }
    compatBlocks();
    // debug();
    goto alloc;
}

void OMMemoryManager::compatBlocks()
{
    if (blockStatus.size() <= 1)
    {
        throw 0;
    }

    std::vector<void *> merged;

    for (auto it = blockStatus.begin();; ++it)
    {
        ++it;
        if (it == blockStatus.end())
        {
            break;
        }
        if (!it->second)
        {
            --it;
            continue;
        }
        auto next = it->first;
        --it;
        if (it->second)
        {
            merged.push_back(next);
        }
    }

    for (auto m : merged)
    {
        blockStatus.erase(m);
    }
}

void OMMemoryManager::debug()
{
    logger.debug("HEAP MEMORY STATUS: ");
    for (auto it = blockStatus.begin(); it != blockStatus.end(); ++it)
    {
        ++it;
        auto m = it == blockStatus.end() ? nullptr : it->first;
        --it;
        logger.debug("{} ~ {} (size {}): {}", it->first, m, m == nullptr ? 0 : ((size_t)m - (size_t)it->first),
                     it->second);
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

void OMMemoryManager::searchFromInstance(void *b, std::vector<void *> &buf)
{
    if (b != nullptr)
    {
        buf.push_back(b);
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
            searchFromInstance(arr[i], buf);
        }
    }
    else
    {
        auto clazz = *((OMClass **)b);
        for (auto fi : clazz->fields)
        {
            if (fi->type == OMFieldType::BytesP && (fi->accessFlag & JVM_Acc_Static) == 0)
            {
                searchFromInstance(*(void **)OBJECT_ACCESS(b, fi->offset), buf);
            }
        }
    }
}
void OMMemoryManager::seatchFromStatic(std::shared_ptr<OMClass> cls, std::vector<void *> &buf)
{
    for (auto fi : cls->fields)
    {
        if (fi->type == OMFieldType::BytesP && (fi->accessFlag & JVM_Acc_Static))
        {
            searchFromInstance(*(void **)OBJECT_ACCESS(cls->staticFieldBlock, fi->offset), buf);
        }
    }
}

void OMMemoryManager::deallocate(void *p)
{
    if (blockStatus.count(p))
    {
        blockStatus[p] = true;
    }
}

} // namespace openminecraft::vm::pixeltower
