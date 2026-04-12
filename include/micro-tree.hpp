#pragma once
#include "core.hpp"
#include "small-to-large.hpp"
#include <algorithm>
#include <cmath>
#include <limits.h>
#include <queue>

const int FULL_MSK = INT_MAX;

class MicroTreeSolver : public DecrementalConnectivitySolver {
public:
    MicroTreeSolver(int _n, const std::vector<Edge>& edges) : n(_n), LOG(log2(_n)) {
        neighbors.resize(n);
        for (const auto& [u, v] : edges) {
            neighbors[u].emplace_back(v);
            neighbors[v].emplace_back(u);
        }

        // INIT

        sub.resize(n, 0);
        depth.resize(n, 0);
        pre.resize(n, 0);
        father.resize(n, 0);
        cut_orig.resize(n, false);
        dfs_init(0, -1);

        // MICRO_TREES_PREPROCESSiNG

        microtree_id.resize(n, -1);
        which_microtree.resize(n, -1);
        microtree_mask.resize(microtree_roots.size(), FULL_MSK);
        microtree_prefix_mask.resize(n, 0);
        for (int i = 0; i < microtree_roots.size(); i++) {
            microtree_id_counter = 0;
            dfs_microtree(microtree_roots[i], father[microtree_roots[i]], microtree_roots[i], i);
        }

        // MACRO_TREES_PREPROCESSING

        macro_neighbors.resize(n);
        for (int i = 0; i < n; i++) {
            for (auto nei : neighbors[i]) {
                if (which_microtree[nei] == -1) {
                    macro_neighbors[i].emplace_back(nei);
                }
            }
        }

        /*
            BEDA POTRZEBNE:
            macrotree_id -> nr w macro
            adj[u].size()
            std::vector<std::vector<int>> on_chunk;
            // chunk_id_macro[u] => default -1 jesli adj
            //
            // chunk_size[]

            pref_chunk
            suf_chunk ale to potem



            // pamietaj jesli sub[u] < LOG => wtedy nie robimy macro
              std::vector<int> macrotree_id;
        std::vector<int> chunk_id;
        std::vector<int> chunk_pref;
        std::vector<int> chuk_suf;
        std::vector<int> chunk_size;
        std::vector<std::vector<int>> chunk_vertices;

        std::vector<int> chunk_roots;

        */

        if (sub[0] >= LOG) {

            macrotree_id.resize(n, -1);
            chunk_id.resize(n, -1);
            chunk_pref.resize(n, -1);
            chunk_suf.resize(n, -1);
            chunk_size.resize(n, -1);
            chunk_vertices.resize(n);
            which_chunk.resize(n, -1);
            chunk_mask.resize(n, 0);

            dfs_macrotree(0, -1);

            for (auto u : chunk_roots) {
                int chunk_number = which_chunk[u];
                assert(chunk_vertices[chunk_number][0] == u);
                chunk_pref[u] = 0;
                chunk_mask[chunk_number] = FULL_MSK;
                for (int i = 1; i < chunk_vertices[chunk_number].size(); i++) {
                    chunk_pref[chunk_vertices[chunk_number][i]] =
                        chunk_pref[chunk_vertices[chunk_number][i - 1]] |
                        (1 << chunk_id[chunk_vertices[chunk_number][i]]);
                }

                chunk_suf[chunk_vertices[chunk_number].back()] = 0;

                for (int i = chunk_vertices[chunk_number].size() - 2; i >= 0; i--) {
                    chunk_suf[chunk_vertices[chunk_number][i]] =
                        chunk_suf[chunk_vertices[chunk_number][i + 1]] |
                        (1 << chunk_id[chunk_vertices[chunk_number][i + 1]]);
                }
            }
            std::vector<Edge> macro_edges;
            for (auto u : chunk_roots) {
                int chunk_number = which_chunk[u];
                int cur_macro = macrotree_id[u];
                macro_edges.push_back({cur_macro, cur_macro + 1});
            }
            for (int i = 0; i < n; i++) {
                if (which_microtree[i] != -1)
                    continue;

                for (auto v : macro_neighbors[i]) {
                    int id_1 = macrotree_id[i];
                    if (chunk_id[i] != -1 && chunk_vertices[which_chunk[i]].back() == i) {
                        id_1++;
                    }
                    int id_2 = macrotree_id[v];
                    if (chunk_id[v] != -1 && chunk_vertices[which_chunk[v]].back() == v) {
                        id_2++;
                    }

                    if (id_1 < id_2) {
                        macro_edges.emplace_back(id_1, id_2);
                    }
                }
            }

            assert(macro_edges.size() == macrotree_id_counter - 1);

            macroTreeSolver.setup(macrotree_id_counter, macro_edges);
        }
    }

