#ifndef OM_BIN_ENDIANS_HPP
#define OM_BIN_ENDIANS_HPP

#include <cstdint>

namespace openminecraft::binary
{
#define be16(data) data = be16ToNative(data);
#define be32(data) data = be32ToNative(data);
#define be64(data) data = be64ToNative(data);
#define bef(data) data = befToNative(data);
#define bed(data) data = bedToNative(data);

#define readbe16(target)                                                                                               \
    read((char *)&target, sizeof(uint16_t));                                                                           \
    be16(target)
#define readbe32(target)                                                                                               \
    read((char *)&target, sizeof(uint32_t));                                                                           \
    be32(target)
#define readbe64(target)                                                                                               \
    read((char *)&target, sizeof(uint64_t));                                                                           \
    be64(target)
#define readbef(target)                                                                                                \
    read((char *)&target, sizeof(float));                                                                              \
    bef(target)
#define readbed(target)                                                                                                \
    read((char *)&target, sizeof(double));                                                                             \
    bed(target)

uint16_t be16ToNative(uint16_t data);
uint32_t be32ToNative(uint32_t data);
uint64_t be64ToNative(uint64_t data);
uint16_t le16ToNative(uint16_t data);
uint32_t le32ToNative(uint32_t data);
uint64_t le64ToNative(uint64_t data);
float befToNative(float data);
double bedToNative(double data);

int16_t be16SignedToNative(uint8_t d1, uint8_t d2);
int32_t be32SignedToNative(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4);

bool checkNativeLe();
} // namespace openminecraft::binary

#endif
