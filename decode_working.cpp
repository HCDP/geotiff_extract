
#include <iostream>
#include <map>
#include <vector>
#include <cstring>

using namespace std;

struct Bitvals {
    uint8_t bitw;
    uint8_t shr;
    long mask;
};

uint32_t next_code(int bitcount, uint8_t *encoded, int len_encoded, struct Bitvals *bitvals) {
    int start = bitcount / 8;
    uint32_t code = 0;
    int bytes_to_read = 4;
    int bytes_remaining = len_encoded - start;
    if(bytes_remaining < bytes_to_read) {
        cout << "!!" << endl;
        bytes_to_read = bytes_remaining;
    }
    for(int i = 0; i < bytes_to_read; i++) {
        int shift = 8 * (3 - i);
        code |= encoded[start + i] << shift;
    }
    code <<= bitcount % 8;
    code &= bitvals->mask;
    code >>= bitvals->shr;
    return code;
}

//can do checks and stuff later
int decode(uint8_t *encoded, uint8_t *decoded, int len_encoded) {
    int bitcount_max = len_encoded * 8;
    //start table with 258 code vectors (0-255 + padding for clear and exit codes)
    vector<vector<uint8_t>> table(258);
    int i;
    for(i = 0; i < 258; i++) {
        table[i].push_back((uint8_t)i);
    }

    struct Bitvals s255 = {9, 23, 4286578688L};
    struct Bitvals s511 = {10, 22, 4290772992L};
    struct Bitvals s1023 = {11, 21, 4292870144L};
    struct Bitvals s2047 = {12, 20, 4293918720L};
    
    map<int, struct Bitvals *> switchbits = {
        { 255, &s255 },
        { 511, &s511 },
        { 1023, &s1023},
        { 2047, &s2047 }
    };
    struct Bitvals *bitvals = switchbits[255];

    int bitcount = 0;

    if(len_encoded < 4) {
        return 1;
    }
    if(next_code(bitcount, encoded, len_encoded, bitvals) != 256) {
        cout << "!!" << endl;
        return 1;
    }

    uint32_t code = next_code(bitcount, encoded, len_encoded, bitvals);
    bitcount += bitvals->bitw;
    uint32_t oldcode = 0;
    int bytes_written = 0;

    while(code != 257 && bitcount < bitcount_max && bytes_written < 8194) {
        //clear
        if(code == 256) {

            table.resize(258);

            bitvals = switchbits[255];
            code = next_code(bitcount, encoded, len_encoded, bitvals);
            bitcount += bitvals->bitw;
            if(code == 257) {
                break;
            }
            cout << "code: " << code << endl;
            //right one byte code
            memcpy(decoded + bytes_written, &code, 1);
            bytes_written += 1;
        }
        else if(code < table.size()) {
            vector<uint8_t> *decode = &table[code];
            vector<uint8_t> new_sequence = table[oldcode];
            new_sequence.push_back((*decode)[0]);
            memcpy(decoded + bytes_written, decode->data(), decode->size());
            bytes_written += decode->size();
            table.push_back(new_sequence);
        }
        else {
            vector<uint8_t> new_sequence = table[oldcode];
            new_sequence.push_back(new_sequence[0]);

            memcpy(decoded + bytes_written, new_sequence.data(), new_sequence.size());
            bytes_written += new_sequence.size();
            table.push_back(new_sequence);
        }
        oldcode = code;
        if(switchbits.find(table.size()) != switchbits.end()) {
            bitvals = switchbits[table.size()];
        }
        code = next_code(bitcount, encoded, len_encoded, bitvals);
        bitcount += bitvals->bitw;
    }

    return 0;
}

