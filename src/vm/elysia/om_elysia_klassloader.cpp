#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "fmt/format.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
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

    klass->methodCount = 1;
    klass->methods = elysium->metaspaceHeap.allocateArray<OMElysiaMethod>(1);
    klass->methods->name = elysium->metaspaceHeap.allocateStr("clone");
    klass->methods->descriptor = elysium->metaspaceHeap.allocateStr("()Ljava/lang/Object;");
    klass->methods->klass = klass;
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

        klass->klassMutex.lock();

        auto kls = elysium->klassLoader->findClass("java/lang/Class");
        auto oop = elysium->oopManager->allocateOop(kls);
        auto field = kls->toInstance()->findField("name", "Ljava/lang/String;");

        auto k = std::string(klass->name);
        for (auto &ch : k)
        {
            if (ch == '/')
            {
                ch = '.';
            }
        }
        auto strobj = elysium->oopManager->allocateString(k);

        elysium->oopManager->oopAccessPointerField(oop, field->offset, strobj);

        auto field2 = kls->toInstance()->findField("classLoader", "Ljava/lang/ClassLoader;");
        elysium->oopManager->oopAccessPointerField(oop, field2->offset, this->klassloader);

        auto field3 = kls->toInstance()->findField("<ptr>", "J");
        *reinterpret_cast<jlong *>(elysium->oopManager->oopAccessField(oop, field3->offset)) = (jlong)klass;

        klass->mirror = oop;
        klass->klassMutex.unlock();
    }

    {
        klass->klassMutex.lock();
        if (klass->isInstance() && !klass->toInstance()->clinitFinished)
        {
            auto m = klass->findMethod("<clinit>", "()V");

            if (!m)
            {
                klass->toInstance()->clinitFinished = true;
                return;
            }

            elysium->executor->callVoidFunction(m, nullptr);
        }
        klass->klassMutex.unlock();
    }
}

void OMElysiaKlassloader::loadClassWithoutMirror(std::string name, bool special)
{
    if (isArray(name))
    {
        auto k = fetchOrLoadClass(decompArray(name));
        auto arrk = constructArrayClass(k);
        fixClassMirror(arrk);
        return;
    }

    std::ifstream istr(fmt::format("vmstd/out/{}.class", name), std::ios::binary);
    if (!istr.good())
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
        return;
    }
    loadClassWithoutMirror(&istr, special);
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

