#include "common.h"
#include "template.h"

TempClustering::TempClustering(int k_, int d_) {
    k = k_;
    d = d_;
}

void TempClustering::update(const Point & point, bool insert) {
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