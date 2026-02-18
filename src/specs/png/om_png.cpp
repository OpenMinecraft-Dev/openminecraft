#include "openminecraft/specs/png/om_png.hpp"
#include "fmt/format.h"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "zlib.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace openminecraft::specs::png
{
OMPngFile::OMPngFile(std::shared_ptr<std::istream> istr)
{
    std::array<uint8_t, 8> hd = {};
    istr->read(reinterpret_cast<char *>(hd.data()), 8);

    if (hd != header)
    {
        throw std::logic_error("Bad png header!");
    }

    std::vector<OMPngChunk> chunks;
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> IDATdata;
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

        chunks.push_back(cnk);

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
            for (auto i : cnk.data)
            {
                IDATdata.push_back(i);
            }
        }
    }

    z_stream strm = {0};
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;

    inflateInit(&strm);

    strm.next_in = IDATdata.data();
    strm.avail_in = IDATdata.size();

    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> unzippedBuffer;
    unzippedBuffer.resize(head.height * (1 + getStride()));

    strm.next_out = unzippedBuffer.data();
    strm.avail_out = unzippedBuffer.size();

    int r = inflate(&strm, Z_FINISH);

    if (r != Z_STREAM_END && r != Z_OK)
    {
        throw std::runtime_error(fmt::format("zlib inflate failed: {}", r));
    }

    inflateEnd(&strm);

    dataBuffer = mem::allocator::tracedMallocParser(head.height * getStride());

    auto stride = getStride();
    auto ccp = unzippedBuffer.data();
    for (int y = 0; y < head.height; y++)
    {
        auto fltType = *ccp;
        ccp++;

        for (int x = 0; x < stride; x++)
        {
            auto filtx = *ccp;
            ++ccp;

#define CurrentByte reinterpret_cast<uint8_t *>(dataBuffer)[y * stride + x]

            switch (fltType)
            {
            case 0:
                CurrentByte = filtx;
                break;
            case 1:
                CurrentByte = filtx + getBufferA(y, x);
                break;
            case 2:
                CurrentByte = filtx + getBufferB(y, x);
                break;
            case 3:
                CurrentByte = filtx + (getBufferA(y, x) + getBufferB(y, x)) / 2;
                break;
            case 4:
                CurrentByte = filtx + getPaethPred(getBufferA(y, x), getBufferB(y, x), getBufferC(y, x));
                break;
            default:
                throw std::runtime_error("unknown filter!");
            }
        }
    }

    // convert to standard RGBA
    {
        auto result = reinterpret_cast<uint8_t *>(mem::allocator::tracedMallocParser(head.width * head.height * 4));
        auto source = reinterpret_cast<uint8_t *>(dataBuffer);

        switch (head.type)
        {
        case RGBA: {
            if (head.bitDepth == 8)
            {
                mem::allocator::tracedFreeParser(result);
                break;
            }

            for (int i = 0; i < head.width * head.height * 4; i++)
            {
                result[i] = source[2 * i];
            }

            mem::allocator::tracedFreeParser(dataBuffer);
            dataBuffer = result;
            break;
        }
        case RGBTriple: {
            for (int i = 0; i < head.width * head.height; i++)
            {
                result[i * 4] = source[i * 3 * (head.bitDepth / 8)];
                result[i * 4 + 1] = source[(i * 3 + 1) * (head.bitDepth / 8)];
                result[i * 4 + 2] = source[(i * 3 + 2) * (head.bitDepth / 8)];
                result[i * 4 + 3] = 0xff;
            }
            mem::allocator::tracedFreeParser(dataBuffer);
            dataBuffer = result;
            break;
        }
        case GrayscaleAlpha: {
            for (int i = 0; i < head.width * head.height; i++)
            {
                result[i * 4] = source[i * 2 * (head.bitDepth / 8)];
                result[i * 4 + 1] = result[i * 4];
                result[i * 4 + 2] = result[i * 4];
                result[i * 4 + 3] = source[(i * 2 + 1) * (head.bitDepth / 8)];
            }
            mem::allocator::tracedFreeParser(dataBuffer);
            dataBuffer = result;
            break;
        }
        case Palette: {
            if (head.bitDepth >= 8)
            {
                for (int i = 0; i < head.width * head.height; i++)
                {
                    int code = palette[source[i * (head.bitDepth / 8)]];
                    result[i * 4] = (code >> 24) & 0xff;
                    result[i * 4 + 1] = (code >> 16) & 0xff;
                    result[i * 4 + 2] = (code >> 8) & 0xff;
                    result[i * 4 + 3] = code & 0xff;
                }
                mem::allocator::tracedFreeParser(dataBuffer);
                dataBuffer = result;
            }
            else
            {
                int rowBytes = getStride();
                int pixelsPerByte = 8 / head.bitDepth;

                for (int y = 0; y < head.height; y++)
                {
                    for (int x = 0; x < head.width; x++)
                    {
                        int byteIdx = x / pixelsPerByte;
                        int shift = (pixelsPerByte - 1 - (x % pixelsPerByte)) * head.bitDepth;

                        uint8_t byte = source[y * rowBytes + byteIdx];
                        uint8_t value = (byte >> shift) & ((1 << head.bitDepth) - 1);
                        int code = palette[value];

                        auto i = y * head.width + x;
                        result[i * 4] = (code >> 24) & 0xff;
                        result[i * 4 + 1] = (code >> 16) & 0xff;
                        result[i * 4 + 2] = (code >> 8) & 0xff;
                        result[i * 4 + 3] = code & 0xff;
                    }
                }

                mem::allocator::tracedFreeParser(dataBuffer);
                dataBuffer = result;
            }
            break;
        }
        case Grayscale: {
            if (head.bitDepth >= 8)
            {
                for (int i = 0; i < head.width * head.height; i++)
                {
                    result[i * 4] = source[i * (head.bitDepth / 8)];
                    result[i * 4 + 1] = result[i];
                    result[i * 4 + 2] = result[i];
                    result[i * 4 + 3] = 0xff;
                }
                mem::allocator::tracedFreeParser(dataBuffer);
                dataBuffer = result;
            }
            else
            {
                int rowBytes = getStride();
                int pixelsPerByte = 8 / head.bitDepth;

                for (int y = 0; y < head.height; y++)
                {
                    for (int x = 0; x < head.width; x++)
                    {
                        int byteIdx = x / pixelsPerByte;
                        int shift = (pixelsPerByte - 1 - (x % pixelsPerByte)) * head.bitDepth;

                        uint8_t byte = source[y * rowBytes + byteIdx];
                        uint8_t value = (byte >> shift) & ((1 << head.bitDepth) - 1);

                        auto i = y * head.width + x;
                        result[i * 4] = (value * 255) / ((1 << head.bitDepth) - 1);
                        result[i * 4 + 1] = result[i];
                        result[i * 4 + 2] = result[i];
                        result[i * 4 + 3] = 0xff;
                    }
                }

                mem::allocator::tracedFreeParser(dataBuffer);
                dataBuffer = result;
            }
            break;
        }
        default:
            break;
        }
    }

    std::ofstream of("test2.bin");
    of.write(reinterpret_cast<char *>(dataBuffer), head.width * head.height * 4);
}

