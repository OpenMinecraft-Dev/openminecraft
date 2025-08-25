#ifndef OM_PIXELTOWER_CLASSBUILDER_HPP
#define OM_PIXELTOWER_CLASSBUILDER_HPP
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"

#include <any>
#include <functional>
#include <memory>
#include <algorithm>

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
    void klassSuperKlass(v0::OMKlass *klass);
    void klassInterface(v0::OMKlass *klass);
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

    uint16_t klassConstantPutInt(int i)
    {
        return klassConstantPutAny(std::make_shared<classfile::OMClassConstantInteger>(i),
                                   [&](const std::shared_ptr<classfile::OMClassConstant> &r) -> bool {
                                       return r.get() && r->type() == classfile::OMClassConstantType::Integer &&
                                              r->to<classfile::OMClassConstantInteger>()->data == i;
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
    void instConst(const std::any& c);
    void instReturn() const;

    void codeStackPush()
    {
        currentStackHeight++;
        maxStackHeight = std::max(maxStackHeight, currentStackHeight);
    }
    void codeStackPop()
    {
        currentStackHeight--;
    }

    void methodCodeFinish() const;

  private:
    std::shared_ptr<classfile::OMClassMethodInfo> result;
    std::shared_ptr<classfile::OMClassAttrCode> code;
    int currentStackHeight = 0;
    int maxStackHeight = 0;
    OMClassBuilder *builder;
};
} // namespace openminecraft::vm::pixeltower::v3

#endif
