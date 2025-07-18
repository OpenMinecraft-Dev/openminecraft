#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_ansi.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_checker.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_memorymanager.hpp"
#include <any>
#include <memory>
#include <sstream>
#include <stack>
#include <typeindex>

using namespace openminecraft::log::ansi;
namespace openminecraft::vm::pixeltower
{
extern std::string ARRAY_TYPE;
OMPixelTower::OMPixelTower() : logger("OMPixelTower", this)
{
    interpreter = std::make_shared<runtime::OMInterpreter>(*this);
    classloader = std::make_shared<OMClassLoader>(interpreter);
    mm = std::make_shared<OMMemoryManager>(classloader);
}
OMPixelTower::~OMPixelTower()
{
}
void OMPixelTower::loadClass(std::shared_ptr<classfile::OMClassFile> file)
{
    auto chk = std::make_unique<bytecode::OMBytecodeChecker>(file);
    auto cons = chk->constantCheck();
    switch (cons.type)
    {
    case util::Ok:
        break;
    case util::Err:
        throw cons.unwrap_err();
    }
    // chk->detail();

    classloader->appendStagingClass(file);
}
void OMPixelTower::loadClass(std::shared_ptr<std::istream> file)
{
    auto parser = std::make_shared<classfile::OMClassFileParser>(file);
    auto clsres = parser->parse();
    switch (clsres.type)
    {
    case util::Ok: {
        loadClass(clsres.unwrap());
        break;
    }
    case util::Err: {
        throw clsres.unwrap_err();
    }
    }
}
std::shared_ptr<OMClass> OMPixelTower::fetchClass(std::string name)
{
    return classloader->forName(name);
}
void OMPixelTower::execute(std::string clazz, std::string name, std::string desc)
{
    std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(interpreter)->execute(clazz, name, desc);
}
void OMPixelTower::debugStackStatus()
{
    auto c = std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(interpreter);
    for (auto pair : c->stack)
    {
        std::stack<std::any> ds(pair.second);
        uint64_t idx = 0;

        logger.debug(log::multithread::acquireThreadName(pair.first));
        logger.debug("");
        logger.debug("Stack Details:");
        while (!ds.empty())
        {
            auto m = fmt::format("{2}[{0}] {1}", idx, printAny(ds.top(), 0), OMLogAnsiBlueLight);
            std::istringstream s(m);
            std::string line;
            while (std::getline(s, line, '\n'))
            {
                logger.debug(line);
            }

            idx++;
            ds.pop();
        }
    }
}
std::string OMPixelTower::fetchType(void *block)
{
    if (block == nullptr)
    {
        return "???";
    }
    else if (((OMArrayHeader *)block)->classifierPointer == &ARRAY_TYPE)
    {
        auto header = ((OMArrayHeader *)block);
        std::string pre = "";
        for (int i = 0; i < header->dim; i++)
        {
            pre += "[]";
        }
        switch (header->type)
        {
        case Byte:
            return "byte" + pre;
        case Char:
            return "char" + pre;
        case Short:
            return "short" + pre;
        case Int:
            return "int" + pre;
        case Float:
            return "float" + pre;
        case Long:
            return "long" + pre;
        case Double:
            return "double" + pre;
        case Boolean:
            return "boolean" + pre;
        case Reference:
            return header->classPointer->name + pre;
        default:
            break;
        }

        return "<array>";
    }
    else
    {
        return (*((OMClass **)block))->name;
    }
}
std::string OMPixelTower::printInstanceData(void *block, int layer, bool isArray)
{
    auto pref = isArray ? "" : std::string(layer * 4, ' ');
    auto arr = (OMArrayHeader *)block;
    if (block == nullptr)
    {
        return pref + fmt::format("{}> ???{}", OMLogAnsiBlackLight, OMLogAnsiReset);
    }
    else if (arr->classifierPointer == &ARRAY_TYPE)
    {
        std::string target = "";
        target += pref + fmt::format("> length: {}\n", arr->length);
        for (int i = 0; i < arr->length; i++)
        {
            target += pref + fmt::format("> {}[{}]{} ", OMLogAnsiBlueLight, i, OMLogAnsiReset);
            switch (arr->type)
            {
            case Byte:
            case Char:
                target += printAny(ARRAY_ACCESS(block, char)[i], layer, true);
                break;
            case Short:
                target += printAny(ARRAY_ACCESS(block, short)[i], layer, true);
                break;
            case Int:
                target += printAny(ARRAY_ACCESS(block, int)[i], layer, true);
                break;
            case Long:
                target += printAny(ARRAY_ACCESS(block, int64_t)[i], layer, true);
                break;
            case Boolean:
                target += printAny((bool)ARRAY_ACCESS(block, char)[i], layer, true);
                break;
            case Double:
                target += printAny(ARRAY_ACCESS(block, double)[i], layer, true);
                break;
            case Float:
                target += printAny(ARRAY_ACCESS(block, float)[i], layer, true);
                break;
            case Reference:
                target += printAny(ARRAY_ACCESS(block, void *)[i], layer, true);
                break;
            }
            target += "\n";
        }
        return target;
    }

    std::string target;

    auto f = (*(OMClass **)block)->fields;

    for (auto field : f)
    {
        if ((field->accessFlag & JVM_Acc_Static) == 0)
        {
            std::string item;
            switch (field->desc[0])
            {
            case 'B':
            case 'C':
                item = printAny(*(char *)OBJECT_ACCESS(block, field->offset), layer + 1, true);
                break;
            case 'S':
                item = printAny(*(short *)OBJECT_ACCESS(block, field->offset), layer + 1, true);
                break;
            case 'I':
                item = printAny(*(int *)OBJECT_ACCESS(block, field->offset), layer + 1, true);
                break;
            case 'J':
                item = printAny(*(int64_t *)OBJECT_ACCESS(block, field->offset), layer + 1, true);
                break;
            case 'Z':
                item = printAny(*(bool *)OBJECT_ACCESS(block, field->offset), layer + 1, true);
                break;
            case 'F':
                item = printAny(*(float *)OBJECT_ACCESS(block, field->offset), layer + 1, true);
                break;
            case 'D':
                item = printAny(*(double *)OBJECT_ACCESS(block, field->offset), layer + 1, true);
                break;
            default:
                item = printAny(*(void **)OBJECT_ACCESS(block, field->offset), layer + 1, true);
                break;
            }
            target +=
                pref + fmt::format("> field {2}{0}{3}: {1}\n", field->name, item, OMLogAnsiCyanLight, OMLogAnsiReset);
        }
    }

    return target;
}
std::string OMPixelTower::printAny(std::any data, int layer, bool isArray)
{
    auto test = isArray ? "" : std::string(layer * 4, ' ');
    auto target = std::type_index(data.type());
    if (target == std::type_index(typeid(std::shared_ptr<runtime::OMFrameMetadata>)))
    {
        auto frame = std::any_cast<std::shared_ptr<runtime::OMFrameMetadata>>(data);
        auto temp = test + fmt::format("{7}frame metadata {4}{0}{7}.{1}{5}{2}{7} + {6}{3}{7}\n", frame->clazz->name,
                                       frame->method->name, frame->method->desc, frame->offset, OMLogAnsiCyanLight,
                                       OMLogAnsiBlackLight, OMLogAnsiYellowLight, OMLogAnsiReset);
        temp += test + "Next bytecodes:\n";
        for (int i = 0; i < 4; i++)
        {
            bool overflow = frame->codeLength - frame->offset <= i;
            temp.append(test + fmt::format("{0}{1:#04x}{2} ", overflow ? OMLogAnsiBlackLight : OMLogAnsiYellowLight,
                                           frame->codePointer[frame->offset + i], OMLogAnsiReset));
        }
        temp += test + "...\nLocals:\n";
        uint64_t i = 0;
        bool isStatic = frame->method->accessFlag & JVM_Acc_Static;
        for (auto &l : frame->local)
        {
            temp += test + fmt::format("* {2}[{0}] {1}\n", (!isStatic && !i) ? "this" : fmt::format("{}", i),
                                       printAny(l, layer + 1, true), OMLogAnsiBlueLight);
            i++;
        }
        return temp;
    }
    else if (target == std::type_index(typeid(void *)))
    {
        return test + fmt::format("{2}instance at {1}{0} {2}with type {4}{3}{2}\n{5}", std::any_cast<void *>(data),
                                  OMLogAnsiRedLight, OMLogAnsiReset, fetchType(std::any_cast<void *>(data)),
                                  OMLogAnsiCyanLight, printInstanceData(std::any_cast<void *>(data), layer, false));
    }
    else if (target == std::type_index(typeid(int)))
    {
        return test + fmt::format("{}{}{}", OMLogAnsiGreenLight, std::any_cast<int>(data), OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(bool)))
    {
        return test + fmt::format("{}{}{}", OMLogAnsiGreenLight, std::any_cast<bool>(data), OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(short)))
    {
        return test + fmt::format("{}{}{}", OMLogAnsiGreenLight, std::any_cast<short>(data), OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(char)))
    {
        return test + fmt::format("{}{}{}", OMLogAnsiGreenLight, (int)std::any_cast<char>(data), OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(int64_t)))
    {
        return test + fmt::format("{}{}{}", OMLogAnsiGreenLight, std::any_cast<int64_t>(data), OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(float)))
    {
        return test + fmt::format("{}{}{}", OMLogAnsiGreenLight, std::any_cast<float>(data), OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(double)))
    {
        return test + fmt::format("{}{}{}", OMLogAnsiGreenLight, std::any_cast<double>(data), OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(void)))
    {
        return test + fmt::format("{0}nothing{1}", OMLogAnsiBlackLight, OMLogAnsiReset);
    }
    else
    {
        return test + fmt::format("{1}??? with descriptor {0}{2}", target.name(), OMLogAnsiBlackLight, OMLogAnsiReset);
    }
}
void *OMPixelTower::allocate(std::shared_ptr<OMClass> cls)
{
    return mm->allocate(cls);
}

void *OMPixelTower::allocateArray(std::shared_ptr<OMClass> cls, int length)
{
    return mm->allocateArray(cls, &length, 1);
}
void *OMPixelTower::allocateMultiArray(std::shared_ptr<OMClass> cls, int *lengths, int dim)
{
    return mm->allocateArray(cls, lengths, dim);
}
void *OMPixelTower::allocateArray(OMArrayType type, int length)
{
    return mm->allocateArray(type, &length, 1);
}
void *OMPixelTower::allocateMultiArray(OMArrayType type, int *lengths, int dim)
{
    return mm->allocateArray(type, lengths, dim);
}
} // namespace openminecraft::vm::pixeltower