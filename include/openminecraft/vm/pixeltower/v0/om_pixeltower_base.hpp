#ifndef OM_PIXELTOWER_BASE_HPP
#define OM_PIXELTOWER_BASE_HPP

#include <cstdint>
#include <variant>

// geopelia: just some random magic number for unsatisfied native functions
#define nullFunction (void *)0x33550336

namespace openminecraft::vm::pixeltower::v0
{
typedef int jint;
typedef int64_t jlong;
typedef float jfloat;
typedef double jdouble;
typedef uint16_t jchar;
typedef bool jboolean;
typedef char jbyte;
typedef short jshort;

typedef int AccessFlags;
} // namespace openminecraft::vm::pixeltower::v0

#endif
