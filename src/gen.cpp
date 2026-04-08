#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <nodes> <queries> [seed]\n";
        return EXIT_FAILURE;
    }

    int n = std::stoi(argv[1]);
    int m = std::stoi(argv[2]);
    uint32_t seed = (argc > 3) ? std::stoi(argv[3]) : std::random_device{}();

    std::mt19937 rng(seed);
    std::cout << n << " " << m << "\n";

    std::vector<std::pair<int, int>> edges;
    for (int i = 1; i < n; ++i) {
        std::uniform_int_distribution<int> dist(0, i - 1);
        int parent = dist(rng);
        edges.push_back({parent, i});
        std::cout << parent << " " << i << "\n";
    }

    std::vector<bool> is_cut(edges.size(), false);
    int cut_count = 0;

    std::uniform_int_distribution<int> op_dist(0, 1);
    std::uniform_int_distribution<int> node_dist(0, n - 1);

    for (int i = 0; i < m; ++i) {
        if (cut_count < n - 1 && op_dist(rng) == 0) {
            std::uniform_int_distribution<int> edge_dist(0, edges.size() - 1);
            int idx;
            do {
                idx = edge_dist(rng);
            } while (is_cut[idx]);

            is_cut[idx] = true;
            cut_count++;
            std::cout << "C " << edges[idx].first << " " << edges[idx].second << "\n";
        } else {
            std::cout << "Q " << node_dist(rng) << " " << node_dist(rng) << "\n";
        }
    }

    return EXIT_SUCCESS;
}