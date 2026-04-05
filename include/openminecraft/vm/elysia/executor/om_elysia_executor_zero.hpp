#ifndef OM_ELYSIA_EXECUTOR_ZERO_HPP
#define OM_ELYSIA_EXECUTOR_ZERO_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
namespace openminecraft::vm::elysia::executor
{
class OMElysiaExecutorZero
{
  public:
    OMElysiaExecutorZero(OMElysiaVirtualWorld *vw);
    ~OMElysiaExecutorZero();

    void execute(OMElysiaMethod *m);

  private:
    OMElysiaVirtualWorld *world;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia::executor

#endif
