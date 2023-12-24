#include "common.h"

static mt19937 mt(time(0));

static double myRand(double interval) {
    // return a random value in [0, interval]
    uniform_real_distribution<double> realDist(0, interval);
    return realDist(mt);
}

static double squaredDistance(const Point & point1, const Point & point2, int d) {
    double sum = 0;
    for (int i = 0; i < d; i++)
        sum += (point1.value[i] - point2.value[i]) * (point1.value[i] - point2.value[i]);
    return sum;
}

static double getDist(const Point & point, const vector<Point> & centers, int num_centers, int d) {
    double minDist = squaredDistance(point, centers[0], d);
    for (int i = 1; i < num_centers; i++)
        minDist = min(minDist, squaredDistance(point, centers[i], d));
    return minDist;
}

static int findNearest(const Point & point, const vector<Point> & centers, int k, int d) {
    int idx = 0;
    double minDist = squaredDistance(point, centers[0], d);
    for (int i = 1; i < k; i++) {
        double dist = squaredDistance(point, centers[i], d);
        if (dist < minDist) minDist = dist, idx = i;
    }
    return idx;
}

vector<Point> KMeans(const vector<Point> & points, int k, int d) {
    int n = points.size();
    if (n < k) {
        throw runtime_error("Number of points is larger than k.");
    }
    vector<bool> is_center(n);
    vector<Point> centers(k);
    vector<double> dis(n);

    // K-means++
    //*
    // get the first center
    int first_center = (int) myRand(n);
    for (int i = 0; i < n; i++) is_center[i] = (i == first_center ? true : false);
    centers[0] = points[first_center];

    for (int i = 1; i < k; i++) {
        // determine the i-th center
        double sum = 0;
        for (int j = 0; j < n; j++)
            if (!is_center[j]) {
                dis[j] = getDist(points[j], centers, i, d);
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
    //*/

    // Select initial centers randomly
    /*
    for (int i = 0; i < k; i++) {
        int center = (int) myRand(n);
        while (is_center[center]) center = (int) myRand(n);
        centers[i] = points[center];
    }
    //*/

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
            for (auto idx : group[i]) {
                for (int j = 0; j < d; j++)
                    sum[j] += points[idx].value[j];
            }
            for (int j = 0; j < d; j++) centers[i].value[j] = sum[j] / (double) group[i].size();
        }
    }

    return centers;

    // Use external library to perform the K-means algorithm
    /*
    // write points to input.txt
    FILE *input = fopen("input.txt", "w");
    for (const auto & p : points) {
        for (int i = 0; i < d; i++) fprintf(input, "%.5lf ", p.value[i]);
        fprintf(input, "\n");
    }
    fclose(input);

    // run the program "kmeans"
    string command = "./kmeans input.txt " + to_string(k) + " output.txt > kmeansoutput.txt";
    int ret = system(command.c_str());
    if (ret) printf("Execute command %s failed, ret = %d\n", command.c_str(), ret);

    // read points from output.txt
    FILE *output = fopen("output.txt", "r");
    vector<Point> clusters;
    for (int i = 0; i < k; i++) {
        Point p;
        p.value.resize(d);
        for (int j = 0; j < d; j++) fscanf(output, "%lf", &p.value[j]);
        clusters.emplace_back(p);
    }
    return clusters;
    */
}