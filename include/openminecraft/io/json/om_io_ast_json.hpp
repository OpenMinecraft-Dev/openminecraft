#ifndef OM_IO_AST_JSON_HPP
#define OM_IO_AST_JSON_HPP

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
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
    virtual OMJsonNodeType type() = 0;
    virtual std::vector<std::shared_ptr<OMJsonNode>> &getArray() = 0;
    virtual std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> &getMap() = 0;
    virtual double getNumberFloating() = 0;
    virtual int64_t getNumber() = 0;
    virtual bool getBoolean() = 0;
    virtual std::string getString() = 0;
};

class OMJsonNodeString : public OMJsonNode
{
  public:
    OMJsonNodeString(std::string value) : value(value)
    {
    }

    OMJsonNodeType type() override
    {
        return String;
    }

    std::vector<std::shared_ptr<OMJsonNode>> &getArray() override
    {
        throw std::logic_error("this is a json string!");
    }
    std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> &getMap() override
    {
        throw std::logic_error("this is a json string!");
    }
    double getNumberFloating() override
    {
        throw std::logic_error("this is a json string!");
    }
    int64_t getNumber() override
    {
        throw std::logic_error("this is a json string!");
    }
    bool getBoolean() override
    {
        throw std::logic_error("this is a json string!");
    }
    std::string getString() override
    {
        return value;
    }

  private:
    std::string value;
};

class OMJsonNodeObject : public OMJsonNode
{
  public:
    OMJsonNodeObject(std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> v) : data(v)
    {
    }

    OMJsonNodeType type() override
    {
        return Object;
    }

    std::vector<std::shared_ptr<OMJsonNode>> &getArray() override
    {
        throw std::logic_error("this is a json object!");
    }
    std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> &getMap() override
    {
        return data;
    }
    double getNumberFloating() override
    {
        throw std::logic_error("this is a json object!");
    }
    int64_t getNumber() override
    {
        throw std::logic_error("this is a json object!");
    }
    bool getBoolean() override
    {
        throw std::logic_error("this is a json object!");
    }
    std::string getString() override
    {
        throw std::logic_error("this is a json object!");
    }

  private:
    std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> data;
};

class OMJsonNodeArray : public OMJsonNode
{
  public:
    OMJsonNodeArray(std::vector<std::shared_ptr<OMJsonNode>> arr) : arr(arr)
    {
    }

    OMJsonNodeType type() override
    {
        return Array;
    }

    std::vector<std::shared_ptr<OMJsonNode>> &getArray() override
    {
        return arr;
    }
    std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> &getMap() override
    {
        throw std::logic_error("this is a json array!");
    }
    double getNumberFloating() override
    {
        throw std::logic_error("this is a json array!");
    }
    int64_t getNumber() override
    {
        throw std::logic_error("this is a json array!");
    }
    bool getBoolean() override
    {
        throw std::logic_error("this is a json array!");
    }
    std::string getString() override
    {
        throw std::logic_error("this is a json array!");
    }

  private:
    std::vector<std::shared_ptr<OMJsonNode>> arr;
};

class OMJsonNodeNumber : public OMJsonNode
{
  public:
    OMJsonNodeNumber(int64_t v) : ivalue(v), dvalue(static_cast<double>(v))
    {
    }

    OMJsonNodeNumber(double v) : dvalue(v), ivalue(static_cast<int64_t>(v))
    {
    }

    OMJsonNodeType type() override
    {
        return Number;
    }

    std::vector<std::shared_ptr<OMJsonNode>> &getArray() override
    {
        throw std::logic_error("this is a json number!");
    }
    std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> &getMap() override
    {
        throw std::logic_error("this is a json number!");
    }
    double getNumberFloating() override
    {
        return dvalue;
    }
    int64_t getNumber() override
    {
        return ivalue;
    }
    bool getBoolean() override
    {
        throw std::logic_error("this is a json number!");
    }
    std::string getString() override
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

    OMJsonNodeType type() override
    {
        return Primitive;
    }

    std::vector<std::shared_ptr<OMJsonNode>> &getArray() override
    {
        throw std::logic_error("this is a json primitive!");
    }
    std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> &getMap() override
    {
        throw std::logic_error("this is a json primitive!");
    }
    double getNumberFloating() override
    {
        throw std::logic_error("this is a json primitive!");
    }
    int64_t getNumber() override
    {
        throw std::logic_error("this is a json primitive!");
    }
    bool getBoolean() override
    {
        return value;
    }
    std::string getString() override
    {
        throw std::logic_error("this is a json primitive!");
    }

  private:
    bool value;
};

class OMJsonNodeNull : public OMJsonNode
{
  public:
    OMJsonNodeNull()
    {
    }

    OMJsonNodeType type() override
    {
        return Null;
    }

    std::vector<std::shared_ptr<OMJsonNode>> &getArray() override
    {
        throw std::logic_error("this is a json null!");
    }
    std::unordered_map<std::string, std::shared_ptr<OMJsonNode>> &getMap() override
    {
        throw std::logic_error("this is a json null!");
    }
    double getNumberFloating() override
    {
        throw std::logic_error("this is a json null!");
    }
    int64_t getNumber() override
    {
        throw std::logic_error("this is a json null!");
    }
    bool getBoolean() override
    {
        throw std::logic_error("this is a json null!");
    }
    std::string getString() override
    {
        throw std::logic_error("this is a json null!");
    }
};
} // namespace openminecraft::io::json

#endif
