#include "openminecraft/vm/pixeltower/om_pixeltower_debugger.hpp"
#include "fmt/color.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_memorymanager.hpp"
#include <any>
#include <memory>
#include <sstream>
#include <stack>
#include <typeindex>

using namespace openminecraft::binary::hash;

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
#define updatePref prefix = std::string(objDepth, '\t')

    auto updatePref;

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
    else if (idx == std::type_index(typeid(uint16_t)))
    {
        return fmt::format("{}{}", prefix,
                           fmt::styled(std::any_cast<uint16_t>(data), fmt::fg(fmt::color::light_green)));
    }
    else if (idx == std::type_index(typeid(char)))
    {
        return fmt::format("{}{:#04x}", prefix,
                           fmt::styled((int)std::any_cast<char>(data), fmt::fg(fmt::color::light_green)));
    }
    else if (idx == std::type_index(typeid(bool)))
    {
        return fmt::format("{}{}", prefix, fmt::styled(std::any_cast<bool>(data), fmt::fg(fmt::color::light_green)));
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
        updatePref;

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
            base +=
                fmt::format("\n{}- {} {}", prefix, fmt::styled(fmt::format("[{}]", i), fmt::fg(fmt::color::light_blue)),
                            removeAnyPrefix(serializeAny(local, objDepth + 1)));
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
            std::string type;
            switch (arrh->type)
            {
            case Byte:
                type = "byte";
                break;
            case Char:
                type = "char";
                break;
            case Short:
                type = "short";
                break;
            case Int:
                type = "int";
                break;
            case Long:
                type = "long";
                break;
            case Boolean:
                type = "boolean";
                break;
            case Double:
                type = "double";
                break;
            case Float:
                type = "float";
                break;
            case Reference:
                type = arrh->classPointer->name;
                break;
            }
            for (int i = 0; i < arrh->dim; i++)
            {
                type += "[]";
            }
            auto target =
                fmt::format("{}array with type {}", prefix, fmt::styled(type, fmt::fg(fmt::color::light_green)));

            if (objDepth == 0)
            {
                objDepth++;
                updatePref;
                objDepth--;
            }

            for (int i = 0; i < arrh->length; i++)
            {
                target +=
                    fmt::format("\n{}{}", prefix, fmt::styled(fmt::format("[{}]", i), fmt::fg(fmt::color::light_blue)));

                if (arrh->dim > 1)
                {
                    target +=
                        fmt::format(" {}", removeAnyPrefix(serializeAny(ARRAY_ACCESS(baseptr, void *)[i], objDepth)));
                    continue;
                }

                switch (arrh->type)
                {
                case Byte:
                    target +=
                        fmt::format(" {}", removeAnyPrefix(serializeAny(ARRAY_ACCESS(baseptr, char)[i], objDepth)));
                    break;
                case Char:
                    target +=
                        fmt::format(" {}", removeAnyPrefix(serializeAny(ARRAY_ACCESS(baseptr, uint16_t)[i], objDepth)));
                    break;
                case Short:
                    target +=
                        fmt::format(" {}", removeAnyPrefix(serializeAny(ARRAY_ACCESS(baseptr, short)[i], objDepth)));
                    break;
                case Int:
                    target +=
                        fmt::format(" {}", removeAnyPrefix(serializeAny(ARRAY_ACCESS(baseptr, int)[i], objDepth)));
                    break;
                case Long:
                    target +=
                        fmt::format(" {}", removeAnyPrefix(serializeAny(ARRAY_ACCESS(baseptr, int64_t)[i], objDepth)));
                    break;
                case Boolean:
                    target +=
                        fmt::format(" {}", removeAnyPrefix(serializeAny(ARRAY_ACCESS(baseptr, bool)[i], objDepth)));
                    break;
                case Double:
                    target +=
                        fmt::format(" {}", removeAnyPrefix(serializeAny(ARRAY_ACCESS(baseptr, double)[i], objDepth)));
                    break;
                case Float:
                    target += fmt::format(" {}", removeAnyPrefix(removeAnyPrefix(
                                                     serializeAny(ARRAY_ACCESS(baseptr, float)[i], objDepth))));
                    break;
                case Reference:
                    target +=
                        fmt::format(" {}", removeAnyPrefix(serializeAny(ARRAY_ACCESS(baseptr, void *)[i], objDepth)));
                    break;
                }
            }
            return target;
        }

        auto cls = *((OMClass **)baseptr);
        auto target =
            fmt::format("{}instance at {} with type {}", prefix, fmt::styled(baseptr, fmt::fg(fmt::color::red)),
                        fmt::styled(cls->name, fmt::fg(fmt::color::light_green)));

        for (auto fi : cls->fields)
        {
            if ((fi->accessFlag & JVM_Acc_Static) == 0)
            {
                target +=
                    fmt::format("\n{}field {}: ", prefix, fmt::styled(fi->name, fmt::fg(fmt::color::light_green)));

                int temp = 0;
                auto pa = bytecode::descriptor::decodeType(fi->desc, &temp);
                if (pa.type == util::Err)
                {
                    target += fmt::format(
                        "\n{}{}", prefix,
                        fmt::styled(fmt::format("unknown descriptor {} with error {}", fi->desc, pa.unwrap_err()),
                                    fmt::fg(fmt::color::red)));
                }
                else
                {
                    objDepth++;
                    switch (hash_compile_time(pa.unwrap().c_str()))
                    {
                    case "int"_hash:
                        target += fmt::format(
                            " {}", removeAnyPrefix(serializeAny(*(int *)OBJECT_ACCESS(baseptr, fi->offset), objDepth)));
                        break;
                    case "long"_hash:
                        target += fmt::format(" {}", removeAnyPrefix(serializeAny(
                                                         *(int64_t *)OBJECT_ACCESS(baseptr, fi->offset), objDepth)));
                        break;
                    case "float"_hash:
                        target += fmt::format(" {}", removeAnyPrefix(serializeAny(
                                                         *(float *)OBJECT_ACCESS(baseptr, fi->offset), objDepth)));
                        break;
                    case "double"_hash:
                        target += fmt::format(" {}", removeAnyPrefix(serializeAny(
                                                         *(double *)OBJECT_ACCESS(baseptr, fi->offset), objDepth)));
                        break;
                    case "byte"_hash:
                        target += fmt::format(" {}", removeAnyPrefix(serializeAny(
                                                         *(char *)OBJECT_ACCESS(baseptr, fi->offset), objDepth)));
                        break;
                    case "char"_hash:
                        target += fmt::format(" {}", removeAnyPrefix(serializeAny(
                                                         *(uint16_t *)OBJECT_ACCESS(baseptr, fi->offset), objDepth)));
                        break;
                    case "boolean"_hash:
                        target += fmt::format(" {}", removeAnyPrefix(serializeAny(
                                                         *(bool *)OBJECT_ACCESS(baseptr, fi->offset), objDepth)));
                        break;
                    case "short"_hash:
                        target += fmt::format(" {}", removeAnyPrefix(serializeAny(
                                                         *(short *)OBJECT_ACCESS(baseptr, fi->offset), objDepth)));
                        break;
                    default:
                        target += fmt::format(" {}", removeAnyPrefix(serializeAny(
                                                         *(void **)OBJECT_ACCESS(baseptr, fi->offset), objDepth)));
                        break;
                    }
                    objDepth--;
                }
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

std::string OMDebugger::removeAnyPrefix(std::string s)
{
    while (s[0] == '\t')
    {
        s.replace(0, 1, "");
    }
    return s;
}
} // namespace openminecraft::vm::pixeltower::v1