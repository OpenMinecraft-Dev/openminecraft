#include "openminecraft-shell/om_application_terrain.hpp"

#include "openminecraft-shell/data/block/om_block_registry.hpp"
#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include "openminecraft/io/json/om_io_tokeniter_json.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>

namespace openminecraft::shell::application
{
struct OMPerlinNoise
{
    std::array<int, 512> perm = {};

    explicit OMPerlinNoise(uint32_t seed)
    {
        std::array<int, 256> p = {};
        for (int i = 0; i < 256; ++i)
        {
            p[i] = i;
        }

        std::mt19937 rng(seed);
        std::shuffle(p.begin(), p.end(), rng);

        for (int i = 0; i < 512; ++i)
        {
            perm[i] = p[i & 255];
        }
    }

    static auto fade(double t) -> double
    {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    static auto lerp(double a, double b, double t) -> double
    {
        return a + t * (b - a);
    }

    static auto grad(int hash, double x, double y) -> double
    {
        switch (hash & 7)
        {
        case 0:
            return x + y;
        case 1:
            return x - y;
        case 2:
            return -x + y;
        case 3:
            return -x - y;
        case 4:
            return x;
        case 5:
            return -x;
        case 6:
            return y;
        default:
            return -y;
        }
    }

    [[nodiscard]] auto noise(double x, double y) const -> double
    {
        const int xi = static_cast<int>(std::floor(x)) & 255;
        const int yi = static_cast<int>(std::floor(y)) & 255;
        const double xf = x - std::floor(x);
        const double yf = y - std::floor(y);
        const double u = fade(xf);
        const double v = fade(yf);

        const int aa = perm[perm[xi] + yi];
        const int ab = perm[perm[xi] + yi + 1];
        const int ba = perm[perm[xi + 1] + yi];
        const int bb = perm[perm[xi + 1] + yi + 1];

        const double x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1.0, yf), u);
        const double x2 = lerp(grad(ab, xf, yf - 1.0), grad(bb, xf - 1.0, yf - 1.0), u);
        return lerp(x1, x2, v);
    }

    [[nodiscard]] auto fbm(double x, double y, int octaves) const -> double
    {
        double sum = 0.0;
        double amp = 1.0;
        double freq = 1.0;
        double norm = 0.0;

        for (int i = 0; i < octaves; ++i)
        {
            sum += amp * noise(x * freq, y * freq);
            norm += amp;
            amp *= 0.5;
            freq *= 2.0;
        }

        return sum / norm;
    }
};

struct OMPreloadInfo
{
    int64_t minChunkX = 0;
    int64_t maxChunkX = 0;
    int64_t minChunkZ = 0;
    int64_t maxChunkZ = 0;
    int64_t minSectionY = 0;
    int64_t maxSectionY = 0;
    int64_t maxBlockY = -64;
    int loaded = 0;
};

static auto preloadChunkBounds() -> OMPreloadInfo
{
    OMPreloadInfo info{};
    bool first = true;

    std::error_code ec;

    for (auto &entry : std::filesystem::directory_iterator("chunks_mapped", ec))
    {
        if (ec || !entry.is_regular_file() || entry.path().extension() != ".omc")
        {
            continue;
        }

        std::ifstream in(entry.path(), std::ios::binary);
        char header[20] = {};

        if (!in.read(header, sizeof(header)) || std::memcmp(header, "OMCH", 4) != 0)
        {
            continue;
        }

        int32_t cx = 0, cz = 0;
        uint32_t sections = 0;
        std::memcpy(&cx, header + 8, 4);
        std::memcpy(&cz, header + 12, 4);
        std::memcpy(&sections, header + 16, 4);

        const int64_t minY = -4;
        const int64_t maxY = minY + static_cast<int64_t>(sections) - 1;

        {
            std::vector<uint32_t> ids(4096);

            for (uint32_t si = 0; si < sections; ++si)
            {
                in.seekg(static_cast<std::streamoff>(20 + static_cast<size_t>(si) * (4 + 4096 * 4 + 64 * 4) + 4));
                in.read(reinterpret_cast<char *>(ids.data()), 4096 * 4);

                if (!in)
                {
                    break;
                }

                for (int i = 0; i < 4096; ++i)
                {
                    if (ids[i] != 0)
                    {
                        const int64_t y = (minY + static_cast<int64_t>(si)) * 16 + (i / 256);

                        if (y > info.maxBlockY)
                        {
                            info.maxBlockY = y;
                        }
                    }
                }
            }
        }

        if (first)
        {
            info.minChunkX = info.maxChunkX = cx;
            info.minChunkZ = info.maxChunkZ = cz;
            info.minSectionY = minY;
            info.maxSectionY = maxY;
            first = false;
        }
        else
        {
            info.minChunkX = std::min<int64_t>(info.minChunkX, cx);
            info.maxChunkX = std::max<int64_t>(info.maxChunkX, cx);
            info.minChunkZ = std::min<int64_t>(info.minChunkZ, cz);
            info.maxChunkZ = std::max<int64_t>(info.maxChunkZ, cz);
            info.maxSectionY = std::max<int64_t>(info.maxSectionY, maxY);
        }
    }

    return info;
}

