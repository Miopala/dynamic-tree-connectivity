#pragma once
#include "core.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

class EulerTourTreesSolver : public DecrementalConnectivitySolver {
public:
    EulerTourTreesSolver(int node_count) : vertex_nodes(node_count) {
        edge_nodes.reserve(2 * node_count);
        for (int i = 0; i < node_count; i++) {
            auto node = std::make_unique<Node>();
            vertex_nodes[i] = node.get();
            edge_nodes[make_edge_key(i, i)] = std::move(node);
        }
    }

    EulerTourTreesSolver(int node_count, const std::vector<Edge>& edges)
        : EulerTourTreesSolver(node_count) {
        for (const auto& e : edges)
            connect(e.u, e.v);
    }

    EulerTourTreesSolver(const EulerTourTreesSolver&) = delete;
    EulerTourTreesSolver& operator=(const EulerTourTreesSolver&) = delete;

    void cut(int u, int v) override {
        const auto key_uv = make_edge_key(u, v);
        const auto key_vu = make_edge_key(v, u);

        const auto uv_it = edge_nodes.find(key_uv);
        if (uv_it == edge_nodes.end())
            return;

        Node* uv = uv_it->second.get();
        Node* vu = edge_nodes.at(key_vu).get();

        splay_tree.make_first(uv);

        // we have (u,v) (subtree of v) (v,u) (subtree containing u)

        splay_tree.splay(vu);

        Node* left = vu->sons[0]; // (u,v) (subtree of v)
        if (left) {
            left->parent = nullptr;
            splay_tree.splay(uv); // smallest element so it has only right son so we just cut
            if (uv->sons[1])
                uv->sons[1]->parent = nullptr;
            uv->sons[1] = nullptr;
        }

        Node* right = vu->sons[1]; // component containing (u,u)
        if (right)
            right->parent = nullptr;

        edge_nodes.erase(uv_it);
        edge_nodes.erase(key_vu);
    }

    bool connected(int u, int v) override {
        if (u == v)
            return true;
        return splay_tree.get_root(vertex_nodes[u]) == splay_tree.get_root(vertex_nodes[v]);
    }

private:
    struct Node {
        Node* sons[2] = {nullptr, nullptr};
        Node* parent = nullptr;

        bool is_root() const {
            return parent == nullptr;
        }

        int side() const {
            if (!parent)
                return -1;

            return (parent->sons[1] == this ? 1 : 0);
        }
    };

    struct SplayTree {
        void rotate(Node* u) {
            Node* parent = u->parent;
            int side = u->side();
            Node* grandparent = parent->parent;

            u->parent = grandparent;
            if (grandparent)
                grandparent->sons[parent->side()] = u;

            parent->sons[side] = u->sons[side ^ 1];
            if (u->sons[side ^ 1])
                u->sons[side ^ 1]->parent = parent;

            u->sons[side ^ 1] = parent;
            parent->parent = u;
        }

        void splay(Node* u) {
            if (!u)
                return;
            while (!u->is_root()) {
                Node* parent = u->parent;
                if (!parent->is_root()) {
                    (u->side() == parent->side()) ? rotate(parent) : rotate(u);
                }
                rotate(u);
            }
        }

        Node* get_root(Node* u) {
            if (!u)
                return nullptr;
            splay(u);
            while (u->sons[0])
                u = u->sons[0];
            splay(u);
            return u;
        }

        Node* merge(Node* l, Node* r) {
            if (!l || !r)
                return l ? l : r;

            splay(l);
            while (l->sons[1])
                l = l->sons[1];
            splay(l);

            l->sons[1] = r;
            r->parent = l;
            return l;
        }

        void make_first(Node* u) { // we shift cyclically u to the beginning of the euler tour
            splay(u);
            Node* l = u->sons[0];
            if (!l)
                return;

            u->sons[0] = nullptr;
            l->parent = nullptr;

            merge(u, l);
            splay(u);
        }
    };

    void connect(int u, int v) {
        if (connected(u, v))
            return;

        Node* uu = vertex_nodes[u];
        Node* vv = vertex_nodes[v];

        splay_tree.make_first(uu);
        splay_tree.make_first(vv);

        auto uv_node = std::make_unique<Node>();
        auto vu_node = std::make_unique<Node>();
        Node* uv = uv_node.get();
        Node* vu = vu_node.get();
        edge_nodes[make_edge_key(u, v)] = std::move(uv_node);
        edge_nodes[make_edge_key(v, u)] = std::move(vu_node);

        splay_tree.merge(splay_tree.merge(uu, uv), splay_tree.merge(vv, vu));
    }

    std::unordered_map<std::uint64_t, std::unique_ptr<Node>> edge_nodes;
    std::vector<Node*> vertex_nodes;
    SplayTree splay_tree;

    static std::uint64_t make_edge_key(int u, int v) {
        return (static_cast<std::uint64_t>(u) << 32) | static_cast<std::uint32_t>(v);
    }
};
