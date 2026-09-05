#pragma once
#include "core.hpp"
#include <cstddef>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

class SmallToLargeSolver : public DecrementalConnectivitySolver {
public:
    SmallToLargeSolver() = default;
    SmallToLargeSolver(int node_count, const std::vector<Edge>& edges) {
        setup(node_count, edges);
    }

    void setup(int node_count, const std::vector<Edge>& edges) {
        component_id_counter = 1;
        visit_id_counter = 1;

        neighbors.clear();
        neighbors.resize(node_count);
        component_id.assign(node_count, 1);
        edge_erased.assign(edges.size(), false);
        visit_ids.assign(node_count, 0);
        edge_to_id.clear();

        for (std::size_t i = 0; i < edges.size(); i++) {
            const auto& [u, v] = edges[i];
            const int edge_id = static_cast<int>(i);
            neighbors[u].emplace_back(v, edge_id);
            neighbors[v].emplace_back(u, edge_id);
            edge_to_id[make_edge_key(u, v)] = edge_id;
        }
    }

    void cut(int u, int v) override {
        const auto key = make_edge_key(u, v);
        const auto edge_it = edge_to_id.find(key);
        if (edge_it == edge_to_id.end())
            return;

        const int edge_id = edge_it->second;

        if (edge_erased[edge_id])
            return;

        edge_erased[edge_id] = true;

        OneStepTraversal u_traversal(this, u);
        OneStepTraversal v_traversal(this, v);

        while (!u_traversal.finished() && !v_traversal.finished()) {
            u_traversal.advance();
            v_traversal.advance();
        }

        const auto& smaller_component = u_traversal.finished() ? u_traversal.component_vertices
                                                               : v_traversal.component_vertices;

        component_id_counter++;
        for (int vertex : smaller_component)
            component_id[vertex] = component_id_counter;
    }

    bool connected(int u, int v) override {
        if (u == v)
            return true;

        return component_id[u] == component_id[v];
    }

private:
    int component_id_counter = 1;
    std::vector<int> component_id;
    std::vector<bool> edge_erased;
    std::vector<int> visit_ids;
    int visit_id_counter = 1;
    std::unordered_map<std::uint64_t, int> edge_to_id;
    std::vector<std::vector<std::pair<int, int>>> neighbors;

    struct OneStepTraversal {
        std::queue<int> queue;
        SmallToLargeSolver* solver;
        int current_vertex;
        std::size_t next_neighbor_index;
        int visit_id;
        std::vector<int> component_vertices;

        OneStepTraversal(SmallToLargeSolver* solver, int source)
            : solver(solver), current_vertex(-1), next_neighbor_index(0) {
            solver->visit_id_counter++;
            visit_id = solver->visit_id_counter;
            queue.push(source);
            component_vertices.push_back(source);
            solver->visit_ids[source] = visit_id;
        }

        bool finished() const {
            return current_vertex == -1 && queue.empty();
        }

        bool advance() {
            while (true) {
                if (current_vertex == -1) {
                    if (queue.empty())
                        return false;
                    current_vertex = queue.front();
                    queue.pop();
                    next_neighbor_index = 0;
                }

                if (next_neighbor_index < solver->neighbors[current_vertex].size()) {
                    auto& neighbor_entry = solver->neighbors[current_vertex][next_neighbor_index];
                    auto neighbor = neighbor_entry.first;
                    auto edge_id = neighbor_entry.second;

                    if (solver->edge_erased[edge_id]) {
                        std::swap(neighbor_entry, solver->neighbors[current_vertex].back());
                        solver->neighbors[current_vertex].pop_back();
                        continue;
                    }

                    next_neighbor_index++;

                    if (solver->visit_ids[neighbor] == visit_id) {
                        continue;
                    }

                    queue.push(neighbor);
                    component_vertices.push_back(neighbor);
                    solver->visit_ids[neighbor] = visit_id;
                    return true;
                } else {
                    current_vertex = -1;
                }
            }
        }
    };

    static std::uint64_t make_edge_key(int u, int v) {
        if (u > v)
            std::swap(u, v);

        return (static_cast<std::uint64_t>(u) << 32) | static_cast<std::uint32_t>(v);
    }
};
