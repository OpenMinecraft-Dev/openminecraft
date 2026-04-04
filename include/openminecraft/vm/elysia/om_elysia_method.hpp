#ifndef OM_ELYSIA_METHOD
#define OM_ELYSIA_METHOD

#include <cstdint>

namespace openminecraft::vm::elysia {
struct OMElysiaMethod {
    char *name;
    char *descriptor;
    uint32_t accessFlag;
    uint32_t codeLength;
    uint8_t *code;
};
}

#endif
