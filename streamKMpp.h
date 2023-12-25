#pragma once

#include "common.h"

// streamKM++ algorithm
class streamKMplusplus {
    int k;  // number of clusters
    int d;  // dimension
    int m;  // coreset size
    uint64_t maxMemUsage;
    vector<vector<Point>> buckets;  // the i-th bucket (i>=1) holds m points that represents 2^(i-1)*m points in the data stream

public:
    streamKMplusplus(int k_, int d_, int m_);

    void update(const Point & point, bool insert);      // insert or delete a point
    vector<Point> getClusters();                        // return the cluster centers
    uint64_t getMemoryUsage();                          // compute the current memory usage
    uint64_t getMaxMemoryUsage();                       // compute the max memory usage

    int insertBucket(const Point & point, int idx);
    void findCoreset(vector<Point> & points);
};