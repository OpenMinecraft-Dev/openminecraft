#ifndef OM_UTIL_RESULT_HPP
#define OM_UTIL_RESULT_HPP

namespace openminecraft::util
{
enum OMResultType
{
    Ok,
    Err
};

template <typename R, typename E> class OMResult
{
  public:
    static auto ok(R result) -> OMResult
    {
        OMResult r(Ok);
        r.result = result;
        return r;
    }

    static auto err(E error) -> OMResult
    {
        OMResult r(Err);
        r.error = error;
        return r;
    }

    auto unwrap() -> R
    {
        return result;
    }
    auto unwrap_err() -> E
    {
        return error;
    }

    const OMResultType type;

    ~OMResult() = default;

  private:
    OMResult(OMResultType t) : type(t)
    {
    }

    R result;
    E error;
};
} // namespace openminecraft::util

#endif
