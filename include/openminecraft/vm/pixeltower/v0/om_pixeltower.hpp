#ifndef OM_PIXELTOWER_HPP
#define OM_PIXELTOWER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klassloader.hpp"
#include <istream>
#include <vector>
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
    void testAppendMethod();
    OMKlassLoader *loader;
    OMPixelTowerHeap *heap;
    OMPixelTowerHeap *metaspace;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif