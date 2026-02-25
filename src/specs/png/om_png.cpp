#include "openminecraft/specs/png/om_png.hpp"
#include "fmt/format.h"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "openminecraft/specs/zlib/om_zlib_inflate.hpp"
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
            filterCache.resize(getStride());

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
                while (unzippedBuffer.size() >= getStride() + 1)
                {
                    int flttype = unzippedBuffer[0];
                    unzippedBuffer.erase(unzippedBuffer.begin());

                    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> prefilter;
                    prefilter.assign(std::make_move_iterator(unzippedBuffer.begin()),
                                     std::make_move_iterator(unzippedBuffer.begin() + getStride()));
                    unzippedBuffer.erase(unzippedBuffer.begin(), unzippedBuffer.begin() + getStride());

                    defilter(flttype, prefilter, y);
                    y++;
                }
            }
            else
            {
                while (unzippedBuffer.size() >= getAdamStride(pass) + 1)
                {
                    auto passSize = getAdamPassSize(pass);
                    int flttype = unzippedBuffer[0];
                    unzippedBuffer.erase(unzippedBuffer.begin());
                    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> prefilter;
                    prefilter.assign(std::make_move_iterator(unzippedBuffer.begin()),
                                     std::make_move_iterator(unzippedBuffer.begin() + getAdamStride(pass)));
                    unzippedBuffer.erase(unzippedBuffer.begin(), unzippedBuffer.begin() + getAdamStride(pass));
                    defilterAdam(flttype, prefilter, y, pass);
                    y++;

                    if (y == passSize.second)
                    {
                        y = 0;
                        pass = std::min(6, pass + 1);
                    }
                }
            }
        }
    }
}

