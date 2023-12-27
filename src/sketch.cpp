#pragma once

#include "common.h"

// Template clustering saves every point and performs KMeans++ at the end.

class TempClustering {
    int k;  // number of clusters
    int d;  // dimension
    vector<Point> saved_points;

public:
    TempClustering(int k_, int d_);
    
    void update(const Point & point, bool insert);      // insert or delete a point
    vector<Point> getClusters();                        // return the cluster centers
    uint64_t getMemoryUsage();                          // compute the current memory usage
    uint64_t getMaxMemoryUsage();                       // compute the max memory usage
};