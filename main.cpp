#include "common.h"
#include "template.h"
#include "streamKMpp.h"
#include "vanilla.h"

/*
Runs all algorithms
*/

#define NUM_ALG 2
#define NUM_DATASET 5
#define TEST_ROUND 3
#define NUM_DIFF_TEST 10

#define MAX_POINTS 200000

const string input_files[NUM_DATASET] = {"census.txt", "Adult.txt", "Bank.txt", "twitter.txt", "USC.txt"};
const char *alg_name[NUM_ALG] = {"streamKM++", "vanilla"};

// get current time in ms
static struct timespec base_time;
static inline void setBaseTime() {
    clock_gettime(CLOCK_REALTIME, &base_time);
}
static inline double getCurrentTime() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (t.tv_sec - base_time.tv_sec) * (double) 1000.0 + (t.tv_nsec - base_time.tv_nsec) / (double) 1000000.0;
}

template<typename T>
void runTest(T & clustering, int idx, vector<Point> & points, vector<Point> *testCenters, double *testDis, uint64_t *used_mem, double *avg_latency, double *p99_latency, double *dis, double *diff, int k, int d) {
    vector<double> latency(points.size());
    int i = 0;
    for (const auto & p : points) {
        double st_time = getCurrentTime();
        clustering.update(p, true);
        latency[i++] = getCurrentTime() - st_time;
    }
    // process latency
    sort(latency.begin(), latency.end());
    double sum = 0;
    for (double l : latency) sum += l;
    avg_latency[idx] = sum / (double) points.size();
    int p99 = points.size() - points.size() / 100;
    p99_latency[idx] = latency[p99];
    // process others
    used_mem[idx] = clustering.getMaxMemoryUsage();
    dis[idx] = squaredDistance(points, clustering.getClusters(), k, d);

    // get max diff
    double mx = 0;
    for (int i = 0; i < NUM_DIFF_TEST; i++) {
        double diff = fabs(clustering.calculateKMeans(testCenters[i]) - testDis[i]) / testDis[i];
        mx = max(mx, diff);
    }
    diff[idx] = mx;
}

int main() {
    setBaseTime();

    for (int dataset = 0; dataset < 1; dataset++) {
        int n, d;
        vector<Point> points;

        // prepare inputs

        string path = "dataset/" + input_files[dataset];
        FILE *input = fopen(path.c_str(), "r");
        if (input == NULL) {
            throw runtime_error("Cannot open input file.");
        }

        fscanf(input, "%d%d", &n, &d);
        n = min(n, MAX_POINTS);
        double mx = -1e18, mn = 1e18;
        for (int j = 0; j < n; j++) {
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

        printf("max %lf\n", mx);
        fclose(input);

        // start running
        
        for (int k = 5; k <= 15; k += 5) {
            printf("----- Running on Dataset %s (%d points) with k = %d -----\n", input_files[dataset].c_str(), n, k);

            uint64_t total_used_mem[NUM_ALG] = {0};
            double total_latency[NUM_ALG] = {0};
            double total_P99_latency[NUM_ALG] = {0};
            double total_dis[NUM_ALG] = {0};
            double total_diff[NUM_ALG] = {0};
            double total_std_dis = 0;

            for (int round = 1; round <= TEST_ROUND; round++) {
                uint64_t used_mem[NUM_ALG];
                double latency[NUM_ALG];
                double P99_latency[NUM_ALG];
                double dis[NUM_ALG];
                double diff[NUM_ALG];

                vector<Point> testCenters[NUM_DIFF_TEST];
                double testDis[NUM_DIFF_TEST];
                for (int i = 0; i < NUM_DIFF_TEST; i++) {
                    // use randomly generated cluster centers
                    auto centers = KMeans(points, k, d);
                    testCenters[i].assign(centers.begin(), centers.end());
                    testDis[i] = squaredDistance(points, testCenters[i], k, d);
                }

                // alg1
                streamKMplusplus streamKMppclustering(k, d, 100);
                runTest(streamKMppclustering, 0, points, testCenters, testDis, used_mem, latency, P99_latency, dis, diff, k, d);

                // alg2
                // Vanilla vanillaclustering(k, d, 1 << ((int)log2(mx) + 1), 1000 / k, 100);
                // runTest(vanillaclustering, 1, points, testCenters, testDis, used_mem, used_time, P99_latency, dis, diff, k, d);

                total_std_dis += squaredDistance(points, KMeans(points, k, d), k, d);

                for (int i = 0; i < NUM_ALG; i++) {
                    printf("Round #%d: Alg %s, memory = %llu bytes, avg latency = %.5lf ms, p99 latency = %.5lf ms, kmeans = %.5lf, diff = %.5lf%%\n", round, alg_name[i], used_mem[i], latency[i], P99_latency[i], dis[i], diff[i] * 100.0);
                    total_used_mem[i] += used_mem[i];
                    total_latency[i] += latency[i];
                    total_P99_latency[i] += P99_latency[i];
                    total_dis[i] += dis[i];
                    total_diff[i] += diff[i];
                }
            }

            printf("\nSummary: \n");
            for (int i = 0; i < NUM_ALG; i++) {
                printf("%s: memory = %llu bytes, avg latency = %.5lf ms, p99 latency = %.5lf ms, kmeans = %.5lf, diff = %.5lf%%\n", alg_name[i], total_used_mem[i] / TEST_ROUND, total_latency[i] / TEST_ROUND, total_P99_latency[i] / TEST_ROUND, total_dis[i] / TEST_ROUND, (total_diff[i] / TEST_ROUND) * 100.0);
            }
            printf("standard k-means = %.5lf\n\n", total_std_dis / TEST_ROUND);
            fflush(stdout);
        }
    }
    return 0;
}