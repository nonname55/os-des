#include<iostream>
#include<pthread.h>
#include<vector>
#include<deque>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/ipc.h>
#include<sys/msg.h>
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

const std::string workdir = "/home/mqr/Workspace/os-des/";

#define RIDER_NUM 3
#define RESTAURANT_NUM 3
#define USER_NUM 3
#define SVKEY1 75
#define SVKEY2 76
#define print(msg) { \
    std::lock_guard<std::mutex> cout_lock(cout_mutex); \
    std::cout << msg; \
}
std::mutex cout_mutex;
#define DEBUG std::cout << "debug" << std::endl;

//main
void log_info(const std::string& s);
void check_processes_status(pid_t sche_pid);
struct orderMsg {
    long msg_type;
    int tarx, tary, userId, orderId, buildingId, restaurantId, riderID;
};
#define MQSIZ 7 * sizeof(int)
//process
#define P_NUM RIDER_NUM + RESTAURANT_NUM + USER_NUM
#define RIDERL USER_NUM + RESTAURANT_NUM
#define RIDERH P_NUM - 1
#define RESTAURANTL USER_NUM
#define RESTAURANTH USER_NUM + RESTAURANT_NUM - 1
#define USERL 0
#define USERH USER_NUM - 1
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
int* system_time;
int create_shm(const char *path, int index, int size);
void init_shm(SHM_Data *shm);
void init_que();
void push_que();
void rider_process(int id);
void restaurant_process(int id);
void user_process(int id);
void schedule();
void create_process();
void create_rider_process(int l, int r);
void create_restaurant_process(int l, int r);
void create_user_process(int l, int r);
//msgque
int create_msgque(int svkey);
template<typename T> void write_msgque(int msgqid, T& msg, int msgsiz);
template<typename T> int read_msgque(int msgqid, T& msg, int msgsiz, int msgtype);

int random_int(int l, int h);

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
    std::pair<int, int> getValidPos() {
        int x, y;
        do {
            x = random_int(0, G_ROW - 1);
            y = random_int(0, G_COL - 1);
        } while (bupt_map[x][y] == POS_ERROR);
        return {x, y};
    }
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

int random_int(int l, int h) {
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
        auto validPos = graph.getValidPos();
        tarx = validPos.first;
        tary = validPos.second;
        required_time = random_int(1, 10);
    }
    Order(
        int _tarx,
        int _tary,
        int _required_time
    ) {
        this->tarx = _tarx;
        this->tary = _tary;
        this->required_time = _required_time;
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
    bool shouldAcOrd(std::shared_ptr<Order>& order) {
        return random_int(0, 1) == 1 ? true : false;
    }
    void manage() {
        pthread_mutex_lock(&lock);
        print("rider" << std::endl);
        // for (int i = 0; i < 5; ++i) {
        //     std::shared_ptr<Order> temp = std::make_shared<Order>();
        //     create_order(temp);
        // }
//         int msgqid = create_msgque(SVKEY2);
//         struct orderMsg msg;
//         std::vector<orderMsg> abandon;
//         while (read_msgque(msgqid, msg, MQSIZ, 0) >= 0) {
//             std::shared_ptr<Order> temp = std::make_shared<Order>();
//             temp->tarx = msg.tarx;
//             temp->tary = msg.tary;
//             if (shouldAcOrd(temp)) {    
//                 create_order(temp);
//                 print("我是骑手" << id << "我接了饭店" << msg.restaurantId << "的订单，顾客位置："
//                         << orders.back()->tarx << ' ' << orders.back()->tary << std::endl);
//             } else {
//                 abandon.emplace_back(msg);
//             }
//         }
//         for (int i = 0; i < abandon.size(); ++i) {
//             write_msgque(msgqid, abandon[i], MQSIZ);
//         }
// SORT_ORDERS:
//         sort_order();
//         // print("rider pos:(" << posx << ',' << posy << ")" << std::endl);
//         // print("sort order: " << std::endl);
//         // for (int i = 0; i < 5; ++i) {
//         //     print("(" << orders[i]->tarx << ',' << orders[i]->tary << ") required time: " << orders[i]->required_time << std::endl);
//         // }
//         // print(std::endl);
//         while (orders.size() > 0) {
//             pthread_cond_signal(&(orders.front()->cond));
//             pthread_cond_wait(&cond, &lock);
//             while (isNewOrder) {
//                 isNewOrder = false;
//                 goto SORT_ORDERS;
//             }
//             orders.pop_front();
//         }
        pthread_mutex_unlock(&lock);
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
        print("骑手" << rider->id << "正在送单，路径：");
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
        ThreadArgs args = {this, newOrder};
        pthread_create(&(newOrder->thread), nullptr, deliver, &args);
        pthread_cond_wait(&cond, &lock);
        orders.push_back(std::move(newOrder));
    }
}rider;

