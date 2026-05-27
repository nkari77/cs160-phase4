#pragma once
#include "graph.h"
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <climits>

using namespace std;

const int INF = INT_MAX;

void RunBspSsspParallel(
    Graph& g,
    vector<int>& dist,
    vector<uint32_t> frontier,
    uint64_t q_version,
    int nthreads)
{
    int n = g.adj.size();

    while (!frontier.empty()) {
        vector<uint32_t> next;
        mutex next_mu;

        int chunk = max(1, (int)frontier.size() / nthreads);
        vector<thread> workers;

        for (int t = 0; t < nthreads; t++) {
            int lo = t * chunk;
            int hi = (t == nthreads - 1) ? frontier.size() : lo + chunk;
            if (lo >= (int)frontier.size()) break;

            workers.emplace_back([&, lo, hi]{
                vector<uint32_t> local_next;
                for (int i = lo; i < hi; i++) {
                    uint32_t u = frontier[i];
                    int du = dist[u];
                    if (du == INF) continue;

                    shared_lock<shared_mutex> lk(g.locks[u]);
                    for (const Edge& e : g.adj[u]) {
                        if (e.version >= q_version) continue;
                        if (du + e.weight < dist[e.dst]) {
                            dist[e.dst] = du + e.weight;
                            local_next.push_back(e.dst);
                        }
                    }
                }
                lock_guard<mutex> lk(next_mu);
                for (auto v : local_next) next.push_back(v);
            });
        }
        for (auto& w : workers) w.join();

        // dedup next frontier
        sort(next.begin(), next.end());
        next.erase(unique(next.begin(), next.end()), next.end());
        frontier = move(next);
    }
}