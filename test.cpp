// #include "reader.cpp"
// #include <boost/asio/thread_pool.hpp>
// #include <boost/asio/post.hpp>
#include <iostream>
#include <vector>
#include <omp.h>
// #include <future>
// #include "ctpl/ctpl.h"

#include <chrono>
using namespace std::chrono;

// g++ test.cpp  -L /usr/lib/ -lboost_system -lboost_thread -lpthread

using namespace std;

struct order {
    int index;
    long res;
};

int t(int i) {
    long k = 0;
    for(int j = 0; j < 1000000 * i; j++) {
        k += j;
    }
    return k;
}

// void runner(vector<future<int>> &res) {
//     ctpl::thread_pool p(4);
//     int i;
//     for(i = 0; i < 10; i++) {
//         res.push_back(p.push([i](int) {
//             return t(i);
//         }));
//     }
// }

int main(int argc, char **argv) {
    
    vector<struct order> res;
    
    // ctpl::thread_pool p(4);
    
    // runner(res);
    auto start = high_resolution_clock::now();
    int i;
    #pragma omp parallel for
    for(i = 0; i < 100; i++) {
        struct order o = {i, t(i)};
        #pragma omp critical
        cout << omp_get_num_threads() << endl;
        res.push_back(o);
    }

    for(i = 0; i < res.size(); i++) {
        cout << res[i].index << " ";
    }
    
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << endl << duration.count() << endl;
    return 0;
}
