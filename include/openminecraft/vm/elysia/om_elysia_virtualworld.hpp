#ifndef OM_ELYSIA_VIRTUALWORLD_HPP
#define OM_ELYSIA_VIRTUALWORLD_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
namespace openminecraft::vm::elysia
{
class OMElysiaKlassloader;

class OMElysiaVirtualWorld
{
  public:
    OMElysiaVirtualWorld();
    ~OMElysiaVirtualWorld();

    OMElysiaHeap metaspaceHeap;
    OMElysiaHeap mainHeap;

  private:
    std::shared_ptr<OMElysiaKlassloader> klassLoader;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia

#endif
