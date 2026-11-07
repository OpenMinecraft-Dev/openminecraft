#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include <array>
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace openminecraft::renderer::common::wrap
{
auto computeSectionVisibility(const std::array<uint8_t, 4096> &blocking) -> uint64_t
{
    std::array<int16_t, 4096> comp;
    comp.fill(-1);

    std::vector<uint8_t> compFaces;
    std::vector<int> stack;

    auto faceBits = [](int x, int y, int z) -> uint8_t {
        uint8_t f = 0;
        if (x == 0)
            f |= 1 << 0;
        if (x == 15)
            f |= 1 << 1;
        if (y == 0)
            f |= 1 << 2;
        if (y == 15)
            f |= 1 << 3;
        if (z == 0)
            f |= 1 << 4;
        if (z == 15)
            f |= 1 << 5;
        return f;
    };

    for (int i = 0; i < 4096; ++i)
    {
        if (blocking[i] != 0 || comp[i] >= 0)
        {
            continue;
        }

        const int id = static_cast<int>(compFaces.size());
        compFaces.push_back(0);
        stack.clear();
        stack.push_back(i);
        comp[i] = static_cast<int16_t>(id);

        while (!stack.empty())
        {
            const int c = stack.back();
            stack.pop_back();

            const int x = c & 15;
            const int z = (c >> 4) & 15;
            const int y = c >> 8;

            compFaces[id] |= faceBits(x, y, z);

            auto push = [&](int n) {
                if (n >= 0 && n < 4096 && blocking[n] == 0 && comp[n] < 0)
                {
                    comp[n] = static_cast<int16_t>(id);
                    stack.push_back(n);
                }
            };

            if (x > 0)
                push(c - 1);
            if (x < 15)
                push(c + 1);
            if (z > 0)
                push(c - 16);
            if (z < 15)
                push(c + 16);
            if (y > 0)
                push(c - 256);
            if (y < 15)
                push(c + 256);
        }
    }

    uint64_t matrix = 0;

    for (const uint8_t faces : compFaces)
    {
        for (int a = 0; a < 6; ++a)
        {
            if ((faces & (1 << a)) == 0)
            {
                continue;
            }
            for (int b = 0; b < 6; ++b)
            {
                if ((faces & (1 << b)) != 0)
                {
                    matrix |= static_cast<uint64_t>(1) << (a * 6 + b);
                }
            }
        }
    }

    return matrix | (static_cast<uint64_t>(1) << 63);
}

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
                    // 没有几何编译任务时，处理遮挡剔除的"纯可见性"请求
                    if (!visRequests.empty())
                    {
                        next = visRequests.back();
                        visRequests.pop_back();
                        visQueued.erase(next);
                        poolMutex.unlock();

                        auto visChunk = manager.getChunk(next);
                        if (visChunk)
                        {
                            const world::OMChunkIndex visOrigin = {visChunk->chunkx, visChunk->chunky,
                                                                   visChunk->chunkz};
                            auto visAccessor = [&](glm::ivec3 pos, int64_t chunkx, int64_t chunky,
                                                   int64_t chunkz) -> uint32_t {
                                return 0; // 可见性扫描只需要本 section 内的方块
                            };

                            const uint64_t vis = compiler.computeVisibility(*visChunk, visAccessor);

                            std::lock_guard<std::mutex> g(bufferMutex);
                            visC[next] = vis;
                            visPending[next] = {visOrigin, vis};
                        }
                        continue;
                    }

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
                if (cnk && cnk->isEmpty())
                {
                    // 防御：空区块不编译，直接写入空结果（不占任何 block）
                    std::lock_guard<std::mutex> g(bufferMutex);
                    cached[next] = true;
                    cachedOrigin[next] = {cnk->chunkx, cnk->chunky, cnk->chunkz};
                    cutout[next].clear();
                    cutoutComplex[next].clear();
                    translucent[next].clear();
                    translucentComplex[next].clear();
                    fluid[next].clear();
                    translucentFluid[next].clear();
                    cutoutC[next].clear();
                    cutoutComplexC[next].clear();
                    translucentC[next].clear();
                    translucentComplexC[next].clear();
                    fluidC[next].clear();
                    translucentFluidC[next].clear();
                    visC[next] = (static_cast<uint64_t>(1) << 63); // 已知：完全可透过
                    visPending[next] = {{cnk->chunkx, cnk->chunky, cnk->chunkz}, static_cast<uint64_t>(1) << 63};
                    continue;
                }
                if (cnk)
                {
                    // slot 可能已被 chunkManager 复用：只有坐标一致时缓存才有效
                    const world::OMChunkIndex origin = {cnk->chunkx, cnk->chunky, cnk->chunkz};

                    // 多个编译线程会并发读写 cached/cachedOrigin 和缓存数据，必须与写入端使用同一把锁，
                    // 否则在 unordered_map rehash 期间读取会访问已释放节点并破坏堆。
                    bool cacheHit = false;
                    {
                        std::lock_guard<std::mutex> g(bufferMutex);

                        auto cachedOriginIt = cachedOrigin.find(next);
                        const bool sameChunk = cachedOriginIt != cachedOrigin.end() && cachedOriginIt->second == origin;
                        cacheHit = useCache && cached.count(next) != 0 && sameChunk && !cnk->isDirty();

                        if (cacheHit)
                        {
                            cutout[next] = cutoutC[next];
                            cutoutComplex[next] = cutoutComplexC[next];
                            translucent[next] = translucentC[next];
                            translucentComplex[next] = translucentComplexC[next];
                            fluid[next] = fluidC[next];
                            translucentFluid[next] = translucentFluidC[next];

                            auto visIt = visC.find(next);
                            if (visIt != visC.end())
                            {
                                visPending[next] = {origin, visIt->second};
                            }
                        }
                    }

                    if (cacheHit)
                    {
                        continue;
                    }

                    std::vector<OMVoxel> m = {}, tm = {};
                    std::vector<OMVoxelComplex> cm = {}, tcm = {};
                    std::vector<OMVoxelFluid> fm = {}, tfm = {};

                    std::array<uint8_t, 4096> blocking = {};

                    compiler.compile(
                        *cnk, externalAccessor, next,
                        [&](OMVoxel v) -> void {
                            m.emplace_back(v);
                            const int vx = (v.voxelBasic >> 28) & 15;
                            const int vy = (v.voxelBasic >> 24) & 15;
                            const int vz = (v.voxelBasic >> 20) & 15;
                            blocking[vy * 256 + vz * 16 + vx] = 1;
                        },
                        [&](OMVoxelComplex v) -> void { cm.emplace_back(v); },
                        [&](OMVoxelFluid v) { fm.emplace_back(v); }, [&](OMVoxel v) -> void { tm.emplace_back(v); },
                        [&](OMVoxelComplex v) -> void { tcm.emplace_back(v); },
                        [&](OMVoxelFluid v) { tfm.emplace_back(v); });

                    const uint64_t sectionVis = computeSectionVisibility(blocking);

                    bufferMutex.lock();
                    cached[next] = true;
                    cachedOrigin[next] = origin;
                    visC[next] = sectionVis;
                    visPending[next] = {origin, sectionVis};
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
                                 OMVoxelLayer<OMVoxelFluid> *translucentFluid,
                                 const std::function<void(int, world::OMChunkIndex, uint64_t)> &visSink)
{
    std::lock_guard g(bufferMutex);

    if (visSink)
    {
        for (const auto &vp : visPending)
        {
            visSink(vp.first, vp.second.first, vp.second.second);
        }
        visPending.clear();
    }

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
}
void OMVoxelCompilerPool::requestVisibility(int i)
{
    std::lock_guard g(poolMutex);

    if (!visQueued.insert(i).second)
    {
        return;
    }

    visRequests.push_back(i);
}

void OMVoxelCompilerPool::compile(int i, bool useCache, float distanceSq)
{
    std::lock_guard g(poolMutex);

    if (!queuedIndex.insert(i).second)
    {
        return;
    }

    queuedChunks.push(OMCompileRequest{distanceSq, i, useCache});
}
} // namespace openminecraft::renderer::common::wrap
