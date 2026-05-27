#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include "concurrent_queue.h"
#include "graph.h"
#include "loader.h"

using namespace std;

using Clock = chrono::high_resolution_clock;
using Ms    = chrono::duration<double, milli>;

long long weight_sum(const Graph& g) {
    const long long MOD = 1000000007;
    long long s = 0;
    for (int i = 0; i < (int)g.adj.size(); i++)
        for (int j = 0; j < (int)g.adj[i].size(); j++)
            s = (s + g.adj[i][j].weight) % MOD;
    return s;
}

int main(int argc, char* argv[]) {
    int N = 1;
    if (argc >= 2) N = stoi(argv[1]);

    // pre-load (not timed)
    Graph g = load_initial("initial.txt");

    vector<AddRequest> adds;
    vector<Delay>      delays;
    parse_updates("updates.txt", adds, delays);

    cout << "vertices: "  << g.adj.size()   << endl;
    cout << "updates:  "  << adds.size()    << endl;
    cout << "delays:   "  << delays.size()  << endl;
    cout << "consumers: " << N              << endl;

    ConcurrentQueue<AddRequest> cq;

    // spawn consumers before clock so they park on the empty queue
    vector<thread> consumers;
    for (int i = 0; i < N; i++) {
        consumers.emplace_back([&]{
            while (auto req = cq.Pop())
                g.apply(*req);
        });
    }

    auto t0 = Clock::now();

    // producer: walk adds[] and delays[] in lockstep
    size_t di = 0;
    for (size_t i = 0; i < adds.size(); i++) {
        while (di < delays.size() && delays[di].before_index == i) {
            this_thread::sleep_for(chrono::milliseconds(delays[di].ms));
            di++;
        }
        cq.Push(adds[i]);
    }
    // flush trailing delays past the last ADD
    while (di < delays.size()) {
        this_thread::sleep_for(chrono::milliseconds(delays[di].ms));
        di++;
    }

    cq.Close();
    for (auto& t : consumers) t.join();

    double elapsed = Ms(Clock::now() - t0).count();
    cout << "wall time: " << elapsed << " ms" << endl;

    long long S = weight_sum(g);
    cout << "weight sum: " << S << endl;
    if (S == 834418737) cout << "CORRECT" << endl;
    else                cout << "WRONG (expected 834418737)" << endl;

    return 0;
}