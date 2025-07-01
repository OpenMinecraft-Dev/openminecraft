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
OMResult<std::string, std::string> decodeType(std::string raw, int *p);
typedef std::pair<std::vector<std::string>, std::string> methodSig;
OMResult<methodSig, std::string> decodeSignature(std::string raw, int *p);
} // namespace openminecraft::vm::bytecode::descriptor

#endif
