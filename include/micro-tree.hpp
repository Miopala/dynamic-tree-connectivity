#pragma once
#include "core.hpp"
#include "small-to-large.hpp"
#include <algorithm>
#include <bit>
#include <cassert>
#include <limits>
#include <vector>

class MicroTreeSolver : public DecrementalConnectivitySolver {
public:
    MicroTreeSolver(int node_count, const std::vector<Edge>& edges)
        : node_count(node_count), neighbors(node_count),
          log_n(std::max(1, std::bit_width(static_cast<unsigned>(node_count)) - 1)) {

        for (const auto& [u, v] : edges) {
            neighbors[u].emplace_back(v);
            neighbors[v].emplace_back(u);
        }

        // Rooted tree preprocessing

        subtree_size.resize(node_count, 0);
        depth.resize(node_count, 0);
        preorder.resize(node_count, 0);
        father.resize(node_count, 0);
        parent_edge_cut.resize(node_count, false);

        dfs_init(0, -1);

        // Microtree preprocessing

        microtree_local_id.resize(node_count, -1);
        microtree_id.resize(node_count, -1);
        microtree_active_mask.resize(microtree_roots.size(), FULL_MASK);
        microtree_prefix_mask.resize(node_count, 0);
        for (size_t i = 0; i < microtree_roots.size(); i++) {
            microtree_local_id_counter = 0;
            dfs_microtree(microtree_roots[i], father[microtree_roots[i]], microtree_roots[i], i);
        }

        // Macrotree adjacency

        macro_neighbors.resize(node_count);
        for (int i = 0; i < node_count; i++) {
            for (auto neighbor : neighbors[i]) {
                if (microtree_id[neighbor] == -1) {
                    macro_neighbors[i].emplace_back(neighbor);
                }
            }
        }

        // Macrotree chain decomposition

        if (subtree_size[0] >= log_n) {

            macro_vertex_id.resize(node_count, -1);
            chunk_local_id.resize(node_count, -1);
            chunk_prefix_mask.resize(node_count, -1);
            chunk_suffix_mask.resize(node_count, -1);
            chunk_size.resize(node_count, -1);
            chunk_vertices.resize(node_count);
            chunk_id.resize(node_count, -1);
            chunk_active_mask.resize(node_count, 0);
            dfs_macrotree(0, -1);

            for (auto u : chunk_roots) {
                int chunk_index = chunk_id[u];
                assert(chunk_vertices[chunk_index][0] == u);
                chunk_prefix_mask[u] = 0;
                chunk_active_mask[chunk_index] = FULL_MASK;
                for (size_t i = 1; i < chunk_vertices[chunk_index].size(); i++) {
                    chunk_prefix_mask[chunk_vertices[chunk_index][i]] =
                        chunk_prefix_mask[chunk_vertices[chunk_index][i - 1]] |
                        (1 << chunk_local_id[chunk_vertices[chunk_index][i]]);
                }

                chunk_suffix_mask[chunk_vertices[chunk_index].back()] = 0;

                for (int i = chunk_vertices[chunk_index].size() - 2; i >= 0; i--) {
                    chunk_suffix_mask[chunk_vertices[chunk_index][i]] =
                        chunk_suffix_mask[chunk_vertices[chunk_index][i + 1]] |
                        (1 << chunk_local_id[chunk_vertices[chunk_index][i + 1]]);
                }
            }
            std::vector<Edge> macro_edges;
            for (auto u : chunk_roots) {
                const int macro_vertex = macro_vertex_id[u];
                macro_edges.push_back({macro_vertex, macro_vertex + 1});
            }
            for (int i = 0; i < node_count; i++) {
                if (microtree_id[i] != -1)
                    continue;

                for (auto v : macro_neighbors[i]) {
                    if (i > v)
                        continue;
                    if (chunk_id[i] != -1 && chunk_id[i] == chunk_id[v])
                        continue;
                    int macro_u = macro_vertex_id[i];
                    if (chunk_local_id[i] != -1 && chunk_vertices[chunk_id[i]].back() == i) {
                        macro_u++;
                    }
                    int macro_v = macro_vertex_id[v];
                    if (chunk_local_id[v] != -1 && chunk_vertices[chunk_id[v]].back() == v) {
                        macro_v++;
                    }

                    macro_edges.emplace_back(macro_u, macro_v);
                }
            }

            assert(macro_edges.size() == static_cast<size_t>(macro_vertex_count) - 1);

            macro_tree_solver.setup(macro_vertex_count, macro_edges);
        }
    }

