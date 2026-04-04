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

void OMElysiaKlassloader::initClasses()
{
    for (auto &c : loadedClasses)
    {
        if (c.second->type == InstanceKlass)
        {
            std::ifstream istr(fmt::format("vmstd/out/{}.class", reinterpret_cast<char *>(c.second->name)),
                               std::ios::binary);
            logger.info("Class loading: {}", reinterpret_cast<char *>(c.second->name));
            loadClass(&istr);
        }
    }
}

void OMElysiaKlassloader::unloadClass(OMElysiaKlass *klass)
{
    if (klass->methods) {
	for (int i = 0; i < klass->methodCount; i++) {
	    auto &m = klass->methods[i];
	    world->metaspaceHeap.deallocateStr(m.name);
	    world->metaspaceHeap.deallocateStr(m.descriptor);
	    if (m.code) { world->metaspaceHeap.deallocate(m.code, m.codeLength); }
	}
	world->metaspaceHeap.deallocateArray(klass->methods, klass->methodCount);
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
        break;
    }
    case PrimitiveKlass:
    default:
        break;
    }
}

void OMElysiaKlassloader::loadClass(std::istream *istr)
{
    classfile::OMClassFileParser par(istr);
    auto clsfile = par.parse().unwrap();

    logger.debug("Class file: {}.{}", clsfile->major, clsfile->minor);
    logger.debug("*** Methods ***");
    for (auto &m : clsfile->methods)
    {
        logger.debug("{}{}", clsfile->mapping[m->nameIndex]->to<classfile::OMClassConstantUtf8>()->data,
                     clsfile->mapping[m->descIndex]->to<classfile::OMClassConstantUtf8>()->data);
    }
    logger.debug("*** Fields ***");
    for (auto &f : clsfile->fields)
    {
        logger.debug("{}:{}", clsfile->mapping[f->nameIndex]->to<classfile::OMClassConstantUtf8>()->data,
                     clsfile->mapping[f->descIndex]->to<classfile::OMClassConstantUtf8>()->data);
    }

    auto clsname = clsfile->mapping[clsfile->mapping[clsfile->thisClass]->to<classfile::OMClassConstantClass>()->nameIndex]->to<classfile::OMClassConstantUtf8>()->data;
    auto klass = loadedClasses[binary::hash::hash_compile_time(clsname.c_str())];

    klass->accessFlag = clsfile->accessFlags;

    klass->methodCount = clsfile->methods.size();
    klass->methods = world->metaspaceHeap.allocateArray<OMElysiaMethod>(klass->methodCount);
    for (int i = 0; i < klass->methodCount; i++) {
        auto &m = klass->methods[i];
	m.codeLength = 0;
	m.code = nullptr;

	m.accessFlag = klass->methods[i].accessFlag;
        m.name = world->metaspaceHeap.allocateStr(clsfile->mapping[clsfile->methods[i]->nameIndex]->to<classfile::OMClassConstantUtf8>()->data);
	m.descriptor = world->metaspaceHeap.allocateStr(clsfile->mapping[clsfile->methods[i]->descIndex]->to<classfile::OMClassConstantUtf8>()->data);
    }
}
} // namespace openminecraft::vm::elysia
