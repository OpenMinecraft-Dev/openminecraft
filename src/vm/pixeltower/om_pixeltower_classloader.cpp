#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include <memory>

using namespace openminecraft::vm::classfile;
using namespace openminecraft::binary::hash;

namespace openminecraft::vm::pixeltower
{
OMClassLoader::OMClassLoader() : logger("pixeltower/OMClassLoader", this)
{
}
OMClassLoader::~OMClassLoader()
{
}
util::OMResult<std::any, err::OMValidationError> OMClassLoader::loadClass(std::shared_ptr<OMClassFile> file)
{
    auto clsdata = std::make_shared<OMClass>();
    clsdata->name = file->mapping[file->mapping[file->thisClass]->to<OMClassConstantClass>()->nameIndex]
                        ->to<OMClassConstantUtf8>()
                        ->data;
    if (file->superClass != 0)
    {
        auto superClassResult =
            forName(file->mapping[file->mapping[file->superClass]->to<OMClassConstantClass>()->nameIndex]
                        ->to<OMClassConstantUtf8>()
                        ->data);
        if (superClassResult.type == util::Err)
        {
            return util::OMResult<std::any, err::OMValidationError>::err(superClassResult.unwrap_err());
        }
        clsdata->superClass = superClassResult.unwrap();
    }
    if (file->superClass == 0 && clsdata->name != "java/lang/Object")
    {
        logger.warn("class {} doesn't have its super class!", clsdata->name);
    }
    for (auto inter : file->interfaces)
    {
        auto interfaceResult = forName(file->mapping[file->mapping[inter]->to<OMClassConstantClass>()->nameIndex]
                                           ->to<OMClassConstantUtf8>()
                                           ->data);
        if (interfaceResult.type == util::Err)
        {
            return util::OMResult<std::any, err::OMValidationError>::err(interfaceResult.unwrap_err());
        }
        clsdata->interfaces.push_back(interfaceResult.unwrap());
    }

    for (auto field : file->fields)
    {
        auto fn = file->mapping[field->nameIndex]->to<OMClassConstantUtf8>();
        auto fd = file->mapping[field->descIndex]->to<OMClassConstantUtf8>();
        auto m = fd->data;
        OMFieldInfo f;
        f.accessFlag = field->accessFlags;
        f.name = m;
        f.rawDesc = fd->data;
        while (m[0] == '[')
        {
            m = std::string(m.c_str()).substr(1);
        }
        switch (m[0])
        {
        case 'Z':
            f.type = Boolean;
            break;
        case 'I':
            f.type = Int;
            break;
        case 'J':
            f.type = Long;
            break;
        case 'B':
            f.type = Byte;
            break;
        case 'S':
            f.type = Short;
            break;
        case 'F':
            f.type = Float;
            break;
        case 'D':
            f.type = Double;
            break;
        case 'C':
            f.type = Char;
            break;
        case 'L': {
            f.type = Reference;
            int temp = 0;
            auto res = bytecode::descriptor::decodeType(m, &temp);
            if (res.type == util::Err)
            {
                return util::OMResult<std::any, err::OMValidationError>::err(
                    {err::ClassLoader, "descriptor parsing error", res.unwrap_err()});
            }
            auto d = forName(res.unwrap());
            if (d.type == util::Err)
            {
                return util::OMResult<std::any, err::OMValidationError>::err(d.unwrap_err());
            }
            f.ref = d.unwrap();
            break;
        }
        default:
            return util::OMResult<std::any, err::OMValidationError>::err(
                {err::ClassLoader, "unknown field descriptor", fn->data});
        }
        clsdata->fields.emplace_back(f);
    }

    for (auto attrs : file->attrs)
    {
        if (attrs->type() == SourceFile)
        {
            clsdata->sourceFile =
                file->mapping[attrs->to<OMClassAttrSourceFile>()->sourcefileIndex]->to<OMClassConstantUtf8>()->data;
        }
    }
    clsdata->calcFieldOffsets();
    classes[hash_compile_time(clsdata->name.c_str())] = clsdata;
    logger.info("{} loaded", clsdata->name);
    return util::OMResult<std::any, err::OMValidationError>::ok(nullptr);
}
util::OMResult<std::shared_ptr<OMClass>, err::OMValidationError> OMClassLoader::forName(std::string name)
{
    logger.info("trying load {}", name);

    if (name[0] == '[')
    {
        return forName(std::string(name.c_str()).substr(1));
    }

    if (classes.count(hash_compile_time(name.c_str())))
    {
    find_class:
        return util::OMResult<std::shared_ptr<OMClass>, err::OMValidationError>::ok(
            classes[hash_compile_time(name.c_str())]);
    }
    else // try to load the class from staging area!
    {
        std::shared_ptr<OMClassFile> cls;
        for (auto sc : stagingClasses)
        {
            if (sc->mapping[sc->mapping[sc->thisClass]->to<OMClassConstantClass>()->nameIndex]
                    ->to<OMClassConstantUtf8>()
                    ->data == name)
            {
                cls = sc;
                goto subclass_loading;
            }
        }
        throw nullptr;
        return util::OMResult<std::shared_ptr<OMClass>, err::OMValidationError>::err(
            {err::ClassLoader, "class not found", name});
    subclass_loading:
        stagingClasses.remove(cls);
        auto result = loadClass(cls);
        switch (result.type)
        {
        case util::Ok:
            goto find_class;
        case util::Err:
            return util::OMResult<std::shared_ptr<OMClass>, err::OMValidationError>::err(result.unwrap_err());
        }
    }
}
void OMClassLoader::appendStagingClass(std::shared_ptr<classfile::OMClassFile> file)
{
    stagingClasses.push_front(file);
}
} // namespace openminecraft::vm::pixeltower