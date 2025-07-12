#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/clazz/om_pixeltower_class.hpp"
#include "openminecraft/vm/pixeltower/stdlib/om_stdlib_object.hpp"
#include <bitset>
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
    classfiles[name] = f;

    classes[name] = std::make_shared<OMClass>();
    for (auto m : f->methods)
    {
        auto desc = f->mapping[m->descIndex]->to<classfile::OMClassConstantUtf8>()->data;
        auto fn = fmt::format("{}{}", f->mapping[m->nameIndex]->to<classfile::OMClassConstantUtf8>()->data, desc);
        for (auto attr : m->attrs)
        {
            if (attr->type() == classfile::OMClassAttrType::Code)
            {
                classes[name]->methods[fn] = {desc, std::bitset<16>(), std::unordered_map<uint64_t, uint64_t>(),
                                              attr->to<classfile::OMClassAttrCode>()};
                if (m->accessFlags & JVM_Acc_Public)
                {
                    classes[name]->methods[fn].flags.set(publicAccessBit);
                }
                else if (m->accessFlags & JVM_Acc_Private)
                {
                    classes[name]->methods[fn].flags.set(privateAccessBit);
                }
                else if (m->accessFlags & JVM_Acc_Protected)
                {
                    classes[name]->methods[fn].flags.set(protectedAccessBit);
                }
                else
                {
                    classes[name]->methods[fn].flags.set(defaultAccessBit);
                }
                break;
            }

            if (attr->type() == classfile::OMClassAttrType::LineNumberTable)
            {
                for (auto pair : attr->to<classfile::OMClassAttrLineNumberTable>()->lineNumberTable)
                {
                    classes[name]->methods[fn].debugMapping[pair.first] = pair.second;
                }
            }
        }
    }
    classes[name]->mapping = &f->mapping;

    for (auto attr : f->attrs)
    {
        if (attr->type() == classfile::OMClassAttrType::SourceFile)
        {
            classes[name]->source = f->mapping[attr->to<classfile::OMClassAttrSourceFile>()->sourcefileIndex]
                                        ->to<classfile::OMClassConstantUtf8>()
                                        ->data;
        }
    }
}
bool OMClassLoader::classLoaded(std::string name)
{
    return loadedNativeClasses.count(name) || classfiles.count(name);
}
bool OMClassLoader::isNative(std::string name)
{
    return loadedNativeClasses.count(name);
}
std::shared_ptr<OMClass> OMClassLoader::fetchClass(std::string name)
{
    return classes[name];
}
void OMClassLoader::invokeNative(std::string cls, std::string name, std::stack<std::any, std::list<std::any>> &stk)
{
    loadedNativeClasses[cls]->invoke(name, stk);
}
} // namespace openminecraft::vm::pixeltower