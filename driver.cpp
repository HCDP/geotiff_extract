#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "reader.h"

//g++ driver.cpp -fopenmp

using namespace std;
using namespace GeotiffExtract;

struct result_data {
    int code;
    float value;
};

int main(int argc, char **argv) {
    struct row_col pos = {-1, -1};
    int index = -1;
    char *infile = nullptr;
    vector<string> files;

    int i;
    for(i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-f") == 0) {
            infile = argv[++i];
        }
        else if(strcmp(argv[i], "-i") == 0) {
            int index = atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-r") == 0) {
            pos.row = atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-c") == 0) {
            pos.col = atoi(argv[++i]);
        }
        else {
            files.push_back(argv[i]);
        }
    }
    if((pos.col < 0 || pos.row < 0) && index < 0) {
        cout << "Arguments must include either an index (-i index) or a row and column (-r row -c col)" << endl;
        cout << "Usage: geotiffextract [-f input_file] [-i index] [-r row] [-c col] tiff1 tiff2 ..." << endl;
        return 1;
    }
    else if(files.size() == 0 && infile == nullptr) {
        cout << "No input files provided. Please provide an input file containing a list of tiff files (-f input_file) or a set of tiff files" << endl;
        cout << "Usage: geotiffextract [-f input_file] [-i index] [-r row] [-c col] tiff1 tiff2 ..." << endl;
    }

    fstream f;
    string fname;
    f.open(infile, std::fstream::in);
    while(getline(f, fname)) {
        files.push_back(fname);
    }
    f.close();
    struct result_data *results = (struct result_data *)malloc(sizeof(result_data) * files.size());

    if(index < 0) {
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
    else {
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