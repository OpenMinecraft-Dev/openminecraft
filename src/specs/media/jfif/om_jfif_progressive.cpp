#include "openminecraft/specs/jfif/om_jfif.hpp"

namespace openminecraft::specs::jfif
{
void OMJfifFile::parseRawBlocksProgressive(std::shared_ptr<std::istream> istr)
{
    if (range.spectralBegin == 0)
    {
        parseRawBlocksProgressiveDC(istr);
    }
    else
    {
        parseRawBlocksProgressiveAC(istr);
    }
}
void OMJfifFile::parseRawBlocksProgressiveDC(std::shared_ptr<std::istream> istr)
{
    if (range.spectralEnd != 0)
    {
        throw std::logic_error("Can't read both AC and DC");
    }

    auto succlow = range.successive & 0xf;
    while (mcuStatus.mcuid < mcuStatus.mcucounts)
    {
        std::memset(blockData.data(), 0x00, sizeof(int) * 64);
        if ((range.successive >> 4) == 0)
        {
            auto dccode = fetchCode(currentBlock->dcTable, [&]() { return bufferReqBits(istr, 1); });
            if (dccode >> 4)
            {
                throw std::logic_error("Can't read both AC and DC");
            }

            bufferReqBits(istr, dccode);
            auto dcvalue = bufferReadExtra(istr, dccode) + dcTemp[currentBlock->id];
            dcTemp[currentBlock->id] = dcvalue;

            blockData[0] = dcvalue * (1 << succlow);
        }
        else
        {
            if (bitBuffer.popBit())
            {
                blockData[0] += (1 << succlow);
            }
        }
        parseBlock();
        bumpBlock();
    }
}
void OMJfifFile::parseRawBlocksProgressiveAC(std::shared_ptr<std::istream> istr)
{
    /*while (mcuStatus.mcuid < mcuStatus.mcucounts)
    {
        std::memset(blockData.data(), 0x00, sizeof(int) * 64);
        if ((range.successive >> 4) == 0)
        {
            int shift = range.successive & 0xf;
	    blockDataIndex = range.spectralBegin;

            if (eobRun)
            {
		auto bll = std::min(eobRun, range.spectralEnd - range.spectralBegin + 1);
		eobRun -= bll;
		blockDataIndex += bll;
		// bumpBlock();
		// continue;
            }

            while (blockDataIndex <= range.spectralEnd)
            {
                auto accode = fetchCode(currentBlock->acTable, [&]() { return bufferReqBits(istr, 1); });
                auto size = accode & 0xf;
                auto run = accode >> 4;

                if (size == 0)
                {
                    if (run < 15)
                    {
                        eobRun = (1 << run);
                        if (run)
                        {
                            bufferReqBits(istr, run);
                            eobRun += bufferReadExtra(istr, run);
                        }
                        --eobRun;
			bumpBlock();
                        break;
                    }
                    blockDataIndex += 16;
                }
                else
                {
                    blockDataIndex += run;
                    bufferReqBits(istr, size);
                    blockData[blockDataIndex] = bufferReadExtra(istr, size) * (1 << shift);
                    blockDataIndex++;
                }
            }
            parseBlock();
            bumpBlock();
        }
        else
        {
        }
    }
    bufferLogStatus(istr);*/
}
} // namespace openminecraft::specs::jfif
