#pragma once
#include "graph.h"
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>
#include <algorithm>

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
        g.adj[get<0>(edges[i])].push_back({get<1>(edges[i]), get<2>(edges[i])});
    return g;
}

void parse_updates(const string& path,
                   vector<AddRequest>& adds,
                   vector<Delay>& delays)
{
    ifstream f(path);
    string line;
    while (getline(f, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string tag; ss >> tag;
        if (tag == "ADD") {
            uint32_t s, d; int wt;
            ss >> s >> d >> wt;
            adds.push_back({s, d, wt});
        } else if (tag == "D") {
            int ms; ss >> ms;
            delays.push_back({adds.size(), ms});
        }
    }
}