#ifndef OM_LOG_THREADNAME_HPP
#define OM_LOG_THREADNAME_HPP

#include <string>
#include <thread>

namespace openminecraft::log::multithraad {
void registerCurrentThreadName(std::string name);
void registerThreadName(std::string name, std::thread::id thrid);
std::string acquireThreadName(std::thread::id thrid);
void unregisterThread(std::thread::id thrid);
} // namespace openminecraft::log::multithraad

#endif