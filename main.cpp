#include "common.h"
#include "template.h"
#include "streamKMpp.h"
#include "vanilla.h"

/*
Runs all algorithms
*/

#define NUM_ALG 2
#define NUM_DATASET 4
#define TEST_ROUND 3

const string input_files[NUM_DATASET] = {"Adult.txt", "Bank.txt", "twitter.txt", "USC.txt"};
const char *alg_name[NUM_ALG] = {"streamKM++", "vanilla"};

// get current time in ms
static struct timeval base_time;
static void setBaseTime() {
    gettimeofday(&base_time, NULL);
}
static double getCurrentTime() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (t.tv_sec - base_time.tv_sec) * (double) 1000.0 + (t.tv_usec - base_time.tv_usec) / (double) 1000.0;
}

template<typename T>
void runTest(T & clustering, int idx, vector<Point> & points, vector<Point> & testCenters, double testDis, uint64_t *used_mem, double *used_time, double *dis, double *diff, int k, int d) {
    double st_time = getCurrentTime();
    for (const auto & p : points) {
        clustering.update(p, true);
    }
    used_time[idx] = getCurrentTime() - st_time;
    used_mem[idx] = clustering.getMaxMemoryUsage();
    dis[idx] = squaredDistance(points, clustering.getClusters(), k, d);
    diff[idx] = fabs(clustering.calculateKMeans(testCenters) - testDis) / testDis;
}

int main() {
    setBaseTime();

    for (int dataset = NUM_DATASET - 1; dataset < NUM_DATASET; dataset++) {
        int n, d;
        vector<Point> points;

        // prepare inputs

        string path = "dataset/" + input_files[dataset];
        FILE *input = fopen(path.c_str(), "r");
        if (input == NULL) {
            throw runtime_error("Cannot open input file.");
        }

        fscanf(input, "%d%d", &n, &d);
        double mx = -1e18, mn = 1e18;
        while (n--) {
            Point p;
            p.weight = 1;
            p.value.resize(d);
            for (int i = 0; i < d; i++) {
                fscanf(input, "%lf", &p.value[i]);
                mx = max(mx, p.value[i]);
                mn = min(mn, p.value[i]);
            }
            points.emplace_back(p);
        }

        fclose(input);

        // start running
        
        for (int k = 5; k <= 15; k += 5) {
            printf("----- Running on Dataset %s with k = %d -----\n", input_files[dataset].c_str(), k);

            uint64_t total_used_mem[NUM_ALG] = {0};
            double total_used_time[NUM_ALG] = {0};
            double total_dis[NUM_ALG] = {0};
            double total_diff[NUM_ALG] = {0};

            for (int round = 1; round <= TEST_ROUND; round++) {
                uint64_t used_mem[NUM_ALG];
                double used_time[NUM_ALG];
                double dis[NUM_ALG];
                double diff[NUM_ALG];

                vector<Point> testCenters(k);
                // randomly generate k centers
                for (int i = 0; i < k; i++) {
                    int idx = myRand(n);
                    testCenters[i] = points[idx];
                }
                double testDis = squaredDistance(points, testCenters, k, d);
  
                // alg1
                streamKMplusplus streamKMppclustering(k, d, 20);
                runTest(streamKMppclustering, 0, points, testCenters, testDis, used_mem, used_time, dis, diff, k, d);

                // alg2
                Vanilla vanillaclustering(k, d, 1 << ((int)log2(mx) + 1), 1000, 100);
                runTest(vanillaclustering, 1, points, testCenters, testDis, used_mem, used_time, dis, diff, k, d);

                for (int i = 0; i < NUM_ALG; i++) {
                    printf("Round #%d: Alg %s, memory = %llu bytes, time = %.5lf ms, kmeans = %.5lf, diff = %.5lf%%\n", round, alg_name[i], used_mem[i], used_time[i], dis[i], diff[i] * 100.0);
                    total_used_mem[i] += used_mem[i];
                    total_used_time[i] += used_time[i];
                    total_dis[i] += dis[i];
                    total_diff[i] += diff[i];
                }
            }

            printf("\nSummary: \n");
            for (int i = 0; i < NUM_ALG; i++) {
                printf("%s: memory = %llu bytes, time = %.5lf ms, kmeans = %.5lf, diff = %.5lf%%\n", alg_name[i], total_used_mem[i] / TEST_ROUND, total_used_time[i] / TEST_ROUND, total_dis[i] / TEST_ROUND, (total_diff[i] / TEST_ROUND) * 100.0);
            }
            printf("\n");
        }
    }
    return 0;
}