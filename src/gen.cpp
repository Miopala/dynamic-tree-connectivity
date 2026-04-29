#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>
struct Edge {
    int u, v;
};

void print_test(int n, int m, std::vector<Edge>& edges, std::mt19937& rng) {
    std::vector<int> labels(n);
    std::iota(labels.begin(), labels.end(), 0);
    std::shuffle(labels.begin(), labels.end(), rng);

    std::cout << n << " " << m << "\n";

    std::shuffle(edges.begin(), edges.end(), rng);
    for (auto& e : edges) {
        int u = labels[e.u];
        int v = labels[e.v];
        if (std::uniform_int_distribution<int>(0, 1)(rng))
            std::swap(u, v);
        std::cout << u << " " << v << "\n";
    }

    std::vector<int> edge_indices(edges.size());
    std::iota(edge_indices.begin(), edge_indices.end(), 0);
    std::shuffle(edge_indices.begin(), edge_indices.end(), rng);

    std::uniform_int_distribution<int> op_random(0, 1);
    std::uniform_int_distribution<int> node_random(0, n - 1);

    int cut_ptr = 0;
    for (int i = 0; i < m; ++i) {
        if (cut_ptr < (int)edges.size() && op_random(rng) == 0) {
            Edge& e = edges[edge_indices[cut_ptr++]];
            std::cout << "C " << labels[e.u] << " " << labels[e.v] << "\n";
        } else {
            std::cout << "Q " << labels[node_random(rng)] << " " << labels[node_random(rng)]
                      << "\n";
        }
    }
}

void gen_random(int n, std::vector<Edge>& edges,
                std::mt19937& rng) { // zupelnie losowe drzewa -> malo glebokie
    for (int i = 1; i < n; ++i) {
        std::uniform_int_distribution<int> dist(0, i - 1);
        edges.push_back({dist(rng), i});
    }
}

void gen_deep_random(int n, std::vector<Edge>& edges,
                     std::mt19937& rng) { // znacznie glebsze losowe drzewa
    std::uniform_int_distribution<int> dist(1, std::max(1, 100));
    for (int i = 1; i < n; ++i) {
        int back = dist(rng);
        int parent = std::max(0, i - back);
        edges.push_back({parent, i});
    }
}

void gen_caterpillar(
    int n, std::vector<Edge>& edges,
    std::mt19937& rng) { // jedna glowna sciezka ze zwisajacymi pojedynczymi punktami
    int spine_size = std::min(n, std::max(2, (int)(n / std::log2(n))));
    for (int i = 1; i < spine_size; ++i) {
        edges.push_back({i - 1, i});
    }
    std::uniform_int_distribution<int> dist(0, spine_size - 1);
    for (int i = spine_size; i < n; ++i) {
        edges.push_back({dist(rng), i});
    }
}

void gen_caterpillar_with_trees(
    int n, std::vector<Edge>& edges,
    std::mt19937& rng) { // jedna glowna sciezka ze zwisajacymi drzewkami
    int spine_size = std::min(n, std::max(2, (int)(n / std::log2(n))));
    for (int i = 1; i < spine_size; ++i) {
        edges.push_back({i - 1, i});
    }

    for (int i = spine_size; i < n; ++i) {
        std::uniform_int_distribution<int> dist(0, i - 1);
        edges.push_back({dist(rng), i});
    }
}

void gen_misc_1(
    int n, std::vector<Edge>& edges,
    std::mt19937& rng) { // tworzymy patyk dlugosci n/2 a potem do koncowki patyka podlaczamy drzewa
    int path_size = n / 2;
    for (int i = 1; i < path_size; ++i) {
        std::uniform_int_distribution<int> stick_dist(1, std::min(i, 5));
        edges.push_back({i - stick_dist(rng), i});
    }
    for (int i = path_size; i < n; ++i) {
        int stick_end_start = path_size - std::max(1, path_size / 20);
        std::uniform_int_distribution<int> head_dist(stick_end_start, i - 1);
        edges.push_back({head_dist(rng), i});
    }
}

void gen_misc_2(
    int n, std::vector<Edge>& edges,
    std::mt19937& rng) { // tworzymy gwizade o sqrt(n) nastepnie dla kazdego kolejnego wierzcholka z
                         // prawd. 15% albo podlaczamy sie do czegokolwiek wczesniej (robimy noise)
                         // albo podlaczamy sie do i-k - cos randomowego malego bardzo, to ma
                         // symulowac takie odnogi z gwiazdy z lekkim szumem
    int k = std::min(n, std::max(2, (int)std::sqrt(n)));
    for (int i = 1; i <= k; ++i) {
        edges.push_back({0, i});
    }
    std::uniform_int_distribution<int> noise_prob(1, 100);
    for (int i = k + 1; i < n; ++i) {
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

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <nodes> <queries> <seed> <type>\n";
        return EXIT_FAILURE;
    }

    int n = std::stoi(argv[1]);
    int m = std::stoi(argv[2]);
    uint32_t seed = std::stoul(argv[3]);
    std::string tree_type = argv[4];
    std::mt19937 rng(seed);
    std::vector<Edge> edges;

    edges.reserve(n - 1);

    if (tree_type == "random") {
        gen_random(n, edges, rng);
    } else if (tree_type == "deep") {
        gen_deep_random(n, edges, rng);
    } else if (tree_type == "caterpillar") {
        gen_caterpillar(n, edges, rng);
    } else if (tree_type == "caterpillar_with_trees") {
        gen_caterpillar_with_trees(n, edges, rng);
    } else if (tree_type == "misc_1") {
        gen_misc_1(n, edges, rng);
    } else if (tree_type == "misc_2") {
        gen_misc_2(n, edges, rng);
    } else {
        gen_random(n, edges, rng);
    }

    print_test(n, m, edges, rng);

    return EXIT_SUCCESS;
}