#ifndef OM_BYTECODE_DESCRIPTOR_HPP
#define OM_BYTECODE_DESCRIPTOR_HPP

#include "openminecraft/util/om_util_result.hpp"
#include <any>
#include <string>
#include <utility>
#include <vector>

using namespace openminecraft::util;

namespace openminecraft::vm::bytecode::descriptor
{
struct TempNonPrimitiveVariable
{
    std::string type;
};
OMResult<std::string, std::string> decodeType(std::string raw, int *p);
typedef std::pair<std::vector<std::string>, std::string> methodSig;
OMResult<methodSig, std::string> decodeSignature(std::string raw, int *p);
OMResult<std::any, std::string> checkArgCompat(std::any data, std::string type);
} // namespace openminecraft::vm::bytecode::descriptor

#endif