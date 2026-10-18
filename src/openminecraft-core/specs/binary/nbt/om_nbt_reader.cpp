#include "openminecraft/specs/nbt/om_nbt_reader.hpp"
#include "fmt/format.h"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/specs/nbt/om_nbt.hpp"
#include <ios>
#include <iostream>
#include <memory>
#include <stack>
#include <stdexcept>
#include <vector>

namespace openminecraft::specs::nbt
{
auto OMNbtReader::readPrefixString() -> std::string
{
    uint16_t l;
    istr->read(reinterpret_cast<char *>(&l), 2);
    l = binary::be16ToNative(l);

    std::string target;
    target.resize(l);
    istr->read(target.data(), l);

    return target;
}
auto OMNbtReader::parse(bool haveName) -> std::shared_ptr<OMNbtTag>
{
    std::stack<OMNbtContext> context;

    OMNbtTagType type;

    std::shared_ptr<OMNbtTag> result = nullptr;

    while (true)
    {
        if (!context.empty() && context.top().current->type == List && context.top().rest <= 0)
        {
            context.pop();

            if (context.empty())
            {
                break;
            }

            continue;
        }

        std::string n = "";
        if (context.empty() || context.top().current->type != List)
        {
            istr->read(reinterpret_cast<char *>(&type), 1);
            if (type != OMNbtTagType::End && (!context.empty() || haveName))
            {
                n = readPrefixString();
            }
        }
        else
        {
            type = context.top().elementType;
        }

        std::shared_ptr<OMNbtTag> tag;
        uint32_t rest = -1;
        OMNbtTagType elemType = End;
        switch (type)
        {
        case Compound:
            tag = std::make_shared<OMNbtTag>(Compound, n, NBTList());
            break;
        case End:
            tag = std::make_shared<OMNbtTag>(End, n, 0);
            if (!context.empty())
            {
                context.pop();
            }
            break;
        case Byte: {
            NBTByte b;
            istr->read(reinterpret_cast<char *>(&b), 1);
            tag = std::make_shared<OMNbtTag>(Byte, n, b);
            break;
        }
        case Short: {
            NBTShort b;
            istr->read(reinterpret_cast<char *>(&b), 2);
            b = binary::be16ToNative(b);
            tag = std::make_shared<OMNbtTag>(Short, n, b);
            break;
        }
        case Int: {
            NBTInt b;
            istr->read(reinterpret_cast<char *>(&b), 4);
            b = binary::be32ToNative(b);
            tag = std::make_shared<OMNbtTag>(Int, n, b);
            break;
        }
        case Long: {
            NBTLong b;
            istr->read(reinterpret_cast<char *>(&b), 8);
            b = binary::be64ToNative(b);
            tag = std::make_shared<OMNbtTag>(Long, n, b);
            break;
        }
        case Float: {
            NBTInt b;
            istr->read(reinterpret_cast<char *>(&b), 4);
            b = binary::be32ToNative(b);
            tag = std::make_shared<OMNbtTag>(Float, n, *reinterpret_cast<NBTFloat *>(&b));
            break;
        }
        case Double: {
            NBTLong b;
            istr->read(reinterpret_cast<char *>(&b), 8);
            b = binary::be64ToNative(b);
            tag = std::make_shared<OMNbtTag>(Double, n, *reinterpret_cast<NBTDouble *>(&b));
            break;
        }
        case ByteArray: {
            tag = std::make_shared<OMNbtTag>(ByteArray, n, NBTByteArray());
            auto &r = tag->get<NBTByteArray>();

            uint32_t c;
            istr->read(reinterpret_cast<char *>(&c), 4);
            c = binary::be32ToNative(c);

            r.resize(c);
            istr->read(reinterpret_cast<char *>(r.data()), c);
            break;
        }
        case String: {
            tag = std::make_shared<OMNbtTag>(String, n, readPrefixString());
            break;
        }
        case List: {
            istr->read(reinterpret_cast<char *>(&elemType), 1);
            istr->read(reinterpret_cast<char *>(&rest), 4);
            rest = binary::be32ToNative(rest);

            tag = std::make_shared<OMNbtTag>(List, n, NBTList());
            break;
        }
        case IntArray: {
            tag = std::make_shared<OMNbtTag>(IntArray, n, NBTIntArray());
            auto &r = tag->get<NBTIntArray>();

            uint32_t c;
            istr->read(reinterpret_cast<char *>(&c), 4);
            c = binary::be32ToNative(c);

            r.resize(c);
            istr->read(reinterpret_cast<char *>(r.data()), c * 4);

            for (int i = 0; i < c; ++i)
            {
                r[i] = binary::be32ToNative(r[i]);
            }

            break;
        }
        case LongArray: {
            tag = std::make_shared<OMNbtTag>(LongArray, n, NBTLongArray());
            auto &r = tag->get<NBTLongArray>();

            uint32_t c;
            istr->read(reinterpret_cast<char *>(&c), 4);
            c = binary::be32ToNative(c);

            r.resize(c);
            istr->read(reinterpret_cast<char *>(r.data()), c * 8);

            for (int i = 0; i < c; ++i)
            {
                r[i] = binary::be64ToNative(r[i]);
            }

            break;
        }
        default:
            throw std::logic_error(fmt::format("unknown tag {:x} at {:x}", (int)type, (int)istr->tellg()));
        }

        if (context.empty())
        {
            if (tag->type != OMNbtTagType::End)
            {
                result = tag;

                // INFO: 根节点不是 Compound/List 时（例如文本组件的 String 根），读完就是整个 NBT
                if (tag->type != List && tag->type != Compound)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            if (tag->type != OMNbtTagType::End)
            {
                context.top().current->get<NBTList>().push_back(tag);

                if (context.top().current->type == OMNbtTagType::List)
                {
                    context.top().rest--;
                }
            }
        }

        if (tag->type == List || tag->type == Compound)
        {
            context.push({tag, elemType, static_cast<int>(rest)});
        }
    }

    return result;
}
} // namespace openminecraft::specs::nbt