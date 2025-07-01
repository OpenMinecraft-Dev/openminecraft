#ifndef OM_BYTECODES_HPP
#define OM_BYTECODES_HPP

#define op_nop 0x0
#define op_aconst_null 0x1
#define op_iconst_i(i) 0x3 + i
#define op_lconst_l(l) 0x9 + l
#define op_fconst_f(f) 0xb + f
#define op_dconst_d(d) 0xe + d
#define op_bipush 0x10
#define op_sipush 0x11
#define op_ldc 0x12
#define op_ldc_w 0x13
#define op_ldc2_w 0x14
#define op_iload 0x15
#define op_lload 0x16
#define op_fload 0x17
#define op_dload 0x18
#define op_aload 0x19
#define op_iload_n(n) 0x1a + n
#define op_lload_n(n) 0x1e + n
#define op_fload_n(n) 0x22 + n
#define op_dload_n(n) 0x26 + n
#define op_aload_n(n) 0x2a + n
#define op_iaload 0x2e
#define op_laload 0x2f
#define op_faload 0x30
#define op_daload 0x31
#define op_aaload 0x32
#define op_baload 0x33
#define op_caload 0x34
#define op_saload 0x35
#define op_istore 0x36
#define op_lstore 0x37
#define op_fstore 0x38
#define op_dstore 0x39
#define op_astore 0x3a
#define op_istore_n(n) 0x3b + n
#define op_lstore_n(n) 0x3f + n
#define op_fstore_n(n) 0x43 + n
#define op_dstore_n(n) 0x47 + n
#define op_astore_n(n) 0x4b + n
#define op_iastore 0x4f
#define op_lastore 0x50
#define op_fastore 0x51
#define op_dastore 0x52
#define op_aastore 0x53
#define op_bastore 0x54
#define op_castore 0x55
#define op_sastore 0x56
#define op_pop 0x57
#define op_pop2 0x58
#define op_dup 0x59
#define op_dup_x1 0x5a
#define op_dup_x2 0x5b
#define op_dup2 0x5c
#define op_dup2_x1 0x5d
#define op_dup2_x2 0x5e
#define op_swap 0x5f
#define op_iadd 0x60
#define op_ladd 0x61
#define op_fadd 0x62
#define op_dadd 0x63
#define op_isub 0x64
#define op_lsub 0x65
#define op_fsub 0x66
#define op_dsub 0x67
#define op_imul 0x68
#define op_lmul 0x69
#define op_fmul 0x6a
#define op_dmul 0x6b
#define op_idiv 0x6c
#define op_ldiv 0x6d
#define op_fdiv 0x6e
#define op_ddiv 0x6f
#define op_irem 0x70
#define op_lrem 0x71
#define op_frem 0x72
#define op_drem 0x73
#define op_ineg 0x74
#define op_lneg 0x75
#define op_fneg 0x76
#define op_dneg 0x77
#define op_ishl 0x78
#define op_lshl 0x79
#define op_ishr 0x7a
#define op_lshr 0x7b
#define op_iushr 0x7c
#define op_lushr 0x7d
#define op_iand 0x7e
#define op_land 0x7f
#define op_ior 0x80
#define op_lor 0x81
#define op_ixor 0x82
#define op_lxor 0x83
#define op_iinc 0x84
#define op_i2l 0x85
#define op_i2f 0x86
#define op_i2d 0x87
#define op_l2i 0x88
#define op_l2f 0x89
#define op_l2d 0x8a
#define op_f2i 0x8b
#define op_f2l 0x8c
#define op_d2i 0x8e
#define op_f2d 0x8d
#define op_d2l 0x8f
#define op_d2f 0x90
#define op_i2b 0x91
#define op_i2c 0x92
#define op_i2s 0x93
#define op_lcmp 0x94
#define op_fcmpl 0x95
#define op_fcmpg 0x96
#define op_dcmpl 0x97
#define op_dcmpg 0x98
#define op_ifeq 0x99
#define op_ifne 0x9a
#define op_iflt 0x9b
#define op_ifge 0x9c
#define op_ifgt 0x9d
#define op_ifle 0x9e
#define op_if_icmpeq 0x9f
#define op_if_icmpne 0xa0
#define op_if_icmplt 0xa1
#define op_if_icmpge 0xa2
#define op_if_icmpgt 0xa3
#define op_if_icmple 0xa4
#define op_if_acmpeq 0xa5
#define op_if_acmpne 0xa6
#define op_goto 0xa7
#define op_jsr 0xa8
#define op_ret 0xa9
#define op_tableswitch 0xaa
#define op_lookupswitch 0xab
#define op_ireturn 0xac
#define op_lreturn 0xad
#define op_freturn 0xae
#define op_dreturn 0xaf
#define op_areturn 0xb0
#define op_return 0xb1
#define op_getstatic 0xb2
#define op_putstatic 0xb3
#define op_getfield 0xb4
#define op_putfield 0xb5
#define op_invokevirtual 0xb6
#define op_invokespecial 0xb7
#define op_invokestatic 0xb8
#define op_invokeinterface 0xb9
#define op_invokedynamic 0xba
#define op_new 0xbb
#define op_newarray 0xbc
#define op_anewarray 0xbd
#define op_arraylength 0xbe
#define op_athrow 0xbf
#define op_checkcast 0xc0
#define op_instanceof 0xc1
#define op_monitorenter 0xc2
#define op_monitorexit 0xc3
#define op_wide 0xc4
#define op_multianewarray 0xc5
#define op_ifnull 0xc6
#define op_ifnonnull 0xc7
#define op_goto_w 0xc8
#define op_jsr_w 0xc9
// 0xca ~ 0xff reserved

#endif
