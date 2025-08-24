#ifndef OM_BYTECODES_HPP
#define OM_BYTECODES_HPP

#include <cstdint>
constexpr uint8_t op_nop = 0x0;
constexpr uint8_t op_aconst_null = 0x1;
constexpr uint8_t op_iconst_i(int i)
{
    return 0x3 + i;
}
constexpr uint8_t op_lconst_l(int64_t i)
{
    return 0x9 + i;
}
constexpr uint8_t op_fconst_f(float i)
{
    return 0xb + static_cast<uint8_t>(i);
}
constexpr uint8_t op_dconst_d(double i)
{
    return 0xe + static_cast<uint8_t>(i);
}
constexpr uint8_t op_bipush = 0x10;
constexpr uint8_t op_sipush = 0x11;
constexpr uint8_t op_ldc = 0x12;
constexpr uint8_t op_ldc_w = 0x13;
constexpr uint8_t op_ldc2_w = 0x14;
constexpr uint8_t op_iload = 0x15;
constexpr uint8_t op_lload = 0x16;
constexpr uint8_t op_fload = 0x17;
constexpr uint8_t op_dload = 0x18;
constexpr uint8_t op_aload = 0x19;
constexpr uint8_t op_iload_n(int n)
{
    return 0x1a + n;
}
constexpr uint8_t op_lload_n(int n)
{
    return 0x1e + n;
}
constexpr uint8_t op_fload_n(int n)
{
    return 0x22 + n;
}
constexpr uint8_t op_dload_n(int n)
{
    return 0x26 + n;
}
constexpr uint8_t op_aload_n(int n)
{
    return 0x2a + n;
}
constexpr uint8_t op_iaload = 0x2e;
constexpr uint8_t op_laload = 0x2f;
constexpr uint8_t op_faload = 0x30;
constexpr uint8_t op_daload = 0x31;
constexpr uint8_t op_aaload = 0x32;
constexpr uint8_t op_baload = 0x33;
constexpr uint8_t op_caload = 0x34;
constexpr uint8_t op_saload = 0x35;
constexpr uint8_t op_istore = 0x36;
constexpr uint8_t op_lstore = 0x37;
constexpr uint8_t op_fstore = 0x38;
constexpr uint8_t op_dstore = 0x39;
constexpr uint8_t op_astore = 0x3a;
constexpr uint8_t op_istore_n(int n)
{
    return 0x3b + n;
}
constexpr uint8_t op_lstore_n(int n)
{
    return 0x3f + n;
}
constexpr uint8_t op_fstore_n(int n)
{
    return 0x43 + n;
}
constexpr uint8_t op_dstore_n(int n)
{
    return 0x47 + n;
}
constexpr uint8_t op_astore_n(int n)
{
    return 0x4b + n;
}
constexpr uint8_t op_iastore = 0x4f;
constexpr uint8_t op_lastore = 0x50;
constexpr uint8_t op_fastore = 0x51;
constexpr uint8_t op_dastore = 0x52;
constexpr uint8_t op_aastore = 0x53;
constexpr uint8_t op_bastore = 0x54;
constexpr uint8_t op_castore = 0x55;
constexpr uint8_t op_sastore = 0x56;
constexpr uint8_t op_pop = 0x57;
constexpr uint8_t op_pop2 = 0x58;
constexpr uint8_t op_dup = 0x59;
constexpr uint8_t op_dup_x1 = 0x5a;
constexpr uint8_t op_dup_x2 = 0x5b;
constexpr uint8_t op_dup2 = 0x5c;
constexpr uint8_t op_dup2_x1 = 0x5d;
constexpr uint8_t op_dup2_x2 = 0x5e;
constexpr uint8_t op_swap = 0x5f;
constexpr uint8_t op_iadd = 0x60;
constexpr uint8_t op_ladd = 0x61;
constexpr uint8_t op_fadd = 0x62;
constexpr uint8_t op_dadd = 0x63;
constexpr uint8_t op_isub = 0x64;
constexpr uint8_t op_lsub = 0x65;
constexpr uint8_t op_fsub = 0x66;
constexpr uint8_t op_dsub = 0x67;
constexpr uint8_t op_imul = 0x68;
constexpr uint8_t op_lmul = 0x69;
constexpr uint8_t op_fmul = 0x6a;
constexpr uint8_t op_dmul = 0x6b;
constexpr uint8_t op_idiv = 0x6c;
constexpr uint8_t op_ldiv = 0x6d;
constexpr uint8_t op_fdiv = 0x6e;
constexpr uint8_t op_ddiv = 0x6f;
constexpr uint8_t op_irem = 0x70;
constexpr uint8_t op_lrem = 0x71;
constexpr uint8_t op_frem = 0x72;
constexpr uint8_t op_drem = 0x73;
constexpr uint8_t op_ineg = 0x74;
constexpr uint8_t op_lneg = 0x75;
constexpr uint8_t op_fneg = 0x76;
constexpr uint8_t op_dneg = 0x77;
constexpr uint8_t op_ishl = 0x78;
constexpr uint8_t op_lshl = 0x79;
constexpr uint8_t op_ishr = 0x7a;
constexpr uint8_t op_lshr = 0x7b;
constexpr uint8_t op_iushr = 0x7c;
constexpr uint8_t op_lushr = 0x7d;
constexpr uint8_t op_iand = 0x7e;
constexpr uint8_t op_land = 0x7f;
constexpr uint8_t op_ior = 0x80;
constexpr uint8_t op_lor = 0x81;
constexpr uint8_t op_ixor = 0x82;
constexpr uint8_t op_lxor = 0x83;
constexpr uint8_t op_iinc = 0x84;
constexpr uint8_t op_i2l = 0x85;
constexpr uint8_t op_i2f = 0x86;
constexpr uint8_t op_i2d = 0x87;
constexpr uint8_t op_l2i = 0x88;
constexpr uint8_t op_l2f = 0x89;
constexpr uint8_t op_l2d = 0x8a;
constexpr uint8_t op_f2i = 0x8b;
constexpr uint8_t op_f2l = 0x8c;
constexpr uint8_t op_d2i = 0x8e;
constexpr uint8_t op_f2d = 0x8d;
constexpr uint8_t op_d2l = 0x8f;
constexpr uint8_t op_d2f = 0x90;
constexpr uint8_t op_i2b = 0x91;
constexpr uint8_t op_i2c = 0x92;
constexpr uint8_t op_i2s = 0x93;
constexpr uint8_t op_lcmp = 0x94;
constexpr uint8_t op_fcmpl = 0x95;
constexpr uint8_t op_fcmpg = 0x96;
constexpr uint8_t op_dcmpl = 0x97;
constexpr uint8_t op_dcmpg = 0x98;
constexpr uint8_t op_ifeq = 0x99;
constexpr uint8_t op_ifne = 0x9a;
constexpr uint8_t op_iflt = 0x9b;
constexpr uint8_t op_ifge = 0x9c;
constexpr uint8_t op_ifgt = 0x9d;
constexpr uint8_t op_ifle = 0x9e;
constexpr uint8_t op_if_icmpeq = 0x9f;
constexpr uint8_t op_if_icmpne = 0xa0;
constexpr uint8_t op_if_icmplt = 0xa1;
constexpr uint8_t op_if_icmpge = 0xa2;
constexpr uint8_t op_if_icmpgt = 0xa3;
constexpr uint8_t op_if_icmple = 0xa4;
constexpr uint8_t op_if_acmpeq = 0xa5;
constexpr uint8_t op_if_acmpne = 0xa6;
constexpr uint8_t op_goto = 0xa7;
constexpr uint8_t op_jsr = 0xa8;
constexpr uint8_t op_ret = 0xa9;
constexpr uint8_t op_tableswitch = 0xaa;
constexpr uint8_t op_lookupswitch = 0xab;
constexpr uint8_t op_ireturn = 0xac;
constexpr uint8_t op_lreturn = 0xad;
constexpr uint8_t op_freturn = 0xae;
constexpr uint8_t op_dreturn = 0xaf;
constexpr uint8_t op_areturn = 0xb0;
constexpr uint8_t op_return = 0xb1;
constexpr uint8_t op_getstatic = 0xb2;
constexpr uint8_t op_putstatic = 0xb3;
constexpr uint8_t op_getfield = 0xb4;
constexpr uint8_t op_putfield = 0xb5;
constexpr uint8_t op_invokevirtual = 0xb6;
constexpr uint8_t op_invokespecial = 0xb7;
constexpr uint8_t op_invokestatic = 0xb8;
constexpr uint8_t op_invokeinterface = 0xb9;
constexpr uint8_t op_invokedynamic = 0xba;
constexpr uint8_t op_new = 0xbb;
constexpr uint8_t op_newarray = 0xbc;
constexpr uint8_t op_anewarray = 0xbd;
constexpr uint8_t op_arraylength = 0xbe;
constexpr uint8_t op_athrow = 0xbf;
constexpr uint8_t op_checkcast = 0xc0;
constexpr uint8_t op_instanceof = 0xc1;
constexpr uint8_t op_monitorenter = 0xc2;
constexpr uint8_t op_monitorexit = 0xc3;
constexpr uint8_t op_wide = 0xc4;
constexpr uint8_t op_multianewarray = 0xc5;
constexpr uint8_t op_ifnull = 0xc6;
constexpr uint8_t op_ifnonnull = 0xc7;
constexpr uint8_t op_goto_w = 0xc8;
constexpr uint8_t op_jsr_w = 0xc9;
// 0xca ~ 0xff reserved

#endif
