#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_meta.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace openminecraft::vm::elysia
{
auto OMElysiaKlass::inherits(OMElysiaKlass *klass) -> bool
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
auto OMElysiaKlass::findMethod(const char *name, const char *desc) -> OMElysiaMethod *
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

auto OMElysiaInstanceKlass::constantPoolFetchNormalW(uint16_t id) -> uint64_t
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

auto OMElysiaInstanceKlass::constantPoolFetchField(uint16_t id) -> void *
{
    if (constantPoolState[id])
    {
        return constantPool[id];
    }

    auto &item = constantPoolRaw[id];
    auto clsname = constantPoolRaw[constantPoolRaw[item.ref.classIndex].classinfo.nameIndex].valueString;
    auto mdname = constantPoolRaw[constantPoolRaw[item.ref.nameAndTypeIndex].nameAndType.nameIndex].valueString;
    auto mddesc = constantPoolRaw[constantPoolRaw[item.ref.nameAndTypeIndex].nameAndType.descriptorIndex].valueString;

    auto kk = execWithState(InsideVM, [&]() -> OMElysiaKlass * { return klassloader->fetchOrLoadClass(clsname); });

    while (kk)
    {
        for (int i = 0; i < kk->toInstance()->fieldCount; i++)
        {
            if (std::strcmp(kk->toInstance()->fields[i].name, mdname.c_str()) == 0 &&
                std::strcmp(kk->toInstance()->fields[i].desc, mddesc.c_str()) == 0)
            {
                constantPool[id] = &kk->toInstance()->fields[i];
                constantPoolState[id] = true;

                klassloader->ensureClassInit(kk);

                return &kk->toInstance()->fields[i];
            }
        }
        kk = kk->superClass;
    }

    return nullptr;
}

auto OMElysiaInstanceKlass::constantPoolFetchDynamic(uint16_t id) -> void *
{
    if (constantPoolState[id])
    {
        return constantPool[id];
    }

    auto &item = constantPoolRaw[id];
    auto &bm = bootstrapMethods[item.dynamic.bootstrapIndex];

    auto &methodref = constantPoolRaw[bm.bootstrapMethodRef].methodHandle;
    auto elysium = klassloader->elysium;

    std::vector<OMElysiaOop *> target;

    OMElysiaOop *lookup;
    {
        auto kl = elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MethodHandles$Lookup", true)->toInstance();
        lookup = elysium->oopManager->allocateOop(kl);
        elysium->oopManager->oopAccessPointerField(lookup, kl->findField("lookupClass", "Ljava/lang/Class;")->offset,
                                                   this->mirror);
        *reinterpret_cast<jint *>(
            elysium->oopManager->oopAccessField(lookup, kl->findField("allowedModes", "I")->offset)) =
            JVM_Acc_Private | JVM_Acc_Public | JVM_Acc_Protected | JVM_Acc_Static;
    }

    auto nt = constantPoolRaw[item.dynamic.nameAndTypeIndex].nameAndType;
    auto invokedName = elysium->oopManager->allocateString(constantPoolRaw[nt.nameIndex].valueString);

    {
        auto r = parseSignature(constantPoolRaw[nt.descriptorIndex].valueString.c_str());
        elysium->klassLoader->fetchOrLoadClass(signatureToType(r.second), true);
        for (auto &p : r.first)
        {
            elysium->klassLoader->fetchOrLoadClass(signatureToType(p), true);
        }
    }

    auto buildTypeFor = [&](std::string dd) -> OMElysiaOop * {
        std::array<OMElysiaNativeValue, 2> vv;
        vv[0].l = elysium->executor->recordLocalRef(elysium->oopManager->allocateString(dd));
        vv[1].l = elysium->executor->recordLocalRef(klassloader->klassloader);
        return elysium->executor->callObjectFunction(
            elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MethodType", true)
                ->findMethod("fromMethodDescriptorString",
                             "(Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/invoke/MethodType;"),
            vv.data());
    };
    auto invokedType = buildTypeFor(constantPoolRaw[nt.descriptorIndex].valueString);

    for (int i = 0; i < bm.numBootstrapArguments; ++i)
    {
        auto oop = constantPoolFetchNormal(bm.bootstrapArguments[i]);
        target.push_back(reinterpret_cast<OMElysiaOop *>(oop));
    }

    auto arg = elysium->oopManager->allocateArr(
        elysium->klassLoader->fetchOrLoadClass("[Ljava/lang/Object;", true)->toArray(), target.size());
    for (int i = 0; i < target.size(); ++i)
    {
        elysium->oopManager->arrAccessPtr(arg, i, target[i]);
    }

    auto hnd = constantPoolFetchNormal(bm.bootstrapMethodRef);

    auto result = elysium->oopManager->allocateArr(
        elysium->klassLoader->fetchOrLoadClass("[Ljava/lang/Object;", true)->toArray(), 1);

    auto mm =
        elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MethodHandleNatives", true)
            ->findMethod("linkCallSite", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/"
                                         "Object;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/invoke/MemberName;");
    std::array<OMElysiaNativeValue, 6> vv;
    vv[0].l = createTempHandle(this->mirror);
    vv[1].l = createTempHandle(reinterpret_cast<OMElysiaOop *>(hnd));
    vv[2].l = createTempHandle(invokedName);
    vv[3].l = createTempHandle(invokedType);
    vv[4].l = createTempHandle(arg);
    vv[5].l = createTempHandle(result);
    auto mn = elysium->executor->callObjectFunction(mm, vv.data());
    auto fieldoff = elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MemberName", true)
                        ->toInstance()
                        ->findField("<ptr>", "J")
                        ->offset;
    auto mthd = reinterpret_cast<OMElysiaMethod *>(
        *reinterpret_cast<jlong *>(elysium->oopManager->oopAccessField(mn, fieldoff)));

    auto mthh = elysium->oopManager->arrAccessPtr(result, 0);

    auto stt = elysium->metaspaceHeap.allocate<OMElysiaKlassDynamic>();
    stt->target = mthd;
    stt->handle = mthh;

    constantPool[id] = stt;
    constantPoolState[id] = true;
    return stt;
}

auto OMElysiaInstanceKlass::constantPoolFetchNormal(uint16_t id, bool flg) -> void *
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

        auto cls = execWithState(InsideVM, [&]() -> OMElysiaKlass * { return klassloader->fetchOrLoadClass(clsname); });
        auto rcls = cls;

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

        if (!mthd)
        {
            auto ins = klassloader->elysium->executor->findRoutine(clsname, mdname);
            if (ins)
            {
                mthd = klassloader->elysium->metaspaceHeap.allocate<OMElysiaMethod>();
                mthd->intrinsic = true;
                mthd->intrinsicRoutine = ins;
                mthd->name = klassloader->elysium->metaspaceHeap.allocateStr(mdname);
                mthd->descriptor = klassloader->elysium->metaspaceHeap.allocateStr(mddesc);
                mthd->argSlots = argSlots(mthd->descriptor);
            }
        }

        klassloader->ensureClassInit(rcls);

        constantPool[id] = mthd;
        constantPoolState[id] = true;
        return mthd;
    }
    case specs::classfile::Class: {
        auto clsname = constantPoolRaw[item.classinfo.nameIndex].valueString;

        auto cls = execWithState(InsideVM, [&]() -> OMElysiaKlass * { return klassloader->fetchOrLoadClass(clsname); });

        constantPool[id] = cls;
        constantPoolState[id] = true;
        if (cls == nullptr)
        {
            while (true)
            {
                continue;
            }
        }
        if (!flg)
        {
            klassloader->ensureClassInit(cls);
        }
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
    case specs::classfile::MethodType: {
        auto &elysium = klassloader->elysium;
        std::array<OMElysiaNativeValue, 2> vv;
        vv[0].l = elysium->executor->recordLocalRef(
            elysium->oopManager->allocateString(constantPoolRaw[item.methodType.descriptorIndex].valueString));
        vv[1].l = elysium->executor->recordLocalRef(klassloader->klassloader);
        auto oop = elysium->executor->callObjectFunction(
            elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MethodType", true)
                ->findMethod("fromMethodDescriptorString",
                             "(Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/invoke/MethodType;"),
            vv.data());
        constantPool[id] = oop;
        constantPoolState[id] = true;
        return constantPool[id];
    }
    case specs::classfile::MethodHandle: {
        auto &elysium = klassloader->elysium;

        OMElysiaOop *lookup;
        {
            auto kl =
                elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MethodHandles$Lookup", true)->toInstance();
            lookup = elysium->oopManager->allocateOop(kl);
            elysium->oopManager->oopAccessPointerField(
                lookup, kl->findField("lookupClass", "Ljava/lang/Class;")->offset, this->mirror);
            *reinterpret_cast<jint *>(
                elysium->oopManager->oopAccessField(lookup, kl->findField("allowedModes", "I")->offset)) =
                JVM_Acc_Private | JVM_Acc_Public | JVM_Acc_Protected | JVM_Acc_Static;
        }

        auto ref = reinterpret_cast<OMElysiaMethod *>(constantPoolFetchNormal(item.methodHandle.refIndex));

        std::array<OMElysiaNativeValue, 5> vv;
        vv[0].l = elysium->executor->recordLocalRef(elysium->oopManager->allocateString(ref->descriptor));
        vv[1].l = elysium->executor->recordLocalRef(klassloader->klassloader);
        auto type = elysium->executor->callObjectFunction(
            elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MethodType", true)
                ->findMethod("fromMethodDescriptorString",
                             "(Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/invoke/MethodType;"),
            vv.data());

        vv[0].l = createTempHandle(lookup);
        vv[1].b = item.methodHandle.refKind;
        vv[2].l = createTempHandle(ref->klass->mirror);
        vv[3].l = createTempHandle(elysium->oopManager->allocateString(ref->name));
        vv[4].l = createTempHandle(type);
        auto mn = elysium->executor->callObjectFunction(
            elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MethodHandles$Lookup", true)
                ->findMethod("resolveOrFail", "(BLjava/lang/Class;Ljava/lang/String;Ljava/lang/invoke/"
                                              "MethodType;)Ljava/lang/invoke/MemberName;"),
            vv.data());

        vv[0].l = createTempHandle(lookup);
        vv[1].b = item.methodHandle.refKind;
        vv[2].l = createTempHandle(ref->klass->mirror);
        vv[3].l = createTempHandle(mn);
        vv[4].l = createTempHandle(mirror);

        auto hnd = elysium->executor->callObjectFunction(
            elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MethodHandles$Lookup", true)
                ->findMethod("getDirectMethod", "(BLjava/lang/Class;Ljava/lang/invoke/MemberName;Ljava/lang/"
                                                "Class;)Ljava/lang/invoke/MethodHandle;"),
            vv.data());

        constantPool[id] = hnd;
        constantPoolState[id] = true;
        return constantPool[id];
    }
    default:
        throw std::logic_error("unknown constant type!");
    }
    return nullptr;
}

auto OMElysiaInstanceKlass::findField(const char *name, const char *desc) -> OMElysiaField *
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