uint32_t OMPngFile::getAdamStride(int pass)
{
    auto wid = getAdamPassSize(pass).first;
    switch (head.type)
    {
    default:
    case RGBA:
        return wid * 4 * head.bitDepth / 8;
    case RGBTriple:
        return wid * 3 * head.bitDepth / 8;
    case GrayscaleAlpha:
        return wid * 2 * head.bitDepth / 8;
    case Palette:
    case Grayscale: {
        if (head.bitDepth >= 8)
        {
            return wid * head.bitDepth / 8;
        }
        else
        {
            auto bits = wid * head.bitDepth;
            return bits + (8 - bits % 8) % 8;
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
void OMPngFile::defilterAdam(int type, std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &raw, int y,
                             int pass)
{
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> current;

    int bpx = getBytesPerPixel();
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

                switch (head.type)
                {
                case RGBA:
                    dataBuffer[pixelIndex * 4] = current[x * 4 * head.bitDepth / 8];
                    dataBuffer[pixelIndex * 4 + 1] = current[(x * 4 + 1) * head.bitDepth / 8];
                    dataBuffer[pixelIndex * 4 + 2] = current[(x * 4 + 2) * head.bitDepth / 8];
                    dataBuffer[pixelIndex * 4 + 3] = current[(x * 4 + 3) * head.bitDepth / 8];
                    break;
                case RGBTriple:
                    dataBuffer[pixelIndex * 4] = current[x * 3 * head.bitDepth / 8];
                    dataBuffer[pixelIndex * 4 + 1] = current[(x * 3 + 1) * head.bitDepth / 8];
                    dataBuffer[pixelIndex * 4 + 2] = current[(x * 3 + 2) * head.bitDepth / 8];
                    dataBuffer[pixelIndex * 4 + 3] = 0xff;
                    break;
                case GrayscaleAlpha:
                    dataBuffer[pixelIndex * 4] = current[x * 2 * head.bitDepth / 8];
                    dataBuffer[pixelIndex * 4 + 1] = dataBuffer[pixelIndex * 4];
                    dataBuffer[pixelIndex * 4 + 2] = dataBuffer[pixelIndex * 4];
                    dataBuffer[pixelIndex * 4 + 3] = current[(x * 2 + 1) * head.bitDepth / 8];
                    break;
                case Palette:
                    if (head.bitDepth == 8)
                    {
                        int pal = palette[current[x]];
                        dataBuffer[pixelIndex * 4] = (pal >> 24) & 0xff;
                        dataBuffer[pixelIndex * 4 + 1] = (pal >> 16) & 0xff;
                        dataBuffer[pixelIndex * 4 + 2] = (pal >> 8) & 0xff;
                        dataBuffer[pixelIndex * 4 + 3] = pal & 0xff;
                    }
                    else
                    {
                        int pixelsPerByte = 8 / head.bitDepth;
                        int byteIdx = x / pixelsPerByte;
                        int shift = (pixelsPerByte - 1 - (x % pixelsPerByte)) * head.bitDepth;

                        uint8_t byte = current[byteIdx];
                        uint8_t value = (byte >> shift) & ((1 << head.bitDepth) - 1);
                        int pal = palette[value];

                        dataBuffer[pixelIndex * 4] = (pal >> 24) & 0xff;
                        dataBuffer[pixelIndex * 4 + 1] = (pal >> 16) & 0xff;
                        dataBuffer[pixelIndex * 4 + 2] = (pal >> 8) & 0xff;
                        dataBuffer[pixelIndex * 4 + 3] = pal & 0xff;
                    }
                    break;
                case Grayscale:
                    if (head.bitDepth >= 8)
                    {
                        dataBuffer[pixelIndex * 4] = current[x * head.bitDepth / 8];
                        dataBuffer[pixelIndex * 4 + 1] = dataBuffer[pixelIndex * 4];
                        dataBuffer[pixelIndex * 4 + 2] = dataBuffer[pixelIndex * 4];
                        dataBuffer[pixelIndex * 4 + 3] = 0xff;
                    }
                    else
                    {
                        int pixelsPerByte = 8 / head.bitDepth;
                        int byteIdx = x / pixelsPerByte;
                        int shift = (pixelsPerByte - 1 - (x % pixelsPerByte)) * head.bitDepth;

                        uint8_t byte = current[byteIdx];
                        uint8_t value = (byte >> shift) & ((1 << head.bitDepth) - 1);
                        value = static_cast<uint8_t>((value * 255) / ((1 << head.bitDepth) - 1));

                        dataBuffer[pixelIndex * 4] = value;
                        dataBuffer[pixelIndex * 4 + 1] = value;
                        dataBuffer[pixelIndex * 4 + 2] = value;
                        dataBuffer[pixelIndex * 4 + 3] = 0xff;
                    }
                    break;
                default:
                    throw std::runtime_error("unsupported pixel format");
                    break;
                }
            }
        }
    }
}
void OMPngFile::writeIntoBuffer(std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &current)
{
    int x = 0;
    for (auto ch = current.begin(); ch != current.end();)
    {
        switch (head.type)
        {
        case RGBA:
            if (head.bitDepth == 8)
            {
                dataBuffer.push_back(*ch); // R value
                ++ch;
                dataBuffer.push_back(*ch); // G value
                ++ch;
                dataBuffer.push_back(*ch); // B value
                ++ch;
                dataBuffer.push_back(*ch); // A value
                ++ch;
            }
            else
            {
                dataBuffer.push_back(*ch); // R value
                ++ch;
                ++ch;
                dataBuffer.push_back(*ch); // G value
                ++ch;
                ++ch;
                dataBuffer.push_back(*ch); // B value
                ++ch;
                ++ch;
                dataBuffer.push_back(*ch); // A value
                ++ch;
                ++ch;
            }
            break;
        case RGBTriple:
            if (head.bitDepth == 8)
            {
                dataBuffer.push_back(*ch); // R value
                ++ch;
                dataBuffer.push_back(*ch); // G value
                ++ch;
                dataBuffer.push_back(*ch); // B value
                ++ch;
                dataBuffer.push_back(0xff); // A value
            }
            else
            {
                dataBuffer.push_back(*ch); // R value
                ++ch;
                ++ch;
                dataBuffer.push_back(*ch); // G value
                ++ch;
                ++ch;
                dataBuffer.push_back(*ch); // B value
                ++ch;
                ++ch;
                dataBuffer.push_back(0xff); // A value
            }
            break;
        case GrayscaleAlpha:
            if (head.bitDepth == 8)
            {
                dataBuffer.push_back(*ch);
                dataBuffer.push_back(*ch);
                dataBuffer.push_back(*ch);
                ++ch;
                dataBuffer.push_back(*ch);
                ++ch;
            }
            else
            {
                dataBuffer.push_back(*ch);
                dataBuffer.push_back(*ch);
                dataBuffer.push_back(*ch);
                ++ch;
                ++ch;
                dataBuffer.push_back(*ch);
                ++ch;
                ++ch;
            }
            break;
        case Palette:
            if (head.bitDepth == 8)
            {
                int pal = palette[*ch];
                ++ch;
                dataBuffer.push_back((pal >> 24) & 0xff);
                dataBuffer.push_back((pal >> 16) & 0xff);
                dataBuffer.push_back((pal >> 8) & 0xff);
                dataBuffer.push_back(pal & 0xff);
            }
            else
            {
                int pixelsPerByte = 8 / head.bitDepth;
                int byteIdx = x / pixelsPerByte;
                int shift = (pixelsPerByte - 1 - (x % pixelsPerByte)) * head.bitDepth;

                uint8_t byte = *ch;
                uint8_t value = (byte >> shift) & ((1 << head.bitDepth) - 1);
                int pal = palette[value];

                dataBuffer.push_back((pal >> 24) & 0xff);
                dataBuffer.push_back((pal >> 16) & 0xff);
                dataBuffer.push_back((pal >> 8) & 0xff);
                dataBuffer.push_back(pal & 0xff);

                if (shift == 0)
                {
                    ++ch;
                }
            }
            break;
        case Grayscale:
            if (head.bitDepth == 16)
            {
                dataBuffer.push_back(*ch);  // R value
                dataBuffer.push_back(*ch);  // G value
                dataBuffer.push_back(*ch);  // B value
                dataBuffer.push_back(0xff); // A value
                ++ch;
                ++ch;
            }
            else if (head.bitDepth == 8)
            {
                dataBuffer.push_back(*ch);  // R value
                dataBuffer.push_back(*ch);  // G value
                dataBuffer.push_back(*ch);  // B value
                dataBuffer.push_back(0xff); // A value
                ++ch;
            }
            else
            {
                int pixelsPerByte = 8 / head.bitDepth;
                int byteIdx = x / pixelsPerByte;
                int shift = (pixelsPerByte - 1 - (x % pixelsPerByte)) * head.bitDepth;

                uint8_t byte = *ch;
                uint8_t value = (byte >> shift) & ((1 << head.bitDepth) - 1);
                value = static_cast<uint8_t>((value * 255) / ((1 << head.bitDepth) - 1));

                dataBuffer.push_back(value);
                dataBuffer.push_back(value);
                dataBuffer.push_back(value);
                dataBuffer.push_back(0xff);

                if (shift == 0)
                {
                    ++ch;
                }
            }
            break;
        default:
            break;
        }

        x++;
    }
}
void OMPngFile::defilter(int type, std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &raw, int y)
{
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> current;

    int bpx = getBytesPerPixel();
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

    writeIntoBuffer(current);
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
