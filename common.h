#pragma once

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <random>
#include <ctime>
#include <vector>
#include <cmath>
#include <map>
#include <sys/sysctl.h>
#include <unistd.h>

using namespace std;

struct Point {
    double weight;
    vector<double> value;   // (value[0], ..., value[d-1])
};

extern vector<Point> KMeans(const vector<Point> & points, int k, int d);
extern double myRand(double interval);
extern double squaredDistance(const Point & point1, const Point & point2, int d);
extern double squaredDistance(const Point & point, const vector<Point> & centers, int num_centers, int d);
extern double squaredDistance(const vector<Point> & points, const vector<Point> & centers, int num_centers, int d);
extern double squaredDistance(const Point *point1, const Point *point2, int d);
extern int findNearest(const Point & point, const vector<Point> & centers, int num_centers, int d);
