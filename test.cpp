
#include <iostream>
#include <vector>
#include <limits>
#include <algorithm>
#include <cstring>

using namespace std;

/// Type used to store and retrieve codes.
using CodeType = uint32_t;

namespace globals {
    /// Dictionary Maximum Size (when reached, the dictionary will be reset)
    const CodeType dms {std::numeric_limits<CodeType>::max()};
} 

int decompress(uint8_t *, uint32_t *, int);

int decompress(uint8_t *encoded, uint32_t *value, int data_start, int encoded_size) {
    int bytes_to_out = 4;
    int bytes_to_decode = data_start + 4;
    std::vector<std::pair<CodeType, char>> dictionary;

    // "named" lambda function, used to reset the dictionary to its initial contents
    const auto reset_dictionary = [&dictionary] {
        dictionary.clear();
        dictionary.reserve(globals::dms);

        const long int minc = std::numeric_limits<char>::min();
        const long int maxc = std::numeric_limits<char>::max();

        for (long int c = minc; c <= maxc; ++c)
            dictionary.push_back({globals::dms, static_cast<char> (c)});
    };

    const auto rebuild_string = [&dictionary](CodeType k) -> const std::vector<char> * {
        static std::vector<char> s; // String

        s.clear();

        // the length of a string cannot exceed the dictionary's number of entries
        s.reserve(globals::dms);

        while (k != globals::dms)
        {
            s.push_back(dictionary[k].second);
            k = dictionary[k].first;
        }

        std::reverse(s.begin(), s.end());
        return &s;
    };

    printf("reset\n");
    reset_dictionary();
    printf("after_reset\n");

    CodeType i {globals::dms}; // Index
    CodeType k; // Key
    int written = 0;
    int read = 0;

    //read sizeof CodeType at a time into key
    int read_size = sizeof(CodeType);
    CodeType *read_pos = (CodeType *)encoded;

    //while (is.read(reinterpret_cast<char *> (&k), sizeof (CodeType)))
    while(bytes_to_out > 0) {
        //make sure not overrunning encoded buffer, if it is then encoding is invalid
        if(read + read_size > encoded_size) {
            return 1;
        }
        //read bytes into k
        memcpy(&k, read_pos, read_size);
        k = *read_pos;
        read_pos += read_size;
        read += read_size;

        // dictionary's maximum size was reached
        if (dictionary.size() == globals::dms) {
            reset_dictionary();
        }
    

        if(k > dictionary.size()) {
            return 1;
        }

        const std::vector<char> *s; // String

        if (k == dictionary.size()) {
            dictionary.push_back({i, rebuild_string(i)->front()});
            s = rebuild_string(k);
        }
        else {
            s = rebuild_string(k);

            if (i != globals::dms)
                dictionary.push_back({i, s->front()});
        }
        int write_end = written + s->size();
        //
        if(write_end > data_start) {
            int offset = data_start - written;
            int to_write = s->size() - offset;
            if(to_write > bytes_to_out) {
                to_write = bytes_to_out;
            }
            const char *start = &s->front() + offset;
            memcpy(value, start, to_write);
            data_start += to_write;
            bytes_to_out -= to_write;
        }
        written = write_end;
        i = k;
    }
    return 0;
}