#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include "openminecraft/log/om_log_ansi.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"

using namespace openminecraft::util;
using namespace openminecraft::log::ansi;

namespace openminecraft::vm::pixeltower::runtime
{
OMInterpreter::OMInterpreter(OMPixelTower &tower) : tower(tower), logger("pixeltower/OMInterpreter", this)
{
}
OMInterpreter::~OMInterpreter()
{
}
OMResult<std::any, err::OMValidationError> OMInterpreter::execute(std::string clazz, std::string method,
                                                                  std::string sig)
{
    auto cls = tower.fetchClass(clazz);
    if (cls.type == Err)
    {
        return OMResult<std::any, err::OMValidationError>::err(cls.unwrap_err());
    }
    return execute(cls.unwrap(), method, sig);
}
OMResult<std::any, err::OMValidationError> OMInterpreter::execute(std::shared_ptr<OMClass> clazz, std::string method,
                                                                  std::string sig)
{
    for (auto &m : clazz->methods)
    {
        if (m.name == method && m.desc == sig)
        {
            return execute(clazz, m);
        }
    }
    return OMResult<std::any, err::OMValidationError>::err(
        {err::ClassLoader, "method not found", fmt::format("{}.{}{}", clazz->name, method, sig)});
}
OMResult<std::any, err::OMValidationError> OMInterpreter::execute(std::shared_ptr<OMClass> clazz, OMMethodInfo &mi)
{
    logger.info("Execute {3}{0}{5}.{1}{4}{2}{5} !", clazz->name, mi.name, mi.desc, OMLogAnsiCyanLight,
                OMLogAnsiBlackLight, OMLogAnsiReset);
    return OMResult<std::any, err::OMValidationError>::ok(nullptr);
}
} // namespace openminecraft::vm::pixeltower::runtime