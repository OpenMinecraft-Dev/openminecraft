#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"

namespace openminecraft::vm::elysia
{
OMElysiaVirtualWorld::OMElysiaVirtualWorld()
    : metaspaceHeap("elysia_metaspace", 1024 * 1024 * 16), mainHeap("elysia_main", 1024 * 1024 * 1024)
{
}
OMElysiaVirtualWorld::~OMElysiaVirtualWorld()
{
}
} // namespace openminecraft::vm::elysia
