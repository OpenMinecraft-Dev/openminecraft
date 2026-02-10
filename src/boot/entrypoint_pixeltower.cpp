#include "openminecraft/boot/om_boot.hpp"
#include <memory>

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_checker.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/v1/om_pixeltower_tracing.hpp"
#include "openminecraft/vm/pixeltower/v3/om_pixeltower_classbuilder.hpp"
#include <chrono>

using namespace openminecraft::vm;

namespace openminecraft::boot
{
std::shared_ptr<pixeltower::v0::OMPixelTower> towerb = nullptr;
log::OMLogger logger("Crash Handler");

void onCrash(int code, int pid, std::vector<openminecraft::vm::pixeltower::v1::tracing::OMTracingFrame> &frames)
{
    logger.debug("tracing stack... (exit code {})", code);
    if (towerb)
    {
        towerb->handleCrash(code, pid, frames);
    }
}

void pixeltowerDynTest()
{
    auto tower = std::make_shared<pixeltower::v0::OMPixelTower>();
    pixeltower::v3::OMClassBuilder builder;
    builder.klassBegin();
    builder.klassAccessFlags(JVM_Acc_Public);
    builder.klassName("openminecraft/DynamicTest");

    pixeltower::v1::tracing::installHandler();
    tower->initCurrentThread(1ul * 1024 * 1024);
    tower->init("vmstd/out");

    bytecode::descriptor::OMTypeDesc tgt = {bytecode::descriptor::Reference, "java/lang/Object"};
    tower->loader->loadClass(tgt);
    auto cls = tower->loader->fetchClass(tgt);

    builder.klassSuperKlass(cls);
    builder.klassVersion(JVM_VERSION_8, 0);

    auto func = builder.klassConstructMethod();
    func->methodBegin();
    func->methodAccessFlags(JVM_Acc_Public);
    func->methodNameAndDesc("<init>", "()V");
    func->methodCodeBegin();
    func->instNop();
    func->instLoad<void *>(0);
    func->instConst(421.f);
    func->instReturn();
    func->methodCodeFinish();
    func->methodFinish();

    bytecode::OMBytecodeChecker chk(builder.file);
    chk.detail();

    tower->loader->stagClass(builder.file);
    tower->loader->loadClass({bytecode::descriptor::Reference, "openminecraft/DynamicTest"});
    towerb = tower;
}
void pixeltowerLoadTest()
{
    auto logger = log::OMLogger("VM Test");
    auto tower = std::make_shared<pixeltower::v0::OMPixelTower>();
    towerb = tower;
    pixeltower::v1::tracing::installHandler();
    tower->initCurrentThread(1ul * 1024 * 1024);
    tower->init("vmstd/out");
    tower->load("../Test.class");

    bytecode::descriptor::OMTypeDesc tgt = {bytecode::descriptor::Reference, "openminecraft/Test"};
    tower->loader->loadClass(tgt);
    auto cls = tower->loader->fetchClass(tgt);
    auto met = cls->methods;
    while (met != nullptr)
    {
        if (strcmp(met->name, "main") == 0 && strcmp(met->desc, "([Ljava/lang/String;)V") == 0)
        {
            auto now = std::chrono::system_clock::now();

            try
            {
                tower->boot(met);
            }
            catch (err::OMValidationError &e)
            {
                logger.info("{}", e.what());
            }
            catch (int g)
            {
            }

            auto now2 = std::chrono::system_clock::now();
            logger.info("VM exited {} ns", std::chrono::duration_cast<std::chrono::nanoseconds>(now2 - now).count());

            break;
        }
        met = met->next;
    }
}
} // namespace openminecraft::boot
