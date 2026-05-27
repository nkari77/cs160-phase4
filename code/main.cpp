#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include "concurrent_queue.h"
#include "graph.h"
#include "loader.h"
#include "apply_tracker.h"
#include "sssp.h"

using namespace std;

using Clock = chrono::high_resolution_clock;
using Ms    = chrono::duration<double, milli>;

const int SOURCE = 0;
const long long MOD = 1000000007;

long long signature(const vector<int>& dist) {
    long long s = 0;
    for (int d : dist)
        if (d != INF) s = (s + d) % MOD;
    return s;
}

int main(int argc, char* argv[]) {
    int TC = 1, TQ = 1;
    bool incremental = true;
    if (argc >= 2) TC = stoi(argv[1]);
    if (argc >= 3) TQ = stoi(argv[2]);
    if (argc >= 4) incremental = (string(argv[3]) != "full");

    // pre-load (not timed)
    Graph g = load_initial("initial.txt");
    int n = g.adj.size();

    ParsedTrace trace = parse_updates("updates.txt");

    int update_count = 0, query_count = 0;
    for (auto& r : trace.records) {
        if (r.type == RecordType::ADD)   update_count++;
        if (r.type == RecordType::QUERY) query_count++;
    }

    cout << "vertices:  " << n            << endl;
    cout << "updates:   " << update_count << endl;
    cout << "queries:   " << query_count  << endl;
    cout << "TC:        " << TC           << endl;
    cout << "TQ:        " << TQ           << endl;
    cout << "mode:      " << (incremental ? "incremental" : "full") << endl;

    ConcurrentQueue<UpdateReq> update_queue;
    ConcurrentQueue<QueryReq>  query_queue;
    ApplyTracker               tracker;

    ofstream out("queries.out");

    // spawn updater threads before clock
    vector<thread> updaters;
    for (int i = 0; i < TC; i++) {
        updaters.emplace_back([&]{
            while (auto req = update_queue.Pop()) {
                g.apply(*req);
                tracker.MarkDone(req->version);
            }
        });
    }

    // spawn query thread before clock
    vector<int> dist(n, INF);
    dist[SOURCE] = 0;

    thread query_thread([&]{
        while (auto req = query_queue.Pop()) {
            // wait until all ADDs before this query are applied
            tracker.WaitUntil(req->version - 1);

            if (!incremental) {
                // full mode: reset dist every query
                fill(dist.begin(), dist.end(), INF);
                dist[SOURCE] = 0;
                req->dirty = {SOURCE};
            }

            // dedup dirty frontier
            sort(req->dirty.begin(), req->dirty.end());
            req->dirty.erase(
                unique(req->dirty.begin(), req->dirty.end()),
                req->dirty.end());

            RunBspSsspParallel(g, dist, req->dirty, req->version, TQ);

            out << req->version << " " << signature(dist) << "\n";

            tracker.MarkDone(req->version);
        }
    });

    auto t0 = Clock::now();

    // producer: walk unified record list in order
    uint64_t ver = 0;
    vector<uint32_t> pending_dirty = {SOURCE};

    for (auto& rec : trace.records) {
        if (rec.type == RecordType::DELAY) {
            this_thread::sleep_for(chrono::milliseconds(rec.ms));
        } else if (rec.type == RecordType::ADD) {
            ++ver;
            UpdateReq req{rec.src, rec.dst, rec.weight, ver};
            update_queue.Push(req);
            pending_dirty.push_back(rec.src);
        } else if (rec.type == RecordType::QUERY) {
            ++ver;
            query_queue.Push({ver, move(pending_dirty)});
            pending_dirty.clear();
        }
    }

    update_queue.Close();
    query_queue.Close();

    for (auto& t : updaters) t.join();
    query_thread.join();

    double elapsed = Ms(Clock::now() - t0).count();
    cout << "wall time: " << elapsed << " ms" << endl;

    return 0;
}