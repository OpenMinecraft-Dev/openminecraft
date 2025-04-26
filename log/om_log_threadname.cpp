#include <map>
#include <openminecraft/log/om_log_threadname.hpp>
#include <sstream>
#include <thread>

namespace openminecraft::log::multithraad
{
static std::map<std::thread::id, std::string> threadNameMap;

void registerCurrentThreadName(std::string name)
{
    registerThreadName(name, std::this_thread::get_id());
}

void registerThreadName(std::string name, std::thread::id thrid)
{
    unregisterThread(thrid);
    threadNameMap.insert(std::pair<std::thread::id, std::string>(thrid, name));
}

std::string acquireThreadName(std::thread::id thrid)
{
    if (threadNameMap.find(thrid) != threadNameMap.end())
    {
        return threadNameMap[thrid];
    }

    std::ostringstream d;
    d << "0x" << std::hex << thrid;
    return d.str();
}

void unregisterThread(std::thread::id thrid)
{
    if (threadNameMap.find(thrid) != threadNameMap.end())
    {
        threadNameMap.erase(thrid);
    }
}
} // namespace openminecraft::log::multithraad
