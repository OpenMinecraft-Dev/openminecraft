#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "fmt/format.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
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

void OMElysiaKlassloader::unloadClass(OMElysiaKlass *klass)
{
    if (klass->methods)
    {
        for (int i = 0; i < klass->methodCount; i++)
        {
            auto &m = klass->methods[i];
            world->metaspaceHeap.deallocateStr(m.name);
            world->metaspaceHeap.deallocateStr(m.descriptor);
            if (m.code)
            {
                world->metaspaceHeap.deallocate(m.code, m.codeLength);
            }
        }

        world->metaspaceHeap.deallocateArray(klass->methods, klass->methodCount);
    }

    if (klass->isInstance() && klass->toInstance()->fields)
    {
        auto iklass = klass->toInstance();
        for (int i = 0; i < iklass->fieldCount; i++)
        {
            auto &f = iklass->fields[i];
            world->metaspaceHeap.deallocateStr(f.name);
            world->metaspaceHeap.deallocateStr(f.desc);
        }

        world->metaspaceHeap.deallocateArray(iklass->fields, iklass->fieldCount);
    }

    world->metaspaceHeap.deallocateStr(klass->name);
    switch (klass->type)
    {
    case InstanceKlass: {
        world->metaspaceHeap.deallocate(klass, sizeof(OMElysiaInstanceKlass));
        auto ii = klass->toInstance();
        if (ii->interfaceImpls)
        {
            world->metaspaceHeap.deallocate(ii->interfaceImpls, sizeof(void *) * ii->interfaceImplCount);
        }

        if (ii->staticBlock)
        {
            world->metaspaceHeap.deallocate(ii->staticBlock, ii->staticLength);
        }

        world->metaspaceHeap.deallocateArray(ii->constantPool, ii->constantPoolCount);
        break;
    }
    case PrimitiveKlass:
    default:
        break;
    }

    world->metaspaceHeap.deallocate(klass);
}

OMElysiaKlass *OMElysiaKlassloader::findClass(std::string s)
{
    return loadedClasses[binary::hash::hash_compile_time(s.c_str())];
}

void OMElysiaKlassloader::loadClass(std::string name)
{
    std::ifstream istr(fmt::format("vmstd/out/{}.class", name), std::ios::binary);
    loadClass(&istr);
}

void OMElysiaKlassloader::loadClass(std::istream *istr)
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
            loadClass(supclsname);
            supk = findClass(supclsname);
        }

        klass->superClass = findClass(supclsname);
    }
    if (!clsfile->interfaces.empty())
    {
        klass->interfaceImplCount = clsfile->interfaces.size();
        klass->interfaceImpls = world->metaspaceHeap.allocateArray<OMElysiaKlass *>(klass->interfaceImplCount);
        int i = 0;
        for (auto i : clsfile->interfaces)
        {
            auto supclsname = clsfile->mapping[clsfile->mapping[i]->to<classfile::OMClassConstantClass>()->nameIndex]
                                  ->to<classfile::OMClassConstantUtf8>()
                                  ->data;
            auto ithash = binary::hash::hash_compile_time(supclsname.c_str());
            if (!loadedClasses.count(ithash))
            {
                loadClass(supclsname);
            }

            klass->interfaceImpls[i] = findClass(supclsname);
            i++;
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

    klass->accessFlag = clsfile->accessFlags;

    klass->methodCount = clsfile->methods.size();
    klass->methods = world->metaspaceHeap.allocateArray<OMElysiaMethod>(klass->methodCount);
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
