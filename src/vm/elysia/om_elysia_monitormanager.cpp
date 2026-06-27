#include "openminecraft/vm/elysia/om_elysia_monitormanager.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include <memory>
#include <mutex>

namespace openminecraft::vm::elysia
{
OMElysiaMonitorManager::OMElysiaMonitorManager()
{
}
OMElysiaMonitorManager::~OMElysiaMonitorManager()
{
}

bool OMElysiaMonitorManager::mutexTryFetch(OMElysiaOop *oop)
{
    std::lock_guard guard(objectMutex);
    if (!objects.count(oop))
    {
        objects[oop] = std::make_pair(0, std::make_shared<std::recursive_mutex>());
    }

    ++objects[oop].first;
    return objects[oop].second->try_lock();
}
void OMElysiaMonitorManager::mutexFetch(OMElysiaOop *oop)
{
    std::lock_guard guard(objectMutex);
    if (!objects.count(oop))
    {
        objects[oop] = std::make_pair(0, std::make_shared<std::recursive_mutex>());
    }

    ++objects[oop].first;
    objects[oop].second->lock();
}
void OMElysiaMonitorManager::mutexRelease(OMElysiaOop *oop)
{
    std::lock_guard guard(objectMutex);
    auto mutex = objects[oop].second;
    --objects[oop].first;
    if (!objects[oop].first)
    {
        objects.erase(oop);
    }

    mutex->unlock();
}
} // namespace openminecraft::vm::elysia
