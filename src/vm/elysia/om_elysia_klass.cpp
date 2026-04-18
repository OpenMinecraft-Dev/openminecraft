#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include <cstring>
#include <stdexcept>

using namespace openminecraft::vm::classfile;

namespace openminecraft::vm::elysia
{
OMElysiaMethod *OMElysiaKlass::findMethod(const char *name, const char *desc)
{
    if (!methods || !methodCount)
    {
        return nullptr;
    }

    for (int i = 0; i < methodCount; i++)
    {
        if (std::strcmp(methods[i].name, name) == 0 && std::strcmp(methods[i].descriptor, desc) == 0)
        {
            return &methods[i];
        }
    }

    return nullptr;
}

void *OMElysiaInstanceKlass::constantPoolFetch(uint16_t id)
{
    if (constantPool[id])
    {
        return constantPool[id];
    }

    auto item = constantPoolRaw->at(id);
    switch (item->type())
    {
    case OMClassConstantType::MethodRef: {
        auto mr = item->to<OMClassConstantMethodRef>();
        auto clsname = constantPoolRaw->at(constantPoolRaw->at(mr->classIndex)->to<OMClassConstantClass>()->nameIndex)
                           ->to<OMClassConstantUtf8>()
                           ->data;
        auto mdname =
            constantPoolRaw->at(constantPoolRaw->at(mr->nameAndTypeIndex)->to<OMClassConstantNameAndType>()->nameIndex)
                ->to<OMClassConstantUtf8>()
                ->data;
        auto mddesc =
            constantPoolRaw->at(constantPoolRaw->at(mr->nameAndTypeIndex)->to<OMClassConstantNameAndType>()->descIndex)
                ->to<OMClassConstantUtf8>()
                ->data;

        if (klassloader)
        {
            throw std::logic_error("not supported uplevel classloader!");
        }

        auto cls = nativeKlassloader->fetchOrLoadClass(clsname);
        auto mthd = cls->findMethod(mdname.c_str(), mddesc.c_str());

        constantPool[id] = mthd;
        return mthd;
    }
    case OMClassConstantType::FieldRef: {
        auto mr = item->to<OMClassConstantFieldRef>();
        auto clsname = constantPoolRaw->at(constantPoolRaw->at(mr->classIndex)->to<OMClassConstantClass>()->nameIndex)
                           ->to<OMClassConstantUtf8>()
                           ->data;
        auto mdname =
            constantPoolRaw->at(constantPoolRaw->at(mr->nameAndTypeIndex)->to<OMClassConstantNameAndType>()->nameIndex)
                ->to<OMClassConstantUtf8>()
                ->data;
        auto mddesc =
            constantPoolRaw->at(constantPoolRaw->at(mr->nameAndTypeIndex)->to<OMClassConstantNameAndType>()->descIndex)
                ->to<OMClassConstantUtf8>()
                ->data;

        if (klassloader)
        {
            throw std::logic_error("not supported uplevel classloader!");
        }

        for (int i = 0; i < fieldCount; i++)
        {
            if (std::strcmp(fields[i].name, mdname.c_str()) == 0 && std::strcmp(fields[i].desc, mddesc.c_str()) == 0)
            {
                constantPool[id] = &fields[i];
                return &fields[i];
            }
            return nullptr;
        }
    }
    default:
        throw 0;
    }
    return nullptr;
}

OMElysiaField *OMElysiaInstanceKlass::findField(const char *name, const char *desc)
{
    if (!name || !desc)
    {
        return nullptr;
    }

    for (int i = 0; i < fieldCount; i++)
    {
        if (std::strcmp(fields[i].name, name) == 0 && std::strcmp(fields[i].desc, desc) == 0)
        {
            return &fields[i];
        }
    }

    return nullptr;
}

void OMElysiaInstanceKlass::initFieldOffsets()
{
    if (fieldOffsetInited)
    {
        return;
    }

    if (superClass && superClass->type == InstanceKlass &&
        !reinterpret_cast<OMElysiaInstanceKlass *>(superClass)->fieldOffsetInited)
    {
        reinterpret_cast<OMElysiaInstanceKlass *>(superClass)->initFieldOffsets();
        length = reinterpret_cast<OMElysiaInstanceKlass *>(superClass)->length;
    }
    else
    {
        length = 0;
    }

    staticLength = 0;
    for (int i = 0; i < fieldCount; i++)
    {
        auto &f = fields[i];

        bool isStatic = f.accessFlag & JVM_Acc_Static;
        auto fieldlength = fieldLength(f.desc, ptrLength);

        if (isStatic)
        {
            staticLength = (staticLength % fieldlength) ? (staticLength + (fieldlength - staticLength % fieldlength))
                                                        : staticLength;
        }
        else
        {
            length = (length % fieldlength) ? (length + (fieldlength - length % fieldlength)) : length;
        }

        f.offset = isStatic ? staticLength : length;
        (isStatic ? staticLength : length) += fieldlength;
    }

    fieldOffsetInited = true;
}
} // namespace openminecraft::vm::elysia
