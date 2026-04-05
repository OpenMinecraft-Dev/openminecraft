#ifndef OM_ELYSIA_VIRTUALWORLD_HPP
#define OM_ELYSIA_VIRTUALWORLD_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
#include <memory>

namespace openminecraft::vm::elysia
{
class OMElysiaKlassloader;
class OMElysiaOopManager;
namespace executor
{
class OMElysiaExecutorZero;
}

class OMElysiaVirtualWorld
{
  public:
    OMElysiaVirtualWorld();
    ~OMElysiaVirtualWorld();

    OMElysiaHeap metaspaceHeap;
    OMElysiaHeap mainHeap;

    std::shared_ptr<OMElysiaKlassloader> klassLoader;
    std::shared_ptr<OMElysiaOopManager> oopManager;
    std::shared_ptr<executor::OMElysiaExecutorZero> executor;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia

#endif
