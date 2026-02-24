#ifndef OM_ZLIB_INFLATE_HPP
#define OM_ZLIB_INFLATE_HPP

#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "zlib.h"
#include <cstdint>
#include <functional>
namespace openminecraft::specs::zlib
{
constexpr const char allocatorTag[] = "zlib";

class OMZLibInflater
{
  public:
    OMZLibInflater(std::function<void(uint8_t *, uint64_t)> dataAcceptor);
    ~OMZLibInflater();

    void input(uint8_t *src, uint64_t length);

  private:
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> buffer;

    std::function<void(uint8_t *, uint64_t)> dataAcceptor;
    z_stream strm;
};
} // namespace openminecraft::specs::zlib

#endif
