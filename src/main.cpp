#include<iostream>
#include<pthread.h>
#include<vector>
#include<deque>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<random>
#include<mutex>
#include<memory>
#include<queue>
#include<bitset>
#include<algorithm>
#include<cstring>
#include<thread>
#include<chrono>

#define RIDER_NUM 1
#define print(msg) { \
    std::lock_guard<std::mutex> cout_lock(cout_mutex); \
    std::cout << msg; \
}
std::mutex cout_mutex;

#define G_ROW 10
#define G_COL 10
#define D_NUM 4
#define POS_ERROR '#'
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
    int H(int sx, int sy, int ex, int ey) {
        return abs(sx - ex) + abs(sy - ey);
    }
    int astar(int sx, int sy, int ex, int ey, bool isPath) {
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
}graph;

static int random_int(int l, int h) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(l, h);
    return distrib(gen);
}

struct Order {
    int tarx, tary, required_time;
    int dis = 0;
    double weight = 0.0;
    pthread_cond_t cond;
    pthread_t thread;

    Order() {
        pthread_cond_init(&cond, nullptr);
        do {
            tarx = random_int(0, G_ROW - 1);
            tary = random_int(0, G_COL - 1);
        } while (graph.bupt_map[tarx][tary] == POS_ERROR);
        required_time = random_int(1, 10);
    }
};

struct Rider {
    static int posx, posy, id, speed;
    static std::deque<std::unique_ptr<Order>> orders;
    static pthread_mutex_t lock;
    static pthread_cond_t cond;
    static bool isNewOrder;

    Rider(
        int _posx = 0, 
        int _posy = 0, 
        int _id = random_int(1, 10),
        int _speed = 10
    ) {
        this->posx = _posx;
        this->posy = _posy;
        this->id = _id;
        this->speed = _speed;
    }
    std::pair<int, int> getPos() const {
        return {posx, posy};
    }
    static void manage(Rider& self) {
        for (int i = 0; i < 5; ++i) {
            std::unique_ptr<Order> temp = std::make_unique<Order>();
            create_order(temp);
        }
SORT_ORDERS:
        sort_order();
        // print("rider pos:(" << posx << ',' << posy << ")" << std::endl);
        // print("sort order: " << std::endl);
        // for (int i = 0; i < 5; ++i) {
        //     print("(" << orders[i]->tarx << ',' << orders[i]->tary << ") required time: " << orders[i]->required_time << std::endl);
        // }
        // print(std::endl);
        while (orders.size() > 0) {
            pthread_cond_signal(&(orders.front()->cond));
            pthread_cond_wait(&cond, &lock);
            while (isNewOrder) {
                isNewOrder = false;
                goto SORT_ORDERS;
            }
            orders.pop_front();
        }
    }
    static double H(const Order& order) {
        int dis = graph.astar(posx, posy, order.tarx, order.tary, false);
        double disH = 1.0 / dis;
        double timeH = 1.0 / order.required_time;
        return 0.3 * disH + 0.7 * timeH;
    };
    static void sort_order() {
        for (const auto& order : orders) {
            order->weight = H(*order);
        }
        std::sort(orders.begin(), orders.end(), [](const auto& o1, const auto& o2) {
            return o1->weight > o2->weight;
        });
    }
    static bool check_newOrder() {
        return false;
    }
    static void* deliver(void* arg) {
        pthread_mutex_lock(&lock);
        std::unique_ptr<Order> order(static_cast<Order*>(arg));
        pthread_cond_signal(&cond);
        pthread_cond_wait(&(order->cond), &lock);

FIND_MIN_PATH:        
        graph.astar(posx, posy, order->tarx, order->tary, true);
        for (const auto& node : graph.shortest_path) {
            while (check_newOrder()) {
                isNewOrder = true;
                pthread_cond_signal(&cond);
                pthread_cond_wait(&(order->cond), &lock);
                goto FIND_MIN_PATH;
            }
            isNewOrder = false;
            int deliver_time = 1000 / speed;
            std::this_thread::sleep_for(std::chrono::milliseconds(deliver_time));
            posx = node.first;
            posy = node.second;
            print(posx << ',' << posy << ' ' << std::flush);
        }
        print(std::endl);
        // print("rider pos:(" << posx << ',' << posy << ") tar:(" << order->tarx << ',' << order->tary << ") path: ");
        // for (const auto& node : graph.shortest_path) {
        //     print('(' << node.first << ',' << node.second << ")->" << std::flush);
        //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // }
        // posx = order->tarx;
        // posy = order->tary;
        
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
        return nullptr;
    }
    static void create_order(std::unique_ptr<Order>& newOrder) {
        pthread_mutex_lock(&lock);

        pthread_create(&(newOrder->thread), nullptr, deliver, newOrder.get());
        pthread_cond_wait(&cond, &lock);
        orders.push_back(std::move(newOrder));

        pthread_mutex_unlock(&lock);
    }
};

std::deque<std::unique_ptr<Order>> Rider::orders;
pthread_mutex_t Rider::lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Rider::cond = PTHREAD_COND_INITIALIZER;
int Rider::posx, Rider::posy, Rider::id, Rider::speed;
bool Rider::isNewOrder = false;

int main() {
    std::vector<Rider> riders(RIDER_NUM);

    for (int i = 0; i < RIDER_NUM; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            Rider::manage(riders[i]);
            exit(0);
        } else if (pid < 0) {
            print("Failed to create rider" << std::endl);
        }
    }

    for (int i = 0; i < RIDER_NUM; ++i) {
        wait(NULL);
    }

    return 0;
}