#pragma once
#include "graph.h"
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

struct Delay {
    size_t before_index;
    int    ms;
};

Graph load_initial(const string& path) {
    ifstream f(path);

    vector<tuple<uint32_t,uint32_t,int>> edges;
    uint32_t src, dst, maxv = 0;
    int w;
    while (f >> src >> dst >> w) {
        edges.emplace_back(src, dst, w);
        if (src > maxv) maxv = src;
        if (dst > maxv) maxv = dst;
    }

    Graph g(maxv + 1);
    for (int i = 0; i < (int)edges.size(); i++)
        g.adj[get<0>(edges[i])].push_back({get<1>(edges[i]), get<2>(edges[i]), 0});
    return g;
}

struct ParsedTrace {
    vector<UpdateReq> updates;
    vector<QueryReq>  queries;
    vector<Delay>     delays;
};

ParsedTrace parse_updates(const string& path, uint32_t source) {
    ifstream f(path);
    string line;

    ParsedTrace out;
    uint64_t ver = 0;
    vector<uint32_t> pending_dirty = {source};

    while (getline(f, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string tag; ss >> tag;

        if (tag == "ADD") {
            uint32_t s, d; int wt;
            ss >> s >> d >> wt;
            ++ver;
            out.updates.push_back({s, d, wt, ver});
            pending_dirty.push_back(s);
        } else if (tag == "Q") {
            ++ver;
            out.queries.push_back({ver, pending_dirty});
            pending_dirty.clear();
        } else if (tag == "D") {
            int ms; ss >> ms;
            out.delays.push_back({out.updates.size(), ms});
        }
    }
    return out;
}