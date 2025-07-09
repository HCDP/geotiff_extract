#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include "reader.h"

//g++ driver.cpp -fopenmp

using namespace std;
using namespace TIFFExtract;

//use two flags to determine which one, type and num bytes
union result_value {
    float value_f;
    double value_d;
    uint8_t value_u8;
    uint16_t value_u16;
    uint32_t value_u32;
    uint64_t value_u64;
    int8_t value_8;
    int16_t value_16;
    int32_t value_32;
    int64_t value_64;
};

struct result_data {
    int code;
    uint16_t value_type;
    result_value value;
};


void print_usage() {
    cout << "Usage: tiffextract.out [-f input_file] [-i index] [-r row] [-c col] [-x x] [-y y] tiff1 tiff2 ..." << endl;
}

void print_value(uint16_t value_type, result_value value) {
    switch(value_type) {
        case 4 | TYPE_IEEEF << 8: {
            cout << value.value_f << " ";
            break;
        }
        case 8 | TYPE_IEEEF << 8: {
            cout << value.value_d << " ";
            break;
        }
        case 1 | TYPE_UINT << 8: {
            cout << value.value_u8 << " ";
            break;
        }
        case 2 | TYPE_UINT << 8: {
            cout << value.value_u16 << " ";
            break;
        }
        case 4 | TYPE_UINT << 8: {
            cout << value.value_u32 << " ";
            break;
        }
        case 8 | TYPE_UINT << 8: {
            cout << value.value_u64 << " ";
            break;
        }
        case 1 | TYPE_INT << 8: {
            cout << value.value_8 << " ";
            break;
        }
        case 2 | TYPE_INT << 8: {
            cout << value.value_16 << " ";
            break;
        }
        case 4 | TYPE_INT << 8: {
            cout << value.value_32 << " ";
            break;
        }
        case 8 | TYPE_INT >> 8: {
            cout << value.value_64 << " ";
            break;
        }
        default: {
            //could not handle data type, just print _
            cout << "_ ";
        }
    }
}

int main(int argc, char **argv) {
    struct row_col pos = {-1, -1};
    struct coords coords = {numeric_limits<double>::max(), numeric_limits<double>::max()};
    int index = -1;
    char *infile = nullptr;
    vector<string> files;

    int i;
    for(i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-f") == 0) {
            infile = argv[++i];
        }
        else if(strcmp(argv[i], "-i") == 0) {
            index = atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-r") == 0) {
            pos.row = atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-c") == 0) {
            pos.col = atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-x") == 0) {
            coords.x = stod(argv[++i]);
        }
        else if(strcmp(argv[i], "-y") == 0) {
            coords.y = stod(argv[++i]);
        }
        else {
            files.push_back(argv[i]);
        }
    }

    if(files.size() == 0 && infile == nullptr) {
        cout << "No input files provided. Please provide an input file containing a list of tiff files (-f input_file) or a set of tiff files" << endl;
        print_usage();
        return 1;
    }

    fstream f;
    string fname;
    f.open(infile, std::fstream::in);
    while(getline(f, fname)) {
        files.push_back(fname);
    }
    f.close();
    struct result_data *results = (struct result_data *)malloc(sizeof(result_data) * files.size());

    if(index >= 0) {
        #pragma omp parallel for
        for(i = 0; i < files.size(); i++) {
            results[i].code = -1;
            try {
                Reader reader(const_cast<char *>(files[i].c_str()));
                //encode value type
                results[i].value_type = reader.bytes_per_sample() | reader.data_type() << 8;
                results[i].code = reader.read(index, sizeof(result_data), &results[i].value, READ_VALUE);
            }
            catch(const exception &e) {
                #pragma omp critical
                cerr << "Error in reader for file: " << files[i] << ": " << e.what() << endl;
            }
        }
    }
    else if(pos.col >= 0 && pos.row >= 0) {
        #pragma omp parallel for
        for(i = 0; i < files.size(); i++) {
            results[i].code = -1;
            try {
                Reader reader(const_cast<char *>(files[i].c_str()));
                //encode value type
                results[i].value_type = reader.bytes_per_sample() | reader.data_type() << 8;
                results[i].code = reader.read(&pos, sizeof(result_data), &results[i].value, READ_VALUE);
            }
            catch(const exception &e) {
                #pragma omp critical
                cerr << "Error in reader for file: " << files[i] << ": " << e.what() << endl;
            }
        }
    }
    else if((coords.x < numeric_limits<double>::max() && coords.y < numeric_limits<double>::max())) {
        #pragma omp parallel for
        for(i = 0; i < files.size(); i++) {
            results[i].code = -1;
            try {
                Reader reader(const_cast<char *>(files[i].c_str()));
                //encode value type
                results[i].value_type = reader.bytes_per_sample() | reader.data_type() << 8;
                results[i].code = reader.read(&coords, sizeof(result_data), &results[i].value, READ_VALUE);
            }
            catch(const exception &e) {
                #pragma omp critical
                cerr << "Error in reader for file: " << files[i] << ": " << e.what() << endl;
            }
        }
    }
    else {
        cout << "Arguments must include either an index (-i index), a row and column (-r row -c col), or x and y coordinates (-x x axis (lng) -y y axis (lat))" << endl;
        print_usage();
        return 1;
    }

    for(i = 0; i < files.size(); i++) {
        if(results[i].code != 0) {
            cout << "_ ";
        }
        else {
            print_value(results[i].value_type, results[i].value);
        }
        
    }
    free(results);
    return 0;
}