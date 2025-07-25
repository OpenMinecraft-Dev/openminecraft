#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klassloader.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"

namespace openminecraft::vm::pixeltower::v0
{
OMKlassLoader::OMKlassLoader() : logger("OMKlassLoader", this)
{
}
OMKlassLoader::~OMKlassLoader()
{
    for (auto k : classes)
    {
        delete k;
    }
}

void OMKlassLoader::loadClass(std::string name)
{
    logger.debug("{} loading", name);
    if (fetchClass(name) != nullptr)
    {
        return;
    }

    for (auto fi = files.begin(); fi != files.end(); ++fi)
    {
        auto f = *fi;
        if (f->mapping[f->mapping[f->thisClass]->to<classfile::OMClassConstantClass>()->nameIndex]
                ->to<classfile::OMClassConstantUtf8>()
                ->data == name)
        {
            auto klass = new OMKlass();
            klass->name = name;
            klass->accessFlags = f->accessFlags;
            klass->raw = f;
            files.erase(fi);
            if (f->superClass != 0)
            {
                auto supClass = f->mapping[f->mapping[f->superClass]->to<classfile::OMClassConstantClass>()->nameIndex]
                                    ->to<classfile::OMClassConstantUtf8>()
                                    ->data;
                loadClass(supClass);
                klass->superClass = fetchClass(supClass);
            }
            else
            {
                klass->superClass = nullptr;
            }
            classes.push_back(klass);
            return;
        }
    }

    throw err::OMValidationError{err::ClassLoader, "class not found", name};
}

OMKlass *OMKlassLoader::fetchClass(std::string name)
{
    for (auto k : classes)
    {
        if (k->name == name)
        {
            return k;
        }
    }
    return nullptr;
}
} // namespace openminecraft::vm::pixeltower::v0