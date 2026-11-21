#include "openminecraft/compiler/gsl/om_compiler_tokeniter_gsl.hpp"
#include "openminecraft/compiler/gsl/om_compiler_token_gsl.hpp"
#include <memory>
#include <unordered_map>

namespace openminecraft::compiler::gsl
{
std::unordered_map<std::string, std::string> fileCache;
void storeCache(std::string path, std::string content)
{
    fileCache[path] = content;
}
auto loadCache(std::string path) -> std::string
{
    return fileCache[path];
}
auto OMGslTokenIter::next() -> std::shared_ptr<OMGslToken>
{
    auto res = std::make_shared<OMGslToken>(ConstantLiteral, (*current).get_value().c_str());
    ++current;
    return res;
}
auto OMGslTokenIter::end() -> bool
{
    return current == final;
}
} // namespace openminecraft::compiler::gsl