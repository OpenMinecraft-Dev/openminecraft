#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"
#include "openminecraft/log/om_log_ansi.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_checker.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include <any>
#include <memory>
#include <sstream>
#include <stack>
#include <typeindex>

using namespace openminecraft::log::ansi;
namespace openminecraft::vm::pixeltower
{
OMPixelTower::OMPixelTower() : logger("OMPixelTower", this)
{
    classloader = std::make_shared<OMClassLoader>();
    interpreter = std::make_shared<runtime::OMInterpreter>(*this);
}
OMPixelTower::~OMPixelTower()
{
}
util::OMResult<std::any, err::OMValidationError> OMPixelTower::loadClass(std::shared_ptr<classfile::OMClassFile> file)
{
    auto chk = std::make_unique<bytecode::OMBytecodeChecker>(file);
    auto cons = chk->constantCheck();
    switch (cons.type)
    {
    case util::Ok:
        break;
    case util::Err:
        return util::OMResult<std::any, err::OMValidationError>::err(cons.unwrap_err());
    }
    // chk->detail();

    classloader->appendStagingClass(file);
    return util::OMResult<std::any, err::OMValidationError>::ok(nullptr);
}
util::OMResult<std::any, err::OMValidationError> OMPixelTower::loadClass(std::shared_ptr<std::istream> file)
{
    auto parser = std::make_shared<classfile::OMClassFileParser>(file);
    auto clsres = parser->parse();
    switch (clsres.type)
    {
    case util::Ok: {
        return loadClass(clsres.unwrap());
        break;
    }
    case util::Err: {
        return util::OMResult<std::any, err::OMValidationError>::err(clsres.unwrap_err());
    }
    }
}
util::OMResult<std::shared_ptr<OMClass>, err::OMValidationError> OMPixelTower::fetchClass(std::string name)
{
    return classloader->forName(name);
}
util::OMResult<std::any, err::OMValidationError> OMPixelTower::execute(std::string clazz, std::string name,
                                                                       std::string desc)
{
    return std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(interpreter)->execute(clazz, name, desc);
}
void OMPixelTower::debugStackStatus()
{
    std::stack<std::any> ds(std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(interpreter)->stack);
    uint64_t idx = 0;

    logger.debug("");
    logger.debug("Stack Details:");
    while (!ds.empty())
    {
        auto m = fmt::format("{2}[{0}] {1}", idx, printAny(ds.top()), OMLogAnsiBlueLight);
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
std::string OMPixelTower::printAny(std::any data)
{
    auto target = std::type_index(data.type());
    if (target == std::type_index(typeid(std::shared_ptr<runtime::OMFrameMetadata>)))
    {
        auto frame = std::any_cast<std::shared_ptr<runtime::OMFrameMetadata>>(data);
        auto temp = fmt::format("{7}frame metadata {4}{0}{7}.{1}{5}{2}{7} + {6}{3}{7}\n", frame->clazz->name,
                                frame->method->name, frame->method->desc, frame->offset, OMLogAnsiCyanLight,
                                OMLogAnsiBlackLight, OMLogAnsiYellowLight, OMLogAnsiReset);
        temp += "Next bytecodes:\n";
        for (int i = 0; i < 4; i++)
        {
            bool overflow = frame->codeLength - frame->offset < i;
            temp.append(fmt::format("{0}{1:#04x}{2} ", overflow ? OMLogAnsiBlackLight : OMLogAnsiYellowLight,
                                    frame->codePointer[frame->offset + i], OMLogAnsiReset));
        }
        temp += "...\nLocals:\n";
        uint64_t i = 0;
        for (auto &l : frame->local)
        {
            temp += fmt::format("\t{2}[{0}] {1}\n", i, printAny(l), OMLogAnsiBlueLight);
            i++;
        }
        return temp;
    }
    else if (target == std::type_index(typeid(void *)))
    {
        return fmt::format("{2}instance at {1}{0}{2}", std::any_cast<void *>(data), OMLogAnsiRedLight, OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(int)))
    {
        return fmt::format("{}{}{}", OMLogAnsiGreenLight, std::any_cast<int>(data), OMLogAnsiReset);
    }
    else
    {
        return fmt::format("{1}??? with descriptor {0}{2}", target.name(), OMLogAnsiBlackLight, OMLogAnsiReset);
    }
}
void *OMPixelTower::allocate(std::shared_ptr<OMClass> cls)
{
    // instance structure:
    // (pointer to the class) (data for this instance)
    auto result = mem::allocator::tracedCallocVMData(1, cls->objectLength + sizeof(void *));
    auto clsp = reinterpret_cast<OMClass *>(result);
    clsp = cls.get();
    return result;
}
} // namespace openminecraft::vm::pixeltower