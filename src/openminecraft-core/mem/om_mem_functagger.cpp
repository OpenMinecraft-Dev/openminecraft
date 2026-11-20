#include "openminecraft/mem/om_mem_functagger.hpp"
#include "openminecraft/log/om_log_common.hpp"

namespace openminecraft::mem::tagger
{
std::unordered_map<void *, std::string> tags;
log::OMLogger logger("FuncTagger");
void tagFunc(void *&func, std::string id)
{
    tags[func] = id;
}
} // namespace openminecraft::mem::tagger