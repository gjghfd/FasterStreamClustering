#include "common.h"
#include "vanilla.h"
#include "murmur3.h"

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
    int query(const Point &point, int w){
        int minC = 1<<30;
        for(int i = 0; i < CMd; i++) minC = min(minC, buckets[i][HashFuc(point, seeds[i]) % CML]);
        return minC;
    }
};


vector<Point> savedata;

Vanilla::Vanilla(int k_, int d_, int Delta_, double opt_, int sz) {
    n = 0;
    k = k_;
    d = d_;
    Delta = Delta_;
    Depth = int(log2(Delta) + 1);
    opt = opt_;
    has_coreset = false;
    coreset_size = sz;
}


void Vanilla::update(const Point & point, bool insert) {
    if (point.weight <= 0) {
        throw runtime_error("Point weight must be positive.");
    }
    if (point.value.size() != d) {
        throw runtime_error("Point dimension mismatch.");
    }

    int InsOrDel = insert ? 1 : -1;
    n += InsOrDel;
    for(int i = 0; i < Depth; i++){
        int g = Delta >> i;
        int dp = 0;
        vector<int> seq;
        for(int j = 0; j < d; j++)
             seq.push_back(int(point.value[j] / g));
        dp = MurmurHash3_x86_32(&seq[0], d, 19260817);
        int curSize = (CM[i][dp] += InsOrDel);
        for(int j = 0; j < coreset_size; j++)
           if(myRand(1) < 1.0 / curSize){
                Sampler[i][j][dp] = point;
            }
    }
    savedata.push_back(point);
}


void Vanilla::getCoreset(int sz){
    has_coreset = true;
    coreset.clear();

    // printf("begin construction\n");
    int tt = 0;
    while(coreset.size() < sz){
        tt++;
        vector<Point> candi;
        for(int cur = 1; cur < Depth; cur++){
            // printf("%d\n", CM[cur].size());
            double gi = Delta >> cur;
            double Ti = (d / gi) * (d / gi) * opt / k;
            // printf("%d %lf\n", cur, Ti);
            int flg = 0;
            vector<Point> sampled;
            for (const auto& kv : CM[cur]) {
                // printf("%d ", kv.second);
                if(kv.second == 0 || kv.second > Ti){
                    // CM[cur].delete(kv.first);
                    continue;
                }
                double Ti1 = Ti / 4, gi1 = Delta >> (cur - 1);
                Point pt = Sampler[cur][tt][kv.first];
                int dp = 0;
                vector<int> seq;
                for(int j = 0; j < d; j++)
                     seq.push_back(int(pt.value[j] / gi1));
                dp = MurmurHash3_x86_32(&seq[0], d, 19260817);
                    // dp.push_back(int(pt.value[j] / gi1));
                if(CM[cur-1][dp] <= Ti1){
                    // CM[cur].delete(kv.first);
                    continue;
                }
                cru[cur].insert(kv.first);
                sampled.push_back(pt);
            }
            // putchar('\n');
            if(sampled.empty()) continue;
            candi.push_back(sampled[(int)myRand(sampled.size())]);
        }
                break;
        for(int j = 0; j < candi.size() && coreset.size() < sz; j++){
            Point pt = candi[j];
            pt.weight *= n / (double)sz;
            coreset.push_back(pt);
        }
        
    }
    // puts("ffffff");
    vector<Point> layer[25];
    for(int i = 0; i < n; i++){
        Point cur = savedata[i];
        int cnt = 0, ly;
        for(int j = 0; j < Depth; j++){
            int dp = 0, g = Delta >> j;
            vector<int> seq;
            for(int z = 0; z < d; z++)
                 seq.push_back(int(cur.value[z] / g));
            dp = MurmurHash3_x86_32(&seq[0], d, 19260817);
            if(cru[j].find(dp) != cru[j].end()) cnt ++, layer[j].push_back(cur);
        }
        if(cnt == 1) continue;
        for(int j = 0; j < Depth; j++){
            int dp = 0, g = Delta >> j;
            vector<int> seq;
            for(int z = 0; z < d; z++)
                 seq.push_back(int(cur.value[z] / g));
            dp = MurmurHash3_x86_32(&seq[0], d, 19260817);
            printf("%d ", CM[j][dp]);
            // if(cru[j].find(dp) != cru[j].end()) cnt ++, layer[j].push_back(cur);
        }
        // puts("");
    }
    int ttt = 0; 
    vector<int> wt; int cs = 0; wt.resize(Depth);
    while(cs < sz){
        int cur = (ttt ++) % Depth;
        if(layer[cur].empty()) continue;
        wt[cur] ++; cs++;

        // Point pt = layer[cur][(int)myRand(layer[cur].size())];
        // pt.weight *= n / (double)sz;
        // coreset.push_back(pt);
    }
    while(coreset.size() < sz){
        int cur = (ttt ++) % Depth;
        if(layer[cur].empty()) continue;
        
        Point pt = layer[cur][(int)myRand(layer[cur].size())];
        pt.weight *= layer[cur].size() / (double)wt[cur];
        coreset.push_back(pt);
    }
    // printf("\t coreset size %d %d\n", sz, (int)coreset.size());
}

vector<Point> Vanilla::getClusters() {
    if(!has_coreset) getCoreset(coreset_size);
    return KMeans(coreset, k, d);
}

double Vanilla::calculateKMeans(vector<Point> & centers) {
    if(!has_coreset) getCoreset(coreset_size);
    return squaredDistanceWeighted(coreset, centers, k, d);
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
