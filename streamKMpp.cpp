#include "common.h"
#include "streamKMpp.h"

// insert a point into the idx bucket, returns the bucket size
int streamKMplusplus::insertBucket(const Point & point, int idx) {
    if (buckets.size() <= idx) {
        buckets.emplace_back(vector<Point>(0));
        if (buckets.size() <= idx) {
            throw runtime_error("Unexpected condition encountered.");
        }
    }
    buckets[idx].emplace_back(point);
    return buckets[idx].size();
}

void streamKMplusplus::findCoreset(vector<Point> & points) {
    int n = points.size();
    if (n < m) {
        throw runtime_error("Number of given points must be larger than m.");
    }
    if (n == m) return;
    struct Node {
        vector<Point *> points;
        double weight;
        int q;
    };
    vector<Node> leafNodes;
    
    // initialize the root
    Node root;
    root.points.resize(n);
    for (int i = 0; i < n; i++) root.points[i] = &points[i];

    // get the first center
    double sum = 0;
    for (int i = 0; i < n; i++) sum += points[i].weight;
    double value = myRand(sum);
    for (int i = 0; i < n; i++) {
        value -= points[i].weight;
        if (value <= 0 || i == n - 1) {
            // i is the first center
            root.q = i;
            root.weight = 0;
            for (int j = 0; j < n; j++)
                root.weight += squaredDistance(points[j], points[i], d) * points[j].weight;
            break;
        }
    }
    leafNodes.emplace_back(root);

    // split one leaf repeatedly
    vector<double> dis(n);
    for (int i = 1; i < m; i++) {
        // select a leaf
        double sum = 0;
        int numNodes = leafNodes.size();
        for (int j = 0; j < numNodes; j++)
            if (leafNodes[j].weight >= 0 && leafNodes[j].points.size() >= 2) sum += leafNodes[j].weight;

        double value = myRand(sum);
        int selIdx = -1;
        for (int j = 0; j < numNodes; j++)
            if (leafNodes[j].weight >= 0 && leafNodes[j].points.size() >= 2) {
                value -= leafNodes[j].weight;
                if (value <= 0 || j == numNodes - 1) {
                    selIdx = j;
                    break;
                }
            }
        Node & u = leafNodes[selIdx];
        u.weight = -1;

        int n = u.points.size();
        sum = 0;
        for (int j = 0; j < n; j++)
            if (j != u.q) {
                dis[j] = squaredDistance(u.points[j], u.points[u.q], d) * u.points[j]->weight;
                sum += dis[j];
            }

        value = myRand(sum);
        int q2 = 0;
        for (int j = 0; j < n; j++)
            if (j != u.q) {
                value -= dis[j];
                if (value <= 0 || j == n - 1) {
                    // choose this point as another center
                    q2 = j;
                    break;
                }
            }
        
        vector<Point *> points[2];
        int q1_index = -1, q2_index = -1;
        for (int j = 0; j < n; j++) {
            if (j == u.q) q1_index = points[0].size(), points[0].emplace_back(u.points[j]);
            else if (j == q2) q2_index = points[1].size(), points[1].emplace_back(u.points[j]);
            else if (squaredDistance(u.points[j], u.points[u.q], d) < squaredDistance(u.points[j], u.points[q2], d)) {
                points[0].emplace_back(u.points[j]);
            } else {
                points[1].emplace_back(u.points[j]);
            }
        }

        for (int k = 0; k < 2; k++) {
            Node newNode;
            newNode.q = !k ? q1_index : q2_index;
            newNode.points.assign(points[k].begin(), points[k].end());
            newNode.weight = 0;
            for (auto p : points[k])
                newNode.weight += squaredDistance(p, points[k][newNode.q], d) * p->weight;

            leafNodes.emplace_back(newNode);
        }
    }

    // get coreset
    vector<Point> coreset;
    for (auto & nd : leafNodes)
        if (nd.weight >= 0) {
            Point p;
            p.weight = 0;
            for (int i = 0; i < nd.points.size(); i++) {
                p.weight += nd.points[i]->weight;
            }
            p.value.assign(nd.points[nd.q]->value.begin(), nd.points[nd.q]->value.end());

            coreset.emplace_back(p);
        }
    
    if (coreset.size() != m) {
        throw runtime_error("Coreset size dose not equal to m.");
    }

    points.assign(coreset.begin(), coreset.end());
}

streamKMplusplus::streamKMplusplus(int k_, int d_, int m_) {
    k = k_;
    d = d_;
    m = m_;
    maxMemUsage = 0;
}

void streamKMplusplus::update(const Point & point, bool insert) {
    if (point.weight <= 0) {
        throw runtime_error("Point weight must be positive.");
    }
    if (point.value.size() != d) {
        throw runtime_error("Point dimension mismatch.");
    }

    if (!insert) {
        printf("Error: streamKM++ does not support deletion.\n");
        return;
    }

    if (insertBucket(point, 0) == m) {
        vector<Point> S;
        S.assign(buckets[0].begin(), buckets[0].end());
        buckets[0].clear();
        int i = 1;
        while (true) {
            if (buckets.size() <= i) buckets.emplace_back(vector<Point>(0));
            if (!buckets[i].size()) {
                buckets[i].assign(S.begin(), S.end());
                break;
            }
            if (buckets[i].size() != m) {
                throw runtime_error("Bucket size error.");
            }
            // merge points in buckets[i] and S
            for (auto & p : buckets[i]) S.emplace_back(p);
            // clear bucket i
            buckets[i].clear();
            // reduce S and proceed
            findCoreset(S);
            i++;
        }
    }

    maxMemUsage = max(maxMemUsage, getMemoryUsage());
}

vector<Point> streamKMplusplus::getClusters() {
    vector<Point> points;
    for (auto & vec : buckets)
        if (vec.size()) {
            for (auto & p : vec) points.emplace_back(p);
        }
    
    // reduce to m points
    findCoreset(points);

    vector<Point> centers = KMeans(points, k, d);

    return centers;
}

double streamKMplusplus::calculateKMeans(vector<Point> & centers) {
    int k = centers.size();
    vector<Point> points;
    for (auto & vec : buckets)
        if (vec.size()) {
            for (auto & p : vec) points.emplace_back(p);
        }

    // reduce to m points
    findCoreset(points);
    
    return squaredDistanceWeighted(points, centers, k, d);
}

uint64_t streamKMplusplus::getMemoryUsage() {
    uint64_t sum = 0;
    sum += 3 * sizeof(int);                                                     // k, d, m
    for (auto & vec : buckets) sum += vec.size() * (d + 1) * sizeof(double);    // buckets
    return sum;
}

uint64_t streamKMplusplus::getMaxMemoryUsage() {
    return maxMemUsage;
}