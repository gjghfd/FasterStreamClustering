#include "common.h"
#include "template.h"

TempClustering::TempClustering(int k_, int d_) {
    k = k_;
    d = d_;
}

void TempClustering::update(const Point & point, bool insert) {
    if (point.weight <= 0) {
        throw runtime_error("Point weight must be positive.");
    }
    if (point.value.size() != d) {
        throw runtime_error("Point dimension mismatch.");
    }

    if (!insert) {
        printf("Error: template clustering does not support deletion.\n");
        return;
    }

    saved_points.emplace_back(point);
}

vector<Point> TempClustering::getClusters() {
    return KMeans(saved_points, k, d);
}

uint64_t TempClustering::getMemoryUsage() {
    uint64_t sum = 0;
    sum += 2 * sizeof(int);                                   // k, d
    sum += saved_points.size() * (d + 1) * sizeof(double);    // saved_points
    return sum;
}
uint64_t TempClustering::getMaxMemoryUsage() {
    return getMemoryUsage();      // the memory usage is always increasing
}