#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_checker.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include <any>
#include <memory>
namespace openminecraft::vm::pixeltower
{
OMPixelTower::OMPixelTower()
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
} // namespace openminecraft::vm::pixeltower