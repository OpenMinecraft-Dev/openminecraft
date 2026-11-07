#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace openminecraft::renderer::common::wrap
{
OMVoxelCompilerPool::OMVoxelCompilerPool(world::OMChunkManager<16> &manager, OMVoxelCompiler &compiler, int num)
    : manager(manager), compiler(compiler)
{
    for (int i = 0; i < num; ++i)
    {
        auto t = new std::thread([&]() {
            while (true)
            {
                if (!active)
                {
                    break;
                }

                int next = -1;
                poolMutex.lock();
                if (queuedChunks.empty())
                {
                    poolMutex.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                auto request = queuedChunks.top();
                queuedChunks.pop();
                next = request.chunk;
                bool useCache = request.useCache;
                queuedIndex.erase(next);
                poolMutex.unlock();

                std::unordered_map<world::OMChunkIndex, std::shared_ptr<const world::OMChunk<16>>> neighborCache;
                auto externalAccessor = [&](glm::ivec3 pos, int64_t chunkx, int64_t chunky,
                                            int64_t chunkz) -> uint32_t {
                    if (pos.x < 0)
                    {
                        chunkx -= 1;
                        pos.x += 16;
                    }
                    else if (pos.x >= 16)
                    {
                        chunkx += 1;
                        pos.x -= 16;
                    }

                    if (pos.y < 0)
                    {
                        chunky -= 1;
                        pos.y += 16;
                    }
                    else if (pos.y >= 16)
                    {
                        chunky += 1;
                        pos.y -= 16;
                    }

                    if (pos.z < 0)
                    {
                        chunkz -= 1;
                        pos.z += 16;
                    }
                    else if (pos.z >= 16)
                    {
                        chunkz += 1;
                        pos.z -= 16;
                    }

                    world::OMChunkIndex idx = {chunkx, chunky, chunkz};

                    // 邻域指针做缓存：每个 chunk 编译只加锁一次，而不是逐方块加锁
                    auto cachedNeighbor = neighborCache.find(idx);
                    if (cachedNeighbor == neighborCache.end())
                    {
                        cachedNeighbor = neighborCache.emplace(idx, manager.tryGetChunk(idx)).first;
                    }

                    if (cachedNeighbor->second)
                    {
                        return cachedNeighbor->second->fetch(pos.x, pos.y, pos.z);
                    }
                    return 0;
                };

                auto cnk = manager.getChunk(next);
                if (cnk)
                {
                    // slot 可能已被 chunkManager 复用：只有坐标一致时缓存才有效
                    const world::OMChunkIndex origin = {cnk->chunkx, cnk->chunky, cnk->chunkz};
                    const bool sameChunk = cachedOrigin.count(next) != 0 && cachedOrigin[next] == origin;

                    if (useCache && cached.count(next) && sameChunk && !cnk->isDirty())
                    {
                        bufferMutex.lock();
                        cutout[next] = cutoutC[next];
                        cutoutComplex[next] = cutoutComplexC[next];
                        translucent[next] = translucentC[next];
                        translucentComplex[next] = translucentComplexC[next];
                        fluid[next] = fluidC[next];
                        translucentFluid[next] = translucentFluidC[next];
                        bufferMutex.unlock();
                        continue;
                    }

                    std::vector<OMVoxel> m = {}, tm = {};
                    std::vector<OMVoxelComplex> cm = {}, tcm = {};
                    std::vector<OMVoxelFluid> fm = {}, tfm = {};
                    compiler.compile(
                        *cnk, externalAccessor, next, [&](OMVoxel v) -> void { m.emplace_back(v); },
                        [&](OMVoxelComplex v) -> void { cm.emplace_back(v); },
                        [&](OMVoxelFluid v) { fm.emplace_back(v); }, [&](OMVoxel v) -> void { tm.emplace_back(v); },
                        [&](OMVoxelComplex v) -> void { tcm.emplace_back(v); },
                        [&](OMVoxelFluid v) { tfm.emplace_back(v); });

                    bufferMutex.lock();
                    cached[next] = true;
                    cachedOrigin[next] = origin;
                    cutout[next].assign(m.begin(), m.end());
                    cutoutComplex[next].assign(cm.begin(), cm.end());
                    translucent[next].assign(tm.begin(), tm.end());
                    translucentComplex[next].assign(tcm.begin(), tcm.end());
                    fluid[next].assign(fm.begin(), fm.end());
                    translucentFluid[next].assign(tfm.begin(), tfm.end());
                    cutoutC[next].assign(m.begin(), m.end());
                    cutoutComplexC[next].assign(cm.begin(), cm.end());
                    translucentC[next].assign(tm.begin(), tm.end());
                    translucentComplexC[next].assign(tcm.begin(), tcm.end());
                    fluidC[next].assign(fm.begin(), fm.end());
                    translucentFluidC[next].assign(tfm.begin(), tfm.end());
                    bufferMutex.unlock();
                }
            }
        });
        thrs.push_back(t);
    }
}
OMVoxelCompilerPool::~OMVoxelCompilerPool()
{
    active = false;
    for (auto t : thrs)
    {
        t->join();
        delete t;
    }
}

void OMVoxelCompilerPool::upload(OMVoxelLayer<OMVoxel> *cutout, OMVoxelLayer<OMVoxelComplex> *cutoutComplex,
                                 OMVoxelLayer<OMVoxelFluid> *fluid, OMVoxelLayer<OMVoxel> *translucent,
                                 OMVoxelLayer<OMVoxelComplex> *translucentComplex,
                                 OMVoxelLayer<OMVoxelFluid> *translucentFluid)
{
    std::lock_guard g(bufferMutex);

    // 每帧上传预算：编译完成的结果可能一次堆积很多（尤其是快速加载/旋转），
    // 一次性全部 memcpy 进 segbuf 会造成几十 ms 的卡顿。这里按字节限流，
    // 未处理完的留到下一帧继续。
    static constexpr uint64_t uploadBudgetBytes = 2 * 1024 * 1024;
    uint64_t uploadSpent = 0;

    auto drain = [&](auto &pending, auto *layer, uint64_t elementSize) {
        for (auto it = pending.begin(); it != pending.end() && uploadSpent < uploadBudgetBytes;)
        {
            uploadSpent += static_cast<uint64_t>(it->second.size()) * elementSize;
            layer->loadData(it->first, it->second, false);
            it = pending.erase(it);
        }
    };

    drain(this->cutout, cutout, sizeof(OMVoxel));
    drain(this->cutoutComplex, cutoutComplex, sizeof(OMVoxelComplex));
    drain(this->translucent, translucent, sizeof(OMVoxel));
    drain(this->translucentComplex, translucentComplex, sizeof(OMVoxelComplex));
    drain(this->fluid, fluid, sizeof(OMVoxelFluid));
    drain(this->translucentFluid, translucentFluid, sizeof(OMVoxelFluid));

    /* 旧的"一次性全部上传"实现保留在下面作为参考
    for (auto &p : this->cutout)
    {
        cutout->loadData(p.first, p.second, false);
    }
    this->cutout.clear();

    for (auto &p : this->cutoutComplex)
    {
        cutoutComplex->loadData(p.first, p.second, false);
    }
    this->cutoutComplex.clear();

    for (auto &p : this->translucent)
    {
        translucent->loadData(p.first, p.second, false);
    }
    this->translucent.clear();

    for (auto &p : this->translucentComplex)
    {
        translucentComplex->loadData(p.first, p.second, false);
    }
    this->translucentComplex.clear();

    for (auto &p : this->fluid)
    {
        fluid->loadData(p.first, p.second, false);
    }
    this->fluid.clear();

    for (auto &p : this->translucentFluid)
    {
        translucentFluid->loadData(p.first, p.second, false);
    }
    this->translucentFluid.clear();
    */
}
void OMVoxelCompilerPool::compile(int i, bool useCache, float distanceSq)
{
    std::lock_guard g(poolMutex);

    // 去重：同一 chunk 已有排队请求时直接复用，避免快速旋转时队列膨胀
    if (!queuedIndex.insert(i).second)
    {
        return;
    }

    queuedChunks.push(OMCompileRequest{distanceSq, i, useCache});
}
} // namespace openminecraft::renderer::common::wrap
