#include "openminecraft/specs/png/om_png.hpp"
#include "fmt/format.h"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "openminecraft/specs/zlib/om_zlib_inflate.hpp"
#include "openminecraft/util/om_util_crc.hpp"
#include "zlib.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

namespace openminecraft::specs::png
{
OMPngFile::OMPngFile()
{
}

static void writePixel(uint8_t *result, uint8_t *source, OMPngColorType type, uint8_t bitdepth, int x, int *palette)
{
    switch (type)
    {
    case RGBA:
        result[0] = source[x * 4 * bitdepth / 8];
        result[1] = source[(x * 4 + 1) * bitdepth / 8];
        result[2] = source[(x * 4 + 2) * bitdepth / 8];
        result[3] = source[(x * 4 + 3) * bitdepth / 8];
        break;
    case RGBTriple:
        result[0] = source[x * 3 * bitdepth / 8];
        result[1] = source[(x * 3 + 1) * bitdepth / 8];
        result[2] = source[(x * 3 + 2) * bitdepth / 8];
        result[3] = 0xff;
        break;
    case GrayscaleAlpha:
        std::memset(result, source[x * 2 * bitdepth / 8], 3);
        result[4] = source[(x * 2 + 1) * bitdepth / 8];
        break;
    case Palette:
        if (bitdepth == 8)
        {
            int pal = palette[source[x]];
            result[0] = (pal >> 24) & 0xff;
            result[1] = (pal >> 16) & 0xff;
            result[2] = (pal >> 8) & 0xff;
            result[3] = pal & 0xff;
        }
        else
        {
            int pixelsPerByte = 8 / bitdepth;
            int byteIdx = x / pixelsPerByte;
            int shift = (pixelsPerByte - 1 - (x % pixelsPerByte)) * bitdepth;

            uint8_t byte = source[byteIdx];
            uint8_t value = (byte >> shift) & ((1 << bitdepth) - 1);
            int pal = palette[value];

            result[0] = (pal >> 24) & 0xff;
            result[1] = (pal >> 16) & 0xff;
            result[2] = (pal >> 8) & 0xff;
            result[3] = pal & 0xff;
        }
        break;
    case Grayscale:
        if (bitdepth == 8)
        {
            std::memset(result, source[x * bitdepth / 8], 3);
            result[4] = 0xff;
        }
        else
        {
            int pixelsPerByte = 8 / bitdepth;
            int byteIdx = x / pixelsPerByte;
            int shift = (pixelsPerByte - 1 - (x % pixelsPerByte)) * bitdepth;

            uint8_t byte = source[byteIdx];
            uint8_t value = (byte >> shift) & ((1 << bitdepth) - 1);
            value = static_cast<uint8_t>((value * 255) / ((1 << bitdepth) - 1));

            std::memset(result, value, 3);
            result[4] = 0xff;
        }
        break;
    default:
        break;
    }
}

static uint8_t getPaethPred(int a, int b, int c)
{
    auto p = a + b - c;
    auto pa = std::abs(p - a);
    auto pb = std::abs(p - b);
    auto pc = std::abs(p - c);

    if (pa <= pb && pa <= pc)
    {
        return a;
    }
    else if (pb <= pc)
    {
        return b;
    }
    else
    {
        return c;
    }
}

void OMPngFile::parse(std::shared_ptr<std::istream> istr)
{
    std::array<uint8_t, 8> hd = {};
    istr->read(reinterpret_cast<char *>(hd.data()), 8);

    if (hd != header)
    {
        throw std::logic_error("Bad png header!");
    }

    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> unzippedBuffer = {};

    zlib::OMZLibInflater inf([&](uint8_t *data, uint64_t len) {
        for (uint64_t i = 0; i < len; i++)
        {
            unzippedBuffer.push_back(data[i]);
        }
    });

    int y = 0;
    int pass = 0; // only used for interlacing
    dataBuffer.clear();

    while (istr->good())
    {
        OMPngChunk cnk;

        uint32_t length;
        istr->read(reinterpret_cast<char *>(&length), 4);
        length = binary::be32ToNative(length);

        cnk.data = {};
        cnk.data.resize(length);

        istr->read(cnk.name, 4);
        istr->read(reinterpret_cast<char *>(cnk.data.data()), length);
        istr->read(reinterpret_cast<char *>(&cnk.crc), 4);
        cnk.crc = binary::be32ToNative(cnk.crc);

        auto res = crc(cnk);
        if (res != cnk.crc)
        {
            throw std::runtime_error(fmt::format("crc check failed at 0x{:x}, expected {:x}, actual {:x}",
                                                 static_cast<uint64_t>(istr->tellg()), cnk.crc, res));
        }

        if (!std::memcmp(cnk.name, "IEND", 4))
        {
            break;
        }

        if (!std::memcmp(cnk.name, "IHDR", 4))
        {
            std::memcpy(&this->head, cnk.data.data(), sizeof(OMPngHead));
            head.width = binary::be32ToNative(head.width);
            head.height = binary::be32ToNative(head.height);

            if (head.interlacing)
            {
                dataBuffer.resize(head.width * head.height * 4);
            }
            else
            {
                dataBuffer.reserve(head.width * head.height * 4);
            }
        }

        if (!std::memcmp(cnk.name, "PLTE", 4))
        {
            auto mm = cnk.data;
            for (int i = 0; i < cnk.data.size() / 3; i++)
            {
                palette.push_back((mm[i * 3] << 24) | (mm[i * 3 + 1] << 16) | (mm[i * 3 + 2] << 8) | 0xff);
            }
        }

        if (!std::memcmp(cnk.name, "tRNS", 4))
        {
            int i = 0;
            for (auto &pp : palette)
            {
                pp &= 0xffffff00;
                pp |= cnk.data[i];
                i++;
            }
        }

        if (!std::memcmp(cnk.name, "IDAT", 4))
        {
            inf.input(cnk.data.data(), cnk.data.size());

            if (!head.interlacing)
            {
                while (unzippedBuffer.size() >= getStride(head.width) + 1)
                {
                    int flttype = unzippedBuffer[0];
                    unzippedBuffer.erase(unzippedBuffer.begin());

                    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> prefilter;
                    prefilter.assign(std::make_move_iterator(unzippedBuffer.begin()),
                                     std::make_move_iterator(unzippedBuffer.begin() + getStride(head.width)));
                    unzippedBuffer.erase(unzippedBuffer.begin(), unzippedBuffer.begin() + getStride(head.width));

                    defilter(flttype, prefilter, y);
                    y++;
                }
            }
            else
            {
                auto passSize = getAdamPassSize(pass);
                while (unzippedBuffer.size() >= getStride(passSize.first) + 1)
                {
                    int flttype = unzippedBuffer[0];
                    unzippedBuffer.erase(unzippedBuffer.begin());
                    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> prefilter;
                    prefilter.assign(std::make_move_iterator(unzippedBuffer.begin()),
                                     std::make_move_iterator(unzippedBuffer.begin() + getStride(passSize.first)));
                    unzippedBuffer.erase(unzippedBuffer.begin(), unzippedBuffer.begin() + getStride(passSize.first));
                    defilterAdam(flttype, prefilter, y, pass);
                    y++;

                    if (y == passSize.second)
                    {
                        y = 0;
                        pass = std::min(6, pass + 1);
                        passSize = getAdamPassSize(pass);
                    }
                }
            }
        }
    }
}

std::pair<uint32_t, uint32_t> OMPngFile::getAdamPassSize(int pass)
{
    auto stat = pngAdam7[pass];
    return std::make_pair(std::ceil((head.width - stat.offsetx) / static_cast<float>(stat.stridex)),
                          std::ceil((head.height - stat.offsety) / static_cast<float>(stat.stridey)));
}

static void defilterBase(std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &current, int type,
                         std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &raw, int y, int bpx,
                         std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &filterCache)
{
    auto getBufferA = [&](int x) { return x >= bpx ? current[x - bpx] : 0; };
    auto getBufferB = [&](int x) { return y > 0 ? filterCache[x] : 0; };
    auto getBufferC = [&](int x) { return (x >= bpx && y > 0) ? filterCache[x - bpx] : 0; };

    for (int i = 0; i < raw.size(); i++)
    {
        switch (type)
        {
        case 0:
            current.push_back(raw[i]);
            break;
        case 1:
            current.push_back(raw[i] + getBufferA(i));
            break;
        case 2:
            current.push_back(raw[i] + getBufferB(i));
            break;
        case 3:
            current.push_back(raw[i] + (getBufferA(i) + getBufferB(i)) / 2);
            break;
        case 4:
            current.push_back(raw[i] + getPaethPred(getBufferA(i), getBufferB(i), getBufferC(i)));
            break;
        default:
            throw std::runtime_error("unknown filter method!");
        }
    }

    filterCache.clear();
    filterCache.assign(current.begin(), current.end());
}

void OMPngFile::defilterAdam(int type, std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &raw, int y,
                             int pass)
{
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> current;

    int bpx = getBytesPerPixel();
    defilterBase(current, type, raw, y, bpx, filterCache);
    writeIntoBufferAdam(current, pass, y);
}
void OMPngFile::writeIntoBufferAdam(std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &current, int pass,
                                    int y)
{
    auto def = pngAdam7[pass];
    auto size = getAdamPassSize(pass);

    auto actualY = def.offsety + y * def.stridey;

    for (int x = 0; x < size.first; x++)
    {
        auto actualX = def.offsetx + x * def.stridex;
        for (int yoff = 0; yoff < def.areay; yoff++)
        {
            auto finalY = actualY + yoff;
            if (finalY >= head.height)
            {
                break;
            }
            for (int xoff = 0; xoff < def.areax; xoff++)
            {
                auto finalX = actualX + xoff;
                if (finalX >= head.width)
                {
                    break;
                }

                int pixelIndex = finalY * head.width + finalX;

                writePixel(dataBuffer.data() + pixelIndex * 4, current.data(), head.type, head.bitDepth, x,
                           palette.data());
            }
        }
    }
}
void OMPngFile::writeIntoBuffer(std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &current)
{
    for (int x = 0; x < head.width; x++)
    {
        uint8_t buffer[4];
        writePixel(buffer, current.data(), head.type, head.bitDepth, x, palette.data());
        dataBuffer.push_back(buffer[0]);
        dataBuffer.push_back(buffer[1]);
        dataBuffer.push_back(buffer[2]);
        dataBuffer.push_back(buffer[3]);
    }
}
void OMPngFile::defilter(int type, std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &raw, int y)
{
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> current;

    int bpx = getBytesPerPixel();
    defilterBase(current, type, raw, y, bpx, filterCache);
    writeIntoBuffer(current);
}

uint32_t OMPngFile::getStride(int width)
{
    switch (head.type)
    {
    default:
    case RGBA:
        return width * 4 * head.bitDepth / 8;
    case RGBTriple:
        return width * 3 * head.bitDepth / 8;
    case GrayscaleAlpha:
        return width * 2 * head.bitDepth / 8;
    case Palette:
    case Grayscale: {
        if (head.bitDepth >= 8)
        {
            return width * head.bitDepth / 8;
        }
        else
        {
            auto bits = width * head.bitDepth;
            return bits + (8 - bits % 8) % 8;
        }
    }
    }
}

int OMPngFile::getBytesPerPixel()
{
    if (head.bitDepth < 8)
    {
        return 1;
    }
    switch (head.type)
    {
    default:
    case RGBA:
        return 4 * head.bitDepth / 8;
    case RGBTriple:
        return 3 * head.bitDepth / 8;
    case GrayscaleAlpha:
        return 2 * head.bitDepth / 8;
    case Grayscale:
    case Palette:
        return head.bitDepth / 8;
    }
}

uint64_t OMPngFile::crc(OMPngChunk chunk)
{
    uint32_t crc = 0xffffffff;
    crc = openminecraft::util::calcCrc(crc, chunk.name, 4);
    crc = openminecraft::util::calcCrc(crc, chunk.data.data(), chunk.data.size());
    return crc ^ 0xffffffff;
}

void *OMPngFile::fetchData()
{
    return dataBuffer.data();
}

int OMPngFile::getWidth()
{
    return head.width;
}
int OMPngFile::getHeight()
{
    return head.height;
}

OMPngFile::~OMPngFile()
{
}
} // namespace openminecraft::specs::png
