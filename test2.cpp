

#include <iostream>
#include <vector>

using namespace std;

// This program is released under the GPL license.
// -----------------------------------------------


class StreamReader
{
    typedef unsigned char byte;
    const std::vector<byte>& stream;
    unsigned curBitIndex;
    size_t curByteIndex;

 public:
    inline StreamReader(const std::vector<byte>& s, size_t startIndex);

    inline unsigned read(unsigned bits);
};

inline StreamReader::StreamReader(const std::vector<byte>& s,
                                  size_t startIndex):
    stream(s), curBitIndex(0), curByteIndex(startIndex) {}

inline unsigned StreamReader::read(unsigned bits)
{
    unsigned res = 0;
    unsigned resBitIndex = 0;

    while(bits > 0)
    {
        unsigned iValue = stream[curByteIndex];

        if(curBitIndex == 0)
        {
            if(bits < 8)
            {
                const unsigned mask = ~(~0U << bits);
                res |= (iValue & mask) << resBitIndex;
                curBitIndex += bits;
                bits = 0;
            }
            else
            {
                res |= iValue << resBitIndex;
                resBitIndex += 8;
                bits -= 8;
                ++curByteIndex;
            }
        }
        else
        {
            const unsigned mask = ~(~0U << bits);
            res |= ((iValue>>curBitIndex) & mask) << resBitIndex;
            const unsigned bitsLeft = 8-curBitIndex;
            if(bits < bitsLeft)
            {
                curBitIndex += bits;
                bits = 0;
            }
            else
            {
                curBitIndex = 0;
                ++curByteIndex;
                bits -= bitsLeft;
                resBitIndex += bitsLeft;
            }
        }
    }

    return res;
}











// Calculates the minimum amount of bits required to store the specified
// value:
unsigned requiredBits(unsigned value)
{
    unsigned bits = 1;
    while((value >>= 1) > 0) ++bits;
    return bits;
}

// The string element:
struct CodeString {
    unsigned prefixIndex;

    // First CodeString using this CodeString as prefix:
    unsigned first;

    // Next CodeStrings using the same prefixIndex as this one:
    unsigned nextLeft, nextRight;

    uint8_t k;

    CodeString(uint8_t newByte = 0, unsigned pI = ~0U):
        prefixIndex(pI), first(~0U),
        nextLeft(~0U), nextRight(~0U),
        k(newByte) {}
};

class Dictionary {
    vector<CodeString> table;
    unsigned codeStart, newCodeStringIndex;
    vector<uint8_t> decodedString;

    // Returns ~0U if c didn't already exist, else the index to the
    // existing CodeString:
    unsigned add(CodeString& c)
    {
        if(c.prefixIndex == ~0U) return c.k;

        unsigned index = table[c.prefixIndex].first;
        if(index == ~0U)
            table[c.prefixIndex].first = newCodeStringIndex;
        else
        {
            while(true)
            {
                if(c.k == table[index].k) return index;
                if(c.k < table[index].k)
                {
                    const unsigned next = table[index].nextLeft;
                    if(next == ~0U)
                    {
                        table[index].nextLeft = newCodeStringIndex;
                        break;
                    }
                    index = next;
                }
                else
                {
                    const unsigned next = table[index].nextRight;
                    if(next == ~0U)
                    {
                        table[index].nextRight = newCodeStringIndex;
                        break;
                    }
                    index = next;
                }
            }
        }
        table[newCodeStringIndex++] = c;

        return ~0U;
    }

    void fillDecodedString(unsigned code) {
        decodedString.clear();
        while(code != ~0U) {
            const CodeString& cs = table[code];
            decodedString.push_back(cs.k);
            code = cs.prefixIndex;
        }
    }


    public:
    Dictionary(unsigned maxBits, unsigned codeStart):
        table(1<<maxBits),
        codeStart(codeStart), newCodeStringIndex(codeStart)
    {
        for(unsigned i = 0; i < codeStart; ++i)
            table[i].k = i;
    }

    bool searchCodeString(CodeString& c)
    {
        unsigned index = add(c);
        if(index != ~0U)
        {
            c.prefixIndex = index;
            return true;
        }
        return false;
    }

    void decode(unsigned oldCode, unsigned code,
            vector<uint8_t>& outStream, const uint8_t* byteMap)
    {
        const bool exists = code < newCodeStringIndex;

        if(exists) fillDecodedString(code);
        else fillDecodedString(oldCode);

        for(size_t i = decodedString.size(); i > 0;)
            outStream.push_back(byteMap[decodedString[--i]]);
        if(!exists) outStream.push_back(byteMap[decodedString.back()]);

        table[newCodeStringIndex].prefixIndex = oldCode;
        table[newCodeStringIndex++].k = decodedString.back();
    }

    unsigned size() const { return newCodeStringIndex; }

    void reset() {
        newCodeStringIndex = codeStart;
        for(unsigned i = 0; i < codeStart; ++i)
            table[i] = CodeString(i);
    }
};


//ampersand pass by ref
void decode(const vector<uint8_t>& encoded, vector<uint8_t>& decoded) {
    printf("start decode\n");
    decoded.clear();
    printf("clear\n");

    uint8_t byteMap[256];
    const unsigned maxBits = encoded[0];
    const unsigned byteMapSize = (encoded[1] == 0 ? 256 : encoded[1]);
    const unsigned eoiCode = byteMapSize;
    const unsigned codeStart = byteMapSize+1;
    const unsigned minBits = requiredBits(codeStart);

    if(byteMapSize < 256) {
        for(unsigned i = 0; i < byteMapSize; ++i) {
            byteMap[i] = encoded[i+2];
        } 
    }
        
    else {
        for(unsigned i = 0; i < 256; ++i) {
            byteMap[i] = uint8_t(i);
        }
    }
        

    Dictionary dictionary(maxBits, codeStart);
    StreamReader reader(encoded, encoded[1]==0 ? 2 : 2+byteMapSize);

    while(true) {
        printf("main loop\n");
        dictionary.reset();
        unsigned currentBits = minBits;
        unsigned nextBitIncLimit = (1 << minBits) - 2;

        printf("before read\n");

        unsigned code = reader.read(currentBits);

        printf("after read\n");

        if(code == eoiCode) return;

        decoded.push_back(byteMap[code]);
        printf("after push to decode\n");

        unsigned oldCode = code;

        while(true) {
            printf("inner loop \n");
            printf("%d\n", maxBits);
            printf("%d\n", currentBits);
            code = reader.read(currentBits);
            printf("after inner loop read\n");
            if(code == eoiCode) {
                return;
            }

            dictionary.decode(oldCode, code, decoded, byteMap);
            if(dictionary.size() == nextBitIncLimit) {
                if(currentBits == maxBits) {
                    break;
                }
                    
                else {
                    currentBits++;
                }
                    
                nextBitIncLimit = (1 << currentBits) - 2;
            }

            oldCode = code;
        }
    }
}
