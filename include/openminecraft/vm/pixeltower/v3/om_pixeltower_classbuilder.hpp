#ifndef OM_PIXELTOWER_CLASSBUILDER_HPP
#define OM_PIXELTOWER_CLASSBUILDER_HPP
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"

#include <algorithm>
#include <any>
#include <functional>
#include <memory>
#include <type_traits>

namespace openminecraft::vm::pixeltower::v3
{
class OMMethodBuilder;

class OMClassBuilder
{
  public:
    OMClassBuilder();
    ~OMClassBuilder() = default;

    void klassBegin();
    void klassVersion(int major, int minor) const;
    void klassAccessFlags(int f) const;
    void klassName(const std::string &name);
    void klassSuperKlass(const v0::OMKlass *klass);
    void klassInterface(const v0::OMKlass *klass);
    [[nodiscard]] uint16_t klassConstantPut(const std::shared_ptr<classfile::OMClassConstant> &constant) const;

    template <typename T>
    uint16_t klassConstantPutAny(const std::shared_ptr<T> &data,
                                 const std::function<bool(std::shared_ptr<classfile::OMClassConstant>)> &checker)
    {
        auto itt = std::find_if(file->constants.begin(), file->constants.end(), checker);
        if (itt != file->constants.end())
        {
            return std::distance(file->constants.begin(), itt);
        }

        return klassConstantPut(data);
    }

    uint16_t klassConstantPutUTF8(std::string name)
    {
        return klassConstantPutAny(std::make_shared<classfile::OMClassConstantUtf8>(name),
                                   [&](const std::shared_ptr<classfile::OMClassConstant> &r) -> bool {
                                       return r.get() && r->type() == classfile::OMClassConstantType::Utf8 &&
                                              r->to<classfile::OMClassConstantUtf8>()->data == name;
                                   });
    }

    uint16_t klassConstantPutClass(uint16_t id)
    {
        return klassConstantPutAny(std::make_shared<classfile::OMClassConstantClass>(id),
                                   [&](const std::shared_ptr<classfile::OMClassConstant> &r) -> bool {
                                       return r.get() && r->type() == classfile::OMClassConstantType::Class &&
                                              r->to<classfile::OMClassConstantClass>()->nameIndex == id;
                                   });
    }

    uint16_t klassConstantPutInt(v0::jint i)
    {
        return klassConstantPutAny(std::make_shared<classfile::OMClassConstantInteger>(i),
                                   [&](const std::shared_ptr<classfile::OMClassConstant> &r) -> bool {
                                       return r.get() && r->type() == classfile::OMClassConstantType::Integer &&
                                              r->to<classfile::OMClassConstantInteger>()->data == i;
                                   });
    }

    template <typename T, typename C, classfile::OMClassConstantType id> uint16_t klassConstantPutNumber(T i)
    {
        return klassConstantPutAny(std::make_shared<C>(i),
                                   [&](const std::shared_ptr<classfile::OMClassConstant> &r) -> bool {
                                       return r.get() && r->type() == id && r->to<C>()->data == i;
                                   });
    }

    std::shared_ptr<OMMethodBuilder> klassConstructMethod();

    std::shared_ptr<classfile::OMClassFile> file;

  private:
    log::OMLogger logger;
};

class OMMethodBuilder
{
  public:
    explicit OMMethodBuilder(OMClassBuilder *builder);
    ~OMMethodBuilder() = default;

    void methodBegin();
    void methodNameAndDesc(std::string s, std::string desc) const;
    void methodAccessFlags(int f) const;
    void methodFinish() const;
    void methodCodeBegin();

    void instNop() const;
    void instConst();
    void instConst(v0::jint i);
    void instConst(v0::jfloat i);
    void instConst(v0::jlong i);
    void instConst(v0::jdouble i);

    template <typename T>
    void instLoad(uint16_t slot)
    {
        uint8_t opcode;
        if constexpr (std::is_same_v<T, v0::jint>)
        {
            opcode = op_iload;
        }
        else if constexpr (std::is_same_v<T, v0::jfloat>)
        {
            opcode = op_fload;
        }
        else if constexpr (std::is_same_v<T, v0::jlong>)
        {
            opcode = op_lload;
        }
        else if constexpr (std::is_same_v<T, v0::jdouble>)
        {
            opcode = op_dload;
        }
        else
        {
            opcode = op_aload;
        }

        if (slot <= 0xff)
        {
            code->code->push_back(opcode);
            code->code->push_back(slot);
        }
        else
        {
            code->code->push_back(op_wide);
            code->code->push_back(opcode);
            codePutId(slot);
        }

        codeLocalAccess(slot);
        codeStackPush();
    }
    void instILoad(uint8_t i);
    void instLLoad(uint8_t i);
    void instFLoad(uint8_t i);
    void instDLoad(uint8_t i);
    void instALoad(uint8_t i);

    void instReturn() const;

    void codeStackPush()
    {
        currentStackHeight++;
        if (currentStackHeight > maxStackHeight)
        {
            maxStackHeight = currentStackHeight;
        }
    }
    void codeStackPop()
    {
        currentStackHeight--;
    }
    void codeLocalAccess(int id)
    {
        if (id > maxLocals)
        {
            maxLocals = id + 1;
        }
    }

    void methodCodeFinish() const;

  private:
    void codePutConstantLoading(uint16_t id) const
    {
        if (id <= 0xff)
        {
            code->code->push_back(op_ldc);
        }
        else
        {
            code->code->push_back(op_ldc_w);
        }
    }
    void codePutId(uint16_t id) const
    {
        code->code->push_back(id >> 8);
        code->code->push_back(id & 0xff);
    }
    void codePutVaryId(uint16_t id) const
    {
        if (id <= 0xff)
        {
            code->code->push_back(id);
        }
        else
        {
            codePutId(id);
        }
    }

    std::shared_ptr<classfile::OMClassMethodInfo> result;
    std::shared_ptr<classfile::OMClassAttrCode> code;
    int currentStackHeight = 0;
    int maxStackHeight = 0;
    int maxLocals = 0;
    OMClassBuilder *builder;
};
} // namespace openminecraft::vm::pixeltower::v3

#endif