    void cut(int u, int v) override {
        if (depth[u] > depth[v])
            std::swap(u, v);
        if (parent_edge_cut[v])
            return;
        parent_edge_cut[v] = true;

        if (microtree_id[v] != -1) {
            if (microtree_id[u] == microtree_id[v]) {
                const int microtree_index = microtree_id[v];
                microtree_active_mask[microtree_index] ^= (1 << microtree_local_id[v]);
            }
            return;
        }

        if (chunk_id[v] != -1 && chunk_id[u] == chunk_id[v]) {
            const int chunk_index = chunk_id[v];
            chunk_active_mask[chunk_index] ^= (1 << chunk_local_id[v]);

            const int macro_vertex = macro_vertex_id[u];
            macro_tree_solver.cut(macro_vertex, macro_vertex + 1);
            return;
        }

        int macro_u = macro_vertex_id[u];
        if (chunk_local_id[u] != -1 && chunk_vertices[chunk_id[u]].back() == u) {
            macro_u++;
        }

        int macro_v = macro_vertex_id[v];
        if (chunk_local_id[v] != -1 && chunk_vertices[chunk_id[v]].back() == v) {
            macro_v++;
        }

        macro_tree_solver.cut(macro_u, macro_v);
    }

    bool connected(int u, int v) override {

        if (microtree_id[u] == microtree_id[v] && microtree_id[u] != -1) {
            int microtree_index = microtree_id[u];
            int path_mask = microtree_prefix_mask[u] ^ microtree_prefix_mask[v];
            return (microtree_active_mask[microtree_index] & path_mask) == path_mask;
        }

        if (microtree_id[u] != -1) {
            int microtree_index = microtree_id[u];
            if ((microtree_active_mask[microtree_index] & microtree_prefix_mask[u]) !=
                microtree_prefix_mask[u]) {
                return false;
            }
            u = microtree_roots[microtree_index];
            if (parent_edge_cut[u])
                return false;

            if (u != 0)
                u = father[u];
        }

        if (microtree_id[v] != -1) {
            int microtree_index = microtree_id[v];
            if ((microtree_active_mask[microtree_index] & microtree_prefix_mask[v]) !=
                microtree_prefix_mask[v]) {
                return false;
            }
            v = microtree_roots[microtree_index];
            if (parent_edge_cut[v])
                return false;
            if (v != 0)
                v = father[v];
        }
        // Both vertices are now in the macrotree.

        if (chunk_id[u] == chunk_id[v] && chunk_id[u] != -1) {
            auto active_mask = chunk_active_mask[chunk_id[u]];
            auto path_mask = chunk_prefix_mask[u] ^ chunk_prefix_mask[v];
            return (path_mask & active_mask) == path_mask;
        }

        // Move u out of its chunk toward v.

        if (chunk_local_id[u] != -1) {
            if (preorder[u] <= preorder[v] && preorder[u] + subtree_size[u] - 1 >= preorder[v]) {
                auto active_mask = chunk_active_mask[chunk_id[u]];
                if ((active_mask & chunk_suffix_mask[u]) != chunk_suffix_mask[u]) {
                    return false;
                }

                u = chunk_vertices[chunk_id[u]].back();
                int next_vertex = -1;
                assert(macro_neighbors[u].size() == 2);
                for (auto neighbor : macro_neighbors[u]) {
                    if (depth[neighbor] > depth[u]) {
                        next_vertex = neighbor;
                        break;
                    }
                }

                assert(next_vertex != -1);

                if (parent_edge_cut[next_vertex])
                    return false;
                u = next_vertex;

            } else {
                auto active_mask = chunk_active_mask[chunk_id[u]];
                if ((active_mask & chunk_prefix_mask[u]) != chunk_prefix_mask[u]) {
                    return false;
                }
                u = chunk_vertices[chunk_id[u]][0];
                if (parent_edge_cut[u])
                    return false;
                if (u != 0)
                    u = father[u];
            }
        }

        if (chunk_id[u] == chunk_id[v] && chunk_id[u] != -1) {
            auto active_mask = chunk_active_mask[chunk_id[u]];
            auto path_mask = chunk_prefix_mask[u] ^ chunk_prefix_mask[v];
            return (path_mask & active_mask) == path_mask;
        }

        if (chunk_local_id[v] != -1) {
            if (preorder[v] <= preorder[u] && preorder[v] + subtree_size[v] - 1 >= preorder[u]) {
                auto active_mask = chunk_active_mask[chunk_id[v]];
                if ((active_mask & chunk_suffix_mask[v]) != chunk_suffix_mask[v]) {
                    return false;
                }

                v = chunk_vertices[chunk_id[v]].back();
                int next_vertex = -1;
                assert(macro_neighbors[v].size() == 2);
                for (auto neighbor : macro_neighbors[v]) {
                    if (depth[neighbor] > depth[v]) {
                        next_vertex = neighbor;
                        break;
                    }
                }

                assert(next_vertex != -1);

                if (parent_edge_cut[next_vertex])
                    return false;
                v = next_vertex;

            } else {
                auto active_mask = chunk_active_mask[chunk_id[v]];
                if ((active_mask & chunk_prefix_mask[v]) != chunk_prefix_mask[v]) {
                    return false;
                }
                v = chunk_vertices[chunk_id[v]][0];
                if (parent_edge_cut[v])
                    return false;
                if (v != 0)
                    v = father[v];
            }
        }

        if (chunk_id[u] == chunk_id[v] && chunk_id[u] != -1) {
            auto active_mask = chunk_active_mask[chunk_id[u]];
            auto path_mask = chunk_prefix_mask[u] ^ chunk_prefix_mask[v];
            return (path_mask & active_mask) == path_mask;
        }

        int macro_u = macro_vertex_id[u];
        if (chunk_id[u] != -1 && chunk_vertices[chunk_id[u]].back() == u) {
            macro_u++;
        }

        int macro_v = macro_vertex_id[v];
        if (chunk_id[v] != -1 && chunk_vertices[chunk_id[v]].back() == v) {
            macro_v++;
        }
        return macro_tree_solver.connected(macro_u, macro_v);
    }

private:
    static constexpr int FULL_MASK = std::numeric_limits<int>::max();
    const int node_count;
    std::vector<std::vector<int>> neighbors;
    std::vector<std::vector<int>> macro_neighbors;
    const int log_n;
    std::vector<int> subtree_size;
    std::vector<int> depth;
    std::vector<int> father;
    std::vector<int> microtree_roots;
    std::vector<int> preorder;

