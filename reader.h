#ifndef _TIFFEXTRACT_READER_H
#define _TIFFEXTRACT_READER_H

#include <iostream>
#include <stdio.h>
#include "decode.h"

namespace TIFFExtract {
    enum ReadType {
        READ_VALUE = 0,
        READ_STRIP = 1
    };

    enum sample_type {
        TYPE_UINT = 1,
        TYPE_INT = 2,
        TYPE_IEEEF = 3,
        TYPE_UNDEFINED = 4
    };

    struct row_col {
        int row;
        int col;
    };

    struct coords {
        double x;
        double y;
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

    class not_supported_error : public exception {
        private:
        string message;

        public:
        not_supported_error(string msg) : message(msg) {}
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
                throw not_supported_error("File must be little endian.");
            }
            uint16_t magic = *(uint16_t *)(buffer + 2);
            //validate file
            if(magic != 42) {
                throw not_supported_error("File is not a valid TIFF format.");
            }
            uint32_t ifd_offset = *(uint32_t *)(buffer + 4);
            fseek(f, ifd_offset, SEEK_SET);
            fread(buffer, 2, 1, f);
            uint16_t num_entries = *(uint16_t *)buffer;


            uint16_t field_tag;
            //garenteed to be offset for needed values for handled tiffs
            //note if value is less than or equal to 4 bytes this will be the value itself
            uint32_t scale_block_offset;
            uint32_t tiepoint_block_offset;
            //get map width, height, offset of strip offset block, and offset of strip byte count block
            int tags = 10;
            for(int i = 0; i < num_entries && tags > 0; i++) {
                fread(buffer, 12, 1, f);
                field_tag = *(uint16_t *)buffer;
                switch(field_tag) {
                    //map width
                    case 256: {
                        _width = *(uint16_t *)(buffer + 8); 
                        tags--;
                        break;
                    }
                    //map height
                    case 257: {
                        _height = *(uint16_t *)(buffer + 8); 
                        tags--;
                        break;
                    }
                    //bits per value (sample)
                    case 258: {
                        _bytes_per_sample = *((uint16_t *)(buffer + 8)) / 8;
                        tags--;
                        break;
                    }
                    //compression
                    case 259: {
                        _compression = *(uint16_t *)(buffer + 8); 
                        if(_compression != COMPRESSION_NONE && _compression != COMPRESSION_LZW) {
                            not_supported_error("The file's compression is not supported. File must be uncompressed or use LZW compression");
                        }
                        tags--;
                        break;
                    }
                    //strip offsets
                    case 273: {
                        strip_offset_block_offset = *(uint32_t *)(buffer + 8);
                        tags--;
                        break;
                    }
                    //rows per strip
                    case 278: {
                        _rows_per_strip = *(uint16_t *)(buffer + 8); 
                        tags--;
                        break;
                    }
                    //strip byte counts
                    case 279: {
                        strip_byte_count_block_offset = *(uint32_t *)(buffer + 8);
                        tags--;
                        break;
                    }
                    //data type
                    case 339: {
                        _data_type = *(uint16_t *)(buffer + 8);
                        tags--;
                        break;
                    }
                    case 33550: {
                        scale_block_offset = *(uint32_t *)(buffer + 8);
                        tags--;
                        break;
                    }
                    case 33922: {
                        tiepoint_block_offset = *(uint32_t *)(buffer + 8);
                        tags--;
                        break;
                    }
                }
            }
            
            //tiepoint is i,j,k,x,y,z, want x and y (8 byte double values)
            //note assumes one tiepoint representing upper left corner, start at 4th value (+24 bytes)
            tiepoint_block_offset += 24;
            double scale_b[2];
            double tiepoint_b[2];
            //get scale data
            fseek(f, scale_block_offset, SEEK_SET);
            fread(scale_b, 8, 2, f);
            _scale.x = scale_b[0];
            _scale.y = scale_b[1];
            //get tiepoint data
            fseek(f, tiepoint_block_offset, SEEK_SET);
            fread(tiepoint_b, 8, 2, f);
            _tiepoint.x = tiepoint_b[0];
            _tiepoint.y = tiepoint_b[1];
        }

        ~Reader() {
            fclose(f);
        }

        int read(struct row_col *pos, size_t buffer_size, void *buffer, ReadType type) {
            //position out of range of map
            if(pos->col >= _width || pos->row >= _height) {
                throw out_of_range("Position provided is outside of the map range.");
            }
            //copy position to strip position
            row_col strip_pos = *pos;
            //update row and col to strip and col based on rows per strip
            strip_pos.col += (strip_pos.row % _rows_per_strip) * _width;
            strip_pos.row /= _rows_per_strip;
            //get strip location and number of bytes
            struct strip_data strip_data;
            get_strip_data(&strip_pos, &strip_data);
            int success = -1;
            switch(_compression) {
                case COMPRESSION_NONE: {
                    success = handle_uncompressed(&strip_pos, &strip_data, buffer_size, buffer, type);
                    break;
                }
                case COMPRESSION_LZW: {
                    success = handle_lzw(&strip_pos, &strip_data, buffer_size, buffer, type);
                    break;
                }
            }
            return success;
        }

