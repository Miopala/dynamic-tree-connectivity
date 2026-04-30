#include "../include/brute-force.hpp"
#include "../include/core.hpp"
#include "../include/euler-tour-trees.hpp"
#include "../include/euler-tour.hpp"
#include "../include/micro-tree.hpp"
#include "../include/small-to-large.hpp"
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    if (argc != 2) {
        std::cerr << "usage: " << argv[0]
                  << " [brute-force|euler-tour|small-to-large|micro-trees|euler-tour-trees]";
        return EXIT_FAILURE;
    }

    int n, m;
    std::cin >> n >> m;
    std::vector<Edge> edges(n - 1);
    for (int i = 0; i < n - 1; i++) {
        std::cin >> edges[i].u >> edges[i].v;
        if (edges[i].u > edges[i].v) {
            std::swap(edges[i].u, edges[i].v);
        }
    }

    std::string_view chosen_algorithm(argv[1]);
    std::unique_ptr<DecrementalConnectivitySolver> solver;

    if (chosen_algorithm == "brute-force") {
        solver = std::make_unique<NaiveSolver>(n, edges);
    } else if (chosen_algorithm == "euler-tour") {
        solver = std::make_unique<EulerTourSolver>(n, edges);
    } else if (chosen_algorithm == "small-to-large") {
        solver = std::make_unique<SmallToLargeSolver>(n, edges);
    } else if (chosen_algorithm == "micro-trees") {
        solver = std::make_unique<MicroTreeSolver>(n, edges);
    } else if (chosen_algorithm == "euler-tour-trees") {
        solver = std::make_unique<EulerTourTreesSolver>(n, edges);
    }

    else {
        return EXIT_FAILURE;
    }

    for (int i = 0; i < m; i++) {
        char type;
        int u, v;
        std::cin >> type >> u >> v;
        if (type == 'C')
            solver->cut(u, v);
        else
            std::cout << (solver->connected(u, v) ? "YES\n" : "NO\n");
    }
    return EXIT_SUCCESS;
}