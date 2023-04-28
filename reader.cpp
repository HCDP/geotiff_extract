#include <iostream>
#include <stdio.h>
#include "decode.cpp"

void get_row_col(int, int, struct row_col *);
int read_index(char *, int);

struct row_col {
    int row;
    int col;
};

void get_row_col(int index, int width, struct row_col *pos) {
    pos->row = index / width;
    pos->col = index % width;
}

int read_index(char *fname, int index) {
    FILE *f;
    f = fopen(fname, "rb");
    uint8_t *buffer = (uint8_t *)malloc(12);
    fread(buffer, 10, 1, f);
    printf("%.2s\n", buffer);
    uint16_t magic = *(uint16_t *)(buffer + 2);
    //validate file
    if(magic != 42) {
        return 1;
    }
    struct row_col pos;
    //hardcode width for efficiency, tag 256 if need to get from header
    get_row_col(index, 2288, &pos);
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
    int index_offset = 4 * pos.row;
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
    //need a new buffer of strip size, just free old buffer and create new allocation
    free(buffer);
    buffer = (uint8_t *)malloc(strip_byte_count);
    fseek(f, strip_offset, SEEK_SET);
    fread(buffer, strip_byte_count, 1, f);

    //uint32_t value;
    int data_start = 4 * pos.col;
    
    // //temp read to vectors for testing, then can optimize if works
    // vector<uint8_t> encoded(buffer, buffer + strip_byte_count);
    uint8_t *decoded = (uint8_t *)malloc(10000);
    float value = 0;
    //uint8_t *encoded, int len_encoded, int index, int element_size, void *value
    

    //int success = decode(buffer, decoded, strip_byte_count);
    //value = *((float *)decoded);

    int success = get_index(buffer, strip_byte_count, 0, sizeof(float), &value);

    cout << "after decode" << endl;
    if(success == 0) {
        cout << hex;
        for(int i = 0; i < sizeof(float); i++) {
            cout << (int)*((uint8_t *)&value + i) << endl;
        }
        cout << dec;
        cout << "success" << endl;
        //test element 0 for now, note need to change to col
        //vector values stored like array, can just get pos
        //4 byte value pointer, deref to float value
        //float value = *(float *)(decoded + 800);
        cout << value << endl;
        // if(decode(buffer, &value, data_start, strip_byte_count)) {
        //     printf("success!?!\n");
        // }
        
    }
    else {
        cout << "failure" << endl;
    }
    cout << "before free decode" << endl;
    free(decoded);
    cout << "after free decode" << endl;


    free(buffer);
    fclose(f);
    return 0;
}

int main(int argc, char **argv) {
    char *fname = (char *)"rainfall_new_month_statewide_data_map_2022_01.tif";
    int index = 0;
    return read_index(fname, index);
}