struct Restaurant {
    int posx, posy, id;
    std::vector<Order> orders;
    Restaurant() {}
    Restaurant(int _posx, int _posy, int _id) {
        this->posx = _posx;
        this->posy = _posy;
        this->id = _id;
    }
    
    void manage() {
        print("rest" << std::endl);
        // int msgqid = create_msgque(SVKEY1);
        // struct orderMsg msg;
        // while (read_msgque(msgqid, msg, MQSIZ, id) >= 0) {
        //     Order newOrder(msg.tarx, msg.tary, random_int(1, 10));
        //     orders.emplace_back(newOrder);
        //     print("这里是饭店 " << id << " 接收到了用户 " 
        //             << msg.userId << " 的订单，位于 " << msg.tarx << ' ' << msg.tary << std::endl);
        // }
        // msgqid = create_msgque(SVKEY2);
        // for (int i = 0; i < orders.size(); ++i) {
        //     msg.msg_type = 1;
        //     msg.tarx = orders[i].tarx;
        //     msg.tary = orders[i].tary;
        //     msg.restaurantId = id;
        //     write_msgque(msgqid, msg, MQSIZ);
        // }
    }
}restaurant;

struct User {
    int posx, posy, id;
    User() {}
    User(int _posx, int _posy, int _id) {
        this->posx = _posx;
        this->posy = _posy;
        this->id = _id;
    }

    void manage() {
        print("user " << std::endl);
        // bool isOrder = random_int(0, 1) == 1 ? true : false;
        // if (isOrder) {
        //     int msgqid = create_msgque(SVKEY1);
        //     struct orderMsg msg;
        //     msg.msg_type = random_int(RESTAURANTL, RESTAURANTH);
        //     msg.tarx = posx;
        //     msg.tary = posy;
        //     msg.userId = id;
        //     write_msgque(msgqid, msg, MQSIZ);
        //     print("我是用户 " << id << " 我订了饭店 " << msg.msg_type << " 的外卖，我在 "
        //             << posx << ' ' << posy << std::endl);
        // }
    }
}user;

int main() {
    pid_t pid = fork();
    if (pid > 0) {
        exit(0);
    }
    setsid();
    umask(022);
    std::string outputDir = workdir + "output";
    // std::string folder = std::string(getenv("HOME")).append("/Workspace/os-des/output");
    const std::filesystem::path path = std::filesystem::path(outputDir);
    if (!(std::filesystem::exists(path) && std::filesystem::is_directory(path))) {
        int re = mkdir(outputDir.c_str(), 0777);
    }
    chdir(outputDir.c_str());
    // int fd = open("/dev/null", O_RDWR);
    outputDir += "/output.txt";
    int fd = open(outputDir.c_str(), O_RDWR | O_TRUNC);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    int shmid[6];
    shmid[0] = shmget(0, sizeof(struct Process) * P_NUM, IPC_CREAT | 0666);
    shmid[1] = shmget(1, sizeof(Que), IPC_CREAT | 0666);
    shmid[2] = shmget(2, sizeof(int), IPC_CREAT | 0666);
    int shmId = create_shm(".", 1, sizeof(SHM_Data));
    pro = (struct Process*)shmat(shmid[0], NULL, 0);
    que = (struct Que*)shmat(shmid[1], NULL, 0);
    system_time = (int*)shmat(shmid[2], NULL, 0);
    shm = (SHM_Data*)shmat(shmId, NULL, 0);
    init_shm(shm);

    pid_t sche_pid = fork();
    if (sche_pid == 0) {
        pthread_mutex_lock(&shm->mylock);
        msgctl(msgget(SVKEY1, 0666), IPC_RMID, NULL);
        msgctl(msgget(SVKEY2, 0666), IPC_RMID, NULL);
        create_process();
        while (true) {
            schedule();
            sleep(1);
        }

        pthread_mutex_unlock(&shm->mylock);
    } else if (sche_pid > 0) {
        while (true) {
            check_processes_status(sche_pid);
            sleep(2);
        }
    } else {
        print("schedule fork error");
    }

    return 0;
}

int create_msgque(int svkey) {
    key_t key = svkey;
    int msgqid = msgget(key, IPC_CREAT | 0666);
    if (msgqid == -1) {
        print("error create message que, svkey = " << svkey << std::endl);
        exit(-1);
    }
    return msgqid;
}

