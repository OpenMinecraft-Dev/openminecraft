#include "openminecraft/specs/jfif/om_jfif.hpp"
#include <istream>
#include <memory>

namespace openminecraft::specs::jfif
{
bool OMJfifFile::bufferReqBits(std::shared_ptr<std::istream> istr, int b)
{
    auto pshBit = [&]() -> bool {
        uint8_t c = istr->peek();
        istr->ignore(1);

        if (c == 0xff)
        {
            if (istr->peek() != 0x00)
            {
                logger.warn("abnormal exit! {}/{}", mcuStatus.mcuid, mcuStatus.mcucounts);
                istr->seekg(-1, std::ios::cur);
                return false;
            }
            else
            {
                istr->ignore(1);
            }
        }

        bitBuffer.push(c);

        return true;
    };

    while (bitBuffer.bitsAvailable() < b)
    {
        if (!pshBit())
        {
            return false;
        }
    }

    return true;
}

void OMJfifFile::bufferLogStatus(std::shared_ptr<std::istream> istr)
{
    int pp = bitBuffer.bitsAvailable();
    size_t off = istr->tellg();
    while (pp > 0)
    {
        pp -= 8;
        off--;
    }

    logger.info("offset +{:02x}.{}", off, -pp);
}

int64_t OMJfifFile::bufferReadExtra(std::shared_ptr<std::istream> istr, int datalen)
{
    auto tempval = (int64_t)bitBuffer.popValue(datalen);
    if ((tempval >> (datalen - 1)) == 0)
    {
        tempval -= (1 << datalen) - 1;
    }
    return tempval;
}
} // namespace openminecraft::specs::jfif
