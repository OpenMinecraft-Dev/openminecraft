#include "openminecraft/vm/impl/om_impl_throwable.hpp"

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interface.hpp"

using namespace openminecraft::vm::pixeltower::v0;

namespace openminecraft::vm::impl
{
std::any java_lang_Throwable_fillInStackTrace(OMPixelTower *tower, std::any *d)
{
    auto arrcls = tower->loader->fetchClass(
        {bytecode::descriptor::Array, "java/lang/StackTraceElement", 1, bytecode::descriptor::Reference});
    auto cls = tower->loader->fetchClass({bytecode::descriptor::Reference, "java/lang/StackTraceElement"});
    auto thr = tower->loader->fetchClass({bytecode::descriptor::Reference, "java/lang/Throwable"});
    auto fr = currentThread.currentFrame;
    auto pc = currentThread.pc;

    std::vector<OMOOPDesc *> frames;
    while (fr)
    {
        auto frame = cls->allocateInstance();
        frames.push_back(frame);

        tower->loader->klassOopCreate(fr->method->klass);
        tower->interface->putField(frame, tower->interface->findField(cls, "declaringClass", "Ljava/lang/Class;"),
                                   fr->method->klass->oop);
        tower->interface->putField(frame, tower->interface->findField(cls, "name", "Ljava/lang/String;"),
                                   tower->createString(fr->method->name));
        tower->interface->putField(frame, tower->interface->findField(cls, "descriptor", "Ljava/lang/String;"),
                                   tower->createString(fr->method->desc));
        tower->interface->putField(frame, tower->interface->findField(cls, "sourceFile", "Ljava/lang/String;"),
                                   tower->createString(fr->method->klass->source));
        if (fr->method->sourceMap)
        {
            tower->interface->putField(
                frame, tower->interface->findField(cls, "line", "I"),
                fr->method->sourceMap->at(static_cast<jint>(static_cast<size_t>(pc - fr->method->code))));
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
