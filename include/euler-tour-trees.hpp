#include "../include/core.hpp"
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <utility>
#include <vector>
struct Node {
    Node* sons[2] = {nullptr, nullptr};
    Node* parent = nullptr;
    int from, to;
    Node(int _from, int _to) : from(_from), to(_to) {}

    bool is_root() {
        return parent == nullptr;
    }

    int side() {
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

    void make_first(Node* u) { // przesuwamy cyklicznie u na poczatek naszego euler toura
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

class EulerTourTreesSolver : public DecrementalConnectivitySolver {
public:
    EulerTourTreesSolver(int n) : vertex_nodes(n) {
        edges_nodes.reserve(2 * n);
        for (int i = 0; i < n; i++) {
            Node* node = new Node(i, i);
            vertex_nodes[i] = node;
            edges_nodes[get_key(i, i)] = node;
        }
    }

    EulerTourTreesSolver(int n, const std::vector<Edge>& edges) : EulerTourTreesSolver(n) {
        for (const auto& e : edges)
            connect(e.u, e.v);
    }

    void connect(int u, int v) {
        if (connected(u, v))
            return;

        Node* uu = vertex_nodes[u];
        Node* vv = vertex_nodes[v];

        st.make_first(uu);
        st.make_first(vv);

        Node* uv = new Node(u, v);
        Node* vu = new Node(v, u);
        edges_nodes[get_key(u, v)] = uv;
        edges_nodes[get_key(v, u)] = vu;

        st.merge(st.merge(uu, uv), st.merge(vv, vu));
    }

    void cut(int u, int v) override {
        long long key_uv = get_key(u, v);
        long long key_vu = get_key(v, u);

        if (edges_nodes.find(key_uv) == edges_nodes.end())
            return;

        Node* uv = edges_nodes[key_uv];
        Node* vu = edges_nodes[key_vu];

        st.make_first(uv);

        // we have (u,v) (subtree of v) (v,u) (subtree containing u)

        st.splay(vu);

        Node* mid = vu->sons[0]; // (u,v) (subtree of v)
        if (mid) {
            mid->parent = nullptr;
            st.splay(uv); // smallest element so it has only right son so we just cut
            if (uv->sons[1])
                uv->sons[1]->parent = nullptr;
            uv->sons[1] = nullptr;
        }

        Node* right = vu->sons[1]; // component containing (u,u)
        if (right)
            right->parent = nullptr;

        edges_nodes.erase(key_uv);
        edges_nodes.erase(key_vu);
        delete uv;
        delete vu;
    }

    bool connected(int u, int v) override {
        if (u == v)
            return true;
        return st.get_root(vertex_nodes[u]) == st.get_root(vertex_nodes[v]);
    }

    ~EulerTourTreesSolver() {
        for (auto& p : edges_nodes)
            delete p.second;
    }

private:
    std::unordered_map<long long, Node*> edges_nodes;
    std::vector<Node*> vertex_nodes;
    SplayTree st;

    long long get_key(int u, int v) {
        return (1LL * ((u * 1LL) << 32)) | v;
    }
};