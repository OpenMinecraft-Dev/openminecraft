#include "openminecraft/specs/jfif/om_jfif.hpp"
#include <stdexcept>

namespace openminecraft::specs::jfif
{
void OMJfifFile::parseRawBlocksBaseline(std::shared_ptr<std::istream> istr)
{
    if (range.spectralBegin != 0x00 || range.spectralEnd != 0x3f || range.successive != 0x00)
    {
        throw std::logic_error("not supported");
    }

    while (mcuStatus.mcuid < mcuStatus.mcucounts)
    {
        blockDataIndex = 0;
        std::memset(blockData.data(), 0x00, sizeof(int) * 64);

        // TODO: DC decode & cache
        auto dccode = fetchCode(currentBlock->dcTable, [&]() { return bufferReqBits(istr, 1); });
        bufferReqBits(istr, dccode);
        auto dcvalue = bufferReadExtra(istr, dccode) + dcTemp[currentBlock->id];
        dcTemp[currentBlock->id] = dcvalue;
        blockData[0] = dcvalue;
	blockDataIndex++;

        while (blockDataIndex <= 63)
        {
            // TODO: AC decode
            auto accode = fetchCode(currentBlock->acTable, [&]() { return bufferReqBits(istr, 1); });
            uint8_t run = accode >> 4;
            uint8_t siz = accode & 0xf;

            if (siz == 0)
            {
                if (accode != 0xf0)
                {
                    break;
                }
                else
                {
                    blockDataIndex += 16;
                }
            }
            else
            {
                blockDataIndex += run;
                bufferReqBits(istr, siz);
                blockData[blockDataIndex] = bufferReadExtra(istr, siz);
                blockDataIndex++;
            }
        }
        parseBlock();
        bumpBlock();
    }
    bufferLogStatus(istr);
    bitBuffer.popValue(bitBuffer.bitsAvailable());
}
} // namespace openminecraft::specs::jfif
