#ifndef OM_ELYSIA_VIRTUALWORLD_HPP
#define OM_ELYSIA_VIRTUALWORLD_HPP

#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
namespace openminecraft::vm::elysia
{
class OMElysiaVirtualWorld
{
  public:
    OMElysiaVirtualWorld();
    ~OMElysiaVirtualWorld();

  private:
    OMElysiaHeap metaspaceHeap;
    OMElysiaHeap mainHeap;
};
} // namespace openminecraft::vm::elysia

#endif
