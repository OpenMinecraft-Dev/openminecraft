#ifndef OM_PIXELTOWER_HPP
#define OM_PIXELTOWER_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klassloader.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include "openminecraft/vm/pixeltower/v1/om_pixeltower_tracing.hpp"
#include <istream>
#include <unordered_map>
#include <vector>
namespace openminecraft::vm::pixeltower::v2
{
class OMGarbageCollector;
}

namespace openminecraft::vm::pixeltower::v0
{
class OMPixelTower
{
  public:
    OMPixelTower();
    ~OMPixelTower();

    void init(std::string basePath);
    void load(std::string path);
    void init(std::vector<std::shared_ptr<std::istream>> &streams);
    void boot(OMMethod *method);
    OMKlassLoader *loader;
    OMPixelTowerHeap *heap;
    OMPixelTowerHeap *metaspace;
    v2::OMGarbageCollector *gc;
    OMInterpreter *interpreter;

    void initCurrentThread(uint64_t tlsSize);
    void destroyCurrentThread();

    void handleCrash(int code, int pid, std::vector<v1::tracing::OMTracingFrame> &frames);

    void *createString(std::string str);

    std::unordered_map<std::thread::id, OMPixelTowerThread *> threadMap;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif