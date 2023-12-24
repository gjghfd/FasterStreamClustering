#pragma once

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <random>
#include <ctime>

#include <sys/sysctl.h>
#include <mach/mach.h>
#include <unistd.h>
#include <sys/types.h>
#include <mach/vm_region.h>
#include <mach/shared_region.h>

using namespace std;

struct Point {
    vector<double> value;   // (value[0], ..., value[d-1])
};

extern vector<Point> KMeans(const vector<Point> & points, int k, int d);