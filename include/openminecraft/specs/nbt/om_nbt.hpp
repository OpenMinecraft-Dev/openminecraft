#ifndef OM_NBT_HPP
#define OM_NBT_HPP

#include <cstdint>
#include <functional>
#include <istream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
namespace openminecraft::specs::nbt
{
enum OMNbtTagType : uint8_t
{
    End = 0,
    Byte,
    Short,
    Int,
    Long,
    Float,
    Double,
    ByteArray,
    String,
    List,
    Compound,
    IntArray,
    LongArray
};
class OMNbtTag;

using NBTByte = int8_t;
using NBTShort = int16_t;
using NBTInt = int32_t;
using NBTLong = int64_t;
using NBTFloat = float;
using NBTDouble = double;
using NBTByteArray = std::vector<int8_t>;
using NBTString = std::string;
using NBTList = std::vector<std::shared_ptr<OMNbtTag>>;
using NBTIntArray = std::vector<int32_t>;
using NBTLongArray = std::vector<int64_t>;

class OMNbtTag
{
  public:
    OMNbtTagType type;
    std::string name;

    std::variant<std::monostate, NBTByte, NBTShort, NBTInt, NBTLong, NBTFloat, NBTDouble, NBTByteArray, NBTString,
                 NBTList, NBTIntArray, NBTLongArray>
        data;

    OMNbtTag() = default;
    OMNbtTag(OMNbtTagType type, std::string name,
             std::variant<std::monostate, NBTByte, NBTShort, NBTInt, NBTLong, NBTFloat, NBTDouble, NBTByteArray,
                          NBTString, NBTList, NBTIntArray, NBTLongArray>
                 data)
        : type(type), name(std::move(name)), data(std::move(data))
    {
    }

    template <typename T> auto get() const -> const T &
    {
        return std::get<T>(data);
    }

    template <typename T> auto get() -> T &
    {
        return std::get<T>(data);
    }

    auto getList(int i) -> std::shared_ptr<OMNbtTag>
    {
        return get<NBTList>()[i];
    }

    auto getCompound(std::string i) -> std::shared_ptr<OMNbtTag>
    {
        const auto &t = get<NBTList>();

        for (const auto &tag : t)
        {
            if (tag->name == i)
            {
                return tag;
            }
        }

        return nullptr;
    }
};

struct OMNbtContext
{
    std::shared_ptr<OMNbtTag> current;
    OMNbtTagType elementType = End;
    int rest = -1;
};
} // namespace openminecraft::specs::nbt

#endif