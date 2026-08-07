#ifndef OM_VERTEX_FORMAT_HPP
#define OM_VERTEX_FORMAT_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <string>
#include <vector>
namespace openminecraft::renderer::common::basics
{
template <typename... Ts> class OMVertex;

template <size_t I, typename T> struct OMVertexAttribute
{
    T value;
};

template <typename IndexSeq, typename... Ts> struct OMVertexImpl;

template <size_t... Is, typename... Ts>
struct OMVertexImpl<std::index_sequence<Is...>, Ts...> : OMVertexAttribute<Is, Ts>...
{
    static constexpr size_t attributeCount = sizeof...(Ts);

    template <size_t I> using TypeAt = std::tuple_element_t<I, std::tuple<Ts...>>;

    OMVertexImpl() = default;

    OMVertexImpl(Ts... vals)
    {
        assign(std::index_sequence_for<Ts...>{}, vals...);
    }

    template <size_t I> auto get() -> auto &
    {
        return static_cast<OMVertexAttribute<I, TypeAt<I>> &>(*this).value;
    }
    template <size_t I> auto get() const -> const auto &
    {
        return static_cast<const OMVertexAttribute<I, TypeAt<I>> &>(*this).value;
    }

    static auto offsets() -> std::array<size_t, attributeCount>
    {
        OMVertexImpl dummy;
        return offsetsImpl(dummy, std::index_sequence_for<Ts...>{});
    }

    static constexpr auto sizes() -> std::array<size_t, attributeCount>
    {
        return {sizeof(Ts)...};
    }

  private:
    template <size_t... Js> void assign(std::index_sequence<Js...>, Ts... vals)
    {
        (void)(int[]){(static_cast<OMVertexAttribute<Js, Ts> &>(*this).value = vals, 0)...};
    }

    template <size_t... Js>
    static auto offsetsImpl(OMVertexImpl &dummy, std::index_sequence<Js...>) -> std::array<size_t, attributeCount>
    {
        const char *base = reinterpret_cast<const char *>(&dummy);
        return {(reinterpret_cast<const char *>(&dummy.get<Js>()) - base)...};
    }
};

template <typename... Ts> class OMVertex : public OMVertexImpl<std::index_sequence_for<Ts...>, Ts...>
{
  public:
    using OMVertexImpl<std::index_sequence_for<Ts...>, Ts...>::OMVertexImpl;
};

enum OMVertexPropType
{
    Float,
    Vec2f,
    Vec3f,
    Vec4f,
    Integer,
    Vec2i,
    Vec3i,
    Vec4i,
    Double,
    Vec2d,
    Vec3d,
    Vec4d
};

struct OMVertexFormatGroup
{
    bool isInstance = false;
    int binding = 0;
    std::vector<std::tuple<std::string, OMVertexPropType, int>> parts;
    int size;
};

class OMVertexFormat
{
  public:
    OMVertexFormat();
    ~OMVertexFormat();

    auto appendPart(std::string, OMVertexPropType) -> OMVertexFormat *;
    auto debugState() -> OMVertexFormat *;
    auto decideStruct() -> OMVertexFormat *;
    static auto typeSize(OMVertexPropType) -> int;
    static auto typeAlign(OMVertexPropType) -> int;

    auto setInstance() -> OMVertexFormat *;
    auto nextGroup() -> OMVertexFormat *;

    std::vector<OMVertexFormatGroup> parts;
    OMVertexFormatGroup currentGroup;

  private:
    int binding = 0;
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common::basics

#endif
