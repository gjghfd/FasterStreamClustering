#ifndef UTILS_H
#define UTILS_H
// general_includes
#include <cstdlib>
#include <cstring>
#include <vector>
#include <ctime>
#include <string>
#include <unordered_map>
#include <cstdio>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <chrono>
#include "HC/BOBHash.h"
using namespace std;

#define TUPLE_LEN 4

class FIVE_TUPLE {
public:
    char key[TUPLE_LEN];
    FIVE_TUPLE () {}
    FIVE_TUPLE (const int a) {
        *((int *) key) = a;
    }
    FIVE_TUPLE (const char* s){
        for (int i = 0; i < TUPLE_LEN; i++){
            key[i] = s[i];
        }
    }

    bool operator == (const FIVE_TUPLE& other) const {
        return memcmp(key, other.key, TUPLE_LEN) == 0;
    }
};

class Hash_Fun{
    static BOBHash general_hash;
public:
    Hash_Fun() {
        general_hash = BOBHash(rand() % MAX_PRIME);
    }
    size_t operator () (const FIVE_TUPLE& tuple) const {
        return general_hash.run(tuple.key, TUPLE_LEN);
    }
};

class FLOW_ITEM{
public:
    FIVE_TUPLE key;
    int freq;

    bool operator < (const FLOW_ITEM& other) const {
        return freq < other.freq;
    }

    FLOW_ITEM () {}
    FLOW_ITEM (const FIVE_TUPLE& _key, int _freq){
        key = _key;
        freq = _freq;
    }
};

static bool cmp_item(const FLOW_ITEM& cur, const FLOW_ITEM& other){
	return other < cur;
}

static vector<uint8_t> cs_tower = {2, 4, 8, 16, 32};
static vector<double> width_mul_tower = {0.8, 0.4, 0.2, 0.1, 0.05};

#endif