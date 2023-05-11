#include <iostream>
#include <stdio.h>
#include "decode.h"

namespace GeotiffExtract {
    struct row_col {
        int row;
        int col;
    };

    class file_open_error : public exception {
        private:
        string message;

        public:
        file_open_error(string msg) : message(msg) {}
        string what () {
            return message;
        }
    };

    class invalid_file_error : public exception {
        private:
        string message;

        public:
        invalid_file_error(string msg) : message(msg) {}
        string what () {
            return message;
        }
    };

    class Reader {
        public:
        Reader(char *fname) { 
            f = fopen(fname, "rb");
            if(f == NULL) {
                throw file_open_error("Could not open file");
            }
            uint8_t *buffer = (uint8_t *)malloc(12);
            fread(buffer, 10, 1, f);
            //only supports little endian (shouldn't need big endian implementation for our purposes)
            //implementation also assumes host system is little endian

            //should throw errors instead of returning
            if(buffer[0] != 'I') {
                throw invalid_file_error("File must be little endian.");
            }
            uint16_t magic = *(uint16_t *)(buffer + 2);
            //validate file
            if(magic != 42) {
                throw invalid_file_error("File is not a valid TIFF format.");
            }
            
            uint32_t ifd_offset = *(uint32_t *)(buffer + 4);
            fseek(f, ifd_offset, SEEK_SET);
            fread(buffer, 2, 1, f);
            uint16_t num_entries = *(uint16_t *)buffer;


            int16_t field_tag;
            //garenteed to be offset for needed values for handled tiffs
            //note if value is less than or equal to 4 bytes this will be the value itself
            uint32_t strip_offset_offset;
            uint32_t strip_byte_count_offset;
            //get map width, height, offset of strip offset block, and offset of strip byte count block
            int tags = 4;
            for(int i = 0; i < num_entries && tags > 0; i++) {
                fread(buffer, 12, 1, f);
                field_tag = *(uint16_t *)buffer;
                //map width
                if(field_tag == 256) {
                    width = *(uint16_t *)(buffer + 8); 
                    tags--;
                }
                //map height
                else if(field_tag == 257) {
                    height = *(uint16_t *)(buffer + 8); 
                    tags--;
                }
                //strip offsets
                else if(field_tag == 273) {
                    strip_offset_block_offset = *(uint32_t *)(buffer + 8);
                    tags--;
                }
                //strip byte counts
                else if(field_tag == 279) {
                    strip_byte_count_block_offset = *(uint32_t *)(buffer + 8);
                    tags--;
                }
            }
        }

        ~Reader() {
            fclose(f);
        }

        int read_value(struct row_col *pos, float &value) {
            //position out of range of map
            if(pos->col > width || pos->row > height) {
                throw out_of_range("Position provided is outside of the map range.");
            }
            //get strip location and number of bytes
            struct strip_data strip_data;
            get_strip_data(pos, &strip_data);
            //create buffer to get encoded data
            uint8_t *buffer = (uint8_t *)malloc(strip_data.byte_count);
            //get encoded strip
            fseek(f, strip_data.offset, SEEK_SET);
            fread(buffer, strip_data.byte_count, 1, f);
            //create strip decoder
            Decoder decoder(buffer, strip_data.byte_count);
            //get col index from strip
            int success = decoder.get_index(pos->col, sizeof(float), &value);
            //free buffer memory
            free(buffer);
            return success;
        }

        int read_value(int index, float &value) {
            //convert index into row and col
            struct row_col pos;
            get_row_col(index, &pos);
            //call overload
            return read_value(&pos, value);
        }

        private:
        FILE *f;
        uint16_t width;
        uint16_t height;
        uint32_t strip_offset_block_offset;
        uint32_t strip_byte_count_block_offset;

        struct strip_data {
            uint32_t offset;
            uint32_t byte_count;
        };

        void get_strip_data(struct row_col *pos, struct strip_data *strip_data) {
            //assumes 4 byte vals (both should be)
            int index_offset = 4 * pos->row;
            //get strip offset offset and byte count offset
            uint32_t strip_offset_offset = strip_offset_block_offset + index_offset;
            uint32_t strip_byte_count_offset = strip_byte_count_block_offset + index_offset;
            //get strip offset
            fseek(f, strip_offset_offset, SEEK_SET);
            fread(&strip_data->offset, 4, 1, f);
            //get strip size
            fseek(f, strip_byte_count_offset, SEEK_SET);
            fread(&strip_data->byte_count, 4, 1, f);
        }

        void get_row_col(int index, struct row_col *pos) {
            pos->row = index / width;
            pos->col = index % width;
        }
    };
}





