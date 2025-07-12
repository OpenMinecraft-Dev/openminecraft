#include "openminecraft/vm/pixeltower/om_pixeltower_linker.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include <any>
#include <vector>

namespace openminecraft::vm::pixeltower
{
OMLinker::OMLinker(OMClassLoader &loader) : loader(loader)
{
}
OMLinker::~OMLinker()
{
}
uint8_t *OMLinker::staticData(std::string clazz)
{
    if (!loader.classLoaded(clazz))
    {
        throw std::logic_error("class not found!");
    }

    if (loader.isNative(clazz))
    {
        return loader.fetchNativeClass(clazz)->staticData();
    }
    else
    {
        return loader.fetchClass(clazz)->staticData;
    }
}
uint64_t OMLinker::fieldOffset(std::string clazz, std::string name, bool isStatic)
{
    if (!loader.classLoaded(clazz))
    {
        throw std::logic_error("class not found!");
    }

    if (loader.isNative(clazz))
    {
        return isStatic ? loader.fetchNativeClass(clazz)->globalFieldOffset(name)
                        : loader.fetchNativeClass(clazz)->fieldOffset(name);
    }
    else
    {
        return isStatic ? loader.fetchClass(clazz)->staticFields[name].offset
                        : loader.fetchClass(clazz)->objectFields[name].offset;
    }
}
void OMLinker::callMethod(std::any interpreter, std::string clazz, std::string func, std::string desc, bool isStatic,
                          std::stack<std::any, std::list<std::any>> &stk)
{
    if (!loader.classLoaded(clazz))
    {
        throw std::logic_error("class not found!");
    }

    int temp = 0;
    auto res = bytecode::descriptor::decodeSignature(desc, &temp);
    switch (res.type)
    {
    case Ok:
        break;
    case Err:
        throw std::logic_error(res.unwrap_err());
    }

    auto argLength = res.unwrap().first.size();

    if (loader.isNative(clazz))
    {
        std::vector<std::any> args;
        for (int i = 0; i < argLength + !isStatic; i++)
        {
            args.push_back(stk.top());
            stk.pop();
        }
        stk.push(std::make_shared<OMFrameMetadata>(OMFrameMetadata{clazz, func, desc, true, 0, 0}));

        for (int i = args.size() - 1; i >= 0; i--)
        {
            stk.push(args[i]);
        }
        loader.fetchNativeClass(clazz)->invoke(func, stk);
    }
    else
    {
        auto f = loader.fetchClass(clazz);

        auto mi = f->methods[fmt::format("{}{}", func, desc)];
        classfile::OMClassAttrCode *codeWrap = mi.code;

        if (codeWrap == nullptr)
        {
            throw std::logic_error("method not found!");
        }
        std::vector<std::any> args;
        for (int i = 0; i < argLength + !isStatic; i++)
        {
            args.push_back(stk.top());
            stk.pop();
        }

        auto frame = std::make_shared<OMFrameMetadata>(OMFrameMetadata{clazz, func, desc, false, 0, codeWrap->maxStack,
                                                                       std::make_shared<std::vector<std::any *>>(),
                                                                       std::vector<void *>(), mi});
        stk.push(frame);

        while (args.size() < codeWrap->maxLocals)
        {
            args.push_back(OMLocalVariablePlaceholder());
        }
        for (int i = args.size() - 1; i >= 0; i--)
        {
            stk.push(args[i]);
            frame->locals->push_back(&stk.top());
        }
        std::reverse(frame->locals->begin(), frame->locals->end());

        std::any_cast<OMInterpreter *>(interpreter)->executeBytecode(f, codeWrap, frame);
    }
}

}; // namespace openminecraft::vm::pixeltower