#include "openminecraft/vm/bytecode/om_bytecode_checker.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <memory>

namespace openminecraft::vm::bytecode
{
OMBytecodeChecker::OMBytecodeChecker(std::shared_ptr<classfile::OMClassFile> cls) : cls(cls)
{
    logger = std::make_unique<log::OMLogger>("OMBytecodeChecker", this);
}

void OMBytecodeChecker::check()
{
    for (auto m : cls->methods)
    {
        logger->info("{}", m->nameIndex);
        if (m->attrs.empty())
        {
            continue;
        }
        auto attr = m->attrs[0]->to<classfile::OMClassAttrCode>();
        for (auto b : attr->code)
        {
            logger->info("{0:#x}", b);
        }
    }
}
} // namespace openminecraft::vm::bytecode