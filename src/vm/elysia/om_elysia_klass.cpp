#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include <cstring>

namespace openminecraft::vm::elysia
{
OMElysiaMethod *OMElysiaKlass::findMethod(char *name, char *desc)
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

OMElysiaField *OMElysiaInstanceKlass::findField(char *name, char *desc)
{
    if (!name || !desc) {
        return nullptr;
    }

    for (int i = 0; i < fieldCount; i++) {
        if (std::strcmp(fields[i].name, name) == 0 && std::strcmp(fields[i].desc, desc) == 0) {
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
