#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/stdlib/om_stdlib_object.hpp"
#include <memory>

namespace openminecraft::vm::pixeltower
{
OMClassLoader::OMClassLoader() : logger("pixeltower/OMClassLoader", this)
{
}
OMClassLoader::~OMClassLoader()
{
}
void OMClassLoader::loadBasicClasses()
{
    loadedNativeClasses["java/lang/Object"] = std::make_shared<stdlib::java::lang::Object>();
}
void OMClassLoader::loadClass(std::shared_ptr<vm::classfile::OMClassFile> f)
{
    auto name = f->mapping[f->mapping[f->thisClass]->to<classfile::OMClassConstantClass>()->nameIndex]
                    ->to<classfile::OMClassConstantUtf8>()
                    ->data;
    loadedClasses[name] = f;

    for (auto field : f->fields)
    {
        if ((field->accessFlags & JVM_Acc_Static) == 0)
        {
            logger.info("{}", f->mapping[field->descIndex]->to<classfile::OMClassConstantUtf8>()->data);
        }
    }
}
bool OMClassLoader::classLoaded(std::string name)
{
    return loadedNativeClasses.count(name) || loadedClasses.count(name);
}
bool OMClassLoader::isNative(std::string name)
{
    return loadedNativeClasses.count(name);
}
std::shared_ptr<vm::classfile::OMClassFile> OMClassLoader::fetchClass(std::string name)
{
    return loadedClasses[name];
}
void OMClassLoader::invokeNative(std::string cls, std::string name, std::stack<std::any, std::list<std::any>> &stk)
{
    loadedNativeClasses[cls]->invoke(name, stk);
}
} // namespace openminecraft::vm::pixeltower