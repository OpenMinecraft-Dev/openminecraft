#include "openminecraft/compiler/gsl/om_compiler_tokeniter_gsl.hpp"

namespace openminecraft::compiler::gsl
{
auto OMGslTokenIter::next() -> std::shared_ptr<OMGslToken>
{
    return nullptr;
}
auto OMGslTokenIter::end() -> bool
{
    return false;
}
} // namespace openminecraft::compiler::gsl