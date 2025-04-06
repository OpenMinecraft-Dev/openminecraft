#ifndef OM_CLASS_FILE_HPP
#define OM_CLASS_FILE_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <istream>
#include <vector>

#define JVM_VERSION_1_1 45
#define JVM_VERSION_1_2 46
#define JVM_VERSION_1_3 47
#define JVM_VERSION_1_4 48
#define JVM_VERSION_5   49
#define JVM_VERSION_6   50
#define JVM_VERSION_7   51
#define JVM_VERSION_8   52
#define JVM_VERSION_9   53
#define JVM_VERSION_10  54
#define JVM_VERSION_11  55
#define JVM_VERSION_12  56
#define JVM_VERSION_13  57
#define JVM_VERSION_14  58
#define JVM_VERSION_15  59
#define JVM_VERSION_16  60
#define JVM_VERSION_17  61
#define JVM_VERSION_18  62
#define JVM_VERSION_19  63
#define JVM_VERSION_20  64
#define JVM_VERSION_21  65
#define JVM_VERSION_22  66
#define JVM_VERSION_23  67
#define JVM_VERSION_24  68

#define JVM_Constant_Class              7
#define JVM_Constant_FieldRef           9
#define JVM_Constant_MethodRef          10
#define JVM_Constant_InterfaceMethodRef 11
#define JVM_Constant_String             8
#define JVM_Constant_Integer            3
#define JVM_Constant_Float              4
#define JVM_Constant_Long               5
#define JVM_Constant_Double             6
#define JVM_Constant_NameAndType        12
#define JVM_Constant_Utf8               1
#define JVM_Constant_MethodHandle       15 // Requires Java 7+
#define JVM_Constant_MethodType         16 // Requires Java 7+
#define JVM_Constant_Dynamic            17 // Requires Java 11+
#define JVM_Constant_InvokeDynamic      18 // Requires Java 7+
#define JVM_Constant_Module             19 // Requires Java 9+
#define JVM_Constant_Package            20 // Requires Java 9+

#define JVM_Acc_Public 0x0001
#define JVM_Acc_Final 0x0010
#define JVM_Acc_Super 0x0020
#define JVM_Acc_Interface 0x0200
#define JVM_Acc_Abstract 0x0400
#define JVM_Acc_Synthetic 0x1000
#define JVM_Acc_Annotation 0x2000
#define JVM_Acc_Enum 0x4000
#define JVM_Acc_Module 0x8000

typedef uint8_t* OMClassConstantItem;

namespace openminecraft::vm::classfile
{
    struct OMClassFile
    {
        uint32_t magicNumber;
        uint16_t minor;
        uint16_t major;
        uint16_t constantPoolCount;
        std::vector<OMClassConstantItem> constants;
        uint16_t accessFlags;
        uint16_t thisClass;
        uint16_t superClass;
        uint16_t interfacesCount;
        std::vector<uint16_t> interfaces;
        uint16_t fieldsCount;
    };

    struct OMClassConstantRef
    {
        uint8_t type;
        uint16_t classIndex;
        uint16_t nameAndTypeIndex;
    };

    struct OMClassConstantClass
    {
        uint8_t type;
        uint16_t nameIndex;
    };

    struct OMClassConstantNameAndType
    {
        uint8_t type;
        uint16_t nameIndex;
        uint16_t descriptorIndex;
    };

    struct OMClassConstantUtf8
    {
        uint8_t type;
        std::string bytes;
    };

    struct OMClassConstantInteger
    {
        uint8_t type;
        uint32_t data;
    };

    struct OMClassConstantFloat
    {
        uint8_t type;
        float data;
    };

    struct OMClassConstantString
    {
        uint8_t type;
        uint16_t stringIndex;
    };

    struct OMClassConstantLong
    {
        uint8_t type;
        uint64_t data;
    };

    struct OMClassConstantDouble
    {
        uint8_t type;
        double data;
    };

    struct OMClassConstantMethodHandle
    {
        uint8_t type;
        uint8_t refKind;
        uint16_t refIndex;
    };

    struct OMClassConstantMethodType
    {
        uint8_t type;
        uint16_t descIndex;
    };

    struct OMClassConstantDynamicInfo
    {
        uint8_t type;
        uint16_t bootMethodAttrIndex;
        uint16_t nameAndTypeIndex;
    };

    struct OMClassConstantModuleInfo
    {
        uint8_t type;
        uint16_t nameIndex;
    };

    struct OMClassConstantPackageInfo
    {
        uint8_t type;
        uint16_t nameIndex;
    };

    class OMClassFileParser
    {
        public:
            OMClassFileParser(std::istream& stream);
            ~OMClassFileParser();
            OMClassFile* parse();

        private:
            std::istream* source;
            log::OMLogger* logger;
            OMClassConstantItem parseConstant(int index);
            OMClassConstantItem parseConstantRef(int index, uint8_t id);
            OMClassConstantItem parseConstantClass(int index, uint8_t id);
            OMClassConstantItem parseConstantNameAndType(int index, uint8_t id);
            OMClassConstantItem parseConstantUtf8(int index, uint8_t id);
            OMClassConstantItem parseConstantInteger(int index, uint8_t id);
            OMClassConstantItem parseConstantFloat(int index, uint8_t id);
            OMClassConstantItem parseConstantString(int index, uint8_t id);
            OMClassConstantItem parseConstantLong(int index, uint8_t id);
            OMClassConstantItem parseConstantDouble(int index, uint8_t id);
            OMClassConstantItem parseConstantMethodHandle(int index, uint8_t id);
            OMClassConstantItem parseConstantMethodType(int index, uint8_t id);
            OMClassConstantItem parseConstantDynamicInfo(int index, uint8_t id);
            OMClassConstantItem parseConstantModuleInfo(int index, uint8_t id);
            OMClassConstantItem parseConstantPackageInfo(int index, uint8_t id);

            char* toStdUtf8(uint8_t* data, int length);
    };
}

#endif
