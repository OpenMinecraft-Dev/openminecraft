#ifndef OM_PIXELTOWER_TRACING_HPP
#define OM_PIXELTOWER_TRACING_HPP
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include <map>
#include <string>
namespace openminecraft::vm::pixeltower::v1::tracing
{
extern std::map<std::string, void *> registers;
void installHandler();

struct OMTracingFrame
{
    void *location;
    std::string name;
};

enum OMFrameType
{
    JavaFrame,
    NativeFrame,
    JavaJITFrame
};

struct OMParsedFrame
{
    OMFrameType type;
    // geopelia: for Java frames, it points to the metaspace bytecode area
    //           for Native / JavaJIT frames, it points to the machine code area (executable pages)
    void *target;
    union {
        struct
        {
            char name[256];
        } native;

        struct
        {
            v0::OMMethod *method;
            int offset;
        } jvm;
    };

    OMParsedFrame *next;
};
OMParsedFrame *fetchFrames();
OMParsedFrame *fetchFrames(std::vector<v1::tracing::OMTracingFrame> &frames);
} // namespace openminecraft::vm::pixeltower::v1::tracing

#endif