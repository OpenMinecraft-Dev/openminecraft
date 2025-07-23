#ifndef OM_PIXELTOWER_MEMORYMANAGER_HPP
#define OM_PIXELTOWER_MEMORYMANAGER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace openminecraft::vm::pixeltower
{
#define OBJECT_ACCESS(p, off) ((void *)((uint8_t *)p + off))
#define ARRAY_ACCESS(p, t) ((t *)((uint8_t *)p + sizeof(openminecraft::vm::pixeltower::OMArrayHeader)))
class OMMemoryManager
{
  public:
    OMMemoryManager(std::any tower, std::shared_ptr<OMClassLoader> cld);
    ~OMMemoryManager();

    void *allocate(std::shared_ptr<OMClass> cls);
    void *allocateArray(std::shared_ptr<OMClass> cls, int *lengths, int dim);
    void *allocateArray(OMArrayType type, int *lengths, int dim);

    void searchFromInstance(void *b, std::vector<void *> &buf);
    void seatchFromStatic(std::shared_ptr<OMClass> cls, std::vector<void *> &buf);

    void deallocate(void *p);

    void debug();
    void compatBlocks();

  private:
    void *fetchInternal(int size);

    log::OMLogger logger;
    void *buffer;
    std::map<void *, bool, std::less<void *>> blockStatus;
    std::mutex cacheLock;
    std::shared_ptr<OMClassLoader> cld;
    std::any tower;
};
} // namespace openminecraft::vm::pixeltower

#endif