void OMElysiaKlassloader::loadClassWithoutMirror(std::istream *istr, bool special)
{
    classfile::OMClassFileParser par(istr);
    auto clsfileres = par.parse();
    auto clsfile = clsfileres.unwrap();

    if (clsfile == nullptr)
    {
        throw std::logic_error(clsfileres.unwrap_err().what());
    }

    auto clsname =
        clsfile->mapping[clsfile->mapping[clsfile->thisClass]->to<classfile::OMClassConstantClass>()->nameIndex]
            ->to<classfile::OMClassConstantUtf8>()
            ->data;
    if (!findClass(clsname))
    {
        constructInstanceClassShell(clsname);
    }
    else
    {
        return;
    }
    auto klassraw = findClass(clsname);

    if (!klassraw->isInstance())
    {
        throw std::logic_error("not allowed!");
    }

    auto klass = klassraw->toInstance();
    klass->klassMutex.lock();

    if (clsfile->superClass)
    {
        auto supclsname =
            clsfile->mapping[clsfile->mapping[clsfile->superClass]->to<classfile::OMClassConstantClass>()->nameIndex]
                ->to<classfile::OMClassConstantUtf8>()
                ->data;

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

    if (!clsfile->interfaces.empty())
    {
        klass->interfaceImplCount = clsfile->interfaces.size();
        klass->interfaceImpls = elysium->metaspaceHeap.allocateArray<OMElysiaKlass *>(klass->interfaceImplCount);
        int ii = 0;
        for (auto i : clsfile->interfaces)
        {
            auto supclsname = clsfile->mapping[clsfile->mapping[i]->to<classfile::OMClassConstantClass>()->nameIndex]
                                  ->to<classfile::OMClassConstantUtf8>()
                                  ->data;
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

            ii++;
        }
    }

    for (auto &att : clsfile->attrs)
    {
        if (att->type() == classfile::OMClassAttrType::EnclosingMethod)
        {
            auto fl = att->to<classfile::OMClassAttrEnclosingMethod>();
            auto kn =
                clsfile->mapping[clsfile->mapping[fl->classIndex]->to<classfile::OMClassConstantClass>()->nameIndex]
                    ->to<classfile::OMClassConstantUtf8>()
                    ->data;
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
            if (!fl->methodIndex)
            {
                break;
            }
            auto nmt = clsfile->mapping[fl->methodIndex]->to<classfile::OMClassConstantNameAndType>();
            auto mname = clsfile->mapping[nmt->nameIndex]->to<classfile::OMClassConstantUtf8>()->data;
            auto mdesc = clsfile->mapping[nmt->descIndex]->to<classfile::OMClassConstantUtf8>()->data;
            klass->enclosingMethod = enclosingKlass->findMethod(mname.c_str(), mdesc.c_str());
        }
    }

    klass->constantPoolRaw =
        std::make_shared<std::unordered_map<uint16_t, std::shared_ptr<classfile::OMClassConstant>>>();
    int l = 0;
    for (auto &pp : clsfile->mapping)
    {
        (*klass->constantPoolRaw)[pp.first] = pp.second;
        l = std::max(l, pp.first + 1);
    }
    klass->constantPoolCount = l;
    klass->constantPool = elysium->metaspaceHeap.allocateArray<void *>(l);
    klass->constantPoolState = elysium->metaspaceHeap.allocateArray<bool>(l);

    klass->accessFlag = clsfile->accessFlags;

    klass->methodCount = clsfile->methods.size();
    klass->methods = elysium->metaspaceHeap.allocateArray<OMElysiaMethod>(klass->methodCount);

    for (int i = 0; i < klass->methodCount; i++)
    {
        auto &m = klass->methods[i];
        m.klass = klass;
        m.codeLength = 0;
        m.code = nullptr;

        m.accessFlag = clsfile->methods[i]->accessFlags;
        m.name = elysium->metaspaceHeap.allocateStr(
            clsfile->mapping[clsfile->methods[i]->nameIndex]->to<classfile::OMClassConstantUtf8>()->data);
        m.descriptor = elysium->metaspaceHeap.allocateStr(
            clsfile->mapping[clsfile->methods[i]->descIndex]->to<classfile::OMClassConstantUtf8>()->data);

        if (m.isNative())
        {
            m.localLength = argSlots(m.descriptor) + (m.isStatic() ? 0 : 1);
        }
        else
        {
            for (auto attr : clsfile->methods[i]->attrs)
            {
                if (attr->type() == classfile::OMClassAttrType::Exceptions)
                {
                    auto att = attr->to<classfile::OMClassAttrExceptions>();

                    m.exceptionsLength = att->numberOfExceptions;
                    if (att->numberOfExceptions)
                    {
                        m.exceptions = elysium->metaspaceHeap.allocateArray<OMElysiaKlass *>(m.exceptionsLength);
                    }

                    for (int i = 0; i < m.exceptionsLength; i++)
                    {
                        auto klsname = clsfile
                                           ->mapping[clsfile->mapping[att->exceptionIndexTable[i]]
                                                         ->to<classfile::OMClassConstantClass>()
                                                         ->nameIndex]
                                           ->to<classfile::OMClassConstantUtf8>()
                                           ->data;
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
                if (attr->type() == classfile::OMClassAttrType::Code)
                {
                    auto ll = attr->to<classfile::OMClassAttrCode>();
                    m.codeLength = ll->codeLength;
                    m.code = elysium->metaspaceHeap.allocateArray<uint8_t>(m.codeLength);
                    m.localLength = ll->maxLocals;
                    std::memcpy(m.code, ll->code->data(), ll->codeLength);

                    m.excTableLength = ll->excTableLength;
                    m.excTable = elysium->metaspaceHeap.allocateArray<OMElysiaMethodExcTable>(m.excTableLength);

                    for (int i = 0; i < ll->excTableLength; i++)
                    {
                        m.excTable[i].begin =
                            reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(m.code) + ll->excTable[i].startPc);
                        m.excTable[i].end =
                            reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(m.code) + ll->excTable[i].endPc);
                        m.excTable[i].handler = reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(m.code) +
                                                                            ll->excTable[i].handlerPc);

                        if (ll->excTable[i].catchType)
                        {
                            auto klsname = clsfile
                                               ->mapping[clsfile
                                                             ->mapping[ll->excTable[i].catchType]

                                                             ->to<classfile::OMClassConstantClass>()
                                                             ->nameIndex]
                                               ->to<classfile::OMClassConstantUtf8>()
                                               ->data;
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
    if (std::strcmp(klass->name, "java/lang/Class") == 0 || std::strcmp(klass->name, "java/lang/ClassLoader") == 0)
    {
        extraFields.push_back({klass, elysium->metaspaceHeap.allocateStr("<ptr>"),
                               elysium->metaspaceHeap.allocateStr("J"), JVM_Acc_Private | JVM_Acc_Final});
    }

    klass->fieldCount = clsfile->fields.size() + extraFields.size();
    klass->fields = elysium->metaspaceHeap.allocateArray<OMElysiaField>(klass->fieldCount);
    for (int i = 0; i < clsfile->fields.size(); i++)
    {
        auto &f = klass->fields[i];
        f.name = elysium->metaspaceHeap.allocateStr(
            clsfile->mapping[clsfile->fields[i]->nameIndex]->to<classfile::OMClassConstantUtf8>()->data);
        f.desc = elysium->metaspaceHeap.allocateStr(
            clsfile->mapping[clsfile->fields[i]->descIndex]->to<classfile::OMClassConstantUtf8>()->data);
        f.accessFlag = clsfile->fields[i]->accessFlags;
        f.klass = klass;
    }
    for (int i = 0; i < extraFields.size(); i++)
    {
        auto &f = klass->fields[i + clsfile->fields.size()];
        f.name = extraFields[i].name;
        f.desc = extraFields[i].desc;
        f.accessFlag = extraFields[i].accessFlag;
        f.klass = extraFields[i].klass;
    }

    klass->initFieldOffsets();

    if (klass->staticLength)
    {
        klass->staticBlock = elysium->metaspaceHeap.allocate(klass->staticLength);
        std::memset(klass->staticBlock, 0x00, klass->staticLength);
    }
    else
    {
        klass->staticBlock = nullptr;
    }

    klass->clinitFinished = false;

    klass->klassMutex.unlock();
}
} // namespace openminecraft::vm::elysia
