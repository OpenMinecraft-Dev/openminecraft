#ifndef OM_CLASSFILE_HPP
#define OM_CLASSFILE_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <cstring>
#include <istream>
#include <memory>

constexpr int JVM_VERSION_1_1 = 45;
constexpr int JVM_VERSION_1_2 = 46;
constexpr int JVM_VERSION_1_3 = 47;
constexpr int JVM_VERSION_1_4 = 48;
constexpr int JVM_VERSION_5 = 49;
constexpr int JVM_VERSION_6 = 50;
constexpr int JVM_VERSION_7 = 51;
constexpr int JVM_VERSION_8 = 52;
constexpr int JVM_VERSION_9 = 53;
constexpr int JVM_VERSION_10 = 54;
constexpr int JVM_VERSION_11 = 55;
constexpr int JVM_VERSION_12 = 56;
constexpr int JVM_VERSION_13 = 57;
constexpr int JVM_VERSION_14 = 58;
constexpr int JVM_VERSION_15 = 59;
constexpr int JVM_VERSION_16 = 60;
constexpr int JVM_VERSION_17 = 61;
constexpr int JVM_VERSION_18 = 62;
constexpr int JVM_VERSION_19 = 63;
constexpr int JVM_VERSION_20 = 64;
constexpr int JVM_VERSION_21 = 65;
constexpr int JVM_VERSION_22 = 66;
constexpr int JVM_VERSION_23 = 67;
constexpr int JVM_VERSION_24 = 68;

constexpr int JVM_Acc_Public = 0x0001;
constexpr int JVM_Acc_Private = 0x0002;
constexpr int JVM_Acc_Protected = 0x0004;
constexpr int JVM_Acc_Static = 0x0008;
constexpr int JVM_Acc_Final = 0x0010;
constexpr int JVM_Acc_Super = 0x0020;
constexpr int JVM_Acc_Synchronized = 0x0020;
constexpr int JVM_Acc_Bridge = 0x0040;
constexpr int JVM_Acc_Varargs = 0x0080;
constexpr int JVM_Acc_Native = 0x0100;
constexpr int JVM_Acc_Interface = 0x0200;
constexpr int JVM_Acc_Abstract = 0x0400;
constexpr int JVM_Acc_Strict = 0x0800;
constexpr int JVM_Acc_Synthetic = 0x1000;
constexpr int JVM_Acc_Annotation = 0x2000;
constexpr int JVM_Acc_Enum = 0x4000;
constexpr int JVM_Acc_Module = 0x8000;

namespace openminecraft::specs::classfile
{
class MemoryReader
{
    const uint8_t *data;
    const uint8_t *pos;
    const uint8_t *end;

  public:
    MemoryReader(const std::vector<uint8_t> &buffer) : data(buffer.data()), pos(data), end(data + buffer.size())
    {
    }

    auto raw() -> const uint8_t *
    {
        return pos;
    }

    auto readu8() -> uint8_t
    {
        return *pos++;
    }
    auto readu16() -> uint16_t
    {
        uint16_t v = (pos[0] << 8) | pos[1];
        pos += 2;
        return v;
    }
    auto readu32() -> uint32_t
    {
        uint32_t v = (pos[0] << 24) | (pos[1] << 16) | (pos[2] << 8) | pos[3];
        pos += 4;
        return v;
    }
    auto readu64() -> uint64_t
    {
        uint64_t v = readu32();
        v <<= 32;
        v |= readu32();
        return v;
    }
    void skip(size_t n)
    {
        pos += n;
    }

