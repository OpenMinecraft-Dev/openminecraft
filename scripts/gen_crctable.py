"""
for (int n = 0; n < 256; n++)
    {
        c = static_cast<uint64_t>(n);
        for (int k = 0; k < 8; k++)
        {
            if (c & 1)
            {
                c = 0xedb88320 ^ (c >> 1);
            }
            else
            {
                c = c >> 1;
            }
        }
        crcTable[n] = c;
    }
"""

for n in range(256):
    c = n
    for k in range(8):
        if c & 1:
            c = 0xedb88320 ^ (c >> 1)
        else:
            c = c >> 1
    print("0x{:x}, ".format(c), end="")
