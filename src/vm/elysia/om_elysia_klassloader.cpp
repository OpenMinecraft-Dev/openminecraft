#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "fmt/format.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstring>
#include <fstream>
#include <istream>
#include <stdexcept>

namespace openminecraft::vm::elysia
{
OMElysiaKlassloader::OMElysiaKlassloader(OMElysiaVirtualWorld *vw) : world(vw), logger("OMElysiaKlassloader", this)
{
}
OMElysiaKlassloader::~OMElysiaKlassloader()
{
}
OMElysiaInstanceKlass *OMElysiaKlassloader::constructInstanceClassShell(std::string s)
{
    auto klass = world->metaspaceHeap.allocate<OMElysiaInstanceKlass>();
    klass->accessFlag = JVM_Acc_Public;
    klass->superClass = nullptr;
    klass->name = world->metaspaceHeap.allocateStr(s);
    klass->type = InstanceKlass;
    klass->interfaceImplCount = 0;
    klass->interfaceImpls = nullptr;
    klass->ptrLength = world->mainHeap.ptrLength();
    klass->mirror = nullptr;

    markKlass(klass);
    return klass;
}
OMElysiaPrimitiveKlass *OMElysiaKlassloader::constructPrimitiveClass(std::string s)
{
    auto klass = world->metaspaceHeap.allocate<OMElysiaPrimitiveKlass>();
    klass->accessFlag = JVM_Acc_Public;
    klass->superClass = nullptr;
    klass->name = world->metaspaceHeap.allocateStr(s);
    klass->type = PrimitiveKlass;
    klass->ptrLength = world->mainHeap.ptrLength();
    klass->mirror = nullptr;

    markKlass(klass);
    return klass;
}
OMElysiaArrayKlass *OMElysiaKlassloader::constructArrayClass(OMElysiaKlass *k)
{
    auto rawname = buildArray(k->name);

    auto klass = world->metaspaceHeap.allocate<OMElysiaArrayKlass>();
    klass->accessFlag = JVM_Acc_Public;
    klass->superClass = nullptr;
    klass->name = world->metaspaceHeap.allocateStr(rawname);
    klass->type = ArrayKlass;
    klass->lowerDim = k;
    klass->ptrLength = world->mainHeap.ptrLength();
    klass->mirror = nullptr;

    if (k->isArray())
    {
        k->toArray()->higherDim = klass;
    }

    markKlass(klass);
    return klass;
}

void OMElysiaKlassloader::markKlass(OMElysiaKlass *klass)
{
    klass->nativeKlassloader = this;
    loadedClasses[binary::hash::hash_compile_time(klass->name)] = klass;
}

OMElysiaKlass *OMElysiaKlassloader::findClass(std::string s)
{
    return loadedClasses[binary::hash::hash_compile_time(s.c_str())];
}

void OMElysiaKlassloader::fixClassMirror(OMElysiaKlass *klass)
{
    if (klass->mirror)
    {
        return;
    }

    auto kls = findClass("java/lang/Class");
    auto oop = world->oopManager->allocateOop(kls);
    auto field = kls->toInstance()->findField("name", "Ljava/lang/String;");

    auto k = std::string(klass->name);
    auto strobj = world->oopManager->allocateString(k);

    world->oopManager->oopAccessPointerField(oop, field->offset, strobj);

    klass->mirror = oop;
}

void OMElysiaKlassloader::loadClassWithoutMirror(std::string name)
{
    std::ifstream istr(fmt::format("vmstd/out/{}.class", name), std::ios::binary);
    loadClassWithoutMirror(&istr);
}

void OMElysiaKlassloader::fixAllClasses()
{
    for (auto &p : loadedClasses)
    {
        fixClassMirror(p.second);
    }
}

void OMElysiaKlassloader::loadClassWithoutMirror(std::istream *istr)
{
    classfile::OMClassFileParser par(istr);
    auto clsfile = par.parse().unwrap();

    auto clsname =
        clsfile->mapping[clsfile->mapping[clsfile->thisClass]->to<classfile::OMClassConstantClass>()->nameIndex]
            ->to<classfile::OMClassConstantUtf8>()
            ->data;
    logger.info("Class loading: {}", clsname);
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

    if (clsfile->superClass)
    {
        auto supclsname =
            clsfile->mapping[clsfile->mapping[clsfile->superClass]->to<classfile::OMClassConstantClass>()->nameIndex]
                ->to<classfile::OMClassConstantUtf8>()
                ->data;

        auto supk = findClass(supclsname);
        if (!supk)
        {
            loadClassWithoutMirror(supclsname);
            supk = findClass(supclsname);
        }

        klass->superClass = findClass(supclsname);
    }

    std::vector<OMElysiaMethod *> rawVtable = {};
    if (!clsfile->interfaces.empty())
    {
        klass->interfaceImplCount = clsfile->interfaces.size();
        klass->interfaceImpls = world->metaspaceHeap.allocateArray<OMElysiaKlass *>(klass->interfaceImplCount);
        int ii = 0;
        for (auto i : clsfile->interfaces)
        {
            auto supclsname = clsfile->mapping[clsfile->mapping[i]->to<classfile::OMClassConstantClass>()->nameIndex]
                                  ->to<classfile::OMClassConstantUtf8>()
                                  ->data;
            auto ithash = binary::hash::hash_compile_time(supclsname.c_str());
            if (!loadedClasses.count(ithash))
            {
                loadClassWithoutMirror(supclsname);
            }

            klass->interfaceImpls[ii] = findClass(supclsname);

            auto kk = klass->interfaceImpls[ii];
            if (kk->vtable && kk->vtableLength)
            {
                for (int i = 0; i < kk->vtableLength; i++)
                {
                    rawVtable.push_back(kk->vtable[i]);
                }
            }

            ii++;
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
    klass->constantPool = world->metaspaceHeap.allocateArray<void *>(l);
    klass->constantPoolState = world->metaspaceHeap.allocateArray<bool>(l);

    klass->accessFlag = clsfile->accessFlags;

    klass->methodCount = clsfile->methods.size();
    klass->methods = world->metaspaceHeap.allocateArray<OMElysiaMethod>(klass->methodCount);

    klass->vtable = nullptr;
    klass->vtableLength = 0;
    // geopeila: insert super class vtable
    if (klass->superClass && klass->superClass->vtable && klass->superClass->vtableLength)
    {
        for (int i = 0; i < klass->superClass->vtableLength; i++)
        {
            rawVtable.push_back(klass->superClass->vtable[i]);
        }
    }

    for (int i = 0; i < klass->methodCount; i++)
    {
        auto &m = klass->methods[i];
        m.klass = klass;
        m.codeLength = 0;
        m.code = nullptr;

        m.accessFlag = clsfile->methods[i]->accessFlags;
        m.name = world->metaspaceHeap.allocateStr(
            clsfile->mapping[clsfile->methods[i]->nameIndex]->to<classfile::OMClassConstantUtf8>()->data);
        m.descriptor = world->metaspaceHeap.allocateStr(
            clsfile->mapping[clsfile->methods[i]->descIndex]->to<classfile::OMClassConstantUtf8>()->data);

        if (m.isNative())
        {
            m.localLength = argSlots(m.descriptor);
        }
        else
        {
            for (auto attr : clsfile->methods[i]->attrs)
            {
                if (attr->type() == classfile::OMClassAttrType::Code)
                {
                    auto ll = attr->to<classfile::OMClassAttrCode>();
                    m.codeLength = ll->codeLength;
                    m.code = world->metaspaceHeap.allocateArray<uint8_t>(m.codeLength);
                    m.localLength = ll->maxLocals;
                    std::memcpy(m.code, ll->code->data(), ll->codeLength);
                    break;
                }
            }
        }

        if (!m.isStatic() && !m.isPrivate() && !m.isInit())
        {
            bool overwrite = false;
            for (int i = 0; i < rawVtable.size(); i++)
            {
                auto currentMethod = rawVtable[i];
                if (currentMethod->isSame(&m))
                {
                    logger.debug("overwrite {}{}", m.name, m.descriptor);
                    rawVtable[i] = &m;
                    overwrite = true;
                    break;
                }
            }

            if (!overwrite)
            {
                rawVtable.push_back(&m);
            }
        }
    }

    std::sort(rawVtable.begin(), rawVtable.end());
    rawVtable.erase(std::unique(rawVtable.begin(), rawVtable.end()), rawVtable.end());

    klass->vtableLength = rawVtable.size();
    if (klass->vtableLength)
    {
        klass->vtable = world->metaspaceHeap.allocateArray<OMElysiaMethod *>(klass->vtableLength);
        for (int i = 0; i < klass->vtableLength; i++)
        {
            klass->vtable[i] = rawVtable[i];
        }
    }

    klass->fieldCount = clsfile->fields.size();
    klass->fields = world->metaspaceHeap.allocateArray<OMElysiaField>(klass->fieldCount);
    for (int i = 0; i < klass->fieldCount; i++)
    {
        auto &f = klass->fields[i];
        f.name = world->metaspaceHeap.allocateStr(
            clsfile->mapping[clsfile->fields[i]->nameIndex]->to<classfile::OMClassConstantUtf8>()->data);
        f.desc = world->metaspaceHeap.allocateStr(
            clsfile->mapping[clsfile->fields[i]->descIndex]->to<classfile::OMClassConstantUtf8>()->data);
        f.accessFlag = clsfile->fields[i]->accessFlags;
        f.klass = klass;
    }

    klass->initFieldOffsets();

    if (klass->staticLength)
    {
        klass->staticBlock = world->metaspaceHeap.allocate(klass->staticLength);
        std::memset(klass->staticBlock, 0x00, klass->staticLength);
    }
    else
    {
        klass->staticBlock = nullptr;
    }
}
} // namespace openminecraft::vm::elysia
