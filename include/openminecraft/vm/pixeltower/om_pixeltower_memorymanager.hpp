#ifndef OM_PIXELTOWER_MEMORYMANAGER_HPP
#define OM_PIXELTOWER_MEMORYMANAGER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include <memory>
#include <vector>

namespace openminecraft::vm::pixeltower
{
#define OBJECT_ACCESS(p) ((void *)((uint8_t *)p + sizeof(void *)))
#define ARRAY_ACCESS(p, t) ((t *)((uint8_t *)p + sizeof(openminecraft::vm::pixeltower::OMArrayHeader)))
class OMMemoryManager
{
  public:
    OMMemoryManager(std::shared_ptr<OMClassLoader> cld);
    ~OMMemoryManager();

    void *allocate(std::shared_ptr<OMClass> cls);
    void *allocateArray(std::shared_ptr<OMClass> cls, int *lengths, int dim);
    void *allocateArray(OMArrayType type, int *lengths, int dim);

    void searchFromInstance(void *b);
    void seatchFromStatic(std::shared_ptr<OMClass> cls);

  private:
    log::OMLogger logger;
    std::vector<void *> blockCache;
    std::shared_ptr<OMClassLoader> cld;
};
} // namespace openminecraft::vm::pixeltower

#endif