        int read(int index, size_t buffer_size, void *buffer, ReadType type) {
            //convert index into row and col
            struct row_col pos;
            get_row_col(index, &pos);
            //call overload
            return read(&pos, buffer_size, buffer, type);
        }

        int read(struct coords *coords, size_t buffer_size, void *buffer, ReadType type) {
            //convert coords into row and col
            struct row_col pos;
            coords_to_pos(coords, &pos);
            return read(&pos, buffer_size, buffer, type);
        }

        uint16_t bytes_per_sample() {
            return _bytes_per_sample;
        }

        uint16_t compression() {
            return _compression;
        }
        
        uint16_t data_type() {
            return _data_type;
        }

        uint16_t width() {
            return _width;
        }

        uint16_t height() {
            return _height;
        }

        struct coords tiepoint() {
            return _tiepoint;
        }

        struct coords scale() {
            return _scale;
        }

        private:
        FILE *f;
        uint16_t _width;
        uint16_t _height;
        uint32_t strip_offset_block_offset;
        uint32_t strip_byte_count_block_offset;
        uint16_t _rows_per_strip;
        uint16_t _compression;
        uint16_t _data_type;
        uint16_t _bytes_per_sample;
        struct coords _tiepoint;
        struct coords _scale;

        enum compression_code {
            COMPRESSION_NONE = 1,
            COMPRESSION_CCITTRLE = 2,
            COMPRESSION_CCITTFAX3 = 3,
            COMPRESSION_CCITT_T4 = 3,
            COMPRESSION_CCITTFAX4 = 4,
            COMPRESSION_CCITT_T6 = 4,
            COMPRESSION_LZW = 5,
            COMPRESSION_OJPEG = 6,
            COMPRESSION_JPEG = 7,
            COMPRESSION_NEXT = 32766,
            COMPRESSION_CCITTRLEW = 32771,
            COMPRESSION_PACKBITS = 32773,
            COMPRESSION_THUNDERSCAN = 32809,
            COMPRESSION_IT8CTPAD = 32895,
            COMPRESSION_IT8LW = 32896,
            COMPRESSION_IT8MP = 32897,
            COMPRESSION_IT8BL = 32898,
            COMPRESSION_PIXARFILM = 32908,
            COMPRESSION_PIXARLOG = 32909,
            COMPRESSION_DEFLATE = 32946,
            COMPRESSION_ADOBE_DEFLATE = 8,
            COMPRESSION_DCS = 32947,
            COMPRESSION_JBIG = 34661,
            COMPRESSION_SGILOG = 34676,
            COMPRESSION_SGILOG24 = 34677,
            COMPRESSION_JP2000 = 34712
        };

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
            pos->row = index / _width;
            pos->col = index % _width;
        }

        int handle_lzw(struct row_col *pos, struct strip_data *strip_data, size_t buffer_size, void *buffer, ReadType type) {
            //create buffer to get encoded data
            uint8_t *data = (uint8_t *)malloc(strip_data->byte_count);
            //get encoded strip
            fseek(f, strip_data->offset, SEEK_SET);
            fread(data, strip_data->byte_count, 1, f);
            //create strip decoder
            Decoder decoder(data, strip_data->byte_count);
            int success = -1;
            switch(type) {
                case READ_VALUE: {
                    size_t bytes_to_read = _bytes_per_sample;
                    if(bytes_to_read > buffer_size) {
                        bytes_to_read = buffer_size;
                    }
                    success = decoder.get_index(pos->col, bytes_to_read, buffer);
                    break;
                }
                case READ_STRIP: {
                    size_t bytes_to_read = strip_data->byte_count;
                    if(bytes_to_read > buffer_size) {
                        bytes_to_read = buffer_size;
                    }
                    success = decoder.decode(bytes_to_read, (uint8_t *)buffer);
                    break;
                }
            }
            //free buffer memory
            free(data);
            return success;
        }

        int handle_uncompressed(struct row_col *pos, struct strip_data *strip_data, size_t buffer_size, void *buffer, ReadType type) {
            size_t bytes_to_read;
            uint32_t offset;
            switch(type) {
                case READ_VALUE: {
                    bytes_to_read = _bytes_per_sample;
                    if(bytes_to_read > buffer_size) {
                        bytes_to_read = buffer_size;
                    }
                    offset = strip_data->offset + _bytes_per_sample * pos->col;
                    break;
                }
                case READ_STRIP: {
                    bytes_to_read = strip_data->byte_count;
                    if(bytes_to_read > buffer_size) {
                        bytes_to_read = buffer_size;
                    }
                    offset = strip_data->offset;
                    break;
                }
            }
            fseek(f, offset, SEEK_SET);
            fread(buffer, bytes_to_read, 1, f);
            
            return 0;
        }

        void coords_to_pos(struct coords *coords, struct row_col *pos) {
            //note tiepoint is upper left not lower left
            struct coords offset = { coords->x - _tiepoint.x, _tiepoint.y - coords->y };
            pos->col = offset.x / _scale.x;
            pos->row = offset.y / _scale.y;
        }
    };
}

#endif