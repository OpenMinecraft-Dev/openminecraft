#ifndef OM_PIXELTOWER_HPP
#define OM_PIXELTOWER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_memorymanager.hpp"
#include <any>
#include <istream>
#include <memory>
namespace openminecraft::vm::pixeltower
{
class OMPixelTower
{
  public:
    OMPixelTower();
    ~OMPixelTower();

    void loadClass(std::shared_ptr<classfile::OMClassFile> file);
    void loadClass(std::shared_ptr<std::istream> file);
    std::shared_ptr<OMClass> fetchClass(std::string name);

    void execute(std::string clazz, std::string name, std::string desc);

    void *allocate(std::shared_ptr<OMClass> cls);
    void *allocateArray(std::shared_ptr<OMClass> cls, int length);
    void *allocateMultiArray(std::shared_ptr<OMClass> cls, int *lengths, int dim);
    void *allocateArray(OMArrayType type, int length);
    void *allocateMultiArray(OMArrayType type, int *lengths, int dim);

    void debugStackStatus();
    std::string printAny(std::any data);
    std::string printInstanceData(void *block);
    std::string fetchType(void *block);
    std::any interpreter;
    std::shared_ptr<OMMemoryManager> mm;
    std::shared_ptr<OMClassLoader> classloader;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower

#endif