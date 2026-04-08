#pragma once
#include "core.hpp"
#include <algorithm>
#include <queue>

class NaiveSolver : public DecrementalConnectivitySolver {
public:
    NaiveSolver(int _n, const std::vector<Edge>& edges) : n(_n) {
        neighbors.resize(n);
        for (const auto& [u, v] : edges) {
            neighbors[u].emplace_back(v);
            neighbors[v].emplace_back(u);
        }
    }

    void cut(int u, int v) override {
        auto remove_from_adj = [&](int a, int b) {
            auto it = std::find(neighbors[a].begin(), neighbors[a].end(), b);
            if (it != neighbors[a].end()) {
                std::iter_swap(it, neighbors[a].end() - 1);
                neighbors[a].pop_back();
            }
        };

        remove_from_adj(u, v);
        remove_from_adj(v, u);
    }

    bool connected(int u, int v) override {
        if (u == v)
            return true;
        std::queue<int> q;
        std::vector<bool> vis(n, false);
        q.push(u);
        vis[u] = true;
        while (!q.empty()) {
            int cur = q.front();
            q.pop();
            for (auto nei : neighbors[cur]) {
                if (nei == v)
                    return true;
                if (vis[nei])
                    continue;
                vis[nei] = true;
                q.push(nei);
            }
        }

        return false;
    }

private:
    const int n;
    std::vector<std::vector<int>> neighbors;
};