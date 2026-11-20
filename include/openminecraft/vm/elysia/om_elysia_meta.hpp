#ifndef OM_ELYSIA_META_HPP
#define OM_ELYSIA_META_HPP

#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "optimizations.hpp"

namespace openminecraft::vm::elysia
{
class OMStateGuard
{
  public:
    OMStateGuard(OMElysiaThreadState state)
    {
        st = thisThread.metadata->state;
        thisThread.switchState(state);
    }
    ~OMStateGuard()
    {
        thisThread.switchState(st);
    }

  private:
    OMElysiaThreadState st;
};
template <typename Func>
HOT_FUNC inline auto execWithState(OMElysiaThreadState state, Func &&func) -> decltype(std::forward<Func>(func)())
{
    OMStateGuard g(state);
    return func();
}
} // namespace openminecraft::vm::elysia

#endif
