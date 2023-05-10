#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "reader.cpp"

//g++ driver.cpp -fopenmp

using namespace std;

int main(int argc, char **argv) {
    if(argc < 3) {
        cout << "Usage: tiffextract tifffile { index | row col }" << endl;
        return 3;
    }

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
        //error, print usage
    }

    fstream f;
    string fname;
    f.open(infile, std::fstream::in);
    while(getline(f, fname)) {
        files.push_back(fname);
    }
    f.close();
    float *values = (float *)malloc(sizeof(float) * files.size());
    int *codes = (int *)malloc(sizeof(int) * files.size());

    if(index < 0) {
        #pragma omp parallel for
        for(i = 0; i < files.size(); i++) {
            int code = read_value(const_cast<char *>(files[i].c_str()), &pos, values[i]);
            codes[i] = code;
            if(code != 0) {
                values[i] = 0;
            }
        }
    }
    else {
        #pragma omp parallel for
        for(i = 0; i < files.size(); i++) {
            int code = read_value(const_cast<char *>(files[i].c_str()), index, values[i]);
            codes[i] = code;
            if(code != 0) {
                values[i] = 0;
            }
        }
    }

    for(i = 0; i < files.size(); i++) {
        if(codes[i] != 0) {
            cout << "_ ";
        }
        else {
            cout << values[i] << " ";
        }
        
    }

    free(values);
    free(codes);

    return 0;
}