#pragma once
#include "core.hpp"
#include <algorithm>
#include <queue>
#include <vector>

class BruteForceSolver : public DecrementalConnectivitySolver {
public:
    BruteForceSolver(int node_count, const std::vector<Edge>& edges)
        : node_count(node_count), neighbors(node_count) {
        for (const auto& [u, v] : edges) {
            neighbors[u].emplace_back(v);
            neighbors[v].emplace_back(u);
        }
    }

    void cut(int u, int v) override {
        auto remove_neighbor = [this](int vertex, int neighbor) {
            auto it = std::find(neighbors[vertex].begin(), neighbors[vertex].end(), neighbor);
            if (it != neighbors[vertex].end()) {
                std::iter_swap(it, neighbors[vertex].end() - 1);
                neighbors[vertex].pop_back();
            }
        };

        remove_neighbor(u, v);
        remove_neighbor(v, u);
    }

    bool connected(int u, int v) override {
        if (u == v)
            return true;
        std::queue<int> queue;
        std::vector<bool> visited(node_count, false);
        queue.push(u);
        visited[u] = true;
        while (!queue.empty()) {
            int current = queue.front();
            queue.pop();
            for (auto neighbor : neighbors[current]) {
                if (neighbor == v)
                    return true;
                if (visited[neighbor])
                    continue;
                visited[neighbor] = true;
                queue.push(neighbor);
            }
        }

        return false;
    }

private:
    const int node_count;
    std::vector<std::vector<int>> neighbors;
};
