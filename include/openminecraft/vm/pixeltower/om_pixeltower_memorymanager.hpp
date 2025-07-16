#ifndef OM_PIXELTOWER_MEMORYMANAGER_HPP
#define OM_PIXELTOWER_MEMORYMANAGER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include <any>
#include <istream>
#include <memory>

namespace openminecraft::vm::pixeltower
{
class OMMemoryManager
{
  public:
    OMMemoryManager();
    ~OMMemoryManager();

    void *allocate(std::shared_ptr<OMClass> cls);
    void *allocateArray(std::shared_ptr<OMClass> cls, int *lengths, int dim);
    void *allocateArray(OMArrayType type, int *lengths, int dim);
};
} // namespace openminecraft::vm::pixeltower

#endif