#ifndef OM_PIXELTOWER_CLASSLOADER_HPP
#define OM_PIXELTOWER_CLASSLOADER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/heap/om_heap_tree.hpp"
#include "openminecraft/vm/pixeltower/clazz/om_pixeltower_class.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_type.hpp"
#include <list>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

namespace openminecraft::vm::pixeltower
{
class OMClassLoader
{
  public:
    OMClassLoader();
    ~OMClassLoader();

    void loadClass(std::shared_ptr<vm::classfile::OMClassFile> f);
    void loadBasicClasses(heap::OMHeapTree mem);
    bool classLoaded(std::string name);
    bool isNative(std::string name);
    uint64_t typeLength(std::string name);
    std::shared_ptr<OMClass> fetchClass(std::string name);
    std::shared_ptr<OMNativeObjectType> fetchNativeClass(std::string name);

  private:
    log::OMLogger logger;
    std::unordered_map<std::string, std::shared_ptr<vm::classfile::OMClassFile>> classfiles;
    std::unordered_map<std::string, std::shared_ptr<OMClass>> classes;
    std::unordered_map<std::string, std::shared_ptr<OMNativeObjectType>> loadedNativeClasses;
};
} // namespace openminecraft::vm::pixeltower

#endif