uint8_t OMPngFile::getPaethPred(int a, int b, int c)
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

uint8_t OMPngFile::getBufferA(int y, int x)
{
    return x >= getBytesPerPixel() ? reinterpret_cast<uint8_t *>(dataBuffer)[y * getStride() + x - getBytesPerPixel()]
                                   : 0;
}

uint8_t OMPngFile::getBufferB(int y, int x)
{
    return (y > 0) ? reinterpret_cast<uint8_t *>(dataBuffer)[(y - 1) * getStride() + x] : 0;
}
uint8_t OMPngFile::getBufferC(int y, int x)
{
    return (x >= getBytesPerPixel() && y > 0)
               ? reinterpret_cast<uint8_t *>(dataBuffer)[(y - 1) * getStride() + x - getBytesPerPixel()]
               : 0;
}

uint32_t OMPngFile::getStride()
{
    switch (head.type)
    {
    default:
    case RGBA:
        return head.width * 4 * head.bitDepth / 8;
    case RGBTriple:
        return head.width * 3 * head.bitDepth / 8;
    case GrayscaleAlpha:
        return head.width * 2 * head.bitDepth / 8;
    case Palette:
    case Grayscale: {
        if (head.bitDepth >= 8)
        {
            return head.width * head.bitDepth / 8;
        }
        else
        {
            auto bits = head.width * head.bitDepth;
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

void OMPngFile::makeCrcTable()
{
    uint64_t c;

    for (int n = 0; n < 256; n++)
    {
        c = static_cast<uint64_t>(n);
        for (int k = 0; k < 8; k++)
        {
            if (c & 1)
            {
                c = 0xedb88320 ^ (c >> 1);
            }
            else
            {
                c = c >> 1;
            }
        }
        crcTable[n] = c;
    }

    crcCalc = true;
}

uint64_t OMPngFile::updateCrc(uint64_t crc, void *dd, int length)
{
    uint64_t c = crc;
    if (!crcCalc)
    {
        makeCrcTable();
    }

    for (int n = 0; n < length; n++)
    {
        c = crcTable[(c ^ reinterpret_cast<uint8_t *>(dd)[n]) & 0xff] ^ (c >> 8);
    }

    return c;
}

uint64_t OMPngFile::crc(OMPngChunk chunk)
{
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> buf;
    buf.resize(chunk.data.size() + 4);
    std::memcpy(buf.data(), chunk.name, 4);
    std::memcpy(buf.data() + 4, chunk.data.data(), chunk.data.size());
    return updateCrc(0xffffffffL, buf.data(), chunk.data.size() + 4) ^ 0xffffffffL;
}

void *OMPngFile::fetchData()
{
    return dataBuffer;
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
    mem::allocator::tracedFreeParser(dataBuffer);
}
} // namespace openminecraft::specs::png
