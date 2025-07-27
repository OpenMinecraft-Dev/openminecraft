#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klassloader.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_field.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include <cstring>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::pixeltower::v0
{
OMKlassLoader::OMKlassLoader(OMPixelTowerHeap *heap, OMPixelTowerHeap *metaspace)
    : logger("OMKlassLoader", this), metaspace(metaspace), heap(heap)
{
}
OMKlassLoader::~OMKlassLoader()
{
    for (auto k : classes)
    {
        if (k->staticBlock)
        {
            mem::allocator::tracedFreeVMData(k->staticBlock);
        }
        if (k->constantPool)
        {
            mem::allocator::tracedFreeVMData(k->constantPool);
        }
        mem::allocator::tracedFreeVMData(k);
    }
}

void OMKlassLoader::loadClass(std::string name)
{
    OMKlass *klass;
    if (fetchClass(name) != nullptr)
    {
        return;
    }

    for (auto fi = files.begin(); fi != files.end(); ++fi)
    {
        auto f = *fi;
        if (f->mapping[f->mapping[f->thisClass]->to<classfile::OMClassConstantClass>()->nameIndex]
                ->to<classfile::OMClassConstantUtf8>()
                ->data == name)
        {
            klass = (OMKlass *)mem::allocator::tracedCallocVMData(1, sizeof(OMKlass));
            klass->kind = Normal;
            klass->heap = heap;
            klass->name = name;
            klass->accessFlags = f->accessFlags;
            klass->raw = f;
            files.erase(fi);
            if (f->superClass != 0)
            {
                auto supClass = f->mapping[f->mapping[f->superClass]->to<classfile::OMClassConstantClass>()->nameIndex]
                                    ->to<classfile::OMClassConstantUtf8>()
                                    ->data;
                loadClass(supClass);
                klass->superClass = fetchClass(supClass);
            }
            else
            {
                klass->superClass = nullptr;
            }

            for (auto i : f->interfaces)
            {
                auto data = f->mapping[f->mapping[i]->to<classfile::OMClassConstantClass>()->nameIndex]
                                ->to<classfile::OMClassConstantUtf8>()
                                ->data;
                loadClass(data);
                klass->interfaces.push_back(fetchClass(data));
            }

            classes.push_back(klass);
            goto loadMethods;
        }
    }

    if (name[0] == '[')
    {
        loadSpecialClass(name);
        return;
    }
    throw err::OMValidationError{err::ClassLoader, "class not found", name};

loadMethods:
    OMMethod *lastMethod = nullptr;
    for (auto method : klass->raw->methods)
    {
        classfile::OMClassAttrCode *code = nullptr;
        for (auto attr : method->attrs)
        {
            if (attr->type() == classfile::OMClassAttrType::Code)
            {
                code = attr->to<classfile::OMClassAttrCode>();
                break;
            }
        }

        uint64_t length = 0;
        if (code != nullptr)
        {
            length = code->codeLength;
        }
        else if ((method->accessFlags & JVM_Acc_Abstract) == 0)
        {
            length = sizeof(void *);
        }

        auto m = (OMMethod *)metaspace->allocate(sizeof(OMMethod) + length);
        m->klass = klass;
        m->codeSize = length;
        m->accessFlags = method->accessFlags;
        m->name = klass->raw->mapping[method->nameIndex]->to<classfile::OMClassConstantUtf8>()->data.c_str();
        m->desc = klass->raw->mapping[method->descIndex]->to<classfile::OMClassConstantUtf8>()->data.c_str();
        if (code != nullptr)
        {
            m->maxLocals = code->maxLocals;
            m->maxStack = code->maxStack;
            memcpy(m->code, code->code->data(), code->codeLength);
        }
        else if ((m->accessFlags & JVM_Acc_Abstract) == 0)
        {
            *(void **)m->code = (void *)33550336;
        }

        if (lastMethod != nullptr)
        {
            lastMethod->next = m;
        }
        else
        {
            klass->methods = m;
        }
        lastMethod = m;
    }

    for (auto field : klass->raw->fields)
    {
        klass->fields.push_back({klass->raw->mapping[field->nameIndex]->to<classfile::OMClassConstantUtf8>()->data,
                                 klass->raw->mapping[field->descIndex]->to<classfile::OMClassConstantUtf8>()->data, 0,
                                 field->accessFlags});
    }

    klass->constantPool = (uint64_t *)mem::allocator::tracedCallocVMData(klass->raw->mapping.size(), sizeof(uint64_t));
    for (auto cppair : klass->raw->mapping)
    {
#define acc (klass->constantPool + cppair.first)
        switch (cppair.second->type())
        {
        case classfile::OMClassConstantType::Long:
            *(jlong *)acc = cppair.second->to<classfile::OMClassConstantLong>()->data;
            break;
        case classfile::OMClassConstantType::Integer:
            *(jint *)acc = cppair.second->to<classfile::OMClassConstantInteger>()->data;
            break;
        case classfile::OMClassConstantType::Float:
            *(jfloat *)acc = cppair.second->to<classfile::OMClassConstantFloat>()->data;
            break;
        case classfile::OMClassConstantType::Double:
            *(jdouble *)acc = cppair.second->to<classfile::OMClassConstantDouble>()->data;
            break;
        case classfile::OMClassConstantType::Class: {
            auto &cls = klass->raw->mapping[cppair.second->to<classfile::OMClassConstantClass>()->nameIndex]
                            ->to<classfile::OMClassConstantUtf8>()
                            ->data;
            loadClass(cls);
            *(OMKlass **)acc = fetchClass(cls);
            break;
        }
        case classfile::OMClassConstantType::MethodRef: {
            auto temp = cppair.second->to<classfile::OMClassConstantMethodRef>();
            auto &clsname =
                klass->raw
                    ->mapping[klass->raw->mapping[temp->classIndex]->to<classfile::OMClassConstantClass>()->nameIndex]
                    ->to<classfile::OMClassConstantUtf8>()
                    ->data;
            loadClass(clsname);
            auto temp2 = klass->raw->mapping[temp->nameAndTypeIndex]->to<classfile::OMClassConstantNameAndType>();
            auto &name = klass->raw->mapping[temp2->nameIndex]->to<classfile::OMClassConstantUtf8>()->data;
            auto &desc = klass->raw->mapping[temp2->descIndex]->to<classfile::OMClassConstantUtf8>()->data;
            auto met = fetchClass(clsname)->methods;
            while (met != nullptr)
            {
                if (strcmp(met->name, name.c_str()) == 0 && strcmp(met->desc, desc.c_str()) == 0)
                {
                    *(OMMethod **)acc = met;
                    break;
                }

                met = met->next;
            }
            break;
        }

        case classfile::OMClassConstantType::Utf8:
        case classfile::OMClassConstantType::String:
        case classfile::OMClassConstantType::FieldRef:
        case classfile::OMClassConstantType::InterfaceMethodRef:
        case classfile::OMClassConstantType::NameAndType:
        case classfile::OMClassConstantType::MethodHandle:
        case classfile::OMClassConstantType::MethodType:
        case classfile::OMClassConstantType::Dynamic:
        case classfile::OMClassConstantType::InvokeDynamic:
        case classfile::OMClassConstantType::Module:
        case classfile::OMClassConstantType::Package:
            break;
        }
    }

    classInit(klass);
}

// for array classes
void OMKlassLoader::loadSpecialClass(std::string name)
{
    int i = 0;
    auto r = bytecode::descriptor::decodeType(name, &i);
    if (r.type == util::Err)
    {
        throw r.unwrap_err();
    }

    auto klass = (OMKlass *)mem::allocator::tracedCallocVMData(1, sizeof(OMKlass));
    klass->kind = Array;
    klass->name = name;
    klass->superClass = fetchClass("java/lang/Object");
    klass->methods = nullptr;
    klass->accessFlags = JVM_Acc_Public;
    klass->length = 0;
    klass->staticBlock = nullptr;
    klass->staticLength = 0;
    klass->heap = heap;
    classes.push_back(klass);
}

void OMKlassLoader::classInit(OMKlass *klass)
{
    klass->staticLength = 0;
    klass->length = klass->superClass ? klass->superClass->length : 0;
    for (auto &f : klass->fields)
    {
        int i = 0;
        auto result = bytecode::descriptor::decodeType(f.desc, &i);
        if (result.type == util::Err)
        {
            throw result.unwrap_err();
        }

        auto &loff = (f.accessFlags & JVM_Acc_Static) ? klass->staticLength : klass->length;

        f.offset = loff;

        switch (hash_compile_time(result.unwrap().c_str()))
        {
        case "byte"_hash:
        case "boolean"_hash:
            loff += 1 - (loff % 1);
            break;
        case "short"_hash:
        case "char"_hash:
            loff += 2 - (loff % 2);
            break;
        case "int"_hash:
        case "float"_hash:
            loff += 4 - (loff % 4);
            break;
        case "long"_hash:
        case "double"_hash:
            loff += 8 - (loff % 8);
            break;
        default:
            loff += heap->ptrSize() - (loff % heap->ptrSize());
            break;
        }
    }

    klass->staticBlock = mem::allocator::tracedCallocVMData(1, klass->staticLength);
}

OMKlass *OMKlassLoader::fetchClass(std::string name)
{
    for (auto k : classes)
    {
        if (k->name == name)
        {
            return k;
        }
    }
    return nullptr;
}
} // namespace openminecraft::vm::pixeltower::v0