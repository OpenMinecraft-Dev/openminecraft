#ifndef OM_PIXELTOWER_KLASSLOADER_HPP
#define OM_PIXELTOWER_KLASSLOADER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include "openminecraft/vm/pixeltower/v3/om_pixeltower_validator.hpp"
#include <any>
#include <memory>
#include <unordered_map>
#include <vector>
namespace openminecraft::vm::pixeltower::v0
{
// geopelia: just some random magic number for unsatisfied native functions
#define nullFunction (void *)0x33550336
class OMKlassLoader
{
  public:
    OMKlassLoader(OMPixelTowerHeap *heap, OMPixelTowerHeap *metaspace, OMInterpreter *interpreter);
    ~OMKlassLoader();

    void stagClass(std::shared_ptr<classfile::OMClassFile> file)
    {
        files.push_back(file);
    }
    void loadClass(std::string name);
    void loadSpecialClass(std::string name);
    void classInit(OMKlass *klass);
    OMKlass *fetchClass(std::string name);

    OMKlass *lazyClassInit(OMKlass *klass, uint16_t id);
    OMField *lazyFieldInit(OMKlass *klass, uint16_t id);
    OMMethod *lazyMethodInit(OMKlass *klass, uint16_t id);

    std::unordered_map<std::string, std::any (*)(OMPixelTower *, std::any *)> nativeMethods;
    std::vector<OMKlass *> classes;

  private:
    OMPixelTowerHeap *metaspace;
    OMPixelTowerHeap *heap;
    OMInterpreter *interpreter;
    v3::OMValidator validator;
    log::OMLogger logger;
    std::vector<std::shared_ptr<classfile::OMClassFile>> files;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif