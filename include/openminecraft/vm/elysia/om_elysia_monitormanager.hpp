#ifndef OM_ELYSIA_MONITORMANAGER_HPP
#define OM_ELYSIA_MONITORMANAGER_HPP

#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
namespace openminecraft::vm::elysia
{
class OMElysiaOop;
class OMElysiaMonitorManager
{
  public:
    OMElysiaMonitorManager();
    ~OMElysiaMonitorManager();

    void mutexFetch(OMElysiaOop *oop);
    void mutexRelease(OMElysiaOop *oop);

  private:
    std::mutex objectMutex;
    std::unordered_map<OMElysiaOop *, std::pair<uint64_t, std::shared_ptr<std::recursive_mutex>>> objects;
};
} // namespace openminecraft::vm::elysia

#endif
