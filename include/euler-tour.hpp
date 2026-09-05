#pragma once
#include "core.hpp"
#include <utility>
#include <vector>

class FenwickTree {
public:
    explicit FenwickTree(int size) : tree(size + 1, 0) {}

    void update(int i, int delta) {
        for (++i; i < static_cast<int>(tree.size()); i += i & -i) {
            tree[i] += delta;
        }
    }

    int query(int i) const {
        int sum = 0;
        for (++i; i > 0; i -= i & -i) {
            sum += tree[i];
        }
        return sum;
    }

    int query(int l, int r) const {
        if (l > r)
            return 0;
        return query(r) - (l > 0 ? query(l - 1) : 0);
    }

private:
    std::vector<int> tree;
};

class EulerTourSolver : public DecrementalConnectivitySolver {
public:
    EulerTourSolver(int node_count, const std::vector<Edge>& edges)
        : parent(node_count), neighbors(node_count), entry_time(node_count), exit_time(node_count),
          depth(node_count, 0), parent_edge_cut(node_count, 0), fenwick_tree(2 * node_count) {
        for (const auto& [u, v] : edges) {
            neighbors[u].emplace_back(v);
            neighbors[v].emplace_back(u);
        }

        int power_of_two = 1;
        while (power_of_two < node_count) {
            max_log++;
            power_of_two *= 2;
        }

        ancestors.assign(node_count, std::vector<int>(max_log + 1, 0));
        dfs(0, 0);
        for (int level = 1; level <= max_log; level++) {
            for (int i = 0; i < node_count; i++) {
                ancestors[i][level] = ancestors[ancestors[i][level - 1]][level - 1];
            }
        }
    }

    void cut(int u, int v) override {
        if (u == parent[v])
            std::swap(u, v);
        update(u, 1);
    }

    bool connected(int u, int v) override {
        return u == v || query(u, v) == 0;
    }

private:
    std::vector<int> parent;
    std::vector<std::vector<int>> ancestors;
    std::vector<std::vector<int>> neighbors;
    std::vector<int> entry_time, exit_time;
    std::vector<int> depth;
    std::vector<int> parent_edge_cut;
    int euler_position = 0;
    int max_log = 0;
    FenwickTree fenwick_tree;

    void update(int u, int value) {
        const int delta = value - parent_edge_cut[u];
        fenwick_tree.update(entry_time[u], delta);
        fenwick_tree.update(exit_time[u], -delta);
        parent_edge_cut[u] = value;
    }

    int query(int u, int v) const {
        const int LCA = lca(u, v);
        return fenwick_tree.query(entry_time[LCA], entry_time[u]) +
               fenwick_tree.query(entry_time[LCA], entry_time[v]) - parent_edge_cut[LCA] * 2;
    }

    int lca(int u, int v) const {
        if (depth[u] > depth[v])
            std::swap(u, v);
        for (int i = max_log; i >= 0; i--) {
            if (depth[ancestors[v][i]] >= depth[u]) {
                v = ancestors[v][i];
            }
        }

        if (u == v)
            return u;

        for (int i = max_log; i >= 0; i--) {
            if (ancestors[v][i] != ancestors[u][i]) {
                v = ancestors[v][i];
                u = ancestors[u][i];
            }
        }
        return ancestors[u][0];
    }

    void dfs(int u, int parent_vertex) {
        ancestors[u][0] = parent[u] = parent_vertex;
        entry_time[u] = euler_position++;
        for (auto v : neighbors[u]) {
            if (parent_vertex == v)
                continue;
            depth[v] = depth[u] + 1;
            dfs(v, u);
        }
        exit_time[u] = euler_position++;
    }
};