static void preloadChunks(world::OMChunkManager<16> &manager, std::function<void(double)> statusReport)
{
    namespace fs = std::filesystem;

    std::unordered_map<int, std::string> tempToName;

    {
        auto in = std::make_shared<std::ifstream>("chunks_mapped/id_mapping.json");

        if (!*in)
        {
            return;
        }

        io::json::OMJsonAstBuilder bld(std::make_shared<io::json::OMJsonTokenIter>(in));
        auto doc = bld.build();

        if (doc->getMap().count("tempIdToBlock"))
        {
            for (auto &p : doc->getMap()["tempIdToBlock"]->getMap())
            {
                tempToName[std::stoi(p.first)] = p.second->getString();
            }
        }
    }

    if (tempToName.empty())
    {
        return;
    }

    std::vector<uint32_t> lut;

    for (auto &p : tempToName)
    {
        if (static_cast<int>(lut.size()) <= p.first)
        {
            lut.resize(p.first + 1, 0);
        }

        try
        {
            lut[p.first] = data::block::blockStateRegistry.id(data::block::makeBlockState(p.second));
        }
        catch (std::exception &)
        {
            lut[p.first] = data::block::blockStateRegistry.id(data::block::makeBlockState("minecraft:stone[]"));
        }
    }

    const uint32_t airId = data::block::blockStateRegistry.id(data::block::makeBlockState("minecraft:air[]"));

    std::error_code ec;
    int loadedChunks = 0;
    int totChunks = 0;

    for (auto &entry : fs::directory_iterator("chunks_mapped", ec))
    {
        ++totChunks;
    }

    for (auto &entry : fs::directory_iterator("chunks_mapped", ec))
    {
        if (ec || !entry.is_regular_file() || entry.path().extension() != ".omc")
        {
            continue;
        }

        std::ifstream in(entry.path(), std::ios::binary);

        if (!in)
        {
            continue;
        }

        std::vector<char> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        if (buf.size() < 20 || std::memcmp(buf.data(), "OMCH", 4) != 0)
        {
            continue;
        }

        int32_t cx = 0, cz = 0;
        uint32_t sections = 0;
        std::memcpy(&cx, buf.data() + 8, 4);
        std::memcpy(&cz, buf.data() + 12, 4);
        std::memcpy(&sections, buf.data() + 16, 4);

        size_t off = 20;

        for (uint32_t si = 0; si < sections; ++si)
        {
            if (off + 4 + 4096 * 4 + 64 * 4 > buf.size())
            {
                break;
            }

            off += 4;

            const auto *ids = reinterpret_cast<const uint32_t *>(buf.data() + off);
            off += 4096 * 4 + 64 * 4;

            const int64_t chunkY = -4 + static_cast<int64_t>(si);
            world::OMChunk<16> cnk(cx, chunkY, cz);
            cnk.setAirId(airId);
            cnk.fill(airId);

            for (int y = 0; y < 16; ++y)
            {
                for (int z = 0; z < 16; ++z)
                {
                    for (int x = 0; x < 16; ++x)
                    {
                        const uint32_t temp = ids[y * 256 + z * 16 + x];
                        const uint32_t state = temp < lut.size() ? lut[temp] : airId;

                        if (state != airId)
                        {
                            cnk.setBlock(x, y, z, state);
                        }
                    }
                }
            }

            manager.loadChunk(cnk);
        }

        ++loadedChunks;
        statusReport(static_cast<double>(loadedChunks) / totChunks);
    }

    std::fprintf(stderr, "[preload] loaded %d chunks from chunks_mapped, tempIds=%zu\n", loadedChunks,
                 tempToName.size());
}

