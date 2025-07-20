#include "openminecraft/vm/pixeltower/om_pixeltower_debugger.hpp"
#include "fmt/color.h"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include <any>
#include <memory>
#include <sstream>
#include <stack>
#include <typeindex>

namespace openminecraft::vm::pixeltower
{
extern std::string ARRAY_TYPE;
}

namespace openminecraft::vm::pixeltower::v1
{
OMDebugger::OMDebugger(std::shared_ptr<runtime::OMInterpreter> i)
    : interpreter(i), logger("pixeltower/OMDebugger", this)
{
}
void OMDebugger::printStack()
{
    for (auto stackItem : interpreter->stack)
    {
        std::stack<std::any> cpy(stackItem.second);

        logger.debug("Stack status for thread: {}", log::multithread::acquireThreadName(stackItem.first));
        int i = 0;
        while (!cpy.empty())
        {
            auto text = fmt::format("* {} {}", fmt::styled(fmt::format("[{}]", i), fmt::fg(fmt::color::light_blue)),
                                    serializeAny(cpy.top()));

            std::istringstream s(text);
            std::string line;
            while (std::getline(s, line, '\n'))
            {
                logger.debug(line);
            }
            cpy.pop();
            i++;
        }
    }
}
std::string OMDebugger::serializeAny(std::any data, int objDepth)
{
    auto prefix = std::string(objDepth * 4, ' ');

    auto idx = std::type_index(data.type());
    if (idx == std::type_index(typeid(int)))
    {
        return fmt::format("{}{}", prefix, fmt::styled(std::any_cast<int>(data), fmt::fg(fmt::color::light_green)));
    }
    else if (idx == std::type_index(typeid(float)))
    {
        return fmt::format("{}{}", prefix, fmt::styled(std::any_cast<float>(data), fmt::fg(fmt::color::light_green)));
    }
    else if (idx == std::type_index(typeid(double)))
    {
        return fmt::format("{}{}", prefix, fmt::styled(std::any_cast<double>(data), fmt::fg(fmt::color::light_green)));
    }
    else if (idx == std::type_index(typeid(int64_t)))
    {
        return fmt::format("{}{}", prefix, fmt::styled(std::any_cast<int64_t>(data), fmt::fg(fmt::color::light_green)));
    }
    else if (idx == std::type_index(typeid(std::shared_ptr<runtime::OMFrameMetadata>)))
    {
        auto frameMd = std::any_cast<std::shared_ptr<runtime::OMFrameMetadata>>(data);
        auto cls = std::string(frameMd->clazz->name.c_str());
        for (int i = 0; i < cls.size(); i++)
        {
            if (cls[i] == '/')
            {
                cls[i] = '.';
            }
        }
        auto base = fmt::format("{}Frame base for {}.{}{}", prefix, fmt::styled(cls, fmt::fg(fmt::color::cyan)),
                                fmt::styled(frameMd->method->name, fmt::fg(fmt::color::purple)),
                                fmt::styled(frameMd->method->desc, fmt::fg(fmt::color::gray)));

        objDepth++;
        prefix = std::string(objDepth * 4, ' ');

        base += "\n" + prefix + "Next bytecodes: \n" + prefix;
        for (uint64_t i = 0; i < 8; i++)
        {
            bool inRange = frameMd->codeLength > (i + frameMd->offset);
            base += fmt::format("{:#04x} ", fmt::styled(frameMd->codePointer[i + frameMd->offset],
                                                        fmt::fg(inRange ? fmt::color::yellow : fmt::color::gray)));
        }
        base += "\n" + prefix + "Locals:";
        int i = 0;
        for (auto local : frameMd->local)
        {
            base += fmt::format("\n{}- {}\n{}", prefix,
                                fmt::styled(fmt::format("[{}]", i), fmt::fg(fmt::color::light_blue)),
                                serializeAny(local, objDepth + 1));
            i++;
        }
        return base;
    }
    else if (idx == std::type_index(typeid(void *)))
    {
        auto baseptr = std::any_cast<void *>(data);
        auto arrh = (OMArrayHeader *)baseptr;

        if (baseptr == nullptr)
        {
            return fmt::format("{}{}", prefix, fmt::styled("null", fmt::fg(fmt::color::gray)));
        }
        if (arrh->classifierPointer == &ARRAY_TYPE)
        {
            return fmt::format("{}{}", prefix, fmt::styled("<array>", fmt::fg(fmt::color::gray)));
        }

        auto cls = *((OMClass **)baseptr);
        auto target =
            fmt::format("{}instance at {} with type {}", prefix, fmt::styled(baseptr, fmt::fg(fmt::color::red)),
                        fmt::styled(cls->name, fmt::fg(fmt::color::light_green)));
        for (auto fi : cls->fields)
        {
            if ((fi->accessFlag & JVM_Acc_Static) == 0)
            {
                target += fmt::format("\n{}field {}", prefix, fmt::styled(fi->name, fmt::fg(fmt::color::light_green)));
            }
        }
        return target;
    }
    else if (idx == std::type_index(typeid(void)))
    {
        return fmt::format("{}{}", prefix, fmt::styled("nothing", fmt::fg(fmt::color::gray)));
    }
    else
    {
        return fmt::format(
            "{}{}", prefix,
            fmt::styled(fmt::format("unknown data with descriptor {}", data.type().name()), fmt::fg(fmt::color::gray)));
    }
}
} // namespace openminecraft::vm::pixeltower::v1