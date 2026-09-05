#include "brute-force.hpp"
#include "core.hpp"
#include "euler-tour-trees.hpp"
#include "euler-tour.hpp"
#include "micro-tree.hpp"
#include "small-to-large.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

struct Operation {
    char type;
    int u;
    int v;
};

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: " << argv[0]
                  << " [brute-force|euler-tour|small-to-large|micro-trees|euler-tour-trees] "
                     "[--benchmark]\n";
        return EXIT_FAILURE;
    }

    bool benchmark_mode = false;
    if (argc == 3) {
        if (std::string_view(argv[2]) != "--benchmark") {
            std::cerr << "Unknown option: " << argv[2] << "\n";
            return EXIT_FAILURE;
        }
        benchmark_mode = true;
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

    std::vector<Operation> operations;

    if (benchmark_mode) {
        operations.resize(m);

        for (Operation& operation : operations) {
            std::cin >> operation.type >> operation.u >> operation.v;
        }
    }

    std::string_view chosen_algorithm(argv[1]);
    std::unique_ptr<DecrementalConnectivitySolver> solver;
    std::chrono::steady_clock::time_point preprocessing_start;
    int64_t preprocessing_time_ns = 0;
    if (chosen_algorithm == "brute-force") {
        if (benchmark_mode)
            preprocessing_start = std::chrono::steady_clock::now();
        solver = std::make_unique<BruteForceSolver>(n, edges);
    } else if (chosen_algorithm == "euler-tour") {
        if (benchmark_mode)
            preprocessing_start = std::chrono::steady_clock::now();
        solver = std::make_unique<EulerTourSolver>(n, edges);
    } else if (chosen_algorithm == "small-to-large") {
        if (benchmark_mode)
            preprocessing_start = std::chrono::steady_clock::now();
        solver = std::make_unique<SmallToLargeSolver>(n, edges);
    } else if (chosen_algorithm == "micro-trees") {
        if (benchmark_mode)
            preprocessing_start = std::chrono::steady_clock::now();
        solver = std::make_unique<MicroTreeSolver>(n, edges);
    } else if (chosen_algorithm == "euler-tour-trees") {
        if (benchmark_mode)
            preprocessing_start = std::chrono::steady_clock::now();
        solver = std::make_unique<EulerTourTreesSolver>(n, edges);
    } else {
        return EXIT_FAILURE;
    }
    if (benchmark_mode) {
        const auto preprocessing_end = std::chrono::steady_clock::now();

        preprocessing_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                    preprocessing_end - preprocessing_start)
                                    .count();
    }
    constexpr std::uint64_t checksum_base = 1000003ULL;
    std::uint64_t benchmark_checksum = 0;

    auto execute_operation = [&](const Operation& operation) {
        const auto& [type, u, v] = operation;
        if (type == 'C')
            solver->cut(u, v);
        else {
            bool answer = solver->connected(u, v);
            if (benchmark_mode) {
                benchmark_checksum =
                    benchmark_checksum * checksum_base + static_cast<std::uint64_t>(answer) + 1;
            } else {
                std::cout << (answer ? "YES\n" : "NO\n");
            }
        }
    };

    int64_t total_operations_time_ns = 0;
    if (benchmark_mode) {
        std::chrono::steady_clock::time_point operations_start, operations_end;
        operations_start = std::chrono::steady_clock::now();
        for (int i = 0; i < m; ++i) {
            execute_operation(operations[i]);
        }
        operations_end = std::chrono::steady_clock::now();
        total_operations_time_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(operations_end - operations_start)
                .count();
    } else {
        for (int i = 0; i < m; ++i) {
            Operation operation;
            std::cin >> operation.type >> operation.u >> operation.v;
            execute_operation(operation);
        }
    }

    if (benchmark_mode) {
        std::cout << preprocessing_time_ns << " " << total_operations_time_ns << " "
                  << benchmark_checksum << "\n";
    }

    return EXIT_SUCCESS;
}
