#include "header.h"
#include "graph.h"
#include "macro.h"
#include "file.h"

std::vector<std::vector<int>> Graph::graph;
std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> Graph::restaurant_set;
std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> Graph::user_set;

int Graph::dx[4] = {-1, 1, 0, 0};
int Graph::dy[4] = {0, 0, -1, 1};
std::vector<int> Graph::road_pos = {786, 787, 713, 750};

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
    auto isValid = [&](int x, int y) -> bool {
        if (x < 0 || x >= row_num || y < 0 || y >= col_num || vis[x][y])
            return false;
        for (int rp : road_pos) {
            if (graph[x][y] == rp)
                return true;
        }
        return false;
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
    if (who == G_REST) {
        int rest_siz = restaurant_set.size();
        int ind = random_int(0, rest_siz - 1);
        return restaurant_set[ind];
    } else {
        int user_siz = user_set.size();
        int ind = random_int(0, user_siz - 1);
        return user_set[ind];
    }
}

void Graph::read_map(std::vector<std::vector<int>> &graph)
{
    std::vector<std::string> map_info;
    ReadFile(workdir.append("resources/front.map"), map_info, true);
    for (auto &line : map_info) {
        std::vector<int> line_num;
        std::stringstream ss(line);
        int num;
        while (ss >> num) {
            line_num.push_back(num);
        }
        graph.push_back(line_num);
    }
}

void Graph::parse_map(const std::vector<std::vector<int>> &graph)
{
    std::vector<int> building_id = {1, 9, 17};
    if (graph.empty())
        return;
    int row_num = graph.size();
    int col_num = graph[0].size();
    for (int i = 0; i < row_num; ++i) {
        for (int j = 0; j < col_num; ++j) {
            for (int id : building_id) {
                if (graph[i][j] == id) {
                    if (graph[i + BUILDING_HEIGHT][j] == REST_IDENTIFY)
                        restaurant_set.push_back({{i, j}, {-1, -1}});
                    else 
                        user_set.push_back({{i, j}, {-1, -1}});
                }
            }
        }
    }
    
    auto bfs = [&](std::pair<int, int> pre) -> std::pair<int, int> {
        std::queue<std::pair<int, int>> que;
        std::vector vis(row_num, std::vector<bool>(col_num));
        vis[pre.first][pre.second] = true;
        que.push(pre);
        auto is_pos_valid = [&](int x, int y) -> bool {
            return x >= 0 && x < row_num && y >= 0 && y < col_num && !vis[x][y];
        };
        while (!que.empty()) {
            pre = que.front();
            que.pop();

            for (int rp : road_pos) {
                if (graph[pre.first][pre.second] == rp) {
                    return pre;
                }
            }

            for (int i = 0; i < 4; ++i) {
                int nx = pre.first + dx[i];
                int ny = pre.second + dy[i];
                if (is_pos_valid(nx, ny)) {
                    que.push({nx, ny});
                    vis[nx][ny] = true;
                }
            }
        }
        return {-1, -1};
    };

    for (auto &rest : restaurant_set) {
        rest.second = bfs(rest.first);
    }
    for (auto &user : user_set) {
        user.second = bfs(user.first);
    }
}