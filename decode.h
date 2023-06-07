#ifndef _TIFFEXTRACT_DECODE_H
#define _TIFFEXTRACT_DECODE_H

#include <iostream>
#include <vector>
#include <cstring>
#include <unordered_map>

using namespace std;

namespace TIFFExtract {
    class Decoder {
        public:
        uint8_t *encoded;
        int len_encoded;
        
        Decoder(uint8_t *encoded, int len_encoded) {
            this->encoded = encoded;
            this->len_encoded = len_encoded;
        }

        int decode(size_t decode_size, uint8_t *decoded) {
            int bitcount_max = len_encoded * 8;
            //start table with 258 code vectors (0-255 + padding for clear and exit codes), note index 256/257 value does not matter
            vector<vector<uint8_t>> table(258);
            //reserve max table size so vector won't have to be resized when adding codes
            table.reserve(4096);
            int i;
            for(i = 0; i < 258; i++) {
                table[i].push_back((uint8_t)i);
            }

            struct Bitvals *bitvals = switchbits[255];

            uint32_t code = next_code(0, encoded, len_encoded, bitvals);
            //encoded must start with clear code (256)
            if(code != 256) {
                return 1;
            }
            
            int bitcount = bitvals->bitw;

            uint32_t oldcode = 0;
            int bytes_written = 0;

            while(code != 257 && bitcount < bitcount_max && bytes_written < decode_size) {
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
                //code in table
                else if(code < table.size()) {
                    //add old code translation to end of tab
                    table.push_back(table[oldcode]);
                    //append first byte of code translation to the end of the new code
                    table[table.size() - 1].push_back(table[code][0]);
                    //get the code translation's size and data to write to decode sequence
                    num_bytes_decoded = table[code].size();
                    decoded_bytes = table[code].data();
                }
                //code not in table
                else {
                    //add old code translation to end of tab
                    table.push_back(table[oldcode]);
                    int new_code_index = table.size() - 1;
                    //append first byte of new code to the end of new code
                    table[new_code_index].push_back(table[new_code_index][0]);
                    //get the size and data for new code for decoded sequence
                    num_bytes_decoded = table[new_code_index].size();
                    decoded_bytes = table[new_code_index].data();
                }

                if(bytes_written + num_bytes_decoded > decode_size) {
                    num_bytes_decoded = decode_size - bytes_written;
                }
                memcpy(decoded + bytes_written, decoded_bytes, num_bytes_decoded);
                bytes_written += num_bytes_decoded;

                oldcode = code;
                if(switchbits.find(table.size()) != switchbits.end()) {
                    bitvals = switchbits[table.size()];
                }
                code = next_code(bitcount, encoded, len_encoded, bitvals);
                bitcount += bitvals->bitw;
            }

            return 0;
        }

        int get_index(int index, size_t element_size, void *value) {
            int bytes_total = 0;
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

            uint16_t code = next_code(0, encoded, len_encoded, bitvals);
            //encoded must have a minimum length of 4 bytes and start with clear code (256)
            if(code != 256) {
                return 1;
            }
            
            int bitcount = (int)bitvals->bitw;

            uint16_t oldcode = 0;

            int num_bytes_decoded;
            uint8_t *decoded_bytes;
            while(code != 257 && bitcount < bitcount_max && index_bytes_decoded < element_size) {
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
                //code in table
                else if(code < table.size()) {
                    //add old code translation to end of tab
                    table.push_back(table[oldcode]);
                    //append first byte of code translation to the end of the new code
                    table[table.size() - 1].push_back(table[code][0]);
                    //get the code translation's size and data to write to decode sequence
                    num_bytes_decoded = table[code].size();
                    decoded_bytes = table[code].data();
                }
                //code not in table
                else {
                    //add old code translation to end of tab
                    table.push_back(table[oldcode]);
                    int new_code_index = table.size() - 1;
                    //append first byte of new code to the end of new code
                    table[new_code_index].push_back(table[new_code_index][0]);
                    //get the size and data for new code for decoded sequence
                    num_bytes_decoded = table[new_code_index].size();
                    decoded_bytes = table[new_code_index].data();
                }

                bytes_total += num_bytes_decoded;

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
            //bit width
            uint8_t bitw;
            //shift right (16 - bitw)
            uint8_t shr;
            //mask over bitw bits
            uint32_t mask;
        };

        //tiff encoding max length of 12 bits
        struct Bitvals s255 = {9, 23, 4286578688};
        struct Bitvals s511 = {10, 22, 4290772992};
        struct Bitvals s1023 = {11, 21, 4292870144};
        struct Bitvals s2047 = {12, 20, 4293918720};

        unordered_map<int, struct Bitvals *> switchbits = {
            { 255, &s255 },
            { 511, &s511 },
            { 1023, &s1023},
            { 2047, &s2047 }
        };

        uint16_t next_code(int bitcount, uint8_t *encoded, int len_encoded, struct Bitvals *bitvals) {
            int start = bitcount / 8;
            uint32_t code = 0;
            //4 byte code but only need to read 3 bytes (shift is a max of 7 bits with 12 bit max code length, fourth byte unused)
            int bytes_to_read = 3;
            int bytes_remaining = len_encoded - start;
            //make sure not reading past the end of encoded sequence
            if(bytes_remaining < bytes_to_read) {
                bytes_to_read = bytes_remaining;
            }
            //read bytes in order (memcpy or pointer cast will reverse assuming machine byte order is little endian)
            for(int i = 0; i < bytes_to_read; i++) {
                int shift = 8 * (3 - i);
                code |= encoded[start + i] << shift;
            }
            code <<= bitcount % 8;
            code &= bitvals->mask;
            code >>= bitvals->shr;
            //code will always fit in 2 bytes (12 bit max)
            return (uint16_t)code;
        }
    };
}

#endif