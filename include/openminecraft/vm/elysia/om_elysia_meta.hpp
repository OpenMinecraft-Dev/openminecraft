#ifndef OM_ELYSIA_META_HPP
#define OM_ELYSIA_META_HPP

#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"

namespace openminecraft::vm::elysia
{
template <typename Func> void execWithState(OMElysiaThreadState state, Func &&func)
{
    auto currentState = thisThread.metadata->state;
    thisThread.switchState(state);
    func();
    thisThread.switchState(currentState);
}
} // namespace openminecraft::vm::elysia

#endif
