#pragma once
#include <cstdint>
#include <vector>
#include <mutex>

using namespace std;

struct Edge {
    uint32_t dst;
    int      weight;
};

struct AddRequest {
    uint32_t src;
    uint32_t dst;
    int      weight;
};

struct Graph {
    vector<vector<Edge>> adj;
    vector<mutex>        locks;

    explicit Graph(uint32_t n) : adj(n), locks(n) {}

    void apply(const AddRequest& r) {
        lock_guard<mutex> lk(locks[r.src]);
        adj[r.src].push_back({r.dst, r.weight});
    }
};