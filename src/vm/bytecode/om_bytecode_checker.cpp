#include "openminecraft/vm/bytecode/om_bytecode_checker.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/om_class_file.hpp"
#include <memory>

namespace openminecraft::vm::bytecode
{
OMBytecodeChecker::OMBytecodeChecker(std::shared_ptr<classfile::OMClassFile> cls) : cls(cls)
{
    logger = std::make_unique<log::OMLogger>("OMBytecodeChecker", this);
}

void OMBytecodeChecker::check()
{
    for (auto m : cls->methods)
    {
        logger->info("function: {}", cls->mapping[m->nameIndex]->to<classfile::OMClassConstantUtf8>()->data);
        if (m->attrs.empty())
        {
            continue;
        }
        auto attr = m->attrs[0]->to<classfile::OMClassAttrCode>();
        auto codeRaw = attr->code.data();
        auto bytes = 0;
        while (bytes < attr->codeLength)
        {
            switch (codeRaw[bytes])
            {
            // nop
            case op_nop: {
                logger->info("nop");
                break;
            }
            // aconst_null
            case op_aconst_null: {
                logger->info("aconst_null");
                break;
            }
            // const operands
            case op_iconst_i(-1):
            case op_iconst_i(0):
            case op_iconst_i(1):
            case op_iconst_i(2):
            case op_iconst_i(3):
            case op_iconst_i(4):
            case op_iconst_i(5):
                logger->info("iconst_{}", codeRaw[bytes] - 0x3);
                break;
            case op_lconst_l(0):
            case op_lconst_l(1):
                logger->info("lconst_{}", codeRaw[bytes] - 0x9);
                break;
            case op_fconst_f(0):
            case op_fconst_f(1):
            case op_fconst_f(2):
                logger->info("fconst_{}", codeRaw[bytes] - 0xb);
                break;
            case op_dconst_d(0):
            case op_dconst_d(1):
                logger->info("dconst_{}", codeRaw[bytes] - 0xe);
                break;
            // getstatic (index:u16)
            case op_getstatic: {
                logger->info("getstatic #{}", binary::be16ToNative(*(uint16_t *)(codeRaw + bytes + 1)));
                bytes += 2;
                break;
            }
            // bipush (value:u8)
            case op_bipush: {
                logger->info("bipush {0:#x}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // sipush (value:u16)
            case op_sipush: {
                logger->info("sipush #{}", binary::be16ToNative(*(uint16_t *)(codeRaw + bytes + 1)));
                bytes += 2;
                break;
            }
            // ldc (index:u8)
            case op_ldc: {
                logger->info("ldc #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // ldc_w (index:u16)
            case op_ldc_w: {
                logger->info("ldc_w #{}", binary::be16ToNative(*(uint16_t *)(codeRaw + bytes + 1)));
                bytes += 2;
                break;
            }
            // ldc2_w (index:u16)
            case op_ldc2_w: {
                logger->info("ldc2_w #{}", binary::be16ToNative(*(uint16_t *)(codeRaw + bytes + 1)));
                bytes += 2;
                break;
            }
            // invokevirtual (index:u16)
            case op_invokevirtual: {
                logger->info("invokevirtual #{}", binary::be16ToNative(*(uint16_t *)(codeRaw + bytes + 1)));
                bytes += 2;
                break;
            }
            // invokestatic (index:u16)
            case op_invokestatic: {
                logger->info("invokestatic #{}", binary::be16ToNative(*(uint16_t *)(codeRaw + bytes + 1)));
                bytes += 2;
                break;
            }
            // return
            case op_return: {
                logger->info("return");
                break;
            }
            default: {
                logger->info("unknown operand");
                goto checkend;
            }
            }
            bytes++;
        }
    checkend:
        continue;
    }
}
} // namespace openminecraft::vm::bytecode