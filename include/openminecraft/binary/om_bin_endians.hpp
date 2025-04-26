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
float befToNative(float data);
double bedToNative(double data);

bool checkNativeLe();
} // namespace openminecraft::binary

#endif