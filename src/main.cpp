#include<iostream>
#include<pthread.h>
#include<vector>
#include<deque>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
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
#include<fstream>
#include<filesystem>
#include<fcntl.h>
#include<sys/shm.h>
#include<signal.h>

#define RIDER_NUM 3
#define print(msg) { \
    std::lock_guard<std::mutex> cout_lock(cout_mutex); \
    std::cout << msg; \
}
std::mutex cout_mutex;
#define DEBUG std::cout << "debug" << std::endl;

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
    int posx, posy, id, speed;
    std::deque<std::shared_ptr<Order>> orders;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool isNewOrder;
    pid_t pid;
    struct ThreadArgs {
        Rider* rider;
        std::shared_ptr<Order> order;
    };

    Rider(
        int _posx = 0, 
        int _posy = 0, 
        int _id = random_int(1, 10),
        int _speed = 10,
        int _pid = 0
    ) {
        this->posx = _posx;
        this->posy = _posy;
        this->id = _id;
        this->speed = _speed;
        this->pid = _pid;
    }
    void manage() {
        for (int i = 0; i < 5; ++i) {
            std::shared_ptr<Order> temp = std::make_shared<Order>();
            // create_order(temp);
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
    double H(const Order& order) {
        int dis = graph.astar(posx, posy, order.tarx, order.tary, false);
        double disH = 1.0 / dis;
        double timeH = 1.0 / order.required_time;
        return 0.3 * disH + 0.7 * timeH;
    };
    void sort_order() {
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
        ThreadArgs* args = static_cast<ThreadArgs*>(arg);
        Rider* rider = args->rider;
        std::shared_ptr<Order> order = args->order;
        pthread_mutex_lock(&(rider->lock));
        // std::shared_ptr<Order> order(static_cast<Order*>(arg));
        pthread_cond_signal(&(rider->cond));
        pthread_cond_wait(&(order->cond), &(rider->lock));

FIND_MIN_PATH:        
        graph.astar(rider->posx, rider->posy, order->tarx, order->tary, true);
        for (const auto& node : graph.shortest_path) {
            while (check_newOrder()) {
                rider->isNewOrder = true;
                pthread_cond_signal(&(rider->cond));
                pthread_cond_wait(&(order->cond), &(rider->lock));
                goto FIND_MIN_PATH;
            }
            rider->isNewOrder = false;
            int deliver_time = 1000 / rider->speed;
            std::this_thread::sleep_for(std::chrono::milliseconds(deliver_time));
            rider->posx = node.first;
            rider->posy = node.second;
            print(rider->posx << ',' << rider->posy << ' ' << std::flush);
        }
        print(std::endl);
        // print("rider pos:(" << posx << ',' << posy << ") tar:(" << order->tarx << ',' << order->tary << ") path: ");
        // for (const auto& node : graph.shortest_path) {
        //     print('(' << node.first << ',' << node.second << ")->" << std::flush);
        //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // }
        // posx = order->tarx;
        // posy = order->tary;
        
        pthread_cond_signal(&(rider->cond));
        pthread_mutex_unlock(&(rider->lock));
        return nullptr;
    }
    void create_order(std::shared_ptr<Order>& newOrder) {
        pthread_mutex_lock(&lock);

        ThreadArgs args = {this, newOrder};
        pthread_create(&(newOrder->thread), nullptr, deliver, &args);
        pthread_cond_wait(&cond, &lock);
        orders.push_back(std::move(newOrder));

        pthread_mutex_unlock(&lock);
    }
}riders[RIDER_NUM];
//main
void create_rider_process();
void rider_process();
void log_info(const std::string& s);
void check_processes_status();
//process
#define P_NUM 3
struct Process {
    int id;
    int priority;
    pid_t pid;
};
typedef struct SHM_Data {
    pthread_mutex_t mylock;
    pthread_cond_t mycond;
    pthread_cond_t cond_tids[P_NUM];
} SHM_Data;
struct Que {
    int head;
    int tail;
    int q[P_NUM + 1];
};
struct Process* pro;
struct Que* que;
SHM_Data* shm;
bool* isProOver;
int create_shm(const char *path, int index, int size);
void init_shm(SHM_Data *shm);
void init_que();
void push_que();
void rider_process(int id);
void schedule();

int main() {
    pid_t pid = fork();
    if (pid > 0) {
        exit(0);
    }
    setsid();
    umask(022);
    std::string folder = std::string(getenv("HOME")).append("/OS-Des");
    const std::filesystem::path path = std::filesystem::path(folder);
    if (!(std::filesystem::exists(path) && std::filesystem::is_directory(path))) {
        int re = mkdir(folder.c_str(), 0777);
    }
    chdir(folder.c_str());
    // int fd = open("/dev/null", O_RDWR);
    int fd = open("/home/mqr/OS-Des/output.txt", O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    int shmid[6];
    shmid[0] = shmget(0, sizeof(struct Process) * P_NUM, IPC_CREAT | 0666);
    shmid[1] = shmget(1, sizeof(Que), IPC_CREAT | 0666);
    shmid[2] = shmget(2, sizeof(bool), IPC_CREAT | 0666);
    int shmId = create_shm(".", 1, sizeof(SHM_Data));
    pro = (struct Process*)shmat(shmid[0], NULL, 0);
    que = (struct Que*)shmat(shmid[1], NULL, 0);
    isProOver = (bool*)shmat(shmid[2], NULL, 0);
    shm = (SHM_Data*)shmat(shmId, NULL, 0);
    init_shm(shm);

    // pthread_mutex_lock(&shm->mylock);
    create_rider_process();
    schedule();
        
    sleep(1);
    // pthread_mutex_unlock(&shm->mylock);

    return 0;
}

void schedule() {
    for (int i = P_NUM - 1; i >= 0; --i) {
        *isProOver = false;
        pthread_cond_signal(&(shm->cond_tids[i]));
        DEBUG
        // pthread_cond_wait(&(shm->cond_tids[i]), &(shm->mylock));
        while (!(*isProOver)) {
            check_processes_status();
            sleep(1);
        }
    }
}

void push_que(int val) {
    (que->q)[que->tail] = val;
    (que->tail) = ((que->tail) + 1) % (P_NUM + 1);
}

void init_que() {
    que->head = 0;
    que->tail = 0;
}

void init_shm(SHM_Data* shm) {
    pthread_mutexattr_t mattr; 
    pthread_condattr_t cattr; 
    pthread_mutexattr_init(&mattr); 
    pthread_condattr_init(&cattr); 
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED); 
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED); 
    pthread_mutex_init(&shm->mylock, &mattr);
    pthread_cond_init(&shm->mycond, &cattr);
    for (int i = 0; i < P_NUM; ++i) {
        pthread_cond_init(&shm->cond_tids[i], &cattr);
    }
}

int create_shm(const char *path, int index, int size) {
    key_t key = ftok(path, index);
    int shmid = shmget(key, size, IPC_CREAT | 0666); 
    return shmid;
}

void log_info(const std::string& s) {
    auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[1024];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&tm));
    std::ofstream file("log.txt", std::ios::app);
    file << buf << "\t\t" << s << std::endl;
    file.close();
}

void create_rider_process() {
    pid_t pid;
    init_que();
    for (int i = 0; i < RIDER_NUM; ++i) {
        pid = fork();
        if (pid < 0) {
            std::cerr << "fork error" << std::endl;
            exit(-1);
        }
        if (pid == 0) {
            log_info("子进程" + std::to_string(getpid()) + "被创建");
            pro[i] = {i, random_int(1, 10), getpid()};
            push_que(i);
            rider_process(i);
            exit(0);
        }
        if (pid > 0) {
            pthread_cond_wait(&(shm->cond_tids[i]), &(shm->mylock));
        }
    }
}

void rider_process(int id) {
    pthread_mutex_lock(&(shm->mylock));
    pthread_cond_signal(&(shm->cond_tids[id]));
    pthread_cond_wait(&(shm->cond_tids[id]), &(shm->mylock));

    print("process " + std::to_string(id) + " is running, pid: " << pro[id].pid << std::endl);
    sleep(2);
    *isProOver = true;
    pthread_cond_signal(&(shm->cond_tids[id]));
    pthread_mutex_unlock(&(shm->mylock));
}

void check_processes_status() {
    int status;
    pid_t pid;
    for (int i = 0; i < P_NUM; ++i) {
        pid = waitpid(pro[i].pid, &status, WNOHANG);
        if (pid == 0) {
            log_info("子进程" + std::to_string(pro[i].pid) + "正在运行");
            continue;
        } 
        if (pid == -1) {
            log_info("子进程" + std::to_string(pro[i].pid) + "不存在了");
            continue;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            log_info("子进程" + std::to_string(pro[i].pid) + "已经结束");
            continue;
        }
    }
}