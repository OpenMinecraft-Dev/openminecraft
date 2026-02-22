#ifndef OM_RENDERER_SHADERCOMPILER_HPP
#define OM_RENDERER_SHADERCOMPILER_HPP

#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include <atomic>
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
    virtual std::shared_ptr<OMShader> compile(std::shared_ptr<OMShader> shader) = 0;

    virtual bool accept(OMShaderFileType ftype) = 0;
    virtual OMShaderFileType outputType() = 0;
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

    void install(std::shared_ptr<OMRendererShaderCompilerBackend> backend);

    int addCompileTask(std::shared_ptr<OMShader> shader);
    std::shared_ptr<OMShader> getResult(int index);

    float getCompleteRatio();

  private:
    std::vector<std::shared_ptr<OMRendererShaderCompilerBackend>> backends;
    std::vector<std::thread> compilerPool;
    std::queue<OMRendererShaderState> shaderQueue;
    std::unordered_map<int, std::shared_ptr<OMShader>> results;

    std::mutex queueLock;
    bool available = true;

    std::atomic_int countFinished = 0;
    float countTotal = 0;
};
} // namespace openminecraft::renderer::common

#endif
