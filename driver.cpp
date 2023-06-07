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

struct result_data {
    int code;
    float value;
};

void print_usage() {
    cout << "Usage: tiffextract [-f input_file] [-i index] [-r row] [-c col] [-x x] [-y y] tiff1 tiff2 ..." << endl;
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
                results[i].code = reader.read(index, 4, &results[i].value, READ_VALUE);
            }
            catch(const exception &e) {
                #pragma omp critical
                cerr << "Error in reader for file: " << files[i] << ": " << e.what() << endl;
            }      
            if(results[i].code != 0) {
                results[i].value = 0;
            }
        }
    }
    else if(pos.col >= 0 && pos.row >= 0) {
        #pragma omp parallel for
        for(i = 0; i < files.size(); i++) {
            results[i].code = -1;
            try {
                Reader reader(const_cast<char *>(files[i].c_str()));
                results[i].code = reader.read(&pos, 4, &results[i].value, READ_VALUE);
            }
            catch(const exception &e) {
                #pragma omp critical
                cerr << "Error in reader for file: " << files[i] << ": " << e.what() << endl;
            }
            if(results[i].code != 0) {
                results[i].value = 0;
            }
        }
    }
    else if((coords.x < numeric_limits<double>::max() && coords.y < numeric_limits<double>::max())) {
        #pragma omp parallel for
        for(i = 0; i < files.size(); i++) {
            results[i].code = -1;
            try {
                Reader reader(const_cast<char *>(files[i].c_str()));
                results[i].code = reader.read(&coords, 4, &results[i].value, READ_VALUE);
            }
            catch(const exception &e) {
                #pragma omp critical
                cerr << "Error in reader for file: " << files[i] << ": " << e.what() << endl;
            }
            if(results[i].code != 0) {
                results[i].value = 0;
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
            cout << results[i].value << " ";
        }
        
    }

    free(results);

    return 0;
}