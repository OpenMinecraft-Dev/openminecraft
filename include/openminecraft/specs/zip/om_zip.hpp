#ifndef OM_ZIP_HPP
#define OM_ZIP_HPP

#include <array>
#include <istream>
#include <memory>
#include <cstdint>
#include <chrono>
#include <vector>
namespace openminecraft::specs::zip
{
inline constexpr const char allocatorTag[] = "parser_zip";

constexpr auto centralDirHeader = {'P', 'K', '\x01', '\x02'};
constexpr auto eocdHeader = {'P', 'K', '\x05', '\x06'};

using DosDate = uint16_t;
using DosTime = uint16_t;

static inline std::tm ParseDosDateTime(DosTime dosTime, DosDate dosDate)
{
    std::tm t = {};
    t.tm_sec = (dosTime & 0x1F) * 2;
    t.tm_min = (dosTime >> 5) & 0x3F;
    t.tm_hour = (dosTime >> 11) & 0x1F;
    t.tm_mday = dosDate & 0x1F;
    t.tm_mon = ((dosDate >> 5) & 0x0F) - 1;
    t.tm_year = ((dosDate >> 9) & 0x7F) + 1980 - 1900;
    t.tm_isdst = -1;
    return t;
}

static inline std::chrono::system_clock::time_point DosToTimePoint(DosTime dosTime, DosDate dosDate)
{
    std::tm t = ParseDosDateTime(dosTime, dosDate);
    std::time_t tt = std::mktime(&t); // 将本地时间转换为 time_t
    return std::chrono::system_clock::from_time_t(tt);
}

#pragma pack(1)
struct OMZipEndOfCentralDirectory
{
    std::array<char, 4> header;
    uint16_t diskNumber, centralDirectoryDiskNumber;
    uint16_t entries, totalEntries;
    uint32_t centralDirectorySize, centralDirectoryOffset;
    uint16_t commentLength;
};

struct OMZipCentralDirectory
{
    std::array<char, 4> header;
    uint16_t versionMade, versionExtract;
    uint16_t flags;
    uint16_t compressionMethod;
    DosTime lastModifyTime;
    DosDate lastModifyDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t fileNameLength;
    uint16_t extraFieldLength;
    uint16_t fileCommentLength;
    uint16_t diskNumber;
    uint16_t internalFileAttributes;
    uint32_t externalFileAttributes;
    uint32_t localHeaderOffset;
};

struct OMZipCentralDirectoryWrap
{
    OMZipCentralDirectory data;
    std::string name;
};
#pragma pack()

class OMZip
{
  private:
    OMZipEndOfCentralDirectory centralDir;
    std::string centralDirComment;

  public:
    OMZip();
    ~OMZip();

    void parse(std::shared_ptr<std::istream> istr);
    void parseCentralDirectory(std::shared_ptr<std::istream> istr, OMZipCentralDirectoryWrap &d);
};
} // namespace openminecraft::specs::zip

#endif