auto startupCamera() -> OMStartupCamera
{
    static const bool preloadFromDisk = std::getenv("OM_CHUNK_PRELOAD") != nullptr;
    constexpr int64_t worldOffsetChunks = 0;

    OMStartupCamera cam;

    if (preloadFromDisk)
    {
        const auto bounds = preloadChunkBounds();
        const int64_t centerChunkX = (bounds.minChunkX + bounds.maxChunkX + 1) / 2;
        const int64_t centerChunkZ = (bounds.minChunkZ + bounds.maxChunkZ + 1) / 2;

        const int64_t cameraY = bounds.maxBlockY + 12;
        const int64_t cameraChunkY = cameraY / 16;

        cam.chunkX = centerChunkX;
        cam.chunkY = cameraChunkY;
        cam.chunkZ = centerChunkZ;
        cam.localX = 8.0f;
        cam.localY = static_cast<float>(cameraY - cameraChunkY * 16);
        cam.localZ = 8.0f;
        cam.yaw = 45.0f;
        cam.pitch = -50.0f;
    }
    else
    {
        cam.chunkX = worldOffsetChunks;
        cam.chunkY = 1;
        cam.chunkZ = worldOffsetChunks;
        cam.localX = cam.localY = cam.localZ = 8.0f;
        cam.yaw = 45.0f;
        cam.pitch = -35.0f;
    }

    return cam;
}