    void dfs_macrotree(int u, int fa) {
        int real_neighbors_count = macro_neighbors[u].size();
        if (fa != -1)
            real_neighbors_count--;

        if (real_neighbors_count != 2 || u == 0) {
            macrotree_id[u] = macrotree_id_counter++;
        } else { // guarantee here that u is not root so fa can't be -1
            if (which_chunk[fa] != -1 && chunk_size[which_chunk[fa]] < LOG) {
                chunk_id[u] = chunk_id[fa] + 1;
                which_chunk[u] = which_chunk[fa];
                chunk_size[which_chunk[u]]++;
                macrotree_id[u] = macrotree_id[fa];
            } else { // nowy lancuch
                which_chunk[u] = chunk_id_counter++;
                chunk_size[which_chunk[u]] = 1;
                chunk_id[u] = 0;
                chunk_roots.push_back(u);
                macrotree_id[u] = macrotree_id_counter;
                macrotree_id_counter += 2;
                // dla kazdego lancucha tworzymy tak ze gorna czesc laczymy i dolna osobna
            }

            chunk_vertices[which_chunk[u]].push_back(u);
        }

        for (auto v : macro_neighbors[u]) {
            if (v == fa)
                continue;
            dfs_macrotree(v, u);
        }
    }

    void dfs_microtree(int u, int fa, int micro_root, int microtree_nr) {
        microtree_id[u] = microtree_id_counter++;
        which_microtree[u] = microtree_nr;
        if (micro_root != u) {
            microtree_prefix_mask[u] = microtree_prefix_mask[fa] | (1 << microtree_id[u]);
        }

        for (auto v : neighbors[u]) {
            if (v == fa)
                continue;
            dfs_microtree(v, u, micro_root, microtree_nr);
        }
    }

    void dfs_init(int u, int fa) {
        sub[u]++;
        pre[u] = ++pre_counter;
        father[u] = fa;
        for (auto v : neighbors[u]) {
            if (v == fa)
                continue;

            depth[v] = depth[u] + 1;

            dfs_init(v, u);
            sub[u] += sub[v];
        }

        if (u == 0 && sub[u] < LOG) {
            microtree_roots.push_back(u);
        }

        if (sub[u] >= LOG) {
            for (auto v : neighbors[u]) {
                if (v == fa)
                    continue;
                if (sub[v] < LOG) {
                    microtree_roots.push_back(v);
                }
            }
        }
    }

    void cut(int u, int v) override {
        if (depth[u] > depth[v])
            std::swap(u, v);

        // we have ensured that u is parent of v
        if (cut_orig[v])
            return;
        cut_orig[v] = true;

        int microtree_nr = which_microtree[u];

        if (microtree_nr != -1) {
            assert(microtree_id[v] != -1);
            microtree_mask[microtree_nr] ^= (1 << microtree_id[v]);
        }

        int chunk_nr = which_chunk[u];
        if (chunk_nr != -1) {
            assert(chunk_id[v] != -1);
            chunk_mask[chunk_nr] ^= (1 << chunk_id[v]);
        }

        if (which_chunk[u] == which_chunk[v] && which_chunk[u] != -1) {
            int get_macro = macrotree_id[u];
            assert(get_macro != -1);
            macroTreeSolver.cut(get_macro, get_macro + 1);
        } else {
            int id_1 = macrotree_id[u];
            if (chunk_id[u] != -1 && chunk_vertices[which_chunk[u]].back() == u) {
                id_1++;
            }

            int id_2 = macrotree_id[v];
            if (chunk_id[v] != -1 && chunk_vertices[which_chunk[v]].back() == v) {
                id_2++;
            }
            macroTreeSolver.cut(id_1, id_2);
        }
    }

