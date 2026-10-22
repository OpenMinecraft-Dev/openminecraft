#include "openminecraft/renderer/common/om_renderer_shadercompiler.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include <chrono>
#include <memory>
#include <thread>

namespace openminecraft::renderer::common
{
OMRendererShaderCompiler::OMRendererShaderCompiler() : logger("OMRendererShaderCompiler", this)
{
    for (int i = 0; i < 12; i++)
    {
        compilerPool.push_back(new std::thread([this]() -> void {
            while (true)
            {
                OMRendererShaderState queued;

                {
                    std::unique_lock<std::mutex> lock(queueLock);
                    queueCv.wait_for(lock, std::chrono::milliseconds(10),
                                     [this]() { return !available.load() || !shaderQueue.empty(); });

                    if (!available.load())
                    {
                        return;
                    }

                    if (shaderQueue.empty())
                    {
                        continue;
                    }

                    queued = shaderQueue.front();
                    shaderQueue.pop();
                }

                // 编译在锁外进行，避免长时间阻塞其它线程
                std::shared_ptr<OMShader> target;

                for (auto &comp : backends)
                {
                    if (comp->accept(queued.source->type))
                    {
                        target = comp->compile(queued.source);
                        break;
                    }
                }

                if (!target)
                {
                    // 线程里不能抛异常（会直接 std::terminate），记录并跳过
                    logger.error("no backend accepted shader {} (type {})", queued.source->filename.c_str(),
                                 static_cast<int>(queued.source->type));
                }

                {
                    std::lock_guard<std::mutex> lock(queueLock);
                    results[queued.id] = target;
                }

                countFinished++;
            }
        }));
    }
}

OMRendererShaderCompiler::~OMRendererShaderCompiler()
{
    stop();
}

void OMRendererShaderCompiler::stop()
{
    if (stopped.exchange(true))
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queueLock);
        available.store(false);
        while (!shaderQueue.empty())
        {
            shaderQueue.pop();
        }
    }
    queueCv.notify_all();

    for (auto &thr : compilerPool)
    {
        if (thr == nullptr)
        {
            continue;
        }
        if (thr->joinable())
        {
            thr->join();
        }
        delete thr;
        thr = nullptr;
    }
    compilerPool.clear();

    std::lock_guard<std::mutex> lock(queueLock);
    results.clear();
}

void OMRendererShaderCompiler::install(std::shared_ptr<OMRendererShaderCompilerBackend> backend)
{
    std::lock_guard<std::mutex> lock(queueLock);
    backends.push_back(backend);
}

auto OMRendererShaderCompiler::addCompileTask(std::shared_ptr<OMShader> shader) -> int
{
    int id = ++countTotal;

    {
        std::lock_guard<std::mutex> lock(queueLock);
        shaderQueue.push({shader, id});
    }
    queueCv.notify_one();

    return id;
}
auto OMRendererShaderCompiler::getResult(int source) -> std::shared_ptr<OMShader>
{
    std::lock_guard<std::mutex> lock(queueLock);
    auto it = results.find(source);
    return it == results.end() ? nullptr : it->second;
}

auto OMRendererShaderCompiler::getCompleteRatio() -> float
{
    int total = countTotal.load();
    return total == 0 ? 1.0f : static_cast<float>(countFinished.load()) / static_cast<float>(total);
}
} // namespace openminecraft::renderer::common
