#pragma once
#include <utility>
#include <vector>

struct Edge {
    int u, v;
};

class DecrementalConnectivitySolver {
public:
    virtual ~DecrementalConnectivitySolver() = default;
    virtual void cut(int u, int v) = 0;
    virtual bool connected(int u, int v) = 0;
};