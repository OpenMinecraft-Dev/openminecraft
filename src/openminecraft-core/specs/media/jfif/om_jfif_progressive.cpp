#include "openminecraft/specs/jfif/om_jfif.hpp"
#include <stdexcept>

namespace openminecraft::specs::jfif
{
void OMJfifFile::parseRawBlocksProgressive(std::shared_ptr<std::istream> istr)
{
    bitBuffer.popValue(bitBuffer.bitsAvailable());
    if (range.spectralBegin == 0)
    {
        parseRawBlocksProgressiveDC(istr);
    }
    else
    {
        try
        {
            parseRawBlocksProgressiveAC(istr);
        }
        catch (std::logic_error &e)
        {
            logger.error("{}", e.what());
        }
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
        loadBlockCache();
        if ((range.successive >> 4) == 0)
        {
            auto dccode = fetchCode(currentBlock->dcTable, [&]() -> bool { return bufferReqBits(istr, 1); });
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
            bufferReqBits(istr, 1);
            if (bitBuffer.popBit())
            {
                blockData[0] += (1 << succlow);
            }
        }
        parseBlock();
        saveBlockCache();
        bumpBlock();
    }
    bitBuffer.popValue(bitBuffer.bitsAvailable());
}
void OMJfifFile::parseRawBlocksProgressiveAC(std::shared_ptr<std::istream> istr)
{
    eobRun = 0;
    int shift = range.successive & 0xf;
    while (mcuStatus.mcuid < mcuStatus.mcucounts)
    {
        if ((range.successive >> 4) == 0)
        {
            if (eobRun)
            {
                --eobRun;
                bumpBlock();
                continue;
            }

            std::memset(blockData.data(), 0x00, sizeof(int) * 64);
            loadBlockCache();
            blockDataIndex = range.spectralBegin;
            while (blockDataIndex <= range.spectralEnd)
            {
                auto accode = fetchCode(currentBlock->acTable, [&]() -> bool { return bufferReqBits(istr, 1); });
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
                            eobRun += bitBuffer.popValue(run);
                        }
                        --eobRun;

                        goto endBlock;
                    }
                    blockDataIndex += 16;
                }
                else
                {
                    blockDataIndex += run;
                    bufferReqBits(istr, size);
                    blockData[blockDataIndex] = bufferReadExtra(istr, size) * (1 << shift);
                    ++blockDataIndex;
                }
            }

        endBlock:
            saveBlockCache();
            parseBlock();
            bumpBlock();
        }
        else
        {
            auto bit = 1 << (range.successive & 0xf);

            std::memset(blockData.data(), 0x00, sizeof(int) * 64);
            loadBlockCache();

            if (eobRun)
            {
                --eobRun;
                for (int k = range.spectralBegin; k <= range.spectralEnd; ++k)
                {
                    int *p = &blockData[k];
                    if (*p != 0)
                    {
                        bufferReqBits(istr, 1);
                        if (bitBuffer.popBit())
                            if ((*p & bit) == 0)
                            {
                                if (*p > 0)
                                    *p += bit;
                                else
                                    *p -= bit;
                            }
                    }
                }

                parseBlock();
                saveBlockCache();
                bumpBlock();
            }
            else
            {
                blockDataIndex = range.spectralBegin;
                do
                {
                    int r, s;
                    auto rs = fetchCode(currentBlock->acTable, [&]() -> bool { return bufferReqBits(istr, 1); });
                    s = rs & 15;
                    r = rs >> 4;
                    if (s == 0)
                    {
                        if (r < 15)
                        {
                            eobRun = (1 << r) - 1;
                            if (r)
                            {
                                bufferReqBits(istr, r);
                                eobRun += bitBuffer.popValue(r);
                            }
                            r = 64; // force end of block
                        }
                        else
                        {
                            // r=15 s=0 should write 16 0s, so we just do
                            // a run of 15 0s and then write s (which is 0),
                            // so we don't have to do anything special here
                        }
                    }
                    else
                    {
                        if (s != 1)
                        {
                            throw std::logic_error("cur");
                        }
                        // sign bit
                        bufferReqBits(istr, 1);
                        if (bitBuffer.popBit())
                            s = bit;
                        else
                            s = -bit;
                    }

                    // advance by r
                    while (blockDataIndex <= range.spectralEnd)
                    {
                        auto *p = &blockData[blockDataIndex];
                        blockDataIndex++;
                        if (*p != 0)
                        {
                            bufferReqBits(istr, 1);
                            if (bitBuffer.popBit())
                                if ((*p & bit) == 0)
                                {
                                    if (*p > 0)
                                        *p += bit;
                                    else
                                        *p -= bit;
                                }
                        }
                        else
                        {
                            if (r == 0)
                            {
                                *p = (short)s;
                                break;
                            }
                            --r;
                        }
                    }
                } while (blockDataIndex <= range.spectralEnd);

                parseBlock();
                saveBlockCache();
                bumpBlock();
            }
        }
    }

    bitBuffer.popValue(bitBuffer.bitsAvailable());
}
} // namespace openminecraft::specs::jfif
