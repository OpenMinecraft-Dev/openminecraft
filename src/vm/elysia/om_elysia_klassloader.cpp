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
    klass->name = reinterpret_cast<jbyte *>(world->metaspaceHeap.allocateStr(s));
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
    klass->name = reinterpret_cast<jbyte *>(world->metaspaceHeap.allocateStr(s));
    klass->type = PrimitiveKlass;
    klass->ptrLength = world->mainHeap.ptrLength();

    markKlass(klass);
    return klass;
}
OMElysiaArrayKlass *OMElysiaKlassloader::constructArrayClass(OMElysiaKlass *k)
{
    auto rawname = buildArray(reinterpret_cast<char *>(k->name));

    auto klass = world->metaspaceHeap.allocate<OMElysiaArrayKlass>();
    klass->accessFlag = JVM_Acc_Public;
    klass->superClass = nullptr;
    klass->name = reinterpret_cast<jbyte *>(world->metaspaceHeap.allocateStr(rawname));
    klass->type = ArrayKlass;
    klass->lowerDim = k;
    klass->ptrLength = world->mainHeap.ptrLength();

    if (k->type == ArrayKlass)
    {
        reinterpret_cast<OMElysiaArrayKlass *>(k)->higherDim = klass;
    }

    markKlass(klass);
    return klass;
}

void OMElysiaKlassloader::markKlass(OMElysiaKlass *klass)
{
    loadedClasses[binary::hash::hash_compile_time(reinterpret_cast<char *>(klass->name))] = klass;
}

void OMElysiaKlassloader::initClass(OMElysiaKlass *c)
{
    if (c->type == InstanceKlass)
    {
        std::ifstream istr(fmt::format("vmstd/out/{}.class", reinterpret_cast<char *>(c->name)), std::ios::binary);

        loadClass(&istr);
    }
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

    if (klass->type == InstanceKlass && reinterpret_cast<OMElysiaInstanceKlass *>(klass)->fields)
    {
        auto iklass = reinterpret_cast<OMElysiaInstanceKlass *>(klass);
        for (int i = 0; i < iklass->fieldCount; i++)
        {
            auto &f = iklass->fields[i];
            world->metaspaceHeap.deallocateStr(f.name);
            world->metaspaceHeap.deallocateStr(f.desc);
        }

        world->metaspaceHeap.deallocateArray(iklass->fields, iklass->fieldCount);
    }

    auto l = reinterpret_cast<char *>(klass->name);
    world->metaspaceHeap.deallocateStr(l);
    switch (klass->type)
    {
    case InstanceKlass: {
        world->metaspaceHeap.deallocate(klass, sizeof(OMElysiaInstanceKlass));
        auto ii = reinterpret_cast<OMElysiaInstanceKlass *>(klass);
        if (ii->interfaceImpls)
        {
            world->metaspaceHeap.deallocate(ii->interfaceImpls, sizeof(void *) * ii->interfaceImplCount);
        }

        if (ii->staticBlock)
        {
            world->metaspaceHeap.deallocate(ii->staticBlock, ii->staticLength);
        }
        break;
    }
    case PrimitiveKlass:
    default:
        break;
    }

    world->metaspaceHeap.deallocate(klass);
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
    auto klasskey = binary::hash::hash_compile_time(clsname.c_str());
    if (!loadedClasses.count(klasskey))
    {
        constructInstanceClassShell(clsname);
    }
    auto klassraw = loadedClasses[klasskey];

    if (klassraw->type != InstanceKlass)
    {
        throw std::logic_error("not allowed!");
    }

    auto klass = reinterpret_cast<OMElysiaInstanceKlass *>(klassraw);

    if (clsfile->superClass)
    {
        auto supclsname =
            clsfile->mapping[clsfile->mapping[clsfile->superClass]->to<classfile::OMClassConstantClass>()->nameIndex]
                ->to<classfile::OMClassConstantUtf8>()
                ->data;

        if (!loadedClasses.count(binary::hash::hash_compile_time(supclsname.c_str())))
        {
            auto sup = constructInstanceClassShell(supclsname);
            initClass(sup);

            klass->superClass = sup;
        }
    }
    if (!clsfile->interfaces.empty())
    {
        for (auto i : clsfile->interfaces)
        {
            auto supclsname = clsfile->mapping[clsfile->mapping[i]->to<classfile::OMClassConstantClass>()->nameIndex]
                                  ->to<classfile::OMClassConstantUtf8>()
                                  ->data;
            auto ithash = binary::hash::hash_compile_time(supclsname.c_str());
            if (!loadedClasses.count(ithash))
            {
                auto sup = constructInstanceClassShell(supclsname);
                initClass(sup);
            }
        }
    }

    klass->constantPoolRaw = clsfile->mapping;

    klass->accessFlag = clsfile->accessFlags;

    klass->methodCount = clsfile->methods.size();
    klass->methods = world->metaspaceHeap.allocateArray<OMElysiaMethod>(klass->methodCount);
    for (int i = 0; i < klass->methodCount; i++)
    {
        auto &m = klass->methods[i];
        m.codeLength = 0;
        m.code = nullptr;

        m.accessFlag = klass->methods[i].accessFlag;
        m.name = world->metaspaceHeap.allocateStr(
            clsfile->mapping[clsfile->methods[i]->nameIndex]->to<classfile::OMClassConstantUtf8>()->data);
        m.descriptor = world->metaspaceHeap.allocateStr(
            clsfile->mapping[clsfile->methods[i]->descIndex]->to<classfile::OMClassConstantUtf8>()->data);

        for (auto attr : clsfile->methods[i]->attrs)
        {
            if (attr->type() == classfile::OMClassAttrType::Code)
            {
                auto ll = attr->to<classfile::OMClassAttrCode>();
                m.codeLength = ll->codeLength;
                m.code = world->metaspaceHeap.allocateArray<uint8_t>(m.codeLength);
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
    }

    klass->initFieldOffsets();

    if (klass->staticLength)
    {
        klass->staticBlock = world->metaspaceHeap.allocate(klass->staticLength);
        std::memset(klass->staticBlock, 0x00, klass->staticLength);
    }
}
} // namespace openminecraft::vm::elysia
