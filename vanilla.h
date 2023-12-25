#pragma once

#include "common.h"

// Template clustering saves every point and performs KMeans++ at the end.

class Vanilla {
    int k;  // number of clusters
    int d;  // dimension
    int Delta_; //precision
    int Depth;
    double opt; //prediction of the optimal clustering
    vector<Point> saved_points;
    int coreset_size;

    bool has_coreset;
    vector<Point> coreset;
public:
    Vanilla(int k_, int d_, int Delta_, double opt_, int sz);
    
    void update(const Point & point, bool insert);      // insert or delete a point
    void getCoreset(int sz);
    vector<Point> getClusters();                        // return the cluster centers
    uint64_t getMemoryUsage();                          // compute the current memory usage
    uint64_t getMaxMemoryUsage();                       // compute the max memory usage
};