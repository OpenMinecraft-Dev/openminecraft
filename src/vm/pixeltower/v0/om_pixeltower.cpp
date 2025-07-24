#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"
#include "openminecraft/log/om_log_ansi.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_checker.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
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
    mm = std::make_shared<OMMemoryManager>(interpreter, classloader);
}
OMPixelTower::~OMPixelTower()
{
}
void OMPixelTower::registerNativeFunc(std::string clazz, std::string func, std::string desc,
                                      std::function<std::any(std::any)> f)
{
    auto cls = fetchClass(clazz);
    for (auto m : cls->methods)
    {
        if (m->name == func && m->desc == desc && (m->accessFlag & JVM_Acc_Native))
        {
            nativeFuncs[fmt::format("{}.{}{}", clazz, func, desc)] = f;
            return;
        }
    }

    throw err::OMValidationError{err::Instructions,
                                 fmt::format("registering native function to a non-native function!"),
                                 fmt::format("{}.{}{}", clazz, func, desc)};
}
std::any OMPixelTower::callNativeFunc(std::string clazz, std::string func, std::string desc, std::any args)
{
    auto key = fmt::format("{}.{}{}", clazz, func, desc);
    if (nativeFuncs.count(key))
    {
        return nativeFuncs[key](args);
    }

    throw err::OMValidationError{err::Instructions, fmt::format("native function not found"),
                                 fmt::format("{}.{}{}", clazz, func, desc)};
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
void OMPixelTower::loadClass(std::istream *file)
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
    std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(interpreter)->execute(fetchClass(clazz), name, desc);
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
