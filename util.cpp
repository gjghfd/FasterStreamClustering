#include "common.h"

static mt19937 mt(19260817);

double myRand(double interval) {
    // return a random value in [0, interval]
    uniform_real_distribution<double> realDist(0, interval);
    return realDist(mt);
}

// calculate squared distance from point1 to point2
double squaredDistance(const Point & point1, const Point & point2, int d) {
    double sum = 0;
    for (int i = 0; i < d; i++)
        sum += (point1.value[i] - point2.value[i]) * (point1.value[i] - point2.value[i]);
    return sum;
}
double squaredDistance(const Point *point1, const Point *point2, int d) {
    double sum = 0;
    for (int i = 0; i < d; i++)
        sum += (point1->value[i] - point2->value[i]) * (point1->value[i] - point2->value[i]);
    return sum;
}

// calculate min_i(dis(point, centers[i]))
double squaredDistance(const Point & point, const vector<Point> & centers, int num_centers, int d) {
    double minDist = squaredDistance(point, centers[0], d);
    for (int i = 1; i < num_centers; i++)
        minDist = min(minDist, squaredDistance(point, centers[i], d));
    return minDist;
}

// calculate sum_i(min_j(dis(points[i],centers[j])))
double squaredDistance(const vector<Point> & points, const vector<Point> & centers, int num_centers, int d) {
    double sum = 0;
    for (const auto & p : points) {
        sum += squaredDistance(p, centers, num_centers, d);
    }
    return sum;
}

// calculate sum_i(min_j(dis(points[i],centers[j]))*points[i].weight)
double squaredDistanceWeighted(const vector<Point> & points, const vector<Point> & centers, int num_centers, int d) {
    double sum = 0;
    for (const auto & p : points) {
        sum += squaredDistance(p, centers, num_centers, d) * p.weight;
    }
    return sum;
}

// find the nearest center to the point
int findNearest(const Point & point, const vector<Point> & centers, int num_centers, int d) {
    int idx = 0;
    double minDist = squaredDistance(point, centers[0], d);
    for (int i = 1; i < num_centers; i++) {
        double dist = squaredDistance(point, centers[i], d);
        if (dist < minDist) minDist = dist, idx = i;
    }
    return idx;
}

// perform a sampling based on the probability of each index
int sampleDistribution(vector<double> & prefixSum) {
    int n = prefixSum.size();
    if (!n) {
        throw runtime_error("sampleDistribution needs at least one element.\n");
    }
    // vector<double> prefixSum(n);
    // prefixSum[0] = pro[0];
    // for (int i = 1; i < n; i++) prefixSum[i] = prefixSum[i-1] + pro[i];

    double value = myRand(prefixSum[n-1]);

    // perform binary search
    int l = 0, r = n - 1;
    while (l <= r) {
        int mid = (l + r) >> 1;
        if (prefixSum[mid] >= value) r = mid - 1;
        else l = mid + 1;
    }
    return l;
}

// a weighted K-means++
vector<Point> KMeans(const vector<Point> & points, int k, int d) {
    int n = points.size();
    if (n < k) {
        throw runtime_error("Number of points is larger than k.");
    }
    vector<bool> is_center(n);
    vector<Point> centers(k);
    vector<double> dis(n);

    #define NUM_TURN 1

    // get the first center
    double sum = 0;
    for (int i = 0; i < n; i++) sum += points[i].weight;
    double value = myRand(sum);
    for (int i = 0; i < n; i++) {
        value -= points[i].weight;
        if (value <= 0 || i == n - 1) {
            // i is the first center
            for (int j = 0; j < n; j++) is_center[j] = (j == i ? true : false);
            centers[0] = points[i];
            break;
        }
    }

    // get other centers
    for (int i = 1; i < k; i++) {
        // determine the i-th center
        double sum = 0;
        for (int j = 0; j < n; j++)
            if (!is_center[j]) {
                dis[j] = squaredDistance(points[j], centers, i, d) * points[j].weight;
                sum += dis[j];
            }
        
        double value = myRand(sum);
        for (int j = 0; j < n; j++)
            if (!is_center[j]) {
                value -= dis[j];
                if (value <= 0 || j == n - 1) {
                    // choose this point as the i-th center
                    centers[i] = points[j];
                    is_center[j] = true;
                    break;
                }
            }
    }

    double mn_dis = squaredDistanceWeighted(points, centers, k, d);

    // repeat multiple times to obtain the minimal-distance initial centers
    for (int turn = 1; turn < NUM_TURN; turn++) {
        // reconstruct a center
        vector<Point> new_centers(k);
        vector<bool> new_is_center(n);

        // get the first center
        double sum = 0;
        for (int i = 0; i < n; i++) sum += points[i].weight;
        double value = myRand(sum);
        for (int i = 0; i < n; i++) {
            value -= points[i].weight;
            if (value <= 0 || i == n - 1) {
                // i is the first center
                for (int j = 0; j < n; j++) new_is_center[j] = (j == i ? true : false);
                new_centers[0] = points[i];
                break;
            }
        }

        // get other centers
        for (int i = 1; i < k; i++) {
            // determine the i-th center
            double sum = 0;
            for (int j = 0; j < n; j++)
                if (!new_is_center[j]) {
                    dis[j] = squaredDistance(points[j], new_centers, i, d) * points[j].weight;
                    sum += dis[j];
                }
            
            double value = myRand(sum);
            for (int j = 0; j < n; j++)
                if (!new_is_center[j]) {
                    value -= dis[j];
                    if (value <= 0 || j == n - 1) {
                        // choose this point as the i-th center
                        new_centers[i] = points[j];
                        new_is_center[j] = true;
                        break;
                    }
                }
        }

        double dis = squaredDistanceWeighted(points, new_centers, k, d);
        if (dis < mn_dis) {
            mn_dis = dis;
            centers.assign(new_centers.begin(), new_centers.end());
            is_center.assign(new_is_center.begin(), new_is_center.end());
        }
    }

    // now initial centers are chosen, proceed with k-means
    for (int iter = 0; iter < 100; iter++) {
        vector<vector<int>> group(k);   // k groups

        // classify points into groups
        for (int i = 0; i < n; i++) {
            int group_idx = findNearest(points[i], centers, k, d);
            group[group_idx].emplace_back(i);
        }

        // recompute group center
        vector<double> sum(d);
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < d; j++) sum[j] = 0;
            double sum_weight = 0;
            for (auto idx : group[i]) {
                sum_weight += points[idx].weight;
                for (int j = 0; j < d; j++)
                    sum[j] += points[idx].value[j] * points[idx].weight;
            }
            for (int j = 0; j < d; j++) centers[i].value[j] = sum[j] / sum_weight;
        }
    }

    return centers;
}