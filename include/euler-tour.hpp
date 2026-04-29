#pragma once
#include "core.hpp"
#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_map>

class FenwickTree {
public:
    FenwickTree() {}
    explicit FenwickTree(int n) : tree(n + 1, 0) {}

    void resize(int n) {
        tree.resize(n + 1, 0);
    }

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
    EulerTourSolver(int _n, const std::vector<Edge>& edges) : n(_n) {
        neighbors.resize(n);
        par.resize(n);
        depth.resize(n, 0);
        cur_val.resize(n, 0);
        pre.resize(n);
        post.resize(n);
        for (int i = 0; i < edges.size(); i++) {
            const auto& [u, v] = edges[i];
            neighbors[u].emplace_back(v);
            neighbors[v].emplace_back(u);
        }
        int POW = 1;
        while (POW < n) {
            LOG++, POW *= 2;
        }
        A = std::move(std::vector<std::vector<int>>(n, std::vector<int>(LOG + 1, 0)));
        dfs(0, 0);
        for (int lvl = 1; lvl <= LOG; lvl++) {
            for (int i = 0; i < n; i++) {
                A[i][lvl] = A[A[i][lvl - 1]][lvl - 1];
            }
        }
        f.resize(euler_tour.size());
    }

    void update(int u, int val) {
        int delta = val - cur_val[u];
        f.update(pre[u], delta);
        f.update(post[u], -delta);
        cur_val[u] = val;
    }

    int query(int u, int v) {
        int LCA = lca(u, v);
        return f.query(pre[LCA], pre[u]) + f.query(pre[LCA], pre[v]) - cur_val[LCA] * 2;
    }

    int lca(int u, int v) {
        if (depth[u] > depth[v])
            std::swap(u, v);
        for (int i = LOG; i >= 0; i--) {
            if (depth[A[v][i]] >= depth[u]) {
                v = A[v][i];
            }
        }

        if (u == v)
            return u;

        for (int i = LOG; i >= 0; i--) {
            if (A[v][i] != A[u][i])
                v = A[v][i], u = A[u][i];
        }
        return A[u][0];
    }

    void dfs(int u, int fa) {
        A[u][0] = par[u] = fa;
        euler_tour.push_back(u);
        pre[u] = euler_tour.size() - 1;
        for (auto v : neighbors[u]) {
            if (fa == v)
                continue;
            depth[v] = depth[u] + 1;
            dfs(v, u);
        }
        euler_tour.push_back(u);
        post[u] = euler_tour.size() - 1;
    }

    void cut(int u, int v) override {
        if (u == par[v])
            std::swap(u, v); 
        update(u, 1);
    }

    bool connected(int u, int v) override {
        if (u == v || query(u, v) == 0)
            return true;
        return false;
    }

private:
    const int n;
    std::vector<int> par;
    std::vector<std::vector<int>> A;
    std::vector<std::vector<int>> neighbors;
    std::vector<int> euler_tour;
    std::vector<int> pre, post;
    std::vector<int> depth;
    std::vector<int> cur_val;
    int LOG = 0;
    FenwickTree f;
};