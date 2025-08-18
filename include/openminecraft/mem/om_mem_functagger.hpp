#ifndef OM_MEM_FUNCTAGGER_HPP
#define OM_MEM_FUNCTAGGER_HPP

#include <string>
#include <unordered_map>
namespace openminecraft::mem::tagger
{
extern std::unordered_map<void *, std::string> tags;
void tagFunc(void *&func, std::string id);
template <typename T> void tagFunc(T func, std::string id)
{
    tagFunc(reinterpret_cast<void *&>(func), id);
}
} // namespace openminecraft::mem::tagger

#endif