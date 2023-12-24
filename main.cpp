#include "common.h"
#include "template.h"

/*
Runs all algorithms
*/

#define NUM_ALG 1
#define NUM_DATASET 4
#define TEST_ROUND 3

const string input_files[NUM_DATASET] = {"Adult.txt", "Bank.txt", "twitter.txt", "USC.txt"};

// get memory usage in byte; return type: unsigned long
// static vm_size_t getMemUsage() {
//     struct task_basic_info info;
//     mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;    //sizeof(info);
//     kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
//     return (kerr == KERN_SUCCESS) ? info.resident_size : 0; // size in byte
// }
vm_size_t getMemUsage()
{
    unsigned int task = 0;
    int error = 0;
    unsigned int count = 0;
    struct task_basic_info ti;

    error = task_for_pid(mach_task_self(), getpid(), &task);
    if (error != KERN_SUCCESS)
    {
        return 0;
    }
    count = TASK_BASIC_INFO_COUNT;
    error = task_info(task, TASK_BASIC_INFO, (task_info_t)&ti, &count);
    if (error != KERN_SUCCESS)
    {
        return 0;
    }

    vm_region_basic_info_data_64_t b_info;
    unsigned long address = SHARED_REGION_BASE_PPC;
    unsigned long size = 0;
    unsigned int object_name = 0;
    count = VM_REGION_BASIC_INFO_COUNT_64;
    error = vm_region_64(task, &address, &size, VM_REGION_BASIC_INFO, (vm_region_info_t)&b_info, &count, &object_name);
    if (error == KERN_SUCCESS) 
    {
        if (b_info.reserved && size == (SHARED_REGION_NESTING_SIZE_PPC) &&
            ti.virtual_size > (SHARED_REGION_NESTING_SIZE_PPC + SHARED_REGION_NESTING_MIN_PPC))
        {
            ti.virtual_size -= (SHARED_REGION_NESTING_SIZE_PPC + SHARED_REGION_NESTING_MIN_PPC);
        }
    }
    printf("memory usage = %ld bytes\n", ti.resident_size);
    return ti.resident_size;
}

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

// get total distance based on the points in coreset
static double getPointDis(const Point & A, const Point & B, int d) {
    double res = 0;
    for (int i = 0; i < d; i++) {
        res += (A.value[i] - B.value[i]) * (A.value[i] - B.value[i]);
    }
    return sqrt(res);
}
static double getDis(const vector<Point> & points, const vector<Point> & clusters, int k, int d) {
    double sum = 0;
    for (const auto & p : points) {
        double minDis = getPointDis(p, clusters[0], d);
        for (int j = 1; j < k; j++) minDis = min(minDis, getPointDis(p, clusters[j], d));
        sum += minDis;
    }
    return sum;
}

int main() {
    setBaseTime();

    for (int dataset = 0; dataset < NUM_DATASET; dataset++) {
        int n, d;
        vector<Point> points;

        // prepare inputs

        string path = "dataset/" + input_files[dataset];
        FILE *input = fopen(path.c_str(), "r");
        if (input == NULL) {
            throw runtime_error("Cannot open input file.");
        }

        fscanf(input, "%d%d", &n, &d);
        while (n--) {
            Point p;
            p.value.resize(d);
            for (int i = 0; i < d; i++) fscanf(input, "%lf", &p.value[i]);
            points.emplace_back(p);
        }

        fclose(input);

        // start running
        
        for (int k = 25; k <= 75; k += 25) {

            printf("----- Running on Dataset %s with k = %d -----\n", input_files[dataset].c_str(), k);

            vm_size_t total_used_mem[NUM_ALG] = {0};
            double total_used_time[NUM_ALG] = {0};
            double total_dis[NUM_ALG] = {0};

            for (int round = 1; round <= TEST_ROUND; round++) {
                int idx;
                vm_size_t used_mem[NUM_ALG];
                double used_time[NUM_ALG];
                double dis[NUM_ALG];
                
                // alg0
                idx = 0;
                vm_size_t st_mem = getMemUsage();

                TempClustering clustering(k, d);

                double st_time = getCurrentTime();

                for (const auto & p : points) {
                    clustering.update(p, true);
                }

                used_mem[idx] = getMemUsage() - st_mem;
                used_time[idx] = getCurrentTime() - st_time;
                dis[idx] = getDis(points, clustering.getClusters(), k, d);

                // alg1
                idx = 1;
                // ...

                for (int i = 0; i < NUM_ALG; i++) {
                    printf("Round #%d: alg %d, memory = %ld bytes, time = %.5lf ms, kmeans = %.5lf\n", round, i, used_mem[i], used_time[i], dis[i]);
                    total_used_mem[i] += used_mem[i];
                    total_used_time[i] += used_time[i];
                    total_dis[i] += dis[i];
                }
            }

            printf("Summary:\n");
            for (int i = 0; i < NUM_ALG; i++) {
                printf("alg %d: memory = %ld bytes, time = %.5lf ms, kmeans = %.5lf\n", i, total_used_mem[i] / TEST_ROUND, total_used_time[i] / TEST_ROUND, total_dis[i] / TEST_ROUND);
            }
        }
    }
    return 0;
}