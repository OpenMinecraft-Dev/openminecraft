#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "fmt/format.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include "optimizations.hpp"
#include <codecvt>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::elysia
{
OMElysiaKlassloader::OMElysiaKlassloader(OMElysium *elysium) : elysium(elysium), logger("OMElysiaKlassloader", this)
{
    loadedClasses = std::make_shared<std::map<binary::hash::hash_t, OMElysiaKlass *>>();
}
OMElysiaKlassloader::~OMElysiaKlassloader()
{
}
OMElysiaInstanceKlass *OMElysiaKlassloader::constructInstanceClassShell(std::string s)
{
    auto klass = elysium->metaspaceHeap.allocate<OMElysiaInstanceKlass>();
    klass->accessFlag = JVM_Acc_Public;
    klass->superClass = nullptr;
    klass->name = elysium->metaspaceHeap.allocateStr(s);
    klass->type = InstanceKlass;
    klass->interfaceImplCount = 0;
    klass->interfaceImpls = nullptr;
    klass->ptrLength = elysium->mainHeap.ptrLength();
    klass->mirror = nullptr;

    markKlass(klass);
    return klass;
}
OMElysiaPrimitiveKlass *OMElysiaKlassloader::constructPrimitiveClass(std::string s)
{
    auto klass = elysium->metaspaceHeap.allocate<OMElysiaPrimitiveKlass>();
    klass->accessFlag = JVM_Acc_Public;
    klass->superClass = nullptr;
    klass->name = elysium->metaspaceHeap.allocateStr(s);
    klass->type = PrimitiveKlass;
    klass->ptrLength = elysium->mainHeap.ptrLength();
    klass->mirror = nullptr;

    markKlass(klass);
    return klass;
}

static OMElysiaNativeHandle *arrayClone(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
{
    auto kls = env->GetObjectClass(hnd)->toArray()->lowerDim;
    if (kls->isPrimitive())
    {
        throw std::logic_error("fail");
    }

    auto arr = env->NewObjectArray(env->GetArrayLength(hnd), kls, nullptr);
    for (int i = 0; i < env->GetArrayLength(hnd); i++)
    {
        env->SetObjectArrayElement(arr, i, env->GetObjectArrayElement(hnd, i));
    }
    return arr;
}

OMElysiaArrayKlass *OMElysiaKlassloader::constructArrayClass(OMElysiaKlass *k)
{
    auto rawname = buildArray(k->name);

    auto klass = elysium->metaspaceHeap.allocate<OMElysiaArrayKlass>();
    klass->accessFlag = JVM_Acc_Public;
    klass->superClass = findClass("java/lang/Object");
    klass->name = elysium->metaspaceHeap.allocateStr(rawname);
    klass->type = ArrayKlass;
    klass->lowerDim = k;
    klass->ptrLength = elysium->mainHeap.ptrLength();
    klass->mirror = nullptr;
    klass->length = 0;
    klass->staticLength = 0;

    klass->methodCount = 1;
    klass->methods = elysium->metaspaceHeap.allocateArray<OMElysiaMethod>(1);
    klass->methods->name = elysium->metaspaceHeap.allocateStr("clone");
    klass->methods->descriptor = elysium->metaspaceHeap.allocateStr("()Ljava/lang/Object;");
    klass->methods->klass = klass;
    klass->methods->argSlots = 1;
    klass->methods->cachedDescriptor =
        std::make_shared<std::pair<std::vector<OMElysiaSignaturePart>, OMElysiaSignaturePart>>(
            std::move(parseSignature(klass->methods->descriptor)));
    klass->methods->accessFlag = JVM_Acc_Public | JVM_Acc_Final | JVM_Acc_Native;
    klass->nativeMethodCount = 1;
    klass->nativeMethods = elysium->metaspaceHeap.allocateArray<OMElysiaNativeMethod>(1);

    klass->nativeMethods[0].name = const_cast<char *>("clone");
    klass->nativeMethods[0].signature = const_cast<char *>("()Ljava/lang/Object;");
    klass->nativeMethods[0].funcPtr = (void *)arrayClone;

    fillVtable(klass);

    jint i = 0;
    switch (hash_compile_time(k->name))
    {
    case "byte"_hash:
    case "boolean"_hash:
        i = 1;
        break;
    case "char"_hash:
    case "short"_hash:
        i = 2;
        break;
    case "int"_hash:
    case "float"_hash:
        i = 4;
        break;
    case "long"_hash:
    case "double"_hash:
        i = 8;
        break;
    default:
        i = klass->ptrLength;
        break;
    }
    klass->itemLength = i;

    if (k->isArray())
    {
        k->toArray()->higherDim = klass;
    }

    markKlass(klass);
    return klass;
}

void OMElysiaKlassloader::markKlass(OMElysiaKlass *klass)
{
    klass->klassloader = this;
    klass->klassMutex = std::make_unique<std::recursive_mutex>();
    (*loadedClasses)[binary::hash::hash_compile_time(klass->name)] = klass;
}

OMElysiaKlass *OMElysiaKlassloader::findClass(std::string s)
{
    return (*loadedClasses)[binary::hash::hash_compile_time(s.c_str())];
}

void OMElysiaKlassloader::fixClassMirror(OMElysiaKlass *klass)
{
    {
        if (klass->mirror)
        {
            return;
        }

        klass->klassMutex->lock();

        auto kls = elysium->klassLoader->findClass("java/lang/Class");
        auto oop = elysium->oopManager->allocateOop(kls, klass->isInstance() ? klass->toInstance()->staticLength : 0);

        auto field2 = kls->toInstance()->findField("classLoader", "Ljava/lang/ClassLoader;");
        elysium->oopManager->oopAccessPointerField(oop, field2->offset, this->klassloader);

        auto field3 = kls->toInstance()->findField("<ptr>", "J");
        *reinterpret_cast<jlong *>(elysium->oopManager->oopAccessField(oop, field3->offset)) = (jlong)klass;

        if (klass->isInstance())
        {
            auto field4 = kls->toInstance()->findField("<static_block>", "V");
            klass->toInstance()->staticBlock =
                reinterpret_cast<void *>(elysium->oopManager->oopAccessField(oop, field4->offset));
        }

        klass->mirror = oop;
        klass->klassMutex->unlock();
    }
}

void OMElysiaKlassloader::ensureClassInit(OMElysiaKlass *klass)
{
    klass->klassMutex->lock();
    if (klass->isInstance() && !klass->toInstance()->clinitFinished)
    {
        klass->toInstance()->clinitFinished = true;
        auto m = klass->findMethod("<clinit>", "()V");

        if (!m)
        {
            goto endf;
        }

        elysium->executor->callVoidFunction(m, nullptr);
    }
endf:
    klass->klassMutex->unlock();
    return;
}

OMElysiaKlass *OMElysiaKlassloader::loadClassWithoutMirror(std::string name, bool special)
{
    if (isArray(name))
    {
        auto k = fetchOrLoadClass(decompArray(name));
        auto arrk = constructArrayClass(k);
        fixClassMirror(arrk);
        return arrk;
    }

    auto istr = std::make_shared<std::ifstream>(fmt::format("vmstd/out/{}.class", name), std::ios::binary);
    if (!istr->good())
    {
        auto kls = fetchOrLoadClass("java/lang/ClassNotFoundException");
        auto inm = kls->findMethod("<init>", "(Ljava/lang/String;)V");
        auto oop = elysium->oopManager->allocateOop(kls);
        OMElysiaNativeValue args[2];
        args[0].l = elysium->executor->recordLocalRef(oop);
        args[1].l = elysium->executor->recordLocalRef(elysium->oopManager->allocateString(name));
        elysium->executor->callVoidFunction(inm, args);
        elysium->throwException(oop);
        logger.warn("class {} not found!", name);
        return nullptr;
    }
    return loadClassWithoutMirror(istr, special);
}

void OMElysiaKlassloader::fixAllClasses()
{
    for (auto &p : *loadedClasses)
    {
        fixClassMirror(p.second);
    }
}

void OMElysiaKlassloader::fillVtable(OMElysiaInstanceKlass *klass)
{
    std::unordered_map<std::string, OMElysiaMethod *> rawVtable;

    if (klass->interfaceImplCount)
    {
        for (int ii = 0; ii < klass->interfaceImplCount; ii++)
        {
            auto kk = klass->interfaceImpls[ii];
            if (kk->vtable && kk->vtableLength)
            {
                for (int i = 0; i < kk->vtableLength; i++)
                {
                    rawVtable[fmt::format("{}{}", kk->vtable[i]->name, kk->vtable[i]->descriptor)] = kk->vtable[i];
                }
            }
        }
    }

    klass->vtable = nullptr;
    klass->vtableLength = 0;
    // geopeila: insert super class vtable
    if (klass->superClass && klass->superClass->vtable && klass->superClass->vtableLength)
    {
        for (int i = 0; i < klass->superClass->vtableLength; i++)
        {
            auto mm = klass->superClass->vtable[i];
            rawVtable[fmt::format("{}{}", mm->name, mm->descriptor)] = mm;
        }
    }

    for (int i = 0; i < klass->methodCount; i++)
    {
        auto &m = klass->methods[i];
        if (!m.isStatic() && !m.isPrivate() && !m.isInit())
        {
            rawVtable[fmt::format("{}{}", m.name, m.descriptor)] = &m;
        }
    }

    klass->vtableLength = rawVtable.size();
    if (klass->vtableLength)
    {
        klass->vtable = elysium->metaspaceHeap.allocateArray<OMElysiaMethod *>(klass->vtableLength);
        int i = 0;
        for (auto &[a, b] : rawVtable)
        {
            klass->vtable[i] = b;
            ++i;
        }
    }
}
OMElysiaKlass *OMElysiaKlassloader::loadClassWithoutMirror(std::shared_ptr<std::istream> istr, bool special,
                                                           std::string repname)
{
    auto clsfile = std::make_shared<specs::classfile::OMClassFile>();
    clsfile->load(istr);
    auto clsname =
        clsfile->constants.data->at(clsfile->constants.data->at(clsfile->basic.thisClass).classinfo.nameIndex)
            .valueString;
    if (repname.size() > 0)
    {
        clsname += repname;
    }

    if (!findClass(clsname))
    {
        constructInstanceClassShell(clsname);
    }
    else
    {
        return findClass(clsname);
    }
    auto klassraw = findClass(clsname);

    if (!klassraw->isInstance())
    {
        throw std::logic_error("not allowed!");
    }

    auto klass = klassraw->toInstance();
    klass->klassMutex->lock();

    if (clsfile->basic.superClass)
    {
        auto supclsname =
            clsfile->constants.data->at(clsfile->constants.data->at(clsfile->basic.superClass).classinfo.nameIndex)
                .valueString;

        auto supk = findClass(supclsname);
        if (!supk)
        {
            if (special)
            {
                loadClassWithoutMirror(supclsname, special);
                supk = findClass(supclsname);
            }
            else
            {
                supk = fetchOrLoadClass(supclsname);
            }
        }

        klass->superClass = findClass(supclsname);
    }

    for (int i = 0; i < clsfile->attributes.length; ++i)
    {
        if (clsfile->constants.data->at(clsfile->attributes.data->at(i).nameIndex).valueString == "BootstrapMethods")
        {
            auto &bm = clsfile->attributes.data->at(i).bootstrapMethod;
            klass->bootstrapMethodCount = bm.numBootstrapMethods;
            klass->bootstrapMethods = bm.bootstrapMethods;
        }
    }

    if (!clsfile->interfaces.data->empty())
    {
        klass->interfaceImplCount = clsfile->interfaces.length;
        klass->interfaceImpls = elysium->metaspaceHeap.allocateArray<OMElysiaKlass *>(klass->interfaceImplCount);
        for (int ii = 0; ii < klass->interfaceImplCount; ++ii)
        {
            auto supclsname =
                clsfile->constants.data
                    ->at(clsfile->constants.data->at(clsfile->interfaces.data->at(ii)).classinfo.nameIndex)
                    .valueString;
            if (!findClass(supclsname))
            {
                if (special)
                {
                    loadClassWithoutMirror(supclsname, special);
                    klass->interfaceImpls[ii] = findClass(supclsname);
                }
                else
                {
                    klass->interfaceImpls[ii] = fetchOrLoadClass(supclsname);
                }
            }
            else
            {
                klass->interfaceImpls[ii] = findClass(supclsname);
            }
        }
    }

    for (auto &att : *clsfile->attributes.data)
    {
        if (std::strcmp(clsfile->constants.data->at(att.nameIndex).valueString.c_str(), "EnclosingMethod") == 0)
        {
            auto kn = clsfile->constants.data
                          ->at(clsfile->constants.data->at(att.enclosingMethod.classIndex).classinfo.nameIndex)
                          .valueString;
            OMElysiaKlass *enclosingKlass;
            if (!findClass(kn))
            {
                if (special)
                {
                    loadClassWithoutMirror(kn, special);
                    enclosingKlass = findClass(kn);
                }
                else
                {
                    enclosingKlass = fetchOrLoadClass(kn);
                }
            }
            else
            {
                enclosingKlass = findClass(kn);
            }

            klass->enclosingKlass = enclosingKlass;
            if (!att.enclosingMethod.methodIndex)
            {
                break;
            }
            auto &nmt = clsfile->constants.data->at(att.enclosingMethod.methodIndex);
            auto mname = clsfile->constants.data->at(nmt.nameAndType.nameIndex).valueString;
            auto mdesc = clsfile->constants.data->at(nmt.nameAndType.descriptorIndex).valueString;

            klass->enclosingMethod = enclosingKlass->findMethod(mname.c_str(), mdesc.c_str());
        }
    }

    klass->constantPoolRaw.resize(clsfile->constants.data->size());
    for (int i = 0; i < clsfile->constants.data->size(); ++i)
    {
        klass->constantPoolRaw[i] = clsfile->constants.data->at(i);
    }

    auto l = clsfile->constants.length;
    klass->constantPoolCount = clsfile->constants.length;
    klass->constantPool = elysium->metaspaceHeap.allocateArray<void *>(l);
    klass->constantPoolState = elysium->metaspaceHeap.allocateArray<bool>(l);

    klass->accessFlag = clsfile->basic.accessFlags;
    klass->thisClass = clsfile->basic.thisClass;

    klass->methodCount = clsfile->methods.length;
    klass->methods = elysium->metaspaceHeap.allocateArray<OMElysiaMethod>(klass->methodCount);

    for (int i = 0; i < klass->methodCount; i++)
    {
        auto &m = klass->methods[i];
        m.klass = klass;
        m.codeLength = 0;
        m.code = nullptr;

        m.accessFlag = clsfile->methods.data->at(i).accessFlags;
        m.name = elysium->metaspaceHeap.allocateStr(
            clsfile->constants.data->at(clsfile->methods.data->at(i).nameIndex).valueString);
        m.descriptor = elysium->metaspaceHeap.allocateStr(
            clsfile->constants.data->at(clsfile->methods.data->at(i).descriptorIndex).valueString);
        m.argSlots = argSlots(m.descriptor) + (m.isStatic() ? 0 : 1);
        m.cachedDescriptor = std::make_shared<std::pair<std::vector<OMElysiaSignaturePart>, OMElysiaSignaturePart>>(
            std::move(parseSignature(m.descriptor)));

        if (m.isNative())
        {
            m.localLength = m.argSlots;
            m.cifprepared = false;
        }
        else
        {
            for (auto &attr : *clsfile->methods.data->at(i).attributes)
            {
                if (std::strcmp(clsfile->constants.data->at(attr.nameIndex).valueString.c_str(), "Exceptions") == 0)
                {
                    m.exceptionsLength = attr.exceptions.count;
                    if (m.exceptionsLength)
                    {
                        m.exceptions = elysium->metaspaceHeap.allocateArray<OMElysiaKlass *>(m.exceptionsLength);
                    }

                    for (int i = 0; i < m.exceptionsLength; i++)
                    {
                        auto klsname =
                            clsfile->constants.data
                                ->at(clsfile->constants.data->at(attr.exceptions.index[i]).classinfo.nameIndex)
                                .valueString;
                        if (special)
                        {
                            loadClassWithoutMirror(klsname, special);
                            m.exceptions[i] = findClass(klsname);
                        }
                        else
                        {
                            m.exceptions[i] = fetchOrLoadClass(klsname);
                        }
                    }
                }
                if (std::strcmp(clsfile->constants.data->at(attr.nameIndex).valueString.c_str(), "Code") == 0)
                {
                    m.codeLength = attr.code.codeLength;
                    m.code = elysium->metaspaceHeap.allocateArray<uint8_t>(m.codeLength);
                    m.localLength = attr.code.maxLocal;
                    std::memcpy(m.code, attr.code.code, m.codeLength);

                    m.excTableLength = attr.code.exceptionTableLength;
                    m.excTable = elysium->metaspaceHeap.allocateArray<OMElysiaMethodExcTable>(m.excTableLength);

                    for (int i = 0; i < m.excTableLength; i++)
                    {
                        m.excTable[i].begin = reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(m.code) +
                                                                          attr.code.exceptionTable[i].start);
                        m.excTable[i].end = reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(m.code) +
                                                                        attr.code.exceptionTable[i].end);
                        m.excTable[i].handler = reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(m.code) +
                                                                            attr.code.exceptionTable[i].handler);

                        if (attr.code.exceptionTable[i].type)
                        {
                            auto klsname = clsfile->constants.data
                                               ->at(clsfile->constants.data->at(attr.code.exceptionTable[i].type)
                                                        .classinfo.nameIndex)
                                               .valueString;
                            if (special)
                            {
                                loadClassWithoutMirror(klsname, special);
                                m.excTable[i].type = findClass(klsname);
                            }
                            else
                            {
                                m.excTable[i].type = fetchOrLoadClass(klsname);
                            }
                        }
                        else
                        {
                            m.excTable[i].type = nullptr;
                        }
                    }

                    break;
                }
            }
        }
    }

    fillVtable(klass);

    std::vector<OMElysiaField> extraFields;
    if (std::strcmp(klass->name, "java/lang/Class") == 0 || std::strcmp(klass->name, "java/lang/ClassLoader") == 0 ||
        std::strcmp(klass->name, "java/lang/reflect/Method") == 0 ||
        std::strcmp(klass->name, "java/lang/reflect/Field") == 0 ||
        std::strcmp(klass->name, "java/lang/reflect/Constructor") == 0 ||
        std::strcmp(klass->name, "java/lang/invoke/MemberName") == 0)
    {
        extraFields.push_back({klass, elysium->metaspaceHeap.allocateStr("<ptr>"),
                               elysium->metaspaceHeap.allocateStr("J"),
                               JVM_Acc_Private | JVM_Acc_Final | JVM_Acc_Bridge});
    }
    if (std::strcmp(klass->name, "java/lang/Class") == 0)
    {
        extraFields.push_back({klass, elysium->metaspaceHeap.allocateStr("<static_block>"),
                               elysium->metaspaceHeap.allocateStr("V"),
                               JVM_Acc_Private | JVM_Acc_Final | JVM_Acc_Bridge});
    }

    klass->fieldCount = clsfile->fields.length + extraFields.size();
    klass->fields = elysium->metaspaceHeap.allocateArray<OMElysiaField>(klass->fieldCount);
    for (int i = 0; i < clsfile->fields.length; i++)
    {
        auto &f = klass->fields[i];
        f.name = elysium->metaspaceHeap.allocateStr(
            clsfile->constants.data->at(clsfile->fields.data->at(i).nameIndex).valueString);
        f.desc = elysium->metaspaceHeap.allocateStr(
            clsfile->constants.data->at(clsfile->fields.data->at(i).descriptorIndex).valueString);

        f.accessFlag = clsfile->fields.data->at(i).accessFlags;
        f.klass = klass;
    }
    for (int i = 0; i < extraFields.size(); i++)
    {
        auto &f = klass->fields[i + clsfile->fields.length];
        f.name = extraFields[i].name;
        f.desc = extraFields[i].desc;
        f.accessFlag = extraFields[i].accessFlag;
        f.klass = extraFields[i].klass;
    }

    klass->initFieldOffsets();

    klass->clinitFinished = false;

    klass->klassMutex->unlock();

    return klass;
}
} // namespace openminecraft::vm::elysia