    bool connected(int u, int v) override {

        if (which_microtree[u] == which_microtree[v] && which_microtree[u] != -1) {
            int microtree_nr = which_microtree[u];
            int ksor = microtree_prefix_mask[u] ^ microtree_prefix_mask[v];
            return (microtree_mask[microtree_nr] & ksor) == ksor;
        }

        if (which_microtree[u] != -1) {
            int microtree_nr = which_microtree[u];
            if ((microtree_mask[microtree_nr] & microtree_prefix_mask[u]) !=
                microtree_prefix_mask[u]) {
                return false;
            }
            u = microtree_roots[microtree_nr];
            if (cut_orig[u])
                return false;

            if (u != 0)
                u = father[u];
        }

        if (which_microtree[v] != -1) {
            int microtree_nr = which_microtree[v];
            if ((microtree_mask[microtree_nr] & microtree_prefix_mask[v]) !=
                microtree_prefix_mask[v]) {
                return false;
            }
            v = microtree_roots[microtree_nr];
            if (cut_orig[v])
                return false;
            if (v != 0)
                v = father[v];
        }
        // next phase u and v are both in macro tree

        // case 1: the same chunk_id

        if (which_chunk[u] == which_chunk[v] && which_chunk[u] != -1) {
            auto msk = chunk_mask[which_chunk[u]];
            auto ksor = chunk_pref[u] ^ chunk_pref[v];
            return (ksor & msk) == ksor;
        }

        // different chunks we have to walk the chunks and transport outside

        // first case v is in subtree of u then we go down, otherwise up

        if (chunk_id[u] != -1) {
            if (pre[u] <= pre[v] && pre[u] + sub[u] - 1 >= pre[v]) {
                auto msk = chunk_mask[which_chunk[u]];
                if ((msk & chunk_suf[u]) != chunk_suf[u]) {
                    return false;
                }

                u = chunk_vertices[which_chunk[u]].back();
                int nxt = -1;
                assert(macro_neighbors[u].size() == 2);
                for (auto it : macro_neighbors[u]) {
                    if (depth[it] > depth[u]) {
                        nxt = it;
                        break;
                    }
                }

                assert(nxt != -1);

                if (cut_orig[nxt])
                    return false;
                u = nxt;

            } else {
                auto msk = chunk_mask[which_chunk[u]];
                if ((msk & chunk_pref[u]) != chunk_pref[u]) {
                    return false;
                }
                u = chunk_vertices[which_chunk[u]][0];
                if (cut_orig[u])
                    return false;
                if (u != 0)
                    u = father[u];
            }
        }

        if (which_chunk[u] == which_chunk[v] && which_chunk[u] != -1) {
            auto msk = chunk_mask[which_chunk[u]];
            auto ksor = chunk_pref[u] ^ chunk_pref[v];
            return (ksor & msk) == ksor;
        }

        if (chunk_id[v] != -1) {
            if (pre[v] <= pre[u] && pre[v] + sub[v] - 1 >= pre[u]) {
                auto msk = chunk_mask[which_chunk[v]];
                if ((msk & chunk_suf[v]) != chunk_suf[v]) {
                    return false;
                }

                v = chunk_vertices[which_chunk[v]].back();
                int nxt = -1;
                assert(macro_neighbors[v].size() == 2);
                for (auto it : macro_neighbors[v]) {
                    if (depth[it] > depth[v]) {
                        nxt = it;
                        break;
                    }
                }

                assert(nxt != -1);

                if (cut_orig[nxt])
                    return false;
                v = nxt;

            } else {
                auto msk = chunk_mask[which_chunk[v]];
                if ((msk & chunk_pref[v]) != chunk_pref[v]) {
                    return false;
                }
                v = chunk_vertices[which_chunk[v]][0];
                if (cut_orig[v])
                    return false;
                if (v != 0)
                    v = father[v];
            }
        }

        if (which_chunk[u] == which_chunk[v] && which_chunk[u] != -1) {
            auto msk = chunk_mask[which_chunk[u]];
            auto ksor = chunk_pref[u] ^ chunk_pref[v];
            return (ksor & msk) == ksor;
        }

        int id_1 = macrotree_id[u];
        if (which_chunk[u] != -1 && chunk_vertices[which_chunk[u]].back() == u) {
            id_1++;
        }

        int id_2 = macrotree_id[v];
        if (which_chunk[v] != -1 && chunk_vertices[which_chunk[v]].back() == v) {
            id_2++;
        }
        return macroTreeSolver.connected(id_1, id_2);
    }

private:
    const int n;
    std::vector<std::vector<int>> neighbors;
    std::vector<std::vector<int>> macro_neighbors; // TODO, this may be not the best approach
    const int LOG;
    std::vector<int> sub;
    std::vector<int> depth;
    std::vector<int> father;
    std::vector<int> microtree_roots;
    std::vector<int> pre;

    std::vector<int> microtree_id;
    std::vector<int> which_microtree;
    std::vector<int> microtree_mask;
    std::vector<int> microtree_prefix_mask;
    std::vector<bool> cut_orig;

    std::vector<int> macrotree_id;
    std::vector<int> chunk_id;
    std::vector<int> chunk_pref;
    std::vector<int> chunk_suf;
    std::vector<int> chunk_size;
    std::vector<int> which_chunk;
    std::vector<int> chunk_mask;
    std::vector<std::vector<int>> chunk_vertices;

    std::vector<int> chunk_roots;

    int microtree_id_counter = 0;
    int macrotree_id_counter = 0;
    int chunk_id_counter = 0;
    int pre_counter = 0;
    SmallToLargeSolver macroTreeSolver;
};