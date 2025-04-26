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
    static OMResult ok(R result)
    {
        OMResult r(Ok);
        r.result = result;
        return r;
    }

    static OMResult err(E error)
    {
        OMResult r(Err);
        r.error = error;
        return r;
    }

    R unwrap()
    {
        return result;
    }
    E unwrap_err()
    {
        return error;
    }

    const OMResultType type;

    ~OMResult()
    {
    }

  private:
    OMResult(OMResultType t) : type(t)
    {
    }

    R result;
    E error;
};
} // namespace openminecraft::util

#endif