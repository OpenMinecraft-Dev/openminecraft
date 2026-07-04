#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_meta.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

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

    auto &item = constantPoolRaw[id];
    switch (item.type)
    {
    case specs::classfile::Long: {
        auto data = item.valueLong;
        constantPool[id] = reinterpret_cast<void *>(data & 0xffffffff);
        constantPool[id + 1] = reinterpret_cast<void *>(data >> 32);
        return *reinterpret_cast<uint64_t *>(&data);
    }
    case specs::classfile::Double: {
        auto datar = item.valueDouble;
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

    auto &item = constantPoolRaw[id];
    auto clsname = constantPoolRaw[constantPoolRaw[item.ref.classIndex].classinfo.nameIndex].valueString;
    auto mdname = constantPoolRaw[constantPoolRaw[item.ref.nameAndTypeIndex].nameAndType.nameIndex].valueString;
    auto mddesc = constantPoolRaw[constantPoolRaw[item.ref.nameAndTypeIndex].nameAndType.descriptorIndex].valueString;

    auto kk = execWithState(InsideVM, [&]() { return klassloader->fetchOrLoadClass(clsname); });

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

void *OMElysiaInstanceKlass::constantPoolFetchDynamic(uint16_t id)
{
    if (constantPoolState[id])
    {
        return constantPool[id];
    }

    auto &item = constantPoolRaw[id];
    auto &bm = bootstrapMethods[item.dynamic.bootstrapIndex];

    auto &methodref = constantPoolRaw[bm.bootstrapMethodRef].methodHandle;
    auto elysium = klassloader->elysium;
    auto kl = elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MethodHandles$Lookup")->toInstance();
    auto lookup = elysium->oopManager->allocateOop(kl);
    elysium->oopManager->oopAccessPointerField(lookup, kl->findField("lookupClass", "Ljava/lang/Class;")->offset,
                                               this->mirror);
    *reinterpret_cast<jint *>(elysium->oopManager->oopAccessField(lookup, kl->findField("allowedModes", "I")->offset)) =
        JVM_Acc_Private | JVM_Acc_Public | JVM_Acc_Protected | JVM_Acc_Static;

    auto nt = constantPoolRaw[item.dynamic.nameAndTypeIndex].nameAndType;
    auto invokedName = elysium->oopManager->allocateString(constantPoolRaw[nt.nameIndex].valueString);

    auto bdfunc = klassloader->fetchOrLoadClass("java/lang/invoke/MethodType")
                      ->findMethod("fromMethodDescriptorString",
                                   "(Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/invoke/MethodType;");
    OMElysiaNativeValue vv[2];
    vv[0].l = elysium->executor->recordLocalRef(
        elysium->oopManager->allocateString(constantPoolRaw[nt.descriptorIndex].valueString));
    vv[1].l = elysium->executor->recordLocalRef(klassloader->klassloader);
    auto res = elysium->executor->callObjectFunction(bdfunc, vv);

    std::cout << thisThread.metadata->currentException << std::endl;

    while (true)
    {
        continue;
    }
    throw std::logic_error("not implemented!");
}

void *OMElysiaInstanceKlass::constantPoolFetchNormal(uint16_t id, bool flg)
{
    if (constantPoolState[id])
    {
        if (!flg || constantPoolRaw[id].type != specs::classfile::Class)
        {
            return constantPool[id];
        }
        else
        {
            return reinterpret_cast<OMElysiaKlass *>(constantPool[id])->mirror;
        }
    }

    auto &item = constantPoolRaw[id];
    switch (item.type)
    {
    case specs::classfile::InterfaceMethodRef:
    case specs::classfile::MethodRef: {
        auto clsname = constantPoolRaw[constantPoolRaw[item.ref.classIndex].classinfo.nameIndex].valueString;
        auto mdname = constantPoolRaw[constantPoolRaw[item.ref.nameAndTypeIndex].nameAndType.nameIndex].valueString;
        auto mddesc =
            constantPoolRaw[constantPoolRaw[item.ref.nameAndTypeIndex].nameAndType.descriptorIndex].valueString;

        auto cls = execWithState(InsideVM, [&]() { return klassloader->fetchOrLoadClass(clsname); });

        OMElysiaMethod *mthd = nullptr;
        while (!mthd)
        {
            mthd = cls->findMethod(mdname.c_str(), mddesc.c_str());

            for (int i = 0; i < cls->toInstance()->interfaceImplCount && !mthd; i++)
            {
                mthd = cls->toInstance()->interfaceImpls[i]->findMethod(mdname.c_str(), mddesc.c_str());
            }

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
    case specs::classfile::Class: {
        auto clsname = constantPoolRaw[item.classinfo.nameIndex].valueString;

        auto cls = execWithState(InsideVM, [&]() { return klassloader->fetchOrLoadClass(clsname); });

        constantPool[id] = cls;
        constantPoolState[id] = true;
        return flg ? reinterpret_cast<void *>(cls->mirror) : cls;
    }
    case specs::classfile::Integer: {
        auto d = item.valueInteger;
        uint32_t rd = *reinterpret_cast<uint32_t *>(&d);

        constantPool[id] = reinterpret_cast<void *>(static_cast<uintptr_t>(rd));
        constantPoolState[id] = true;
        return constantPool[id];
    }
    case specs::classfile::Float: {
        auto d = item.valueFloat;
        uint32_t rd = *reinterpret_cast<uint32_t *>(&d);

        constantPool[id] = reinterpret_cast<void *>(static_cast<uintptr_t>(rd));
        constantPoolState[id] = true;
        return constantPool[id];
    }
    case specs::classfile::String: {
        auto strWrp =
            klassloader->upper()->oopManager->allocateString(constantPoolRaw[item.stringRef.stringIndex].valueString);

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

        if (fieldlength)
        {
            if (isStatic)
            {
                staticLength = (staticLength % fieldlength)
                                   ? (staticLength + (fieldlength - staticLength % fieldlength))
                                   : staticLength;
            }
            else
            {
                length = (length % fieldlength) ? (length + (fieldlength - length % fieldlength)) : length;
            }
        }

        f.offset = isStatic ? staticLength : length;
        (isStatic ? staticLength : length) += fieldlength;
    }

    fieldOffsetInited = true;
}
} // namespace openminecraft::vm::elysia
