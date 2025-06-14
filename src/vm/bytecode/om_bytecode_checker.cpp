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
        if (m->nameIndex != 0)
        {
            logger->info("function: {}", cls->mapping[m->nameIndex]->to<classfile::OMClassConstantUtf8>()->data);
        }
        else
        {
            logger->info("function: <unnamed>");
        }

        if (m->attrs.empty())
        {
            continue;
        }

        auto attr = m->attrs[0]->to<classfile::OMClassAttrCode>();
        auto codeRaw = attr->code.data();
        auto bytes = 0;

#define simpleCommand(operand, msg)                                                                                    \
    case operand:                                                                                                      \
        logger->info(msg);                                                                                             \
        break

#define tbyteCommand(operand, msg)                                                                                     \
    case operand:                                                                                                      \
        logger->info(msg, binary::be16ToNative(*(uint16_t *)(codeRaw + bytes + 1)));                                   \
        bytes += 2;                                                                                                    \
        break

        while (bytes < attr->codeLength)
        {
            switch (codeRaw[bytes])
            {
                simpleCommand(op_nop, "nop");
                simpleCommand(op_aconst_null, "aconst_null");
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
            // bipush (value:u8)
            case op_bipush: {
                logger->info("bipush {0:#x}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
                tbyteCommand(op_sipush, "sipush #{}");
            // ldc (index:u8)
            case op_ldc: {
                logger->info("ldc #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
                tbyteCommand(op_ldc_w, "ldc_w #{}");
                tbyteCommand(op_ldc2_w, "ldc_w #{}");
            // iload (index:u8)
            case op_iload: {
                logger->info("iload #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // lload (index:u8)
            case op_lload: {
                logger->info("lload #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // fload (index:u8)
            case op_fload: {
                logger->info("fload #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // dload (index:u8)
            case op_dload: {
                logger->info("dload #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // aload (index:u8)
            case op_aload: {
                logger->info("aload #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // iload_<n>
            case op_iload_n(0):
            case op_iload_n(1):
            case op_iload_n(2):
            case op_iload_n(3):
                logger->info("aload_{}", (int)(codeRaw[bytes] - op_iload_n(0)));
                break;

            // lload_<n>
            case op_lload_n(0):
            case op_lload_n(1):
            case op_lload_n(2):
            case op_lload_n(3):
                logger->info("aload_{}", (int)(codeRaw[bytes] - op_lload_n(0)));
                break;
            // fload_<n>
            case op_fload_n(0):
            case op_fload_n(1):
            case op_fload_n(2):
            case op_fload_n(3):
                logger->info("aload_{}", (int)(codeRaw[bytes] - op_fload_n(0)));
                break;
            // dload_<n>
            case op_dload_n(0):
            case op_dload_n(1):
            case op_dload_n(2):
            case op_dload_n(3):
                logger->info("aload_{}", (int)(codeRaw[bytes] - op_dload_n(0)));
                break;
            // aload_<n>
            case op_aload_n(0):
            case op_aload_n(1):
            case op_aload_n(2):
            case op_aload_n(3):
                logger->info("aload_{}", (int)(codeRaw[bytes] - op_aload_n(0)));
                break;

                simpleCommand(op_iaload, "iaload");
                simpleCommand(op_laload, "laload");
                simpleCommand(op_faload, "faload");
                simpleCommand(op_daload, "daload");
                simpleCommand(op_aaload, "aaload");
                simpleCommand(op_baload, "baload");
                simpleCommand(op_caload, "caload");
                simpleCommand(op_saload, "saload");

            // istore (index:u8)
            case op_istore: {
                logger->info("istore #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // lstore (index:u8)
            case op_lstore: {
                logger->info("lstore #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // fstore (index:u8)
            case op_fstore: {
                logger->info("fstore #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // dstore (index:u8)
            case op_dstore: {
                logger->info("dstore #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // astore (index:u8)
            case op_astore: {
                logger->info("astore #{}", codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            case op_istore_n(0):
            case op_istore_n(1):
            case op_istore_n(2):
            case op_istore_n(3):
                logger->info("istore_{}", (int)(codeRaw[bytes] - op_istore_n(0)));
                break;
            case op_lstore_n(0):
            case op_lstore_n(1):
            case op_lstore_n(2):
            case op_lstore_n(3):
                logger->info("lstore_{}", (int)(codeRaw[bytes] - op_lstore_n(0)));
                break;
            case op_fstore_n(0):
            case op_fstore_n(1):
            case op_fstore_n(2):
            case op_fstore_n(3):
                logger->info("fstore_{}", (int)(codeRaw[bytes] - op_fstore_n(0)));
                break;
            case op_dstore_n(0):
            case op_dstore_n(1):
            case op_dstore_n(2):
            case op_dstore_n(3):
                logger->info("lstore_{}", (int)(codeRaw[bytes] - op_dstore_n(0)));
                break;
            case op_astore_n(0):
            case op_astore_n(1):
            case op_astore_n(2):
            case op_astore_n(3):
                logger->info("lstore_{}", (int)(codeRaw[bytes] - op_astore_n(0)));
                break;

                simpleCommand(op_iastore, "iastore");
                simpleCommand(op_lastore, "lastore");
                simpleCommand(op_fastore, "fastore");
                simpleCommand(op_dastore, "dastore");
                simpleCommand(op_aastore, "aastore");
                simpleCommand(op_bastore, "bastore");
                simpleCommand(op_castore, "castore");
                simpleCommand(op_sastore, "sastore");
                simpleCommand(op_pop, "pop");
                simpleCommand(op_pop2, "pop2");
                simpleCommand(op_dup, "dup");
                simpleCommand(op_dup_x1, "dup_x1");
                simpleCommand(op_dup_x2, "dup_x2");
                simpleCommand(op_dup2, "dup2");
                simpleCommand(op_dup2_x1, "dup2_x1");
                simpleCommand(op_dup2_x2, "dup2_x2");
                simpleCommand(op_swap, "swap");
                simpleCommand(op_iadd, "iadd");
                simpleCommand(op_ladd, "ladd");
                simpleCommand(op_fadd, "fadd");
                simpleCommand(op_dadd, "dadd");
                simpleCommand(op_isub, "isub");
                simpleCommand(op_lsub, "lsub");
                simpleCommand(op_fsub, "fsub");
                simpleCommand(op_dsub, "dsub");
                simpleCommand(op_imul, "imul");
                simpleCommand(op_lmul, "lmul");
                simpleCommand(op_fmul, "fmul");
                simpleCommand(op_dmul, "dmul");
                simpleCommand(op_idiv, "idiv");
                simpleCommand(op_ldiv, "ldiv");
                simpleCommand(op_fdiv, "fdiv");
                simpleCommand(op_ddiv, "ddiv");
                simpleCommand(op_irem, "irem");
                simpleCommand(op_lrem, "lrem");
                simpleCommand(op_frem, "frem");
                simpleCommand(op_drem, "drem");
                simpleCommand(op_ineg, "ineg");
                simpleCommand(op_lneg, "lneg");
                simpleCommand(op_fneg, "fneg");
                simpleCommand(op_dneg, "dneg");
                simpleCommand(op_ishr, "ishr");
                simpleCommand(op_ishl, "ishl");
                simpleCommand(op_lshr, "lshr");
                simpleCommand(op_lshl, "lshl");
                simpleCommand(op_iushr, "iushr");
                simpleCommand(op_lushr, "lushr");
                simpleCommand(op_iand, "iand");
                simpleCommand(op_land, "land");
                simpleCommand(op_ior, "ior");
                simpleCommand(op_lor, "lor");
                simpleCommand(op_ixor, "ixor");
                simpleCommand(op_lxor, "lxor");

            // iinc (index:u8, const:u8)
            case op_iinc: {
                logger->info("iinc {} {}", codeRaw[bytes + 1], codeRaw[bytes + 2]);
                bytes += 2;
                break;
            }

                simpleCommand(op_i2l, "i2l");
                simpleCommand(op_i2f, "i2f");
                simpleCommand(op_i2d, "i2d");
                simpleCommand(op_l2i, "l2i");
                simpleCommand(op_l2f, "l2f");
                simpleCommand(op_l2d, "l2d");
                simpleCommand(op_f2i, "f2i");
                simpleCommand(op_f2l, "f2l");
                simpleCommand(op_f2d, "f2d");
                simpleCommand(op_d2i, "d2i");
                simpleCommand(op_d2l, "d2l");
                simpleCommand(op_d2f, "d2f");
                simpleCommand(op_i2b, "i2b");
                simpleCommand(op_i2c, "i2c");
                simpleCommand(op_i2s, "i2s");
                simpleCommand(op_fcmpl, "fcmpl");
                simpleCommand(op_fcmpg, "fcmpg");
                simpleCommand(op_dcmpl, "dcmpl");
                simpleCommand(op_dcmpg, "dcmpg");
                tbyteCommand(op_ifeq, "ifeq {}");
                tbyteCommand(op_ifne, "ifne {}");
                tbyteCommand(op_iflt, "iflt {}");
                tbyteCommand(op_ifge, "ifge {}");
                tbyteCommand(op_ifgt, "ifgt {}");
                tbyteCommand(op_ifle, "ifle {}");
                tbyteCommand(op_if_icmpeq, "if_icmpeq {}");
                tbyteCommand(op_if_icmpne, "if_icmpne {}");
                tbyteCommand(op_if_icmplt, "if_icmplt {}");
                tbyteCommand(op_if_icmpge, "if_icmpge {}");
                tbyteCommand(op_if_icmpgt, "if_icmpgt {}");
                tbyteCommand(op_if_icmple, "if_icmple {}");
                tbyteCommand(op_if_acmpeq, "if_acmpeq {}");
                tbyteCommand(op_if_acmpne, "if_acmpne {}");
                tbyteCommand(op_goto, "if_goto {}");
                tbyteCommand(op_jsr, "if_jsr {}");
            // ret (index:u8)
            case op_ret: {
                logger->info("ret {}", (int)codeRaw[bytes + 1]);
                bytes++;
                break;
            }
            // tableswitch (padding:variable, default:u32, low:u32, high:u32, offsets:u32[high-low+1])
            case op_tableswitch: {
                bytes++;
                while (bytes % 4 != 0)
                {
                    bytes++;
                }
                int def = (codeRaw[bytes] << 24) | (codeRaw[bytes + 1] << 16) | (codeRaw[bytes + 2] << 8) |
                          (codeRaw[bytes + 3]);
                int low = (codeRaw[bytes + 4] << 24) | (codeRaw[bytes + 5] << 16) | (codeRaw[bytes + 6] << 8) |
                          (codeRaw[bytes + 7]);
                int high = (codeRaw[bytes + 8] << 24) | (codeRaw[bytes + 9] << 16) | (codeRaw[bytes + 10] << 8) |
                           (codeRaw[bytes + 11]);
                int cont = high - low + 1;
                logger->info("tableswitch {} {} {} ...", def, low, high);
                bytes += 12;
                bytes += cont * 4;
                break;
            }
            // tableswitch (padding:variable, default:u32, npairs:u32, pairs:u32[npairs])
            case op_lookupswitch: {
                bytes++;
                while (bytes % 4 != 0)
                {
                    bytes++;
                }
                int def = (codeRaw[bytes] << 24) | (codeRaw[bytes + 1] << 16) | (codeRaw[bytes + 2] << 8) |
                          (codeRaw[bytes + 3]);
                int npairs = (codeRaw[bytes + 4] << 24) | (codeRaw[bytes + 5] << 16) | (codeRaw[bytes + 6] << 8) |
                             (codeRaw[bytes + 7]);
                logger->info("lookupswitch {} {} ...", def, npairs);
                bytes += 8;
                bytes += npairs * 4;
                break;
            }
                simpleCommand(op_ireturn, "ireturn");
                simpleCommand(op_lreturn, "lreturn");
                simpleCommand(op_freturn, "freturn");
                simpleCommand(op_dreturn, "dreturn");
                simpleCommand(op_areturn, "areturn");
                simpleCommand(op_return, "return");
                tbyteCommand(op_getstatic, "getstatic #{}");
                tbyteCommand(op_putstatic, "putstatic #{}");
                tbyteCommand(op_getfield, "getfield #{}");
                tbyteCommand(op_putfield, "putfield #{}");
                tbyteCommand(op_invokevirtual, "invokevirtual #{}");
                tbyteCommand(op_invokespecial, "invokespecial #{}");
                tbyteCommand(op_invokestatic, "invokestatic #{}");
            case op_invokeinterface: {
                logger->info("invokeinterface #{} {}", binary::be16ToNative(*(uint16_t *)(codeRaw + bytes + 1)),
                             (int)codeRaw[bytes + 3]);
                bytes += 4;
                break;
            }
            case op_invokedynamic: {
                logger->info("invokedynamic #{} {}", binary::be16ToNative(*(uint16_t *)(codeRaw + bytes + 1)),
                             (int)codeRaw[bytes + 3]);
                bytes += 4;
                break;
            }
            default: {
                logger->info("unknown operand");
                goto checkend;
            }
            }
            bytes++;
        }

        /*if (cls->mapping[m->nameIndex]->to<classfile::OMClassConstantUtf8>()->data == "main")
        {
            break;
        }*/
    checkend:
        continue;
    }
}
} // namespace openminecraft::vm::bytecode
