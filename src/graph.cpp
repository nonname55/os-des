#include "header.h"
#include "graph.h"
#include "macro.h"

std::vector<std::vector<int>> Graph::graph = {
    {0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 3, 4, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 1, 0, 1, 2, 1, 1, 1, 1, 1, 0, 1, 1, 1, 2, 4, 1, 1, 1},
    {1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 3, 3, 0, 0, 1},
    {1, 0, 1, 3, 2, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1},
    {4, 3, 1, 0, 1, 1, 1, 1, 1, 4, 1, 0, 1, 1, 1, 1, 1, 2, 3, 1},
    {1, 0, 1, 0, 1, 0, 0, 0, 0, 3, 0, 0, 1, 1, 4, 1, 1, 1, 0, 1},
    {1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 3, 1, 1, 1, 0, 1},
    {1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1},
    {2, 3, 1, 0, 0, 0, 1, 1, 0, 2, 1, 1, 2, 1, 0, 1, 1, 1, 0, 1},
    {1, 0, 1, 2, 1, 4, 1, 1, 0, 3, 0, 0, 3, 1, 0, 1, 1, 2, 3, 1},
    {4, 3, 0, 3, 0, 3, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1},
    {1, 4, 3, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1},
    {1, 1, 0, 1, 0, 1, 1, 2, 3, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1},
    {1, 2, 3, 1, 0, 1, 1, 1, 0, 1, 1, 2, 1, 1, 3, 4, 0, 1, 1, 1},
    {1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 4, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1},
    {1, 1, 1, 2, 3, 0, 3, 0, 0, 1, 4, 3, 1, 0, 0, 1, 3, 4, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 3, 0, 1, 0, 1, 0, 1},
    {3, 0, 0, 3, 0, 0, 0, 0, 0, 3, 0, 0, 1, 0, 3, 1, 0, 2, 3, 1},
    {2, 1, 1, 4, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 4, 2, 3, 0, 0, 1}
};
int Graph::dx[4] = {-1, 1, 0, 0};
int Graph::dy[4] = {0, 0, -1, 1};
std::vector<int> Graph::error_pos = {1, 2, 4};

int Graph::H(int sx, int sy, int ex, int ey) { return std::abs(sx - ex) + std::abs(sy - ey); }

int Graph::astar(int sx, int sy, int ex, int ey, const std::vector<std::vector<int>> &graph) 
{
    if (graph.size() == 0) 
        return -1;
    int row_num = graph.size();
    int col_num = graph[0].size();
    if (sx == ex && sy == ey) {
        return 0;
    }
    std::priority_queue<Point> que;
    Point pre = {sx, sy, 0, H(sx, sy, ex, ey)}, nex;
    que.push(pre);
    std::vector vis(row_num, std::vector<bool>(col_num));
    vis[sx][sy] = true;
    auto check_pos_valid = [&graph](int x, int y) -> bool {
        for (const int &err : error_pos) {
            if (graph[x][y] == err)
                return false;
        }
        return true;
    };
    auto isValid = [&](int x, int y) -> bool {
        return x >= 0 && x < row_num && y >= 0 && y < col_num && 
            !vis[x][y] && check_pos_valid(x, y);
    };
    while (!que.empty()) {
        pre = que.top();
        que.pop();
        if (pre.x == ex && pre.y == ey) {
            return pre.step;
        }
        for (int i = 0; i < 4; ++i) {
            int nx = pre.x + dx[i], ny = pre.y + dy[i];
            if (!isValid(nx, ny)) {
                continue;
            }
            vis[nx][ny] = 1;
            nex = {nx, ny, pre.step + 1, H(nx, ny, ex, ey)};
            que.push(nex);
        }
    }
    return -1;
}

std::pair<std::pair<int, int>, std::pair<int, int>> Graph::get_valid_pos(int who)
{  
    int row_num = Graph::graph.size();
    int col_num = Graph::graph[0].size();
    if (row_num == 0)
        return {{-1, -1}, {-1, -1}};
    int x, y;
    do {
        x = random_int(0, row_num - 1);
        y = random_int(0, col_num - 1);
    } while (Graph::graph[x][y] != who);
    int receive_x, receive_y;
    auto check_pos_valid = [&](int x, int y, int who) -> bool {
        if (x < 0 || x >= row_num || y < 0 || y >= col_num)
            return false;
        return Graph::graph[x][y] == who;
    };
    for (int i = 0; i < 4; ++i) {
        receive_x = x + Graph::dx[i];
        receive_y = y + Graph::dy[i];
        
        if (check_pos_valid(receive_x, receive_y, G_RECEIV))
            break;
    }
    std::swap(x, y);
    std::swap(receive_x, receive_y);
    return {{x, y}, {receive_x, receive_y}};
}