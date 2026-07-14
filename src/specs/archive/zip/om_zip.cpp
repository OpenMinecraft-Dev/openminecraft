#include "openminecraft/specs/zip/om_zip.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include <array>
#include <cstdint>
#include <initializer_list>
#include <ios>
#include <iostream>
#include <istream>
#include <memory>
#include <stdexcept>

namespace openminecraft::specs::zip
{
OMZip::OMZip() = default;
OMZip::~OMZip() = default;

auto findHeader(std::initializer_list<char> header, std::shared_ptr<std::istream> istr) -> uint64_t
{
    char temp = 0x00;
header:
    while (temp != header.begin()[0])
    {
        istr->read(&temp, 1);
    }
    for (int i = 1; i < header.size(); ++i)
    {
        istr->read(&temp, 1);
        if (temp != header.begin()[i])
            goto header;
    }

    istr->seekg(-header.size(), std::ios::cur);

    return istr->tellg();
}

void OMZip::parseCentralDirectory(std::shared_ptr<std::istream> istr, OMZipCentralDirectoryWrap &d)
{
    istr->read(reinterpret_cast<char *>(&d.data), sizeof(d.data));
    d.data.versionMade = binary::le16ToNative(d.data.versionMade);
    d.data.versionExtract = binary::le16ToNative(d.data.versionExtract);
    d.data.flags = binary::le16ToNative(d.data.flags);
    d.data.compressionMethod = static_cast<OMZipCompressionMethod>(binary::le16ToNative(d.data.compressionMethod));
    d.data.lastModifyTime = binary::le16ToNative(d.data.lastModifyTime);
    d.data.lastModifyDate = binary::le16ToNative(d.data.lastModifyDate);
    d.data.crc32 = binary::le32ToNative(d.data.crc32);
    d.data.compressedSize = binary::le32ToNative(d.data.compressedSize);
    d.data.uncompressedSize = binary::le32ToNative(d.data.uncompressedSize);
    d.data.fileNameLength = binary::le16ToNative(d.data.fileNameLength);
    d.data.extraFieldLength = binary::le16ToNative(d.data.extraFieldLength);
    d.data.fileCommentLength = binary::le16ToNative(d.data.fileCommentLength);
    d.data.diskNumber = binary::le16ToNative(d.data.diskNumber);
    d.data.internalFileAttributes = binary::le16ToNative(d.data.internalFileAttributes);
    d.data.externalFileAttributes = binary::le32ToNative(d.data.externalFileAttributes);
    d.data.localHeaderOffset = binary::le32ToNative(d.data.localHeaderOffset);

    d.name.resize(d.data.fileNameLength);
    istr->read(const_cast<char *>(d.name.data()), d.data.fileNameLength);
    istr->seekg(d.data.extraFieldLength, std::ios::cur);

    auto c = istr->tellg();
    istr->seekg(d.data.localHeaderOffset, std::ios::beg);

    auto &hd = d.file;
    istr->read(reinterpret_cast<char *>(&hd), sizeof(hd));
    hd.version = binary::le16ToNative(hd.version);
    hd.flags = binary::le16ToNative(hd.flags);
    hd.compressionMethod = static_cast<OMZipCompressionMethod>(binary::le16ToNative(hd.compressionMethod));
    hd.lastModifyTime = binary::le16ToNative(hd.lastModifyTime);
    hd.lastModifyDate = binary::le16ToNative(hd.lastModifyDate);
    hd.crc32 = binary::le32ToNative(hd.crc32);
    hd.compressedSize = binary::le32ToNative(hd.compressedSize);
    hd.uncompressedSize = binary::le32ToNative(hd.uncompressedSize);
    hd.fileNameLength = binary::le16ToNative(hd.fileNameLength);
    hd.extraFieldLength = binary::le16ToNative(hd.extraFieldLength);
    istr->seekg(hd.fileNameLength + hd.extraFieldLength, std::ios::cur);
    d.offset = static_cast<uint64_t>(istr->tellg());
    istr->seekg(c, std::ios::beg);
}

void OMZip::parse(std::shared_ptr<std::istream> istr)
{
    istr->seekg(-65536, std::ios::end);

    auto eocd = findHeader(eocdHeader, istr);
    istr->read(reinterpret_cast<char *>(&centralDir), sizeof(centralDir));
    centralDir.diskNumber = binary::le16ToNative(centralDir.diskNumber);
    centralDir.centralDirectoryDiskNumber = binary::le16ToNative(centralDir.centralDirectoryDiskNumber);
    centralDir.entries = binary::le16ToNative(centralDir.entries);
    centralDir.totalEntries = binary::le16ToNative(centralDir.totalEntries);
    centralDir.centralDirectorySize = binary::le32ToNative(centralDir.centralDirectorySize);
    centralDir.centralDirectoryOffset = binary::le32ToNative(centralDir.centralDirectoryOffset);
    centralDir.commentLength = binary::le16ToNative(centralDir.commentLength);
    centralDirComment.resize(centralDir.commentLength);
    istr->read(&centralDirComment[0], centralDir.commentLength);

    if (centralDir.centralDirectoryOffset == 0xffffffff || centralDir.totalEntries == 0xffff)
    {
        throw std::logic_error("zip64 file not supported!");
    }

    istr->seekg(centralDir.centralDirectoryOffset, std::ios::beg);

    entries.resize(centralDir.entries);
    for (int i = 0; i < centralDir.entries; ++i)
    {
        parseCentralDirectory(istr, entries[i]);
        std::cout << entries[i].name << std::endl;
    }
}

auto OMZip::findFile(std::string &s) -> OMZipCentralDirectoryWrap *
{
    for (auto &e : entries)
    {
        if (e.name == s)
        {
            return &e;
        }
    }

    return nullptr;
}

auto OMZip::read() -> std::shared_ptr<std::ostream>
{
    return nullptr;
}
} // namespace openminecraft::specs::zip
