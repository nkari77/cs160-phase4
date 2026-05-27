#pragma once
#include <cstdint>
#include <vector>
#include <shared_mutex>

using namespace std;

struct Edge {
    uint32_t dst;
    int      weight;
    uint64_t version;
};

struct UpdateReq {
    uint32_t src;
    uint32_t dst;
    int      weight;
    uint64_t version;
};

struct QueryReq {
    uint64_t         version;
    vector<uint32_t> dirty;
};

struct Graph {
    vector<vector<Edge>>  adj;
    vector<shared_mutex>  locks;

    explicit Graph(uint32_t n) : adj(n), locks(n) {}

    void apply(const UpdateReq& r) {
        unique_lock<shared_mutex> lk(locks[r.src]);
        adj[r.src].push_back({r.dst, r.weight, r.version});
    }
};