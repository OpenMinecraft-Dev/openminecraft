#include "openminecraft/vm/pixeltower/internal/om_pixeltower_funcs.hpp"
#include "openminecraft/mem/om_mem_functagger.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"

using namespace openminecraft::vm::pixeltower::v0;

namespace openminecraft::vm::pixeltower
{
void registerFuncs()
{
    /*mem::tagger::tagFunc(&OMPixelTower::boot, "OMPixelTower::boot");
    mem::tagger::tagFunc(&OMPixelTower::init, "OMPixelTower::int");
    mem::tagger::tagFunc(&OMPixelTower::initStreams, "OMPixelTower::initStreams");
    mem::tagger::tagFunc(&OMPixelTower::load, "OMPixelTower::load");
    mem::tagger::tagFunc(&OMPixelTower::initCurrentThread, "OMPixelTower::initCurrentThread");
    mem::tagger::tagFunc(&OMPixelTower::destroyCurrentThread, "OMPixelTower::destroyCurrentThread");
    mem::tagger::tagFunc(&OMPixelTower::createString, "OMPixelTower::createString");
    mem::tagger::tagFunc(&OMPixelTower::handleCrash, "OMPixelTower::handleCrash");
    mem::tagger::tagFunc(&OMPixelTowerHeap::allocate, "OMPixelTowerHeap::allocate");
    mem::tagger::tagFunc(&OMPixelTowerHeap::deallocate, "OMPixelTowerHeap::deallocate");
    mem::tagger::tagFunc(&OMPixelTowerHeap::debug, "OMPixelTowerHeap::debug");
    mem::tagger::tagFunc(&OMPixelTowerHeap::merge, "OMPixelTowerHeap::merge");
    mem::tagger::tagFunc(&OMPixelTowerHeap::heapBase, "OMPixelTowerHeap::heapBase");
    mem::tagger::tagFunc(&OMPixelTowerHeap::heapTop, "OMPixelTowerHeap::heapTop");
    mem::tagger::tagFunc(&OMPixelTowerHeap::ptrCompEnabled, "OMPixelTowerHeap::ptrCompEnabled");
    mem::tagger::tagFunc(&OMPixelTowerHeap::ptrSize, "OMPixelTowerHeap::ptrSize");
    mem::tagger::tagFunc(&OMPixelTowerHeap::compressPtr, "OMPixelTowerHeap::compressPtr");
    mem::tagger::tagFunc(&OMPixelTowerHeap::decompressPtr, "OMPixelTowerHeap::decompressPtr");
    mem::tagger::tagFunc(&OMPixelTowerHeap::inside, "OMPixelTowerHeap::inside");
    mem::tagger::tagFunc(&OMPixelTowerHeap::nextPtr, "OMPixelTowerHeap::nextPtr");
    mem::tagger::tagFunc(&OMPixelTowerHeap::usage, "OMPixelTowerHeap::usage");
    mem::tagger::tagFunc(&OMPixelTowerHeap::totalUsage, "OMPixelTowerHeap::totalUsage");*/
}
} // namespace openminecraft::vm::pixeltower