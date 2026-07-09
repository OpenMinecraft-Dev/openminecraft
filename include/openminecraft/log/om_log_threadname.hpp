#ifndef OM_LOG_THREADNAME_HPP
#define OM_LOG_THREADNAME_HPP

#include <string>
#include <thread>

namespace openminecraft::log::multithread
{
void registerCurrentThreadName(std::string name);
void registerThreadName(std::string name, std::thread::id thrid);
auto acquireThreadName(std::thread::id thrid) -> std::string;
void unregisterThread(std::thread::id thrid);
} // namespace openminecraft::log::multithread

#endif
