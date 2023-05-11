

#include <iostream>
#include <map>
#include <vector>
#include <cstring>

using namespace std;

namespace GeotiffExtract {
    class Decoder {
        public:
        uint8_t *encoded;
        int len_encoded;
        
        Decoder(uint8_t *encoded, int len_encoded) {
            this->encoded = encoded;
            this->len_encoded = len_encoded;
        }

        int decode(uint8_t *decoded) {
            int bitcount_max = len_encoded * 8;
            //start table with 258 code vectors (0-255 + padding for clear and exit codes), note index 256/257 value does not matter
            vector<vector<uint8_t>> table(258);
            int i;
            for(i = 0; i < 258; i++) {
                table[i].push_back((uint8_t)i);
            }

            struct Bitvals *bitvals = switchbits[255];

            int bitcount = 0;

            //encoded must have a minimum length of 4 bytes and start with clear code (256)
            if(len_encoded < 4 || next_code(bitcount, encoded, len_encoded, bitvals) != 256) {
                return 1;
            }

            uint32_t code = next_code(bitcount, encoded, len_encoded, bitvals);
            bitcount += bitvals->bitw;
            uint32_t oldcode = 0;
            int bytes_written = 0;

            while(code != 257 && bitcount < bitcount_max) {
                //clear
                if(code == 256) {
                    table.resize(258);

                    bitvals = switchbits[255];
                    code = next_code(bitcount, encoded, len_encoded, bitvals);
                    bitcount += bitvals->bitw;
                    if(code == 257) {
                        break;
                    }
                    //write one byte code
                    memcpy(decoded + bytes_written, &code, 1);
                    bytes_written += 1;
                }
                else if(code < table.size()) {
                    vector<uint8_t> new_sequence = table[oldcode];
                    new_sequence.push_back(table[code][0]);
                    memcpy(decoded + bytes_written, table[code].data(), table[code].size());
                    bytes_written += table[code].size();
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

        int get_index(int index, int element_size, void *value) {
            int bytes_to_index = index * element_size;
            int index_bytes_decoded = 0;
            int bitcount_max = len_encoded * 8;
            //start table with 258 code vectors (0-255 + padding for clear and exit codes)
            vector<vector<uint8_t>> table(258);
            int i;
            for(i = 0; i < 258; i++) {
                table[i].push_back((uint8_t)i);
            }

            struct Bitvals *bitvals = switchbits[255];

            int bitcount = 0;

            //encoded must have a minimum length of 4 bytes and start with clear code (256)
            if(len_encoded < 4 || next_code(bitcount, encoded, len_encoded, bitvals) != 256) {
                return 1;
            }

            uint32_t code = next_code(bitcount, encoded, len_encoded, bitvals);
            bitcount += bitvals->bitw;
            uint32_t oldcode = 0;

            while(code != 257 && bitcount < bitcount_max && index_bytes_decoded < element_size) {
                int num_bytes_decoded;
                uint8_t *decoded_bytes;
                
                //clear
                if(code == 256) {
                    table.resize(258);
                    bitvals = switchbits[255];
                    code = next_code(bitcount, encoded, len_encoded, bitvals);
                    bitcount += bitvals->bitw;
                    if(code == 257) {
                        break;
                    }
                    //write one byte code (note code must be <256)
                    num_bytes_decoded = 1;
                    decoded_bytes = (uint8_t *)&code;
                }
                else if(code < table.size()) {
                    //CAN'T STORE POINTER TO VECTOR ELEMENT, APPARENTLY CAN CHANGE
                    //just access directly rather than through decode variable
                    vector<uint8_t> new_sequence = table[oldcode];
                    new_sequence.push_back(table[code][0]);
                    table.push_back(new_sequence);
                    num_bytes_decoded = table[code].size();
                    decoded_bytes = table[code].data();
                }
                else {
                    vector<uint8_t> new_sequence = table[oldcode];
                    new_sequence.push_back(new_sequence[0]);
                    table.push_back(new_sequence);
                    num_bytes_decoded = new_sequence.size();
                    decoded_bytes = new_sequence.data();
                }

                //check if should write, all of the methods write
                //if decoded past the start of the desired item index then read into the output value
                if(num_bytes_decoded > bytes_to_index) {
                    //move pointer to first byte in index
                    decoded_bytes += bytes_to_index;
                    //subtract shift from number of bytes
                    num_bytes_decoded -= bytes_to_index;
                    //at index, set tracker to 0
                    bytes_to_index = 0;
                    //set max bytes to write to the remaining decoded bytes
                    int bytes_to_write = num_bytes_decoded;
                    //check if the bytes already written to the value plus the new bytes exceed the total element size
                    //if it does, set to the size of the remainder of the element
                    if(index_bytes_decoded + bytes_to_write > element_size) {
                        bytes_to_write = element_size - index_bytes_decoded;
                    }
                    //start writing after any bytes that have already been written
                    uint8_t *write_start = (uint8_t *)value + index_bytes_decoded;
                    //copy the computed number of bytes to the output pointer
                    memcpy(write_start, decoded_bytes, bytes_to_write);
                    //add the written bytes to the index bytes that have been decoded
                    index_bytes_decoded += bytes_to_write;
                }
                //otherwise ignore this data and move on, subtracting the decoded bytes from the remaining bytes
                else {
                    bytes_to_index -= num_bytes_decoded;
                }

                oldcode = code;
                if(switchbits.find(table.size()) != switchbits.end()) {
                    bitvals = switchbits[table.size()];
                }
                code = next_code(bitcount, encoded, len_encoded, bitvals);
                bitcount += bitvals->bitw;
            }
            
            if(index_bytes_decoded < element_size) {
                return 2;
            }
            return 0;
            
        }

        private:
        struct Bitvals {
            uint8_t bitw;
            uint8_t shr;
            unsigned long mask;
        };

        struct Bitvals s255 = {9, 23, 4286578688UL};
        struct Bitvals s511 = {10, 22, 4290772992UL};
        struct Bitvals s1023 = {11, 21, 4292870144UL};
        struct Bitvals s2047 = {12, 20, 4293918720UL};

        map<int, struct Bitvals *> switchbits = {
            { 255, &s255 },
            { 511, &s511 },
            { 1023, &s1023},
            { 2047, &s2047 }
        };

        uint32_t next_code(int bitcount, uint8_t *encoded, int len_encoded, struct Bitvals *bitvals) {
            int start = bitcount / 8;
            uint32_t code = 0;
            int bytes_to_read = 4;
            int bytes_remaining = len_encoded - start;
            if(bytes_remaining < bytes_to_read) {
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
    };
}



