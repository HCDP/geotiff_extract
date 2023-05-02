#include <iostream>
#include <stdio.h>
#include "decode.cpp"

void get_row_col(int, int, struct row_col *);
int read_value(char *, struct row_col *, float &);
int read_value(char *, int, float &);

struct row_col {
    int row;
    int col;
};

void get_row_col(int index, int width, struct row_col *pos) {
    pos->row = index / width;
    pos->col = index % width;
}

int read_value(char *fname, struct row_col *pos, float &value) {
    FILE *f;
    f = fopen(fname, "rb");
    uint8_t *buffer = (uint8_t *)malloc(12);
    fread(buffer, 10, 1, f);
    //only supports little endian (shouldn't need big endian implementation for our purposes)
    //implementation also assumes host system is little endian
    if(buffer[0] != 'I') {
        return 12;
    }
    uint16_t magic = *(uint16_t *)(buffer + 2);
    //validate file
    if(magic != 42) {
        return 1;
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
    
    //assumes 4 byte vals (both tags should be)
    int index_offset = 4 * pos->row;
    int tags = 2;
    for(int i = 0; i < num_entries && tags > 0; i++) {
        fread(buffer, 12, 1, f);
        field_tag = *(uint16_t *)buffer;
        //strip offsets
        if(field_tag == 273) {
            strip_offset_offset = *(uint32_t *)(buffer + 8) + index_offset;
            tags--;
        }
        //strip byte counts
        else if(field_tag == 279) {
            strip_byte_count_offset = *(uint32_t *)(buffer + 8) + index_offset;
            tags--;
        }
    }

    //get strip offset
    fseek(f, strip_offset_offset, SEEK_SET);
    fread(buffer, 4, 1, f);
    uint32_t strip_offset = *(uint32_t *)buffer;
    //get strip size
    fseek(f, strip_byte_count_offset, SEEK_SET);
    fread(buffer, 4, 1, f);
    uint32_t strip_byte_count = *(uint32_t *)buffer;
    //get strip
    //need a new buffer of strip size, just free old buffer and create new allocation since don't need data
    free(buffer);
    buffer = (uint8_t *)malloc(strip_byte_count);
    fseek(f, strip_offset, SEEK_SET);
    fread(buffer, strip_byte_count, 1, f);
    //done with file, close
    fclose(f);
    int success = get_index(buffer, strip_byte_count, pos->col, sizeof(float), &value);
    free(buffer);
    return success;
}

int read_value(char *fname, int index, float &value) {
    cout << index << endl;
    FILE *f;
    f = fopen(fname, "rb");
    uint8_t *buffer = (uint8_t *)malloc(12);
    fread(buffer, 10, 1, f);
    //only supports little endian (shouldn't need big endian implementation for our purposes)
    //implementation also assumes host system is little endian
    if(buffer[0] != 'I') {
        return 12;
    }
    uint16_t magic = *(uint16_t *)(buffer + 2);
    //validate file
    if(magic != 42) {
        return 1;
    }
    struct row_col pos;
    
    uint32_t ifd_offset = *(uint32_t *)(buffer + 4);
    fseek(f, ifd_offset, SEEK_SET);
    fread(buffer, 2, 1, f);
    uint16_t num_entries = *(uint16_t *)buffer;
    int16_t field_tag;
    //garenteed to be offset for needed values for handled tiffs
    //note if value is less than or equal to 4 bytes this will be the value itself
    uint32_t strip_offset_offset;
    uint32_t strip_byte_count_offset;
    
    int tags = 3;
    for(int i = 0; i < num_entries && tags > 0; i++) {
        fread(buffer, 12, 1, f);
        field_tag = *(uint16_t *)buffer;
        //map width
        if(field_tag == 256) {
            get_row_col(index, *(uint16_t *)(buffer + 8), &pos);     
            tags--;
        }
        //strip offsets
        else if(field_tag == 273) {
            strip_offset_offset = *(uint32_t *)(buffer + 8);
            tags--;
        }
        //strip byte counts
        else if(field_tag == 279) {
            strip_byte_count_offset = *(uint32_t *)(buffer + 8);
            tags--;
        }
    }
    //assumes 4 byte vals (both tags should be)
    int index_offset = 4 * pos.row;
    strip_byte_count_offset += index_offset;
    strip_offset_offset+= index_offset;

    //get strip offset
    fseek(f, strip_offset_offset, SEEK_SET);
    fread(buffer, 4, 1, f);
    uint32_t strip_offset = *(uint32_t *)buffer;
    //get strip size
    fseek(f, strip_byte_count_offset, SEEK_SET);
    fread(buffer, 4, 1, f);
    uint32_t strip_byte_count = *(uint32_t *)buffer;
    //get strip
    //need a new buffer of strip size, just free old buffer and create new allocation since don't need data
    free(buffer);
    buffer = (uint8_t *)malloc(strip_byte_count);
    fseek(f, strip_offset, SEEK_SET);
    fread(buffer, strip_byte_count, 1, f);
    //done with file, close
    fclose(f);
    int success = get_index(buffer, strip_byte_count, pos.col, sizeof(float), &value);
    free(buffer);
    return success;
}

int main(int argc, char **argv) {
    if(argc < 3) {
        cout << "Usage: tiffextract tifffile { index | row col }" << endl;
        return 3;
    }

    int success;
    float value;
    if(argc == 3) {
        success = read_value(argv[1], atoi(argv[2]), value);
    }
    else {
        struct row_col pos = {atoi(argv[2]), atoi(argv[3])};
        success = read_value(argv[1], &pos, value);
    }
    cout << value;
    return success;
}