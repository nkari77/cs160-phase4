#pragma once
#include "graph.h"
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

enum class RecordType { ADD, QUERY, DELAY };

struct Record {
    RecordType type;
    uint32_t   src = 0;
    uint32_t   dst = 0;
    int        weight = 0;
    int        ms = 0;
};

struct ParsedTrace {
    vector<Record> records;
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

ParsedTrace parse_updates(const string& path) {
    ifstream f(path);
    string line;
    ParsedTrace out;

    while (getline(f, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string tag; ss >> tag;

        if (tag == "ADD") {
            uint32_t s, d; int wt;
            ss >> s >> d >> wt;
            out.records.push_back({RecordType::ADD, s, d, wt, 0});
        } else if (tag == "Q") {
            out.records.push_back({RecordType::QUERY});
        } else if (tag == "D") {
            int ms; ss >> ms;
            out.records.push_back({RecordType::DELAY, 0, 0, 0, ms});
        }
    }
    return out;
}