#ifndef OM_NBT_HPP
#define OM_NBT_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
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
class OMNbtTag
{
  public:
    OMNbtTagType type;
    std::string name;

    std::variant<std::monostate, int8_t, int16_t, int32_t, int64_t, float, double, std::vector<int8_t>, std::string,
                 std::vector<std::shared_ptr<OMNbtTag>>, std::vector<int32_t>, std::vector<int64_t>>
        data;

    template <typename T> auto get() const -> const T &
    {
        return std::get<T>(data);
    }

    template <typename T> auto get() -> T &
    {
        return std::get<T>(data);
    }
};

class OMNbtReader
{
  public:
    OMNbtReader(std::function<uint8_t()>)
    {
    }
};
} // namespace openminecraft::specs::nbt

#endif