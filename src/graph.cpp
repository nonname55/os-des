#include "header.h"
#include "graph.h"
#include "macro.h"

Graph graph;

std::pair<int, int> Graph::getValidPos() {
    int x, y;
    do {
        x = random_int(0, G_ROW - 1);
        y = random_int(0, G_COL - 1);
    } while (bupt_map[x][y] == POS_ERROR);
    return {x, y};
}
int Graph::astar(int sx, int sy, int ex, int ey, bool isPath) {
    if (isPath) {
        memset(parent, 0, sizeof(parent));
        shortest_path.clear();
    }
    if (sx == ex && sy == ey) {
        if (isPath) {
            shortest_path.emplace_back(sx, sy);
        }
        return 0;
    }
    std::priority_queue<Point> que;
    Point pre = {sx, sy, 0, H(sx, sy, ex, ey)}, nex;
    que.push(pre);
    std::bitset<G_COL> vis[G_ROW];
    vis[sx][sy] = 1;
    parent[sx][sy] = {-1, -1};
    auto isValid = [&](int x, int y) -> bool {
        return x >= 0 && x < G_ROW && y >= 0 && y < G_COL && 
            !vis[x].test(y) && bupt_map[x][y] != POS_ERROR;
    };
    while (!que.empty()) {
        pre = que.top();
        que.pop();
        if (pre.x == ex && pre.y == ey) {
            if (isPath) {
                std::pair<int, int> trace = {pre.x, pre.y};
                while (trace.first != -1) {
                    shortest_path.emplace_back(trace);
                    trace = parent[trace.first][trace.second];
                }
                std::reverse(shortest_path.begin(), shortest_path.end());
            }
            return pre.step;
        }
        for (int i = 0; i < D_NUM; ++i) {
            int nx = pre.x + dx[i], ny = pre.y + dy[i];
            if (!isValid(nx, ny)) {
                continue;
            }
            vis[nx][ny] = 1;
            nex = {nx, ny, pre.step + 1, H(nx, ny, ex, ey)};
            parent[nx][ny] = {pre.x, pre.y};
            que.push(nex);
        }
    }
    return -1;
}

