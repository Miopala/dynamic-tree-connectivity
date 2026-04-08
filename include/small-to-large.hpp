#pragma once
#include "core.hpp"
#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_map>

// TODO : add physically erasing edges not only marking as erased

class SmallToLargeSolver : public DecrementalConnectivitySolver {
public:
    SmallToLargeSolver(int _n, const std::vector<Edge>& edges) : n(_n) {
        neighbors.resize(n);
        component_id.resize(n, 1);
        edge_erased.resize(edges.size(), false);
        vis.resize(n, 0);
        for (int i = 0; i < edges.size(); i++) {
            const auto& [u, v] = edges[i];
            neighbors[u].emplace_back(v, i);
            neighbors[v].emplace_back(u, i);
            edge_to_id[make_key(u, v)] = i;
        }
    }

    static inline long long make_key(int u, int v) {
        if (u > v)
            std::swap(u, v);
        return (static_cast<long long>(u) << 32) | static_cast<unsigned int>(v);
    }

    struct OneStepTraverse {
        std::queue<int> q;
        SmallToLargeSolver* ptr;
        int source;
        int current_vertex;
        int neighbor_idx_last;
        int our_vis_ptr;
        std::vector<int> componentVertices;
        OneStepTraverse(SmallToLargeSolver* _ptr, int _source) : source(_source), ptr(_ptr) {
            current_vertex = -1;
            neighbor_idx_last = -1;
            ptr->vis_ptr++;
            our_vis_ptr = ptr->vis_ptr;
            q.push(source);
            componentVertices.push_back(source);
            ptr->vis[source] = ptr->vis_ptr;
        }

        bool onestep() {
            while (true) {
                if (current_vertex == -1) {
                    if (q.empty())
                        return false;
                    current_vertex = q.front();
                    q.pop();
                    neighbor_idx_last = -1;
                }
                neighbor_idx_last++;

                if (neighbor_idx_last < ptr->neighbors[current_vertex].size()) {
                    auto& [v, edge_idx] = ptr->neighbors[current_vertex][neighbor_idx_last];
                    if (ptr->edge_erased[edge_idx] || ptr->vis[v] == our_vis_ptr) {
                        continue;
                    }
                    q.push(v);
                    componentVertices.push_back(v);
                    ptr->vis[v] = our_vis_ptr;
                    return true;
                } else {
                    current_vertex = -1;
                }
            }
            return false;
        }
    };

    void cut(int u, int v) override {
        long long key = make_key(u, v);
        if (edge_to_id.find(key) == edge_to_id.end())
            return;
        int id = edge_to_id[key];
        if (edge_erased[id])
            return;
        edge_erased[id] = true;

        OneStepTraverse s1(this, u);
        OneStepTraverse s2(this, v);
        std::vector<int>* choose = nullptr;
        while (true) {
            if (s1.q.empty() && s1.current_vertex == -1) {
                choose = &s1.componentVertices;
                break;
            } else if (s2.q.empty() && s2.current_vertex == -1) {
                choose = &s2.componentVertices;
                break;
            } else {
                s1.onestep();
                s2.onestep();
            }
        }

        assert(choose != nullptr);
        counter++;
        for (const auto& u : *choose)
            component_id[u] = counter;
    }

    bool connected(int u, int v) override {
        if (u == v)
            return true;

        return component_id[u] == component_id[v];
    }

private:
    const int n;
    int counter = 1;
    std::vector<int> component_id;
    std::vector<bool> edge_erased;
    std::vector<int> vis;
    int vis_ptr = 1;
    std::unordered_map<long long, int> edge_to_id;
    std::vector<std::vector<std::pair<int, int>>> neighbors;
};