void runTerrainGeneration(world::OMChunkManager<16> &manager, std::atomic<bool> &stopProducing,
                          std::function<void(double)> statusReport)
{
    static const bool preloadFromDisk = std::getenv("OM_CHUNK_PRELOAD") != nullptr;
    constexpr int64_t worldOffsetChunks = 0;

    if (preloadFromDisk)
    {
        preloadChunks(manager, statusReport);
        return;
    }

    const OMPerlinNoise noise(0x9E3779B9u);

    int width = 16;
    int height = 16;
    int depth = 8;

    int gMinHeight = 255;
    int gMaxHeight = -255;
    long long gHeightSum = 0;
    long long gHeightCount = 0;
    std::array<char, 256> gHeightSeen = {};
    int gChunkUniqueMin = 999;
    int gChunkUniqueMax = 0;
    long long gChunkUniqueSum = 0;
    long long gChunkUniqueCount = 0;
    unsigned long long gHeightHash = 1469598103934665603ull;

    auto totChunks = 2 * width * (depth + 1) * 2 * height;
    int loadedChunks = 0;

    for (int cx = -width; cx < width; ++cx)
    {
        for (int cy = -depth; cy <= 0; ++cy)
        {
            for (int cz = -height; cz < height; ++cz)
            {
                if (stopProducing.load())
                {
                    return;
                }

                using data::block::blockStateRegistry;

                static constexpr int seaLevel = 7;

                const uint32_t airId = blockStateRegistry.id(data::block::makeBlockState("minecraft:air[]"));
                const uint32_t stoneId = blockStateRegistry.id(data::block::makeBlockState("minecraft:stone[]"));
                const uint32_t dirtId = blockStateRegistry.id(data::block::makeBlockState("minecraft:dirt[]"));
                const uint32_t grassId =
                    blockStateRegistry.id(data::block::makeBlockState("minecraft:grass_block[snowy=false]"));
                const uint32_t waterId = blockStateRegistry.id(data::block::makeBlockState("minecraft:water[level=0]"));

                const int64_t bx = static_cast<int64_t>(cx) + worldOffsetChunks;
                const int64_t bz = static_cast<int64_t>(cz) + worldOffsetChunks;

                world::OMChunk<16> cnk(bx, cy, bz);
                cnk.setAirId(airId);

                std::array<int, 16 * 16> heights = {};
                int minHeight = 255;
                int maxHeight = -255;

                for (int x = 0; x < 16; ++x)
                {
                    for (int z = 0; z < 16; ++z)
                    {
                        const double wx = static_cast<double>(bx) * 16.0 + static_cast<double>(x);
                        const double wz = static_cast<double>(bz) * 16.0 + static_cast<double>(z);

                        const double h =
                            9.0 + 5.0 * noise.fbm(wx * 0.02, wz * 0.02, 4) + 1.2 * noise.fbm(wx * 0.09, wz * 0.09, 2);
                        const int height = static_cast<int>(std::floor(h));

                        heights[x * 16 + z] = height;
                        minHeight = std::min(minHeight, height);
                        maxHeight = std::max(maxHeight, height);
                    }
                }

                if (cy == 0)
                {
                    gMinHeight = std::min(gMinHeight, minHeight);
                    gMaxHeight = std::max(gMaxHeight, maxHeight);

                    for (auto h : heights)
                    {
                        if (h >= -128 && h < 128)
                        {
                            gHeightSeen[h + 128] = 1;
                        }
                        gHeightSum += h;
                        ++gHeightCount;
                    }

                    std::array<int, 16 * 16> local = heights;
                    std::sort(local.begin(), local.end());
                    int localUnique = 1;
                    for (size_t i = 1; i < local.size(); ++i)
                    {
                        if (local[i] != local[i - 1])
                        {
                            ++localUnique;
                        }
                    }
                    gChunkUniqueMin = std::min(gChunkUniqueMin, localUnique);
                    gChunkUniqueMax = std::max(gChunkUniqueMax, localUnique);
                    gChunkUniqueSum += localUnique;
                    ++gChunkUniqueCount;

                    for (auto h : heights)
                    {
                        gHeightHash = gHeightHash * 1000003ull + static_cast<unsigned long long>(h + 1000);
                    }
                }

                const int chunkMinY = cy * 16;
                const int chunkMaxY = chunkMinY + 15;

                if (chunkMaxY < minHeight)
                {
                    cnk.fill(stoneId);
                }
                else if (chunkMinY > maxHeight)
                {
                    if (chunkMinY > seaLevel)
                    {
                        cnk.fill(airId);
                    }
                    else
                    {
                        cnk.fill(airId);

                        for (int x = 0; x < 16; ++x)
                        {
                            for (int z = 0; z < 16; ++z)
                            {
                                const int from = std::max(chunkMinY, heights[x * 16 + z] + 1);

                                for (int wy = from; wy <= seaLevel && wy <= chunkMaxY; ++wy)
                                {
                                    cnk.setBlock(x, wy - chunkMinY, z, waterId);
                                }
                            }
                        }
                    }
                }
                else
                {
                    cnk.fill(airId);

                    for (int x = 0; x < 16; ++x)
                    {
                        for (int z = 0; z < 16; ++z)
                        {
                            const int height = heights[x * 16 + z];

                            for (int y = 0; y < 16; ++y)
                            {
                                const int wy = chunkMinY + y;

                                if (wy > height)
                                {
                                    if (wy <= seaLevel)
                                    {
                                        cnk.setBlock(x, y, z, waterId);
                                    }
                                    continue;
                                }

                                if (wy == height)
                                {
                                    cnk.setBlock(x, y, z, grassId);
                                }
                                else if (wy > height - 4)
                                {
                                    cnk.setBlock(x, y, z, dirtId);
                                }
                                else
                                {
                                    cnk.setBlock(x, y, z, stoneId);
                                }
                            }
                        }
                    }
                }

                manager.loadChunk(cnk);
                ++loadedChunks;

                statusReport(static_cast<double>(loadedChunks) / totChunks);
            }
        }
    }

    int gUnique = 0;
    for (auto v : gHeightSeen)
    {
        gUnique += v != 0;
    }

    std::fprintf(stderr,
                 "[terrain] offsetChunks=%lld chunks=%lld heights min=%d max=%d unique=%d mean=%.2f "
                 "chunkUnique min=%d max=%d avg=%.2f hash=%016llx\n",
                 static_cast<long long>(worldOffsetChunks), gHeightCount / 256, gMinHeight, gMaxHeight, gUnique,
                 static_cast<double>(gHeightSum) / std::max<long long>(gHeightCount, 1), gChunkUniqueMin,
                 gChunkUniqueMax, static_cast<double>(gChunkUniqueSum) / std::max<long long>(gChunkUniqueCount, 1),
                 gHeightHash);
}
} // namespace openminecraft::shell::application
