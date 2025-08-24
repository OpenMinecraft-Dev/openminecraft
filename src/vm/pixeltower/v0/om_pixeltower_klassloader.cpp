#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klassloader.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_field.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include <cstring>
#include <unordered_map>
#include <vector>

using openminecraft::vm::bytecode::descriptor::OMTypeDesc;

namespace openminecraft::vm::pixeltower::v0
{
OMKlassLoader::OMKlassLoader(OMPixelTowerHeap *heap, OMPixelTowerHeap *metaspace, OMInterpreter *interpreter)
    : logger("OMKlassLoader", this), metaspace(metaspace), heap(heap), interpreter(interpreter)
{
}
OMKlassLoader::~OMKlassLoader()
{
    for (auto k : classes)
    {
        if (k->staticBlock)
        {
            metaspace->deallocate(k->staticBlock, k->staticLength);
        }
        if (k->constantPool)
        {
            metaspace->deallocate(k->constantPool, k->raw->mapping.size() * sizeof(void *));
        }
        if (k->vtable)
        {
            k->vtable->~unordered_map<std::string, OMMethod *>();
            metaspace->deallocate(k->vtable, sizeof(std::unordered_map<std::string, OMMethod *>));
        }

        auto m = k->methods;
        while (m)
        {
            auto nxt = m->next;
            if (m->argCheck)
            {
                m->argCheck->~unordered_map<jint, OMKlass *>();
                metaspace->deallocate(m->argCheck, sizeof(std::unordered_map<jint, OMKlass *>));
            }
            metaspace->deallocate(m, sizeof(OMMethod) + m->codeSize);
            if (m->exceptionHandlers)
            {
                m->exceptionHandlers->~vector<OMMethodExceptionCaught>();
                metaspace->deallocate(m->exceptionHandlers, sizeof(std::vector<OMMethodExceptionCaught>));
            }
            m = nxt;
        }

        metaspace->deallocate(k, sizeof(OMKlass));
    }
}

void OMKlassLoader::initBase()
{
    loadClass({bytecode::descriptor::Reference, "java/lang/String"});
}

void OMKlassLoader::loadClass(OMTypeDesc name)
{
    if (fetchClass(name) != nullptr)
    {
        return;
    }

    if (name.type == bytecode::descriptor::Array)
    {
        loadSpecialClass(name);
        return;
    }

    if (name.type != bytecode::descriptor::Reference)
    {
        return;
    }

    OMKlass *klass;

    logger.debug("try loading class {}", bytecode::descriptor::restore(name));
    for (auto fi = files.begin(); fi != files.end(); ++fi)
    {
        auto f = *fi;
        if (f->mapping[f->mapping[f->thisClass]->to<classfile::OMClassConstantClass>()->nameIndex]
                ->to<classfile::OMClassConstantUtf8>()
                ->data == name.name)
        {
            validator.validate(f, name.name);
            klass = klassConstruct(fi, name);

            klassVtableInit(klass);
            klassMethodInit(klass);
            klassConstantPoolLoad(klass);
            klassFieldInit(klass);
            klassOopCreate(klass);

            return;
        }
    }

    // geopelia: we need this check for teh case below:
    // String -> CharSequence -> String -> ...
    // CharSequence has method toString, and it will loads String recursively if this check doesn't exist
    for (auto l : classes)
    {
        if (l->name == bytecode::descriptor::restore(name))
        {
            return;
        }
    }
    throw err::OMValidationError{err::ClassLoader, "class not found", name.name};
}

void OMKlassLoader::klassOopCreate(OMKlass *klass)
{
    auto clsklass = fetchClass({bytecode::descriptor::Reference, "java/lang/String"});
    auto tgt = clsklass->allocateInstance();
    klass->oop = tgt;
    auto ii = interpreter->tower->createString(klass->name);

    for (auto &f : clsklass->fields)
    {
        if (f.name == "name")
        {
            stackPushAccess<void *>(tgt);
            stackPushAccess<void *>(ii);
            accessField(&f);
        }
        if (f.name == "nativePtr")
        {
            stackPushAccess<void *>(tgt);
            stackPushAccessW<jlong>((jlong)(size_t)klass);
            accessField(&f);
        }
    }
}

OMKlass *OMKlassLoader::klassConstruct(
    std::vector<std::shared_ptr<openminecraft::vm::classfile::OMClassFile>>::iterator fi, OMTypeDesc desc)
{
    auto f = *fi;
    auto klass = (OMKlass *)metaspace->allocate(sizeof(OMKlass));
    memset((void *)klass, 0, sizeof(OMKlass));
    void *rawmap = metaspace->allocate(sizeof(std::unordered_map<std::string, OMMethod *>));
    klass->vtable = new (rawmap) std::unordered_map<std::string, OMMethod *>();
    klass->heap = heap;
    klass->name = bytecode::descriptor::restore(desc);
    klass->accessFlags = f->accessFlags;
    klass->raw = f;
    files.erase(fi);
    classes.push_back(klass);

    // geopelia: determine the type of the class
    if (klass->accessFlags & JVM_Acc_Abstract)
    {
        if (klass->accessFlags & JVM_Acc_Interface)
        {
            klass->kind = Interface;
        }
        else if (klass->accessFlags & JVM_Acc_Annotation)
        {
            klass->kind = Annotation;
        }
        else
        {
            klass->kind = AbstractClass;
        }
    }
    else if (klass->accessFlags & JVM_Acc_Enum)
    {
        klass->kind = Enum;
    }
    else
    {
        klass->kind = Normal;
    }

    klass->superClass = nullptr;
    if (f->superClass != 0)
    {
        auto supClass = f->mapping[f->mapping[f->superClass]->to<classfile::OMClassConstantClass>()->nameIndex]
                            ->to<classfile::OMClassConstantUtf8>()
                            ->data;
        OMTypeDesc dd = {bytecode::descriptor::Reference, supClass};
        loadClass(dd);
        klass->superClass = fetchClass(dd);
    }

    for (auto i : f->interfaces)
    {
        auto data = f->mapping[f->mapping[i]->to<classfile::OMClassConstantClass>()->nameIndex]
                        ->to<classfile::OMClassConstantUtf8>()
                        ->data;
        OMTypeDesc dd = {bytecode::descriptor::Reference, data};
        loadClass(dd);
        klass->interfaces.push_back(fetchClass(dd));
    }

    return klass;
}

void OMKlassLoader::klassMethodInit(OMKlass *klass)
{
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

        int length = 0;
        if (code != nullptr)
        {
            length = code->codeLength;
        }
        else if ((method->accessFlags & JVM_Acc_Abstract) == 0)
        {
            length = sizeof(void *);
        }

        auto m = static_cast<OMMethod *>(metaspace->allocate(sizeof(OMMethod) + length));
        m->klass = klass;
        m->codeSize = length;
        m->accessFlags = method->accessFlags;
        m->name = klass->raw->mapping[method->nameIndex]->to<classfile::OMClassConstantUtf8>()->data.c_str();
        m->desc = klass->raw->mapping[method->descIndex]->to<classfile::OMClassConstantUtf8>()->data.c_str();

        auto target = bytecode::descriptor::decodeSignatureTo(
            klass->raw->mapping[method->descIndex]->to<classfile::OMClassConstantUtf8>()->data, &m->args);

        auto rawcheck = metaspace->allocate(sizeof(std::unordered_map<jint, OMKlass *>));
        m->argCheck = new (rawcheck) std::unordered_map<jint, OMKlass *>();
        loadClass(target.second);
        (*m->argCheck)[-1] = fetchClass(target.second);

        m->args = 0;
        if ((m->accessFlags & JVM_Acc_Static) == 0)
        {
            (*m->argCheck)[0] = klass;
            m->args++;
        }

        for (auto g : target.first)
        {
            loadClass(g);
            auto dd = fetchClass(g);
            if (dd)
            {
                (*m->argCheck)[-1] = dd;
            }

            m->args++;
            if (g.type == bytecode::descriptor::Double || g.type == bytecode::descriptor::Long)
            {
                m->args++;
            }
        }

        if (code != nullptr)
        {
            m->maxLocals = code->maxLocals;
            m->maxStack = code->maxStack;
            memcpy(m->code, code->code->data(), code->codeLength);

            auto hnd = metaspace->allocate(sizeof(std::vector<OMMethodExceptionCaught>));
            m->exceptionHandlers = new (hnd) std::vector<OMMethodExceptionCaught>();
            for (auto handler : code->excTable)
            {
                auto d = klass->raw->mapping[klass->raw->mapping[handler.catchType]->to<classfile::OMClassConstantClass>()->nameIndex]->to<classfile::OMClassConstantUtf8>()->data;
                OMTypeDesc desc = {bytecode::descriptor::Reference, d};

                loadClass(desc);
                m->exceptionHandlers->push_back({handler.startPc, handler.endPc, handler.handlerPc, fetchClass(desc)});
            }
        }
        else if ((m->accessFlags & JVM_Acc_Abstract) == 0)
        {
            // geopelia: load native function handles
            auto fnn = fmt::format("{}.{}{}", klass->name, m->name, m->desc);
            if (nativeMethods.count(fnn))
            {
                *reinterpret_cast<void **>(m->code) = (void *)nativeMethods[fnn];
            }
            else
            {
                *reinterpret_cast<void **>(m->code) = nullFunction;
            }
        }

        if ((m->accessFlags & JVM_Acc_Static) == 0 && (m->accessFlags & JVM_Acc_Private) == 0 &&
            (m->accessFlags & JVM_Acc_Final) == 0 && strcmp(m->name, "<init>") != 0)
        {
            (*klass->vtable)[fmt::format("{}{}", m->name, m->desc)] = m;
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
}

void OMKlassLoader::klassVtableInit(OMKlass *klass)
{
    if (klass->superClass)
    {
        for (auto p : *klass->superClass->vtable)
        {
            (*klass->vtable)[p.first] = p.second;
        }
    }
}

void OMKlassLoader::klassFieldInit(OMKlass *klass)
{
    for (auto field : klass->raw->fields)
    {
        klass->fields.push_back({klass->raw->mapping[field->nameIndex]->to<classfile::OMClassConstantUtf8>()->data,
                                 klass->raw->mapping[field->descIndex]->to<classfile::OMClassConstantUtf8>()->data,
                                 klass, 0, field->accessFlags});
    }

    klass->staticLength = 0;
    klass->length = klass->superClass ? klass->superClass->length : 0;
    for (auto &f : klass->fields)
    {
        int i = 0;
        auto result = bytecode::descriptor::decodeTypeTo(f.desc, &i);

        auto &loff = (f.accessFlags & JVM_Acc_Static) ? klass->staticLength : klass->length;

        f.offset = loff;

        switch (result.type)
        {
        case bytecode::descriptor::Byte:
        case bytecode::descriptor::Boolean:
            loff += 1 - (loff % 1);
            break;
        case bytecode::descriptor::Char:
        case bytecode::descriptor::Short:
            loff += 2 - (loff % 2);
            break;
        case bytecode::descriptor::Int:
        case bytecode::descriptor::Float:
            loff += 4 - (loff % 4);
            break;
        case bytecode::descriptor::Long:
        case bytecode::descriptor::Double:
            loff += 8 - (loff % 8);
            break;
        default:
        case bytecode::descriptor::Array:
        case bytecode::descriptor::Void:
        case bytecode::descriptor::Reference:
            loff += heap->ptrSize() - (loff % heap->ptrSize());
            break;
        }
    }

    klass->staticBlock = metaspace->allocate(klass->staticLength);
    memset(klass->staticBlock, 0, klass->staticLength);

    auto me = klass->methods;
    while (me)
    {
        if (strcmp(me->name, "<clinit>") == 0)
        {
            logger.info("clinit found for {}, executing", klass->name);

            interpreter->call(me, currentThread.pc);
        }
        me = me->next;
    }
}

void OMKlassLoader::klassConstantPoolLoad(OMKlass *klass)
{
    auto cpool = klass->raw->mapping.size() * sizeof(void *);
    klass->constantPool = (void **)metaspace->allocate(cpool);
    memset(klass->constantPool, 0, cpool);
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
        case classfile::OMClassConstantType::String:
            *(void **)acc = interpreter->tower->createString(
                klass->raw->mapping[cppair.second->to<classfile::OMClassConstantString>()->stringIndex]
                    ->to<classfile::OMClassConstantUtf8>()
                    ->data);
            break;
        case classfile::OMClassConstantType::MethodRef:
        case classfile::OMClassConstantType::Class:
        case classfile::OMClassConstantType::FieldRef:
        case classfile::OMClassConstantType::Utf8:
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
}

OMMethod *OMKlassLoader::lazyMethodInit(OMKlass *klass, uint16_t id)
{
    auto target = reinterpret_cast<OMKlass **>(klass->constantPool + id);
    auto temp = klass->raw->mapping[id]->to<classfile::OMClassConstantMethodRef>();
    auto &clsname =
        klass->raw->mapping[klass->raw->mapping[temp->classIndex]->to<classfile::OMClassConstantClass>()->nameIndex]
            ->to<classfile::OMClassConstantUtf8>()
            ->data;
    loadClass({bytecode::descriptor::Reference, clsname});
    auto temp2 = klass->raw->mapping[temp->nameAndTypeIndex]->to<classfile::OMClassConstantNameAndType>();
    auto &name = klass->raw->mapping[temp2->nameIndex]->to<classfile::OMClassConstantUtf8>()->data;
    auto &desc = klass->raw->mapping[temp2->descIndex]->to<classfile::OMClassConstantUtf8>()->data;
    auto met = fetchClass({bytecode::descriptor::Reference, clsname})->methods;
    while (met != nullptr)
    {
        if (strcmp(met->name, name.c_str()) == 0 && strcmp(met->desc, desc.c_str()) == 0)
        {
            *(OMMethod **)target = met;
            return met;
        }

        met = met->next;
    }

    return nullptr;
}

OMField *OMKlassLoader::lazyFieldInit(OMKlass *klass, uint16_t id)
{
    auto target = reinterpret_cast<OMKlass **>(klass->constantPool + id);
    auto temp = klass->raw->mapping[id]->to<classfile::OMClassConstantFieldRef>();
    auto &clsname =
        klass->raw->mapping[klass->raw->mapping[temp->classIndex]->to<classfile::OMClassConstantClass>()->nameIndex]
            ->to<classfile::OMClassConstantUtf8>()
            ->data;
    loadClass({bytecode::descriptor::Reference, clsname});
    auto temp2 = klass->raw->mapping[temp->nameAndTypeIndex]->to<classfile::OMClassConstantNameAndType>();
    auto &name = klass->raw->mapping[temp2->nameIndex]->to<classfile::OMClassConstantUtf8>()->data;
    auto &desc = klass->raw->mapping[temp2->descIndex]->to<classfile::OMClassConstantUtf8>()->data;

    auto cls = fetchClass({bytecode::descriptor::Reference, clsname});
    while (cls)
    {
        for (auto &fi : cls->fields)
        {
            if (fi.name == name && fi.desc == desc)
            {
                *(OMField **)target = &fi;
                return &fi;
            }
        }
        cls = cls->superClass;
    }

    return nullptr;
}

OMKlass *OMKlassLoader::lazyClassInit(OMKlass *klass, uint16_t id)
{
    auto target = reinterpret_cast<OMKlass **>(klass->constantPool + id);
    if (!*target)
    {
        auto &cls = klass->raw->mapping[klass->raw->mapping[id]->to<classfile::OMClassConstantClass>()->nameIndex]
                        ->to<classfile::OMClassConstantUtf8>()
                        ->data;
        loadClass({bytecode::descriptor::Reference, cls});
        *target = fetchClass({bytecode::descriptor::Reference, cls});
        return *target;
    }

    return nullptr;
}

// gino: for array classes, we use this method to construct OMKlass objects
void OMKlassLoader::loadSpecialClass(OMTypeDesc name)
{
    auto klass = (OMKlass *)metaspace->allocate(sizeof(OMKlass));
    memset((void *)klass, 0, sizeof(OMKlass));
    klass->kind = Array;
    // gino: msvc bug? string doesn't copy with the operator equals call
    auto tgt = bytecode::descriptor::restore(name);
    klass->name = tgt;
    tgt.copy((char *)klass->name.c_str(), tgt.length());
    klass->superClass = fetchClass({bytecode::descriptor::Reference, "java/lang/Object"});
    klass->methods = nullptr;
    klass->accessFlags = JVM_Acc_Public;

    if (name.depth == 1)
    {
        switch (name.subtype)
        {
        case bytecode::descriptor::Boolean:
        case bytecode::descriptor::Byte:
            klass->length = 1;
            break;
        case bytecode::descriptor::Char:
        case bytecode::descriptor::Short:
            klass->length = 2;
            break;
        case bytecode::descriptor::Int:
        case bytecode::descriptor::Float:
            klass->length = 4;
            break;
        case bytecode::descriptor::Long:
        case bytecode::descriptor::Double:
            klass->length = 8;
            break;
        case bytecode::descriptor::Array:
        case bytecode::descriptor::Void:
        case bytecode::descriptor::Reference:
            klass->length = heap->ptrSize();
            break;
        }
    }
    else
    {
        klass->length = heap->ptrSize();
    }

    if (name.subtype == bytecode::descriptor::Reference)
    {
        loadClass({bytecode::descriptor::Reference, name.name});
    }

    klass->staticBlock = nullptr;
    klass->staticLength = 0;
    klass->heap = heap;
    classes.push_back(klass);
}

OMKlass *OMKlassLoader::fetchClass(OMTypeDesc name)
{
    for (auto k : classes)
    {
        if (k->name == bytecode::descriptor::restore(name))
        {
            return k;
        }
    }
    return nullptr;
}
} // namespace openminecraft::vm::pixeltower::v0
