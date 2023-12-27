#include "common.h"
#include "vanilla.h"
#define subSample 2000

vector<Point> savedata;

Vanilla::Vanilla(int k_, int d_, int Delta_, double opt_, int sz, bool sketch_) {
    n = 0;
    k = k_;
    d = d_;
    Delta = Delta_;
    Depth = int(log2(Delta) + 1);
    opt = opt_;
    has_coreset = false;
    coreset_size = sz;
    for(int i = 0; i < Depth; i++)
        CM.push_back(CountMap(-1, sketch_, sz / 5));
    CM.resize(Depth);
    sketch = sketch_;
    maxMem = 0;
}

vector<int> discrete(const Point &point, double scal){
    vector<int> seq;
    for(int j = 0; j < point.value.size(); j++)
        seq.push_back(int(point.value[j] / scal));
    return seq;
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
        double g = Delta >> i;
        double T = (d / g) * (d / g) * opt; 
        int Pr = max(T/subSample, 1.0);
        if(MurmurHash3_x86_32((int*)&point.value[0], d, 1403) % Pr) continue; 
        int dp = 0;
        vector<int> seq = discrete(point, g);
        if(insert){
            CM[i].ins(&seq[0], d, point);
        }
        else{
            CM[i].del(&seq[0], d);
        }
    }
    savedata.push_back(point);
}


void Vanilla::getCoreset(int sz){

    has_coreset = true;
    coreset.clear();

    vector<set<uint32_t> > cru;
    vector<int> cru_size;
    vector<double> cru_weight;
    cru.resize(Depth);
    cru_size.resize(Depth);
    cru_weight.resize(Depth);

    for(int i = 0; i < Depth; i++){
        double g = Delta >> i;
        double T = (d / g) * (d / g) * opt; 
        int Pr = max(T/subSample,1.0);
        double T1 = T / 4, g1 = g * 2;
        int Pr1 = max(T1/subSample,1.0);

        vector<uint32_t> nonempty = CM[i].getNonempty();
        sort(nonempty.begin(), nonempty.end()); nonempty.resize(unique(nonempty.begin(), nonempty.end()) - nonempty.begin());

        for(int j = 0; j < nonempty.size(); j++){
            uint32_t curHashValue = nonempty[j];
            if(CM[i].query(curHashValue) * Pr > T) continue;
            Point pt = CM[i].sample(curHashValue);

            int flg = 1;
            for(int t = -1; t < i; t++){
                double  g2;
                if(t < 0) g2 = Delta * 2; else g2 = Delta >> t;
                double T2 = (d / g2) * (d / g2) * opt; 

                int Pr2 = max(T2/subSample,1.0);
                vector<int> seq = discrete(pt, g2);
                if(t > 0 && CM[t].query(&seq[0], d) * Pr2 <= T2){
                    flg = 0; break;
                }
            }
            if(!flg) continue;
            cru[i].insert(curHashValue);
            cru_size[i] += CM[i].query(curHashValue) * Pr;
        }
        cru_weight[i] = 1/T;
    }

    double sum_weight = 0;

    vector<int> gLayer;
    for(int  i = 0; i < Depth; i++)
        if(cru_size[i] > 0) gLayer.push_back(i);

    vector<double> dist;    
    for(int i = 0; i < gLayer.size(); i++){
        int idx = gLayer[i];
        sum_weight += cru_size[idx]  * cru_weight[idx];
        dist.push_back(cru_size[idx]  * cru_weight[idx]);
        if(i) dist[i] += dist[i-1];
    }
    vector<int> sample_num(Depth);
    for(int i = 0; i < sz; i++){
        int ind = sampleDistribution(dist);
        sample_num[gLayer[ind]] ++;
    }

    for(int i = 0; i < Depth; i++){
        if(sample_num[i] == 0) continue;

        double g = Delta >> i;
        double T = (d / g) * (d / g) * opt; 
        int Pr = max(T/subSample,1.0);

        vector<uint32_t> cru_vec;
        vector<int> snum;
        int remain_sz = sample_num[i];
        for(auto x :  cru[i]) cru_vec.push_back(x);
        snum.resize(cru_vec.size());
        vector<double> distri;
        for(int j = 0; j < cru_vec.size(); j++){
            distri.push_back(CM[i].query(cru_vec[j]));
            if(j) distri[j] += distri[j-1];
        }
        for(int j = 0; j < sample_num[i]; j++){
            int ind = sampleDistribution(distri);
            snum[ind] ++;
        }

        for(int j = 0; j < cru_vec.size(); j++){
            int total_size = CM[i].query(cru_vec[j]);
            for(int t = 0; t < snum[j]; t++){
                Point pt = CM[i].sample(cru_vec[j]);
                pt.weight *= cru_size[i] / (double) sample_num[i];
                coreset.push_back(pt);
            }
        }
    }

    double sumw = 0;
    for(int i = 0; i < coreset.size(); i++) sumw+=coreset[i].weight;
    for(int i = 0; i < coreset.size(); i++) coreset[i].weight *= n / sumw;
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
    for(int i = 0; i < CM.size(); i++) sum += CM[i].getMemoryUsage();
    return sum;
}

uint64_t Vanilla::getMaxMemoryUsage() {
    return maxMem = max(maxMem, getMemoryUsage());      // the memory usage is always increasing
}
