#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_meta.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/encoding/om_encoding_utf.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace openminecraft::vm::classfile;

namespace openminecraft::vm::elysia
{
bool OMElysiaKlass::inherits(OMElysiaKlass *klass)
{
    if (this == klass)
    {
        return true;
    }

    if (this->superClass && this->superClass->inherits(klass))
    {
        return true;
    }

    if (this->isInstance())
    {
        auto inst = this->toInstance();
        for (int i = 0; i < inst->interfaceImplCount; i++)
        {
            if (inst->interfaceImpls[i]->inherits(klass))
            {
                return true;
            }
        }
    }

    if (this->isArray() && klass->isArray())
    {
        if (this->toArray()->lowerDim->inherits(klass->toArray()->lowerDim))
        {
            return true;
        }
    }

    return false;
}
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

uint64_t OMElysiaInstanceKlass::constantPoolFetchNormalW(uint16_t id)
{
    if (constantPoolState[id] && constantPoolState[id + 1])
    {
        auto low = reinterpret_cast<uint64_t>(constantPool[id]);
        auto high = reinterpret_cast<uint64_t>(constantPool[id + 1]);

        return high << 32 | low;
    }

    auto item = constantPoolRaw->at(id);
    switch (item->type())
    {
    case OMClassConstantType::Long: {
        auto data = item->to<OMClassConstantLong>()->data;
        constantPool[id] = reinterpret_cast<void *>(data & 0xffffffff);
        constantPool[id + 1] = reinterpret_cast<void *>(data >> 32);
        return *reinterpret_cast<uint64_t *>(&data);
    }
    case OMClassConstantType::Double: {
        auto datar = item->to<OMClassConstantDouble>()->data;
        auto data = *reinterpret_cast<uint64_t *>(&datar);
        constantPool[id] = reinterpret_cast<void *>(data & 0xffffffff);
        constantPool[id + 1] = reinterpret_cast<void *>(data >> 32);
        return data;
    }
    default: {
        throw std::logic_error("unknown constant type!");
    }
    }

    return 0;
}

void *OMElysiaInstanceKlass::constantPoolFetchField(uint16_t id)
{
    if (constantPoolState[id])
    {
        return constantPool[id];
    }

    auto item = constantPoolRaw->at(id);
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

    OMElysiaKlass *kk;
    execWithState(InsideVM, [&]() { kk = klassloader->fetchOrLoadClass(clsname); });

    while (kk)
    {
        for (int i = 0; i < kk->toInstance()->fieldCount; i++)
        {
            if (std::strcmp(kk->toInstance()->fields[i].name, mdname.c_str()) == 0 &&
                std::strcmp(kk->toInstance()->fields[i].desc, mddesc.c_str()) == 0)
            {
                constantPool[id] = &kk->toInstance()->fields[i];
                constantPoolState[id] = true;
                return &kk->toInstance()->fields[i];
            }
        }
        kk = kk->superClass;
    }

    return nullptr;
}

void *OMElysiaInstanceKlass::constantPoolFetchNormal(uint16_t id, bool flg)
{
    if (constantPoolState[id])
    {
        if (!flg || constantPoolRaw->at(id)->type() != OMClassConstantType::Class)
        {
            return constantPool[id];
        }
        else
        {
            return reinterpret_cast<OMElysiaKlass *>(constantPool[id])->mirror;
        }
    }

    auto item = constantPoolRaw->at(id);
    switch (item->type())
    {
    case OMClassConstantType::InterfaceMethodRef:
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

        OMElysiaKlass *cls;
        execWithState(InsideVM, [&]() { cls = klassloader->fetchOrLoadClass(clsname); });

        OMElysiaMethod *mthd = nullptr;
        while (!mthd)
        {
            mthd = cls->findMethod(mdname.c_str(), mddesc.c_str());

            if (cls->superClass)
            {
                cls = cls->superClass;
            }
            else
            {
                break;
            }
        }

        constantPool[id] = mthd;
        constantPoolState[id] = true;
        return mthd;
    }
    case OMClassConstantType::Class: {
        auto mr = item->to<OMClassConstantClass>();
        auto clsname = constantPoolRaw->at(mr->nameIndex)->to<OMClassConstantUtf8>()->data;

        OMElysiaKlass *cls;
        execWithState(InsideVM, [&]() { cls = klassloader->fetchOrLoadClass(clsname); });

        constantPool[id] = cls;
        constantPoolState[id] = true;
        return flg ? reinterpret_cast<void *>(cls->mirror) : cls;
    }
    case OMClassConstantType::Integer: {
        auto d = item->to<OMClassConstantInteger>()->data;
        uint32_t rd = *reinterpret_cast<uint32_t *>(&d);

        constantPool[id] = reinterpret_cast<void *>(static_cast<uintptr_t>(rd));
        constantPoolState[id] = true;
        return constantPool[id];
    }
    case OMClassConstantType::Float: {
        auto d = item->to<OMClassConstantFloat>()->data;
        uint32_t rd = *reinterpret_cast<uint32_t *>(&d);

        constantPool[id] = reinterpret_cast<void *>(static_cast<uintptr_t>(rd));
        constantPoolState[id] = true;
        return constantPool[id];
    }
    case OMClassConstantType::String: {
        auto &target =
            constantPoolRaw->at(item->to<OMClassConstantString>()->stringIndex)->to<OMClassConstantUtf8>()->data;
        auto strWrp = klassloader->upper()->oopManager->allocateString(const_cast<std::string &>(target));

        constantPool[id] = strWrp;
        constantPoolState[id] = true;
        return constantPool[id];
    }
    default:
        throw std::logic_error("unknown constant type!");
    }
    return nullptr;
}

OMElysiaField *OMElysiaInstanceKlass::findField(const char *name, const char *desc)
{
    if (!name)
    {
        return nullptr;
    }

    for (int i = 0; i < fieldCount; i++)
    {
        if (std::strcmp(fields[i].name, name) == 0 && (desc == nullptr || std::strcmp(fields[i].desc, desc) == 0))
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

    if (superClass && superClass->isInstance() && superClass->toInstance()->fieldOffsetInited)
    {
        superClass->toInstance()->initFieldOffsets();
        length = superClass->toInstance()->length;
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
        auto fieldlength = descriptorLength(f.desc, ptrLength);

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