    std::vector<int> microtree_local_id;
    std::vector<int> microtree_id;
    std::vector<int> microtree_active_mask;
    std::vector<int> microtree_prefix_mask;
    std::vector<bool> parent_edge_cut;

    std::vector<int> macro_vertex_id;
    std::vector<int> chunk_local_id;
    std::vector<int> chunk_prefix_mask;
    std::vector<int> chunk_suffix_mask;
    std::vector<int> chunk_size;
    std::vector<int> chunk_id;
    std::vector<int> chunk_active_mask;
    std::vector<std::vector<int>> chunk_vertices;

    std::vector<int> chunk_roots;

    int microtree_local_id_counter = 0;
    int macro_vertex_count = 0;
    int chunk_id_counter = 0;
    int preorder_counter = 0;
    SmallToLargeSolver macro_tree_solver;

    void dfs_macrotree(int u, int parent_vertex) {
        const auto macro_degree = macro_neighbors[u].size();

        if (macro_degree != 2 || u == 0) {
            macro_vertex_id[u] = macro_vertex_count++;
        } else { // The root is handled above, so parent_vertex is valid.
            if (chunk_id[parent_vertex] != -1 && chunk_size[chunk_id[parent_vertex]] < log_n) {
                chunk_local_id[u] = chunk_local_id[parent_vertex] + 1;
                chunk_id[u] = chunk_id[parent_vertex];
                chunk_size[chunk_id[u]]++;
                macro_vertex_id[u] = macro_vertex_id[parent_vertex];
            } else { // Start a new chunk.
                chunk_id[u] = chunk_id_counter++;
                chunk_size[chunk_id[u]] = 1;
                chunk_local_id[u] = 0;
                chunk_roots.push_back(u);
                // Reserve the endpoints of the macrotree edge representing this chunk.
                macro_vertex_id[u] = macro_vertex_count;
                macro_vertex_count += 2;
            }

            chunk_vertices[chunk_id[u]].push_back(u);
        }

        for (auto v : macro_neighbors[u]) {
            if (v == parent_vertex)
                continue;
            dfs_macrotree(v, u);
        }
    }

    void dfs_microtree(int u, int parent_vertex, int microtree_root, int microtree_index) {
        microtree_local_id[u] = microtree_local_id_counter++;
        microtree_id[u] = microtree_index;
        if (microtree_root != u) {
            microtree_prefix_mask[u] =
                microtree_prefix_mask[parent_vertex] | (1 << microtree_local_id[u]);
        }

        for (auto v : neighbors[u]) {
            if (v == parent_vertex)
                continue;
            dfs_microtree(v, u, microtree_root, microtree_index);
        }
    }

    void dfs_init(int u, int parent_vertex) {
        subtree_size[u]++;
        preorder[u] = ++preorder_counter;
        father[u] = parent_vertex;
        for (auto v : neighbors[u]) {
            if (v == parent_vertex)
                continue;

            depth[v] = depth[u] + 1;

            dfs_init(v, u);
            subtree_size[u] += subtree_size[v];
        }

        if (u == 0 && subtree_size[u] < log_n) {
            microtree_roots.push_back(u);
        }

        if (subtree_size[u] >= log_n) {
            for (auto v : neighbors[u]) {
                if (v == parent_vertex)
                    continue;
                if (subtree_size[v] < log_n) {
                    microtree_roots.push_back(v);
                }
            }
        }
    }
};
