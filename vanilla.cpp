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
    // for(int i = 0; i < Depth; i++){
    //     Sampler.push_back(SampleMap(&CM[i], coreset_size));
    // }
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
        // int Pr = max(d/g/subSample, 1.0);
        // printf("%d %d %lf\n", i, Pr, T);
        if(MurmurHash3_x86_32((int*)&point.value[0], d, 1403) % Pr) continue; 
        int dp = 0;
        vector<int> seq = discrete(point, g);
        // MM[i][seq] = 1;
        if(insert){
            CM[i].ins(&seq[0], d, point);
            // Sampler[i].ins(&seq[0], d, point);
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

    printf("begin construction\n");

    vector<set<uint32_t> > cru;
    vector<int> cru_size;
    vector<double> cru_weight;
    cru.resize(Depth);
    cru_size.resize(Depth);
    cru_weight.resize(Depth);

    // printf("%d\n", Depth);

    for(int i = 0; i < Depth; i++){
        // printf("%d %d\n", i, MM[i].size());
        // printf("%d: ", i);
        double g = Delta >> i;
        double T = (d / g) * (d / g) * opt; 
        int Pr = max(T/subSample,1.0);
        // int Pr = max(d/g/subSample,1.0);
        printf("%d %d %lf\n", i, Pr, T);
        double T1 = T / 4, g1 = g * 2;
        int Pr1 = max(T1/subSample,1.0);

        vector<uint32_t> nonempty = CM[i].getNonempty();
        sort(nonempty.begin(), nonempty.end()); nonempty.resize(unique(nonempty.begin(), nonempty.end()) - nonempty.begin());
        // printf("%d\n", nonempty.size());
        // for(int j = 0; j < nonempty.size(); j++) printf("%u ", nonempty[j]);
        // putchar('\n');

        for(int j = 0; j < nonempty.size(); j++){
            uint32_t curHashValue = nonempty[j];
            if(CM[i].query(curHashValue) * Pr > T) continue;
            // Point pt = Sampler[i].query(curHashValue, 1);
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
            // vector<int> seq = discrete(pt, g1);
            // if(i > 0 && CM[i-1].query(&seq[0], d) * Pr1 <= T1) continue;
            cru[i].insert(curHashValue);
            cru_size[i] += CM[i].query(curHashValue) * Pr;
            // cru_weight[i] += CM[i].query(curHashValue) / T;
        }
        cru_weight[i] = 1/T;
    }


    // printf("begin construction\n");
    //debug
    // vector<Point> layer[24];
    // printf("n = %d\n",n);
    // int wrontcnt = 0;
    // for(int i = 0; i < n; i++){
    //     Point cur = savedata[i];
    //     int cnt = 0, ly;
    //     for(int j = 0; j < Depth; j++){
    //         double g = Delta >> j;
    //         vector<int> seq = discrete(cur, g);
    //         uint32_t dp = CM[j].hash(&seq[0], d);
    //         if(cru[j].find(dp) != cru[j].end()) cnt ++, layer[j].push_back(cur);
    //     }
    //     if(cnt != 1) wrontcnt ++;
    // }
    double sum_weight = 0;
    // printf("missed %d\n", wrontcnt);

    vector<int> gLayer;
    for(int  i = 0; i < Depth; i++)
        if(cru_size[i] > 0) gLayer.push_back(i);

    vector<double> dist;    
    for(int i = 0; i < gLayer.size(); i++){
        int idx = gLayer[i];
        // printf("%d %d\n", layer[i].size(), cru_size[i]);
        // if(layer[i].size() != cru_size[i]) puts("ffffjurujk");
        sum_weight += cru_size[idx]  * cru_weight[idx];
        dist.push_back(cru_size[idx]  * cru_weight[idx]);
        if(i) dist[i] += dist[i-1];
    }
    vector<int> sample_num(Depth);
    for(int i = 0; i < sz; i++){
        int ind = sampleDistribution(dist);
        sample_num[gLayer[ind]] ++;
    }

    // int remain_sz = sz;
    // for(int i = 0; i < Depth; i++){
    //     sample_num.push_back((int) (sz / sum_weight * cru_size[i] * cru_weight[i]) );
    //     remain_sz -= (int) (sz / sum_weight * cru_size[i] * cru_weight[i]);
    // }
    // for(int i = 0; remain_sz > 0; i++){
    //     sample_num[gLayer[(int)myRand(gLayer.size())]] ++; 
    //     remain_sz --;
    // }

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
        // double sum_pr =0;
        // for(int j = 0; j < cru_vec.size(); j++) sum_pr += distri[j];
        // printf("%lf\n", sum_pr);
        // for(int j = 0; j < cru_vec.size(); j++ ){
        //     snum.push_back((int)(CM[i].query(cru_vec[j]) / (double) cru_size[i] * sample_num[i]));
        //     remain_sz-= (int)(CM[i].query(cru_vec[j]) / (double) cru_size[i] * sample_num[i]);
        // }
        // for(int j = 0; remain_sz > 0; j ++){
        //     snum[(int)myRand(snum.size())] ++;
        //     remain_sz --;
        // }
        for(int j = 0; j < sample_num[i]; j++){
            int ind = sampleDistribution(distri);
            snum[ind] ++;
        }

        for(int j = 0; j < cru_vec.size(); j++){
            int total_size = CM[i].query(cru_vec[j]);
            // printf("%d ", snum[j]);
            for(int t = 0; t < snum[j]; t++){
                // Point pt = Sampler[i].query(cru_vec[j]);
                Point pt = CM[i].sample(cru_vec[j]);
                // pt.weight *= n / (double)sz;
                // pt.weight *= sum_weight / cru_weight[i] / (double)sz;


                pt.weight *= cru_size[i] / (double) sample_num[i];
                // pt.weight *= cru_size[i] * Pr / (double) sample_num[i];
                // pt.weight *=  n * total_size / (double)snum[j];
                // pt.weight *=  sum_weight / (double)(cru_weight[i]) * Pr / sz;
                coreset.push_back(pt);
                // for(int  tt = 0; tt < d; tt++) printf("%lf ",pt.value[tt]); putchar('\n');
            }
        }
        // for(int j = 0; j < sample_num[i]; j++){
        //     uint32_t pickHash = cru_vec[(int)myRand(cru_vec.size())];
        //     Point pt = layer[i][(int)myRand(layer[i].size())];
        //     // Point pt = Sampler[i].query(pickHash);
        //     // printf("%lf\n", pt.weight);
        //     pt.weight *= cru_size[i] / (double) sample_num[i];
        //     // if(pt.weight < 1e-6) puts("ffffff");
        //     // printf("%lf\n", pt.weight);
        //     coreset.push_back(pt);
        // }
    }

    savedata.clear();
    printf("coreset size %d\n", (int)coreset.size());
    double sumw = 0;
    for(int i = 0; i < coreset.size(); i++) sumw+=coreset[i].weight;
    printf("total weight: %lf\n", sumw);
    for(int i = 0; i < coreset.size(); i++) coreset[i].weight *= n / sumw;

    // while(sz--){

    // }

    // int tt = 0;
    // while(coreset.size() < sz){
    //     tt++;
    //     vector<Point> candi;
    //     for(int cur = 1; cur < Depth; cur++){
    //         // printf("%d\n", CM[cur].size());
    //         double gi = Delta >> cur;
    //         double Ti = (d / gi) * (d / gi) * opt / k;
    //         // printf("%d %lf\n", cur, Ti);
    //         int flg = 0;
    //         vector<Point> sampled;
    //         for (const auto& kv : CM[cur]) {
    //             // printf("%d ", kv.second);
    //             if(kv.second == 0 || kv.second > Ti){
    //                 // CM[cur].delete(kv.first);
    //                 continue;
    //             }
    //             double Ti1 = Ti / 4, gi1 = Delta >> (cur - 1);
    //             Point pt = Sampler[cur][tt][kv.first];
    //             int dp = 0;
    //             vector<int> seq;
    //             for(int j = 0; j < d; j++)
    //                  seq.push_back(int(pt.value[j] / gi1));
    //             dp = MurmurHash3_x86_32(&seq[0], d, 19260817);
    //                 // dp.push_back(int(pt.value[j] / gi1));
    //             if(CM[cur-1][dp] <= Ti1){
    //                 // CM[cur].delete(kv.first);
    //                 continue;
    //             }
    //             cru[cur].insert(kv.first);
    //             sampled.push_back(pt);
    //         }
    //         // putchar('\n');
    //         if(sampled.empty()) continue;
    //         candi.push_back(sampled[(int)myRand(sampled.size())]);
    //     }
    //             break;
    //     for(int j = 0; j < candi.size() && coreset.size() < sz; j++){
    //         Point pt = candi[j];
    //         pt.weight *= n / (double)sz;
    //         coreset.push_back(pt);
    //     }
        
    // }
    // // puts("ffffff");
    // vector<Point> layer[25];
    // for(int i = 0; i < n; i++){
    //     Point cur = savedata[i];
    //     int cnt = 0, ly;
    //     for(int j = 0; j < Depth; j++){
    //         int dp = 0, g = Delta >> j;
    //         vector<int> seq;
    //         for(int z = 0; z < d; z++)
    //              seq.push_back(int(cur.value[z] / g));
    //         dp = MurmurHash3_x86_32(&seq[0], d, 19260817);
    //         if(cru[j].find(dp) != cru[j].end()) cnt ++, layer[j].push_back(cur);
    //     }
    //     if(cnt == 1) continue;
    //     for(int j = 0; j < Depth; j++){
    //         int dp = 0, g = Delta >> j;
    //         vector<int> seq;
    //         for(int z = 0; z < d; z++)
    //              seq.push_back(int(cur.value[z] / g));
    //         dp = MurmurHash3_x86_32(&seq[0], d, 19260817);
    //         printf("%d ", CM[j][dp]);
    //         // if(cru[j].find(dp) != cru[j].end()) cnt ++, layer[j].push_back(cur);
    //     }
    //     // puts("");
    // }
    // int ttt = 0; 
    // vector<int> wt; int cs = 0; wt.resize(Depth);
    // while(cs < sz){
    //     int cur = (ttt ++) % Depth;
    //     if(layer[cur].empty()) continue;
    //     wt[cur] ++; cs++;

    //     // Point pt = layer[cur][(int)myRand(layer[cur].size())];
    //     // pt.weight *= n / (double)sz;
    //     // coreset.push_back(pt);
    // }
    // while(coreset.size() < sz){
    //     int cur = (ttt ++) % Depth;
    //     if(layer[cur].empty()) continue;
        
    //     Point pt = layer[cur][(int)myRand(layer[cur].size())];
    //     pt.weight *= layer[cur].size() / (double)wt[cur];
    //     coreset.push_back(pt);
    // }

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
    // vector<SampleMap> Sampler;

    for(int i = 0; i < CM.size(); i++) sum += CM[i].getMemoryUsage();
    // sum += saved_points.size() * (d + 1) * sizeof(double);    // saved_points
    return sum;
}

uint64_t Vanilla::getMaxMemoryUsage() {
    return maxMem = max(maxMem, getMemoryUsage());      // the memory usage is always increasing
}
