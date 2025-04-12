#ifndef OM_CLASS_FILE_HPP
#define OM_CLASS_FILE_HPP

#include <cstdint>
#include <istream>
#include <map>
#include <vector>

#include "openminecraft/log/om_log_common.hpp"

#define JVM_VERSION_1_1 45
#define JVM_VERSION_1_2 46
#define JVM_VERSION_1_3 47
#define JVM_VERSION_1_4 48
#define JVM_VERSION_5 49
#define JVM_VERSION_6 50
#define JVM_VERSION_7 51
#define JVM_VERSION_8 52
#define JVM_VERSION_9 53
#define JVM_VERSION_10 54
#define JVM_VERSION_11 55
#define JVM_VERSION_12 56
#define JVM_VERSION_13 57
#define JVM_VERSION_14 58
#define JVM_VERSION_15 59
#define JVM_VERSION_16 60
#define JVM_VERSION_17 61
#define JVM_VERSION_18 62
#define JVM_VERSION_19 63
#define JVM_VERSION_20 64
#define JVM_VERSION_21 65
#define JVM_VERSION_22 66
#define JVM_VERSION_23 67
#define JVM_VERSION_24 68

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

namespace openminecraft::vm::classfile {
enum class OMClassConstantType : uint8_t {
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
    MethodHandle = 15, // Requires Java 7+
    MethodType = 16, // Requires Java 7+
    Dynamic = 17, // Requires Java 11+
    InvokeDynamic = 18, // Requires Java 7+
    Module = 19, // Requires Java 9+
    Package = 20 // Requires Java 9+
};

class OMClassConstant {
public:
    virtual OMClassConstantType type() = 0;
    template <typename T>
    T* to()
    {
        return reinterpret_cast<T*>(this);
    }
};

class OMClassConstantMethodRef : public OMClassConstant {
public:
    OMClassConstantMethodRef(uint16_t ci, uint16_t nti);
    virtual OMClassConstantType type() override;
    const uint16_t classIndex;
    const uint16_t nameAndTypeIndex;
};

class OMClassConstantFieldRef : public OMClassConstant {
public:
    OMClassConstantFieldRef(uint16_t ci, uint16_t nti);
    virtual OMClassConstantType type() override;
    const uint16_t classIndex;
    const uint16_t nameAndTypeIndex;
};

class OMClassConstantInterfaceMethodRef : public OMClassConstant {
public:
    OMClassConstantInterfaceMethodRef(uint16_t ci, uint16_t nti);
    virtual OMClassConstantType type() override;
    const uint16_t classIndex;
    const uint16_t nameAndTypeIndex;
};

class OMClassConstantClass : public OMClassConstant {
public:
    OMClassConstantClass(uint16_t ni);
    virtual OMClassConstantType type() override;
    const uint16_t nameIndex;
};

class OMClassConstantNameAndType : public OMClassConstant {
public:
    OMClassConstantNameAndType(uint16_t ni, uint16_t di);
    virtual OMClassConstantType type() override;
    const uint16_t nameIndex;
    const uint16_t descIndex;
};

class OMClassConstantUtf8 : public OMClassConstant {
public:
    OMClassConstantUtf8(std::string data);
    virtual OMClassConstantType type() override;
    const std::string data;
};

class OMClassConstantString : public OMClassConstant {
public:
    OMClassConstantString(uint16_t si);
    virtual OMClassConstantType type() override;
    const uint16_t stringIndex;
};

class OMClassConstantInteger : public OMClassConstant {
public:
    OMClassConstantInteger(int data);
    virtual OMClassConstantType type() override;
    const int data;
};

class OMClassConstantFloat : public OMClassConstant {
public:
    OMClassConstantFloat(float data);
    virtual OMClassConstantType type() override;
    const float data;
};

class OMClassConstantLong : public OMClassConstant {
public:
    OMClassConstantLong(int64_t data);
    virtual OMClassConstantType type() override;
    const int64_t data;
};

class OMClassConstantDouble : public OMClassConstant {
public:
    OMClassConstantDouble(double data);
    virtual OMClassConstantType type() override;
    const double data;
};

class OMClassConstantMethodHandle : public OMClassConstant {
public:
    OMClassConstantMethodHandle(uint8_t rk, uint16_t ri);
    virtual OMClassConstantType type() override;
    const uint8_t refKind;
    const uint16_t refIndex;
};

class OMClassConstantMethodType : public OMClassConstant {
public:
    OMClassConstantMethodType(uint16_t di);
    virtual OMClassConstantType type() override;
    const uint16_t descIndex;
};

class OMClassConstantDynamic : public OMClassConstant {
public:
    OMClassConstantDynamic(uint16_t bmai, uint16_t nti);
    virtual OMClassConstantType type() override;
    const uint16_t bootstrapMethodAttrIndex;
    const uint16_t nameAndTypeIndex;
};

class OMClassConstantInvokeDynamic : public OMClassConstant {
public:
    OMClassConstantInvokeDynamic(uint16_t bmai, uint16_t nti);
    virtual OMClassConstantType type() override;
    const uint16_t bootstrapMethodAttrIndex;
    const uint16_t nameAndTypeIndex;
};

class OMClassConstantModule : public OMClassConstant {
public:
    OMClassConstantModule(uint16_t ni);
    virtual OMClassConstantType type() override;
    const uint16_t nameIndex;
};

class OMClassConstantPackage : public OMClassConstant {
public:
    OMClassConstantPackage(uint16_t ni);
    virtual OMClassConstantType type() override;
    const uint16_t nameIndex;
};

enum OMClassAttrType {
    ConstantValue,
    Code,
    StackMapTable,
    Exceptions,
    InnerClasses,
    EnclosingMethod,
    Synthetic,
    Signature,
    SourceFile,
    SourceDebugExtension,
    LineNumberTable,
    LocalVariableTable,
    LocalVariableTypeTable,
    Deprecated,
    RuntimeVisibleAnnotations,
    RuntimeInvisibleAnnotations,
    RuntimeVisibleParameterAnnotations,
    RuntimeInvisibleParameterAnnotations,
    RuntimeVisibleTypeAnnotations,
    RuntimeInvisibleTypeAnnotations,
    AnnotationDefault,
    BootstrapMethods,
    MethodParameters,
    Module,
    ModulePackages,
    ModuleMainClass,
    NestHost,
    NestMembers,
    Record,
    PermittedSubclasses
};

class OMClassAttr {
public:
    virtual OMClassAttrType type() = 0;
};

class OMClassAttrConstantValue : public OMClassAttr {
public:
    OMClassAttrConstantValue(uint16_t vi);
    virtual OMClassAttrType type() override;
    const uint16_t valueIndex;
};

struct OMClassFieldInfo {
    uint16_t accessFlags;
    uint16_t nameIndex;
    uint16_t descIndex;
    uint16_t attrCount;
    std::vector<OMClassAttr*> attrs;
};

struct OMClassFile {
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
    std::vector<OMClassFieldInfo*> fields;
};

class OMClassFileParser {
public:
    OMClassFileParser(std::istream& stream);
    ~OMClassFileParser();
    OMClassFile* parse();

private:
    std::istream* source;
    log::OMLogger* logger;

    OMClassConstant* parseConstant(uint16_t* idx);
    std::map<uint16_t, OMClassConstant*> buildConstantMapping(std::vector<OMClassConstant*> c);
    OMClassFieldInfo* parseField(std::map<uint16_t, OMClassConstant*> m);
    OMClassAttr* parseAttr(std::map<uint16_t, OMClassConstant*> m);
    char* toStdUtf8(uint8_t* data, int length);
};
} // namespace openminecraft::vm::classfile

#endif
