#ifndef OM_HEAP_TREE_HPP
#define OM_HEAP_TREE_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <any>
#include <cstdint>
#include <unordered_map>
#include <vector>
namespace openminecraft::vm::heap
{
constexpr uint64_t heapRoot = 0;
class OMHeapTree
{
  public:
    OMHeapTree();
    ~OMHeapTree();
    uint64_t allocateId();
    bool allocate(uint64_t id, uint64_t length);
    bool attach(uint64_t from, uint64_t to);
    bool detach(uint64_t from, uint64_t to);
    void checkUnreachable(std::vector<uint64_t> *target, int id = 0);
    void deconstructUnreachable();

    void *operator[](int i);
    std::unordered_map<uint64_t, std::any> externalData;

  private:
    void deconstruct(uint64_t id);
    std::unordered_map<uint64_t, void *> data;
    std::unordered_map<uint64_t, std::vector<uint64_t>> refs;
    log::OMLogger logger;
};
}; // namespace openminecraft::vm::heap

#endif
