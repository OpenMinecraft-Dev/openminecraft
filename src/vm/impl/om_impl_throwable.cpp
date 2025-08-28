#include "openminecraft/vm/impl/om_impl_throwable.hpp"

using namespace openminecraft::vm::pixeltower::v0;

namespace openminecraft::vm::impl
{
std::any java_lang_Throwable_fillInStackTrace(OMPixelTower *tower, std::any *d)
{
    auto arrcls = tower->loader->fetchClass({bytecode::descriptor::Array, "java/lang/StackTraceElement", 1, bytecode::descriptor::Reference});
    auto cls = tower->loader->fetchClass({bytecode::descriptor::Reference, "java/lang/StackTraceElement"});
    auto thr = tower->loader->fetchClass({bytecode::descriptor::Reference, "java/lang/Throwable"});
    auto fr = currentThread.currentFrame;
    auto pc = currentThread.pc;

    std::vector<OMOOPDesc *> frames;
    while (fr)
    {
        auto frame = cls->allocateInstance();
        frames.push_back(frame);
        for (auto &f : cls->fields)
        {
            if (f.name == "declaringClass")
            {
                stackPushAccess<void *>(frame);
                stackPushAccess<void *>(fr->method->klass->oop);
                accessField(&f);
            }
            else if (f.name == "name")
            {
                stackPushAccess<void *>(frame);
                stackPushAccess<void *>(tower->createString(fr->method->name));
                accessField(&f);
            }
            else if (f.name == "descriptor")
            {
                stackPushAccess<void *>(frame);
                stackPushAccess<void *>(tower->createString(fr->method->desc));
                accessField(&f);
            }
            else if (f.name == "sourceFile")
            {
                stackPushAccess<void *>(frame);
                stackPushAccess<void *>(tower->createString(fr->method->klass->source));
                accessField(&f);
            }
            else if (fr->method->sourceMap && f.name == "line")
            {
                stackPushAccess<void *>(frame);
                stackPushAccess<jint>(fr->method->sourceMap->at(static_cast<jint>(static_cast<size_t>(pc - fr->method->code))));
                accessField(&f);
            }
        }
        pc = static_cast<uint8_t *>(fr->returnAddr);
        fr = fr->prev;
    }

    auto result = arrcls->allocateArray(static_cast<jint>(frames.size()));
    for (int i = 0; i < frames.size(); i++)
    {
        if (tower->heap->ptrCompEnabled())
        {
            result->array<uint32_t>()[i] = tower->heap->compressPtr(frames[i]);
        }
        else
        {
            result->array<void *>()[i] = frames[i];
        }
    }

    for (auto &field : thr->fields)
    {
        if (field.name == "stacktrace")
        {
            stackPushAccess<void *>(std::any_cast<void *>(d[0]));
            stackPushAccess<void *>(result);
            accessField(&field);
        }
    }

    return nullptr;
}
} // namespace openminecraft::vm::impl