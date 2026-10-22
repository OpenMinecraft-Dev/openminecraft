#ifndef OM_RENDERER_SHADERCOMPILER_HPP
#define OM_RENDERER_SHADERCOMPILER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
namespace openminecraft::renderer::common
{
class OMRendererShaderCompilerBackend
{
  public:
    virtual auto compile(std::shared_ptr<OMShader> shader) -> std::shared_ptr<OMShader> = 0;

    virtual auto accept(OMShaderFileType ftype) -> bool = 0;
    virtual auto outputType() -> OMShaderFileType = 0;
};

struct OMRendererShaderState
{
    std::shared_ptr<OMShader> source;
    int id;
};

class OMRendererShaderCompiler
{
  public:
    OMRendererShaderCompiler();
    ~OMRendererShaderCompiler();

    // 停止所有编译线程并丢弃未完成的任务；可重复调用（析构时也会调用）
    void stop();

    void install(std::shared_ptr<OMRendererShaderCompilerBackend> backend);

    auto addCompileTask(std::shared_ptr<OMShader> shader) -> int;
    auto getResult(int id) -> std::shared_ptr<OMShader>;

    auto getCompleteRatio() -> float;

  private:
    std::vector<std::shared_ptr<OMRendererShaderCompilerBackend>> backends;
    std::vector<std::thread *> compilerPool;
    std::queue<OMRendererShaderState> shaderQueue;
    std::unordered_map<int, std::shared_ptr<OMShader>> results;

    std::mutex queueLock;
    std::condition_variable queueCv;
    std::atomic<bool> available = true;
    std::atomic<bool> stopped = false;

    std::atomic_int countFinished = 0;
    std::atomic<int> countTotal = 0;

    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

#endif
