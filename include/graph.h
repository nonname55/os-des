#ifndef GRAPH_H
#define GRAPH_H

#include "header.h"
#include "macro.h"
#include "util.h"

struct Graph {
    char bupt_map[G_ROW][G_COL + 1] = {
        {".........."},
        {"...#####.."},
        {".......#.."},
        {".......#.."},
        {".########."},
        {".....#...."},
        {"..#..####."},
        {"..#..#...."},
        {"..####...."},
        {".........."},
    };
    int dx[D_NUM] = {-1, 1, 0, 0};
    int dy[D_NUM] = {0, 0, -1, 1};
    std::vector<std::pair<int, int>> shortest_path;
    std::pair<int, int> parent[G_ROW][G_COL] = {};

    struct Point {
        int x, y;
        int step, est;
        bool operator < (const Point& p) const {
            return step + est > p.step + p.est;
        }
    };
    std::pair<int, int> getValidPos();
    int H(int sx, int sy, int ex, int ey) { return abs(sx - ex) + abs(sy - ey); }
    int astar(int sx, int sy, int ex, int ey, bool isPath);
};

extern Graph graph;

#endif