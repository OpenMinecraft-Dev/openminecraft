#ifndef OM_ELYSIUM_HPP
#define OM_ELYSIUM_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

namespace openminecraft::vm::elysia
{
class OMElysiaKlassloader;
class OMElysiaOopManager;
struct OMElysiaOop;
struct OMElysiaNativeHandle;
namespace executor
{
class OMElysiaExecutorZero;
}

#define registerNative(n) nativeFuncMap[#n] = reinterpret_cast<void *>(&n);

class OMElysium
{
  public:
    OMElysium();
    ~OMElysium();

    void setupThreadObject();
    void startThread(OMElysiaNativeHandle *thread);

    OMElysiaHeap metaspaceHeap;
    OMElysiaHeap mainHeap;

    std::shared_ptr<OMElysiaKlassloader> klassLoader;
    std::shared_ptr<OMElysiaOopManager> oopManager;
    std::shared_ptr<executor::OMElysiaExecutorZero> executor;

    std::unordered_map<std::string, void *> nativeFuncMap;

    std::vector<std::shared_ptr<std::thread>> threads;

    struct
    {
        OMElysiaNativeHandle *systemGroup = nullptr;
        OMElysiaNativeHandle *mainGroup = nullptr;
    } threadObjects;

  private:
    log::OMLogger logger;

    std::thread *mainThread;
};
} // namespace openminecraft::vm::elysia

#endif
