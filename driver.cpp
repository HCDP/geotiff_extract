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
    vector<char *> files(argc);

    int i;
    for(i = 0; i < argc; i++) {
        if(strcmp(argv[i], "-f")) {
            infile = argv[++i];
        }
        else if(strcmp(argv[i], "-i")) {
            int index = atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-r")) {
            pos.row = atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-c")) {
            pos.col = atoi(argv[++i]);
        }
        else {
            files.push_back(argv[i]);
        }
    }
    cout << "1" << endl;
    if((pos.col < 0 || pos.row < 0) && index < 0) {
        //error, print usage
    }

    fstream f;
    string fname;
    f.open(infile, std::fstream::in);
    while(getline(f, fname)) {
        char *fname_c = const_cast<char *>(fname.c_str());
        files.push_back(fname_c);
    }
    f.close();
cout << "2" << endl;
    float *values = (float *)malloc(sizeof(float) * files.size());

    if(pos.col < 0 || pos.row < 0) {
        cout << "3" << endl;
        #pragma omp parallel for
        for(i = 0; i < files.size(); i++) {
            cout << files.size() << endl;
            cout << files[i] << endl;
            int success = read_value(files[i], &pos, values[i]);

        }
    }
    else {
        #pragma omp parallel for
        for(i = 0; i < files.size(); i++) {
            int success = read_value(files[i], index, values[i]);
        }
    }

    for(i = 0; i < files.size(); i++) {
        cout << values[i] << " ";
    }

    free(values);

    return 0;
}