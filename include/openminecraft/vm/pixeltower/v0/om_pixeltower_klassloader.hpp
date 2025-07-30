#ifndef OM_PIXELTOWER_KLASSLOADER_HPP
#define OM_PIXELTOWER_KLASSLOADER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include <memory>
#include <vector>
namespace openminecraft::vm::pixeltower::v0
{
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

  private:
    OMPixelTowerHeap *metaspace;
    OMPixelTowerHeap *heap;
    OMInterpreter *interpreter;
    log::OMLogger logger;
    std::vector<OMKlass *> classes;
    std::vector<std::shared_ptr<classfile::OMClassFile>> files;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif