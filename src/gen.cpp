#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

struct Edge {
    int u;
    int v;
};

struct GeneratorDSU {
    std::vector<int> parent;
    std::vector<std::vector<int>> members;

    explicit GeneratorDSU(int node_count) : parent(node_count), members(node_count) {
        std::iota(parent.begin(), parent.end(), 0);
        for (int i = 0; i < node_count; i++)
            members[i].push_back(i);
    }

    int find(int u) {
        if (parent[u] != u)
            parent[u] = find(parent[u]);
        return parent[u];
    }

    bool unite(int u, int v) {
        int root_u = find(u);
        int root_v = find(v);
        if (root_u == root_v)
            return false;

        if (members[root_u].size() > members[root_v].size())
            std::swap(root_u, root_v);

        parent[root_u] = root_v;
        members[root_v].insert(members[root_v].end(), members[root_u].begin(),
                               members[root_u].end());
        members[root_u].clear();
        return true;
    }

    int get_random_same_component_node(int u, std::mt19937& rng) {
        int root = find(u);
        const auto& component_nodes = members[root];
        if (component_nodes.size() <= 1) {
            return u;
        }

        std::uniform_int_distribution<size_t> dist(0, component_nodes.size() - 1);
        int v = component_nodes[dist(rng)];
        while (v == u) {
            v = component_nodes[dist(rng)];
        }
        return v;
    }
};

void gen_line(int node_count, std::vector<Edge>& edges) {
    for (int i = 1; i < node_count; ++i) {
        edges.push_back({i - 1, i});
    }
}

void gen_star(int node_count, std::vector<Edge>& edges) {
    for (int i = 1; i < node_count; ++i) {
        edges.push_back({0, i});
    }
}

void gen_balanced_binary(int node_count, std::vector<Edge>& edges) {
    for (int i = 1; i < node_count; ++i) {
        edges.push_back({i / 2, i});
    }
}

void gen_random(int node_count, std::vector<Edge>& edges, std::mt19937& rng) {
    for (int i = 1; i < node_count; ++i) {
        std::uniform_int_distribution<int> dist(0, i - 1);
        edges.push_back({dist(rng), i});
    }
}

void gen_deep_random(int node_count, std::vector<Edge>& edges, std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(1, std::max(1, 100));
    for (int i = 1; i < node_count; ++i) {
        int back = dist(rng);
        int parent = std::max(0, i - back);
        edges.push_back({parent, i});
    }
}

void gen_caterpillar(int node_count, std::vector<Edge>& edges, std::mt19937& rng) {
    if (node_count <= 1)
        return;
    int spine_size =
        std::min(node_count, std::max(2, static_cast<int>(node_count / std::log2(node_count))));
    for (int i = 1; i < spine_size; ++i) {
        edges.push_back({i - 1, i});
    }
    std::uniform_int_distribution<int> dist(0, spine_size - 1);
    for (int i = spine_size; i < node_count; ++i) {
        edges.push_back({dist(rng), i});
    }
}

void gen_caterpillar_with_trees(int node_count, std::vector<Edge>& edges, std::mt19937& rng) {
    if (node_count <= 1)
        return;
    int spine_size =
        std::min(node_count, std::max(2, static_cast<int>(node_count / std::log2(node_count))));
    for (int i = 1; i < spine_size; ++i) {
        edges.push_back({i - 1, i});
    }
    for (int i = spine_size; i < node_count; ++i) {
        std::uniform_int_distribution<int> dist(0, i - 1);
        edges.push_back({dist(rng), i});
    }
}

void gen_misc_1(int node_count, std::vector<Edge>& edges, std::mt19937& rng) {
    if (node_count <= 1)
        return;
    int path_size = node_count / 2;
    for (int i = 1; i < path_size; ++i) {
        std::uniform_int_distribution<int> stick_dist(1, std::min(i, 5));
        edges.push_back({i - stick_dist(rng), i});
    }
    for (int i = path_size; i < node_count; ++i) {
        int stick_end_start = path_size - std::max(1, path_size / 20);
        std::uniform_int_distribution<int> head_dist(stick_end_start, i - 1);
        edges.push_back({head_dist(rng), i});
    }
}

void gen_misc_2(int node_count, std::vector<Edge>& edges, std::mt19937& rng) {
    if (node_count <= 1)
        return;

    int k = std::min(node_count - 1, std::max(2, static_cast<int>(std::sqrt(node_count))));

    for (int i = 1; i <= k; ++i) {
        edges.push_back({0, i});
    }

    std::uniform_int_distribution<int> noise_prob(1, 100);
    for (int i = k + 1; i < node_count; ++i) {
        if (noise_prob(rng) <= 15) {
            std::uniform_int_distribution<int> rand_p(0, i - 1);
            edges.push_back({rand_p(rng), i});
        } else {
            std::uniform_int_distribution<int> jitter(0, std::min(i, 2));
            int parent = std::max(0, i - k - jitter(rng));
            edges.push_back({parent, i});
        }
    }
}

struct Query {
    char type;
    int u;
    int v;
};

void print_test(int node_count, int query_count, std::vector<Edge>& edges, std::mt19937& rng,
                double cut_ratio, bool same_comp_heavy, double same_comp_ratio = 0.8) {
    std::vector<int> labels(node_count);
    std::iota(labels.begin(), labels.end(), 0);
    std::shuffle(labels.begin(), labels.end(), rng);

    std::cout << node_count << " " << query_count << "\n";

    std::shuffle(edges.begin(), edges.end(), rng);
    for (auto& edge : edges) {
        int u = labels[edge.u];
        int v = labels[edge.v];
        if (std::uniform_int_distribution<int>(0, 1)(rng)) {
            std::swap(u, v);
        }
        std::cout << u << " " << v << "\n";
    }

    std::vector<int> edge_indices(edges.size());
    std::iota(edge_indices.begin(), edge_indices.end(), 0);
    std::shuffle(edge_indices.begin(), edge_indices.end(), rng);

    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<int> node_dist(0, node_count - 1);

    std::vector<Query> queries(query_count);
    std::vector<bool> edge_ever_cut(edges.size(), false);
    size_t cut_ptr = 0;

    for (int i = 0; i < query_count; ++i) {
        bool do_cut = (cut_ptr < edges.size()) && (prob_dist(rng) < cut_ratio);
        if (do_cut) {
            int idx = edge_indices[cut_ptr++];
            edge_ever_cut[idx] = true;
            queries[i] = {'C', edges[idx].u, edges[idx].v};
        } else {
            queries[i] = {'Q', node_dist(rng), node_dist(rng)};
        }
    }

    if (same_comp_heavy) {
        GeneratorDSU dsu(node_count);
        for (size_t i = 0; i < edges.size(); ++i) {
            if (!edge_ever_cut[i]) {
                dsu.unite(edges[i].u, edges[i].v);
            }
        }

        for (int i = query_count - 1; i >= 0; --i) {
            if (queries[i].type == 'C') {
                dsu.unite(queries[i].u, queries[i].v);
            } else {
                if (prob_dist(rng) < same_comp_ratio) {
                    int root = dsu.find(queries[i].u);
                    if (dsu.members[root].size() > 1)
                        queries[i].v = dsu.get_random_same_component_node(queries[i].u, rng);
                }
            }
        }
    }

    for (const auto& q : queries) {
        std::cout << q.type << " " << labels[q.u] << " " << labels[q.v] << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    constexpr size_t io_buffer_size = 1 << 20;
    std::vector<char> io_buffer(io_buffer_size);
    std::cout.rdbuf()->pubsetbuf(io_buffer.data(), io_buffer_size);

    if (argc < 5 || argc > 8) {
        std::cerr << "Usage: " << argv[0]
                  << " <node_count> <query_count> <seed> <tree_type> [cut_ratio] [same_comp_heavy] "
                     "[same_comp_ratio]\n";
        return EXIT_FAILURE;
    }

    int node_count = std::stoi(argv[1]);

    if (node_count < 1) {
        std::cerr << "Node count must be at least 1!\n";
        return EXIT_FAILURE;
    }

    int query_count = std::stoi(argv[2]);
    uint32_t seed = std::stoul(argv[3]);
    std::string tree_type = argv[4];

    if (query_count < 0) {
        std::cerr << "Query count must be at least zero!\n";
        return EXIT_FAILURE;
    }

    double cut_ratio = (argc > 5) ? std::stod(argv[5]) : 0.2;
    int same_comp_heavy_value = (argc > 6) ? std::stoi(argv[6]) : 1;

    if (same_comp_heavy_value != 0 && same_comp_heavy_value != 1) {
        std::cerr << "same_comp_heavy must be 0 or 1.\n";
        return EXIT_FAILURE;
    }

    bool same_comp_heavy = same_comp_heavy_value == 1;

    double same_comp_ratio = (argc > 7) ? std::stod(argv[7]) : 0.8;

    if (std::min(cut_ratio, same_comp_ratio) < 0 || std::max(cut_ratio, same_comp_ratio) > 1) {
        std::cerr << "One of the ratio is not in the range [0,1]!\n";
        return EXIT_FAILURE;
    }
    std::mt19937 rng(seed);
    std::vector<Edge> edges;
    edges.reserve(node_count - 1);

    if (tree_type == "random") {
        gen_random(node_count, edges, rng);
    } else if (tree_type == "star") {
        gen_star(node_count, edges);
    } else if (tree_type == "line") {
        gen_line(node_count, edges);
    } else if (tree_type == "balanced_binary") {
        gen_balanced_binary(node_count, edges);
    } else if (tree_type == "deep") {
        gen_deep_random(node_count, edges, rng);
    } else if (tree_type == "caterpillar") {
        gen_caterpillar(node_count, edges, rng);
    } else if (tree_type == "caterpillar_with_trees") {
        gen_caterpillar_with_trees(node_count, edges, rng);
    } else if (tree_type == "misc_1") {
        gen_misc_1(node_count, edges, rng);
    } else if (tree_type == "misc_2") {
        gen_misc_2(node_count, edges, rng);
    } else {
        std::cerr << "Wrong topology of the tree!\n";
        return EXIT_FAILURE;
    }

    print_test(node_count, query_count, edges, rng, cut_ratio, same_comp_heavy, same_comp_ratio);

    return EXIT_SUCCESS;
}