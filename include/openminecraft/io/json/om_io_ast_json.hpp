#ifndef OM_IO_AST_JSON_HPP
#define OM_IO_AST_JSON_HPP

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace openminecraft::io::json
{
enum OMJsonNodeType
{
    Object,
    Array,
    Number,
    Primitive,
    String,
    Null
};

class OMJsonNode
{
  public:
    virtual auto type() -> OMJsonNodeType = 0;
    virtual auto getArray() -> std::vector<std::shared_ptr<OMJsonNode>> & = 0;
    virtual auto getMap() -> std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> & = 0;
    virtual auto getNumberFloating() -> double = 0;
    virtual auto getNumber() -> int64_t = 0;
    virtual auto getBoolean() -> bool = 0;
    virtual auto getString() -> std::string = 0;
    virtual void merge(std::shared_ptr<OMJsonNode>)
    {
    }
};

class OMJsonNodeString : public OMJsonNode
{
  public:
    OMJsonNodeString(std::string value) : value(std::move(value))
    {
    }
    virtual ~OMJsonNodeString() = default;

    auto type() -> OMJsonNodeType override
    {
        return String;
    }

    auto getArray() -> std::vector<std::shared_ptr<OMJsonNode>> & override
    {
        throw std::logic_error("this is a json string!");
    }
    auto getMap() -> std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> & override
    {
        throw std::logic_error("this is a json string!");
    }
    auto getNumberFloating() -> double override
    {
        throw std::logic_error("this is a json string!");
    }
    auto getNumber() -> int64_t override
    {
        throw std::logic_error("this is a json string!");
    }
    auto getBoolean() -> bool override
    {
        throw std::logic_error("this is a json string!");
    }
    auto getString() -> std::string override
    {
        return value;
    }

  private:
    std::string value;
};

class OMJsonNodeObject : public OMJsonNode
{
  public:
    OMJsonNodeObject(std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> v) : data(std::move(v))
    {
    }
    virtual ~OMJsonNodeObject() = default;

    auto type() -> OMJsonNodeType override
    {
        return Object;
    }

    auto getArray() -> std::vector<std::shared_ptr<OMJsonNode>> & override
    {
        throw std::logic_error("this is a json object!");
    }
    auto getMap() -> std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> & override
    {
        return data;
    }
    auto getNumberFloating() -> double override
    {
        throw std::logic_error("this is a json object!");
    }
    auto getNumber() -> int64_t override
    {
        throw std::logic_error("this is a json object!");
    }
    auto getBoolean() -> bool override
    {
        throw std::logic_error("this is a json object!");
    }
    auto getString() -> std::string override
    {
        throw std::logic_error("this is a json object!");
    }
    void merge(std::shared_ptr<OMJsonNode> other) override
    {
        if (other->type() != Object)
            return;

        auto &other_map = other->getMap();
        for (auto &[key, val] : other_map)
        {
            auto it = data.find(key);
            if (it != data.end())
            {
                if (it->second->type() == val->type())
                {
                    it->second->merge(val);
                }
                else
                {
                    it->second = val;
                }
            }
            else
            {
                data[key] = val;
            }
        }
    }

  private:
    std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> data;
};

class OMJsonNodeArray : public OMJsonNode
{
  public:
    OMJsonNodeArray(std::vector<std::shared_ptr<OMJsonNode>> arr) : arr(std::move(arr))
    {
    }
    virtual ~OMJsonNodeArray() = default;

    auto type() -> OMJsonNodeType override
    {
        return Array;
    }

    auto getArray() -> std::vector<std::shared_ptr<OMJsonNode>> & override
    {
        return arr;
    }
    auto getMap() -> std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> & override
    {
        throw std::logic_error("this is a json array!");
    }
    auto getNumberFloating() -> double override
    {
        throw std::logic_error("this is a json array!");
    }
    auto getNumber() -> int64_t override
    {
        throw std::logic_error("this is a json array!");
    }
    auto getBoolean() -> bool override
    {
        throw std::logic_error("this is a json array!");
    }
    auto getString() -> std::string override
    {
        throw std::logic_error("this is a json array!");
    }
    void merge(std::shared_ptr<OMJsonNode> other) override
    {
        if (other->type() != Array)
            return;

        auto &other_arr = other->getArray();
        arr.insert(arr.end(), other_arr.begin(), other_arr.end());
    }

  private:
    std::vector<std::shared_ptr<OMJsonNode>> arr;
};

class OMJsonNodeNumber : public OMJsonNode
{
  public:
    OMJsonNodeNumber(int64_t v) : dvalue(static_cast<double>(v)), ivalue(v)
    {
    }
    virtual ~OMJsonNodeNumber() = default;

    OMJsonNodeNumber(double v) : dvalue(v), ivalue(static_cast<int64_t>(v))
    {
    }

    auto type() -> OMJsonNodeType override
    {
        return Number;
    }

    auto getArray() -> std::vector<std::shared_ptr<OMJsonNode>> & override
    {
        throw std::logic_error("this is a json number!");
    }
    auto getMap() -> std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> & override
    {
        throw std::logic_error("this is a json number!");
    }
    auto getNumberFloating() -> double override
    {
        return dvalue;
    }
    auto getNumber() -> int64_t override
    {
        return ivalue;
    }
    auto getBoolean() -> bool override
    {
        throw std::logic_error("this is a json number!");
    }
    auto getString() -> std::string override
    {
        throw std::logic_error("this is a json number!");
    }

  private:
    double dvalue;
    int64_t ivalue;
};

class OMJsonNodePrimitive : public OMJsonNode
{
  public:
    OMJsonNodePrimitive(bool v) : value(v)
    {
    }
    virtual ~OMJsonNodePrimitive() = default;

    auto type() -> OMJsonNodeType override
    {
        return Primitive;
    }

    auto getArray() -> std::vector<std::shared_ptr<OMJsonNode>> & override
    {
        throw std::logic_error("this is a json primitive!");
    }
    auto getMap() -> std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> & override
    {
        throw std::logic_error("this is a json primitive!");
    }
    auto getNumberFloating() -> double override
    {
        throw std::logic_error("this is a json primitive!");
    }
    auto getNumber() -> int64_t override
    {
        throw std::logic_error("this is a json primitive!");
    }
    auto getBoolean() -> bool override
    {
        return value;
    }
    auto getString() -> std::string override
    {
        throw std::logic_error("this is a json primitive!");
    }

  private:
    bool value;
};

class OMJsonNodeNull : public OMJsonNode
{
  public:
    OMJsonNodeNull() = default;
    virtual ~OMJsonNodeNull() = default;

    auto type() -> OMJsonNodeType override
    {
        return Null;
    }

    auto getArray() -> std::vector<std::shared_ptr<OMJsonNode>> & override
    {
        throw std::logic_error("this is a json null!");
    }
    auto getMap() -> std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> & override
    {
        throw std::logic_error("this is a json null!");
    }
    auto getNumberFloating() -> double override
    {
        throw std::logic_error("this is a json null!");
    }
    auto getNumber() -> int64_t override
    {
        throw std::logic_error("this is a json null!");
    }
    auto getBoolean() -> bool override
    {
        throw std::logic_error("this is a json null!");
    }
    auto getString() -> std::string override
    {
        throw std::logic_error("this is a json null!");
    }
};
} // namespace openminecraft::io::json

#endif
