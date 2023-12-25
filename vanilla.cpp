#include "common.h"
#include "vanilla.h"

const int CMd = 10;
const int CML = 1000;

int HashFuc(const Point &point, int seed){
    return 0;
}

struct CountMin{
    int buckets[CMd][CML];
    int seeds[CMd];
    CountMin(){
        memset(buckets, 0, sizeof(buckets));

    }
    void insert(const Point &point, int w){
        for(int i = 0; i < CMd; i++) buckets[i][HashFuc(point, seeds[i]) % CML] += w;
    }
    void query(const Point &point, int w){
        int minC = 1<<30;
        for(int i = 0; i < CMd; i++) minC = min(minC, buckets[i][HashFuc(point, seeds[i]) % CML]);
        return minC;
    }
};



Vanilla::Vanilla(int k_, int d_, int Delta_, double opt_, int sz) {
    k = k_;
    d = d_;
    Delta = Delta_;
    Depth = int(log2(Delta) + 1);
    opt = opt_;
    has_coreset = false;
    coreset_size = sz;
}

map<vector<int>, int> CM;
map<vector<int>, Point> Sampler[1010];

void Vanilla::update(const Point & point, bool insert) {
    if (point.weight <= 0) {
        throw runtime_error("Point weight must be positive.");
    }
    if (point.value.size() != d) {
        throw runtime_error("Point dimension mismatch.");
    }

    int InsOrDel = insert ? 1 : -1;
    for(int i = 0; i < Depth; i++){
        g = Delta >> i;
        vector<int> dp;
        for(int j = 0; j < d; j++) dp.push_back(int(point[i] / g));
        int curSize = (CM[dp] += InsOrDel);
        for(int i = 0; i < coreset_size; i++)
           if(myRand() %  == 0) Sampler[i][dp] = point;
    }
}

void Vanilla:getCoreset(int sz){
    has_coreset = true;
    coreset.clear();
    for(int i = 1; i < sz; i++){
        int cur = i % (Depth - 1) + 1, gi = Delta >> cur;
        double Ti = (d / gi) * (d / gi) * opt / k;
        for (const auto& kv : CM) {
            if(kv.second == 0 || kv.second > T_i) continue;
            double Ti1 = Ti << 2, gi1 = g1 << 1;
            pt = Sampler[i][kv.first];
            vector<int> dp;
            for(int j = 0; j < d; j++) dp.push_back(int(pt[i] / gi1));
            if(CM[dp] <= Ti1) continue;
            coreset.push_back(pt);
        }
    }
}

vector<Point> Vanilla::getClusters(int sz) {
    if(!has_coreset) getCoreset(sz);
    return KMeans(coreset, k, d);
}

uint64_t Vanilla::getMemoryUsage() {
    uint64_t sum = 0;
    sum += 2 * sizeof(int);                                   // k, d
    sum += saved_points.size() * (d + 1) * sizeof(double);    // saved_points
    return sum;
}

uint64_t Vanilla::getMaxMemoryUsage() {
    return getMemoryUsage();      // the memory usage is always increasing
}
