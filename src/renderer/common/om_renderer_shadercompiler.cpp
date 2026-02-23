#include "openminecraft/renderer/common/om_renderer_shadercompiler.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include <chrono>
#include <memory>
#include <thread>

namespace openminecraft::renderer::common
{
OMRendererShaderCompiler::OMRendererShaderCompiler()
{
    for (int i = 0; i < 12; i++)
    {
        compilerPool.push_back(std::thread([&]() {
            while (available)
            {
                while (!queueLock.try_lock())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                if (!shaderQueue.empty())
                {
                    auto ll = shaderQueue.front();
                    shaderQueue.pop();

                    std::shared_ptr<OMShader> target;

                    for (auto &comp : backends)
                    {
                        for (auto &comp : backends)
                        {
                            if (comp->accept(ll.source->type))
                            {
                                ll.source = comp->compile(ll.source);
                                goto compend;
                            }
                        }

                        throw OMRendererException("Unknown shader type to compile!");

                    compend:
                        results[ll.id] = ll.source;
                        countFinished++;
                        break;
                    }
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                queueLock.unlock();
            }
        }));
    }
}
OMRendererShaderCompiler::~OMRendererShaderCompiler()
{
    available = false;
    for (auto &thr : compilerPool)
    {
        thr.join();
    }
}

void OMRendererShaderCompiler::install(std::shared_ptr<OMRendererShaderCompilerBackend> backend)
{
    backends.push_back(backend);
}

int OMRendererShaderCompiler::addCompileTask(std::shared_ptr<OMShader> shader)
{
    countTotal++;
    shaderQueue.push({shader, static_cast<int>(countTotal)});
    return countTotal;
}
std::shared_ptr<OMShader> OMRendererShaderCompiler::getResult(int source)
{
    return results[source];
}

float OMRendererShaderCompiler::getCompleteRatio()
{
    return static_cast<float>(countFinished) / countTotal;
}
} // namespace openminecraft::renderer::common
