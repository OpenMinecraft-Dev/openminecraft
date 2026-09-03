#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include <chrono>
#include <iostream>
#include <mutex>
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
                next = queuedChunks.back().first;
                bool useCache = queuedChunks.back().second;
                queuedChunks.pop_back();
                poolMutex.unlock();

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
                    if (manager.chunkLoaded(idx))
                    {
                        return manager.getChunk(idx).fetch(pos.x, pos.y, pos.z);
                    }
                    else
                    {
                        return 0;
                    }
                };

                auto cnk = manager.getChunk(next);
                if (cnk.has_value())
                {
                    if (useCache && cached.count(next) && !cnk->isDirty())
                    {
                        bufferMutex.lock();
                        cutout[next] = cutoutC[next];
                        cutoutComplex[next] = cutoutComplexC[next];
                        translucent[next] = translucentC[next];
                        translucentComplex[next] = translucentComplexC[next];
                        bufferMutex.unlock();
                        continue;
                    }

                    std::vector<OMVoxel> m = {}, tm = {};
                    std::vector<OMVoxelComplex> cm = {}, tcm = {};
                    compiler.compile(
                        cnk.value(), externalAccessor, next, [&](OMVoxel v) -> void { m.emplace_back(v); },
                        [&](OMVoxelComplex v) -> void { cm.emplace_back(v); },
                        [&](OMVoxel v) -> void { tm.emplace_back(v); },
                        [&](OMVoxelComplex v) -> void { tcm.emplace_back(v); });

                    bufferMutex.lock();
                    cached[next] = true;
                    cutout[next].assign(m.begin(), m.end());
                    cutoutComplex[next].assign(cm.begin(), cm.end());
                    translucent[next].assign(tm.begin(), tm.end());
                    translucentComplex[next].assign(tcm.begin(), tcm.end());
                    cutoutC[next].assign(m.begin(), m.end());
                    cutoutComplexC[next].assign(cm.begin(), cm.end());
                    translucentC[next].assign(tm.begin(), tm.end());
                    translucentComplexC[next].assign(tcm.begin(), tcm.end());
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
                                 OMVoxelLayer<OMVoxel> *translucent, OMVoxelLayer<OMVoxelComplex> *translucentComplex)
{
    std::lock_guard g(bufferMutex);

    for (auto &p : this->cutout)
    {
        cutout->loadData(p.first, p.second);
    }
    this->cutout.clear();

    for (auto &p : this->cutoutComplex)
    {
        cutoutComplex->loadData(p.first, p.second);
    }
    this->cutoutComplex.clear();

    for (auto &p : this->translucent)
    {
        translucent->loadData(p.first, p.second);
    }
    this->translucent.clear();

    for (auto &p : this->translucentComplex)
    {
        translucentComplex->loadData(p.first, p.second);
    }
    this->translucentComplex.clear();
}
void OMVoxelCompilerPool::compile(int i, bool useCache)
{
    std::lock_guard g(poolMutex);
    queuedChunks.emplace_back(i, useCache);
}
} // namespace openminecraft::renderer::common::wrap