    void readn(void *target, int n)
    {
        std::memcpy(target, pos, n);
        pos += n;
    }
};

constexpr const char allocatorTag[] = "parser_classfile";
constexpr uint32_t headerMagic = 0xcafebabe;

enum OMClassFileConstantType : uint8_t
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
    MethodHandle = 15,  // Requires Java 7+
    MethodType = 16,    // Requires Java 7+
    Dynamic = 17,       // Requires Java 11+
    InvokeDynamic = 18, // Requires Java 7+
    Module = 19,        // Requires Java 9+
    Package = 20        // Requires Java 9+
};
enum OMClassRefKind : uint8_t
{
    RefGetField = 1,
    RefGetStatic = 2,
    RefSetField = 3,
    RefSetStatic = 4,
    RefInvokeVirtual = 5,
    RefInvokeStatic = 6,
    RefInvokeSepcial = 7,
    RefNewInvokeSpecial = 8,
    RefInvokeInterface = 9
};
struct OMClassFileConstant
{
    OMClassFileConstantType type;
    std::string valueString;
    union {
        struct
        {
            uint16_t nameIndex;
        } classinfo;
        struct
        {
            uint16_t classIndex;
            uint16_t nameAndTypeIndex;
        } ref;
        int valueInteger;
        int64_t valueLong;
        float valueFloat;
        double valueDouble;
        struct
        {
            uint16_t stringIndex;
        } stringRef;
        struct
        {
            uint16_t nameIndex;
            uint16_t descriptorIndex;
        } nameAndType;
        struct
        {
            OMClassRefKind refKind;
            uint16_t refIndex;
        } methodHandle;
        struct
        {
            uint16_t descriptorIndex;
        } methodType;
        struct
        {
            uint16_t bootstrapIndex;
            uint16_t nameAndTypeIndex;
        } dynamic;
        struct
        {
            uint16_t nameIndex;
        } module;
        struct
        {
            uint16_t nameIndex;
        } package;
    };
};

struct OMClassExceptionTableEntry
{
    uint16_t start;
    uint16_t end;
    uint16_t handler;
    uint16_t type;
};

struct OMClassBootstrapMethodEntry
{
    uint16_t bootstrapMethodRef, numBootstrapArguments;
    uint16_t *bootstrapArguments;
};

struct OMClassAttribute
{
    uint16_t nameIndex;
    uint32_t length;
    union {
        uint16_t constantValueIndex;
        uint16_t signatureIndex;
        struct
        {
            uint16_t maxStack;
            uint16_t maxLocal;
            uint32_t codeLength;
            uint8_t *code;

            uint16_t exceptionTableLength;
            OMClassExceptionTableEntry *exceptionTable;

            uint16_t attrCount;
            OMClassAttribute *attrs;
        } code;
        struct
        {
            uint16_t count;
            uint16_t *index;
        } exceptions;
        struct
        {
            uint16_t classIndex;
            uint16_t methodIndex;
        } enclosingMethod;
        struct
        {
            uint16_t numBootstrapMethods;
            OMClassBootstrapMethodEntry *bootstrapMethods;
        } bootstrapMethod;
    };
};

struct OMClassField
{
    uint16_t accessFlags;
    uint16_t nameIndex;
    uint16_t descriptorIndex;
    uint16_t attributesCount;
    std::shared_ptr<std::vector<OMClassAttribute>> attributes = nullptr;
};

struct OMClassMethod
{
    uint16_t accessFlags;
    uint16_t nameIndex;
    uint16_t descriptorIndex;
    uint16_t attributesCount;
    std::shared_ptr<std::vector<OMClassAttribute>> attributes = nullptr;
};

class OMClassFile
{
  public:
#pragma pack(1)
    struct
    {
        uint32_t magic;
        uint16_t minorVersion;
        uint16_t majorVersion;
    } header;
#pragma pack()

    struct
    {
        uint16_t length;
        std::shared_ptr<std::vector<OMClassFileConstant>> data = nullptr;
    } constants;

    struct
    {
        uint16_t accessFlags;
        uint16_t thisClass;
        uint16_t superClass;
    } basic;

    struct
    {
        uint16_t length;
        std::shared_ptr<std::vector<uint16_t>> data = nullptr;
    } interfaces;

    struct
    {
        uint16_t length;
        std::shared_ptr<std::vector<OMClassField>> data = nullptr;
    } fields;

    struct
    {
        uint16_t length;
        std::shared_ptr<std::vector<OMClassMethod>> data = nullptr;
    } methods;

    struct
    {
        uint16_t length;
        std::shared_ptr<std::vector<OMClassAttribute>> data = nullptr;
    } attributes;

    OMClassFile();
    ~OMClassFile()
    {
    }

    void load(std::shared_ptr<std::istream> istr);
    void loadConstant(MemoryReader &reader, OMClassFileConstant &c);
    void loadField(MemoryReader &reader, OMClassField &f);
    void loadMethod(MemoryReader &reader, OMClassMethod &m);
    void loadAttr(MemoryReader &reader, OMClassAttribute &a);

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::specs::classfile

#endif
