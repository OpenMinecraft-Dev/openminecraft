#ifndef OM_PIXELTOWER_THREADS_HPP
#define OM_PIXELTOWER_THREADS_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include <string>
#include <thread>
namespace openminecraft::vm::pixeltower::v0
{
class OMPixelTowerThread
{
  public:
    OMPixelTowerThread() = default;
    ~OMPixelTowerThread() = default;

    std::thread::id id;
    std::string name;
    jbyte *pc;
    OMFrame *currentFrame;
    void *stack;
};

extern thread_local OMPixelTowerThread currentThread;
} // namespace openminecraft::vm::pixeltower::v0

#endif