template<typename T>
void write_msgque(int msgqid, T& msg, int msgsiz) {
    int ret = msgsnd(msgqid , &msg, msgsiz, 0);
    if (ret == -1) {
        print("write msg que error at msgqid " << msgqid << std::endl);
        exit(-1);
    }
}

template<typename T>
int read_msgque(int msgqid, T& msg, int msgsiz, int msgtype) {
    int ret = msgrcv(msgqid, &msg, msgsiz, msgtype, IPC_NOWAIT);
    if (ret == -1) {
        // print("read msg que error at msg qid " << msgqid << std::endl);
    }
    return ret;
}

void create_process() {
    init_que();
    create_user_process(USERL, USERH);
    create_restaurant_process(RESTAURANTL, RESTAURANTH);
    create_rider_process(RIDERL, RIDERH);
}

void schedule() {
    for (int i = 0; i < P_NUM; ++i) {
        // if (!(i >= RIDERL && i <= RIDERH)) continue;
        pthread_cond_signal(&(shm->cond_tids[i]));
        pthread_cond_wait(&(shm->cond_tids[i]), &(shm->mylock));
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

void create_rider_process(int l, int r) {
    pid_t pid;
    for (int i = l; i <= r; ++i) {
        pid = fork();
        if (pid < 0) {
            std::cerr << "fork error" << std::endl;
            exit(-1);
        } else if (pid == 0) {
            pro[i] = {i, random_int(1, 10), getpid()};
            push_que(i);
            rider_process(i);
            exit(0);
        } else if (pid > 0) {
            pthread_cond_wait(&(shm->cond_tids[i]), &(shm->mylock));
        }
    }
}

void create_user_process(int l, int r) {
    pid_t pid;
    for (int i = l; i <= r; ++i) {
        pid = fork();
        if (pid < 0) {
            std::cerr << "fork error" << std::endl;
            exit(-1);
        } else if (pid == 0) {
            pro[i] = {i, random_int(1, 10), getpid()};
            push_que(i);
            user_process(i);
            exit(0);
        } else if (pid > 0) {
            pthread_cond_wait(&(shm->cond_tids[i]), &(shm->mylock));
        }
    }
}

void create_restaurant_process(int l, int r) {
    pid_t pid;
    for (int i = l; i <= r; ++i) {
        pid = fork();
        if (pid < 0) {
            std::cerr << "fork error" << std::endl;
            exit(-1);
        } else if (pid == 0) {
            pro[i] = {i, random_int(1, 10), getpid()};
            push_que(i);
            restaurant_process(i);
            exit(0);
        } else if (pid > 0) {
            pthread_cond_wait(&(shm->cond_tids[i]), &(shm->mylock));
        }
    }
}

void rider_process(int id) {
    pthread_mutex_lock(&(shm->mylock));
    while (true) {
        pthread_cond_signal(&(shm->cond_tids[id]));
        pthread_cond_wait(&(shm->cond_tids[id]), &(shm->mylock));

        rider.id = id;
        rider.manage();
    }

    pthread_cond_signal(&(shm->cond_tids[id]));
    pthread_mutex_unlock(&(shm->mylock));
}

void restaurant_process(int id) {
    pthread_mutex_lock(&(shm->mylock));
    while (true) {
        pthread_cond_signal(&(shm->cond_tids[id]));
        pthread_cond_wait(&(shm->cond_tids[id]), &(shm->mylock));

        auto validPos = graph.getValidPos();
        restaurant = {validPos.first, validPos.second, id};
        restaurant.manage();
    }

    pthread_cond_signal(&(shm->cond_tids[id]));
    pthread_mutex_unlock(&(shm->mylock));
}

void user_process(int id) {
    pthread_mutex_lock(&(shm->mylock));
    while (true) {
        pthread_cond_signal(&(shm->cond_tids[id]));
        pthread_cond_wait(&(shm->cond_tids[id]), &(shm->mylock));

        auto validPos = graph.getValidPos();
        user = {validPos.first, validPos.second, id};
        user.manage();
    }

    pthread_cond_signal(&(shm->cond_tids[id]));
    pthread_mutex_unlock(&(shm->mylock));
}

void check_processes_status(pid_t sche_pid) {
    int status;
    pid_t pid = waitpid(sche_pid, &status, WNOHANG);
    if (pid == 0) {
        log_info("子进程" + std::to_string(sche_pid) + "正在运行");
    } else if (pid == -1) {
        log_info("子进程" + std::to_string(sche_pid) + "不存在了");
    } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
        log_info("子进程" + std::to_string(sche_pid) + "已经结束");
    }
}