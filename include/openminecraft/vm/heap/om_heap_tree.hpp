#ifndef OM_HEAP_TREE_HPP
#define OM_HEAP_TREE_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <map>
#include <vector>
namespace openminecraft::vm::heap
{
class OMHeapTree
{
  public:
    OMHeapTree();
    ~OMHeapTree();
    void allocate(uint64_t id, uint64_t length);
    void attach(uint64_t from, uint64_t to);
    void checkUnreachable(std::vector<uint64_t> *target, int id = 0);

  private:
    std::map<uint64_t, void *> data;
    std::map<uint64_t, std::vector<uint64_t>> refs;
    log::OMLogger logger;
};
}; // namespace openminecraft::vm::heap

#endif