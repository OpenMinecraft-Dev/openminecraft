#ifndef OM_CLASS_FILE_HPP
#define OM_CLASS_FILE_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <istream>
#include <ostream>
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

#define JVM_Acc_Public 0x0001
#define JVM_Acc_Private 0x0002
#define JVM_Acc_Protected 0x0004
#define JVM_Acc_Static 0x0008
#define JVM_Acc_Final 0x0010
#define JVM_Acc_Super 0x0020
#define JVM_Acc_Synchronized 0x0020
#define JVM_Acc_Bridge 0x0040
#define JVM_Acc_Varargs 0x0080
#define JVM_Acc_Native 0x0100
#define JVM_Acc_Interface 0x0200
#define JVM_Acc_Abstract 0x0400
#define JVM_Acc_Strict 0x0800
#define JVM_Acc_Synthetic 0x1000
#define JVM_Acc_Annotation 0x2000
#define JVM_Acc_Enum 0x4000
#define JVM_Acc_Module 0x8000

namespace openminecraft::vm::classfile
{
    enum class OMClassConstantType : uint8_t
    {
        Utf8 = 1,
        Integer = 3,
        Float = 4,
        Long = 5,
        Double = 6, 
        Class = 7, 
        String = 8, 
        FieldRef = 9,
        MethodRef = 10, 
        InterfaceMethodRef = 11,
        NameAndType = 12,
        MethodHandle = 15,      // Requires Java 7+
        MethodType = 16,        // Requires Java 7+
        Dynamic = 17,           // Requires Java 11+
        InvokeDynamic = 18,     // Requires Java 7+
        Module = 19,            // Requires Java 9+
        Package = 20            // Requires Java 9+
    };

    class OMClassConstant
    {
        public: 
            virtual OMClassConstantType type() = 0;
    };

    class OMClassConstantMethodRef : public OMClassConstant
    {
        public:
            OMClassConstantMethodRef(uint16_t ci, uint16_t nti);
            virtual OMClassConstantType type() override;
            const uint16_t classIndex;
            const uint16_t nameAndTypeIndex;
    };

    struct OMClassFile
    {
        uint32_t magicNumber;
        uint16_t minor;
        uint16_t major;
        uint16_t constantPoolCount;
        std::vector<OMClassConstant*> constants;
        uint16_t accessFlags;
        uint16_t thisClass;
        uint16_t superClass;
        uint16_t interfacesCount;
        std::vector<uint16_t> interfaces;
        uint16_t fieldsCount;
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

            OMClassConstant* parseConstant(uint16_t* idx);
            char* toStdUtf8(uint8_t* data, int length);
    };
}

#endif
