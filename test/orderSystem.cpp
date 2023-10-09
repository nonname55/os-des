#include<iostream>
#include<pthread.h>
#include<vector>
#include<deque>
#include<sys/types.h>
#include<sys/wait.h>
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

#define RIDER_NUM 1
#define BUILDING_NUM 10
#define SHOP_NUM 5

#define SVKEY 75
#define SVKEY2 76
#define SVKEY3 77
#define SVKEY4 78
#define REQ 1

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

//订单消息
struct orderMsg{
    long msg_type;
    int tarx, tary, userId, orderId, buildingId, shopId,riderID;
};

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

/*struct User {
    static int userId, buildingId, orderId;
    static pthread_cond_t cond;
    static pthread_mutex_t lock;
    int msqid;
    User() {
        pthread_cond_init(&cond, nullptr);

        struct orderMsg reqmsg,replymsg;
        key_t key=SVKEY;
        if((msqid=msgget(key,IPC_CREAT|0666))==-1){  //0666 means user has access to read,write and run
            perror("msgget");
            exit(1);
        }

    }

    static void create_order(std::shared_ptr<Order>& newOrder) {
        pthread_mutex_lock(&lock);

        pthread_create(&(newOrder->thread), nullptr, deliver, newOrder.get());
        pthread_cond_wait(&cond, &lock);
        orders.push_back(std::move(newOrder));

        pthread_mutex_unlock(&lock);
    }
};*/

struct Building{
    int posx, posy, id;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int msqid;

    Building(
        int _posx = 0, 
        int _posy = 0, 
        int _id = random_int(1, 10)
    ) {
        this->posx = _posx;
        this->posy = _posy;
        this->id = _id;
    }

    void manage() {
        while(true){
            int waitTime = random_int(5, 10);
            sleep(waitTime);
            std::shared_ptr<Order> temp = std::make_shared<Order>();
            //create_order(temp);
            struct orderMsg reqmsg,replymsg;
            key_t key=SVKEY;
            key_t key4=SVKEY4;
            if((msqid=msgget(key,IPC_CREAT|0666))==-1){  //0666 means user has access to read,write and run
                perror("建立订餐消息队列");
                exit(1);
            }
            int shopId = random_int(1, SHOP_NUM);
            reqmsg.shopId=shopId;
            reqmsg.msg_type=REQ;
            reqmsg.tarx=posx;
            reqmsg.tary=posy;
            reqmsg.buildingId=id;
            if(msgsnd(msqid,&reqmsg,sizeof(int),0)==-1){  //send the message to message queue
                perror("发送order给商家");
                exit(1);
            }
            printf("order request has send\n");
            //收到快递员送到的消息
            if((msqid=msgget(key4,IPC_CREAT|0666))==-1){  //0666 means user has access to read,write and run
                perror("建立收餐消息队列");
                exit(1);
            }
            if(msgrcv(msqid,&reqmsg,sizeof(int),REQ,0)==-1){   //receive the msg from message queue
            perror("接收收餐消息");
            exit(1);
            }
        }
    }

    /*static void create_order(std::shared_ptr<Order>& newOrder) {
        pthread_mutex_lock(&lock);

        pthread_create(&(newOrder->thread), nullptr, deliver, newOrder.get());
        pthread_cond_wait(&cond, &lock);
        orders.push_back(std::move(newOrder));

        pthread_mutex_unlock(&lock);
    }*/
};

struct Shop{
    int posx, posy, id;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int msqid;
    struct orderMsg reqmsg,replymsg,reqmsg2;

    Shop(
        int _posx = 0, 
        int _posy = 0, 
        int _id = random_int(1, 10)
    ) {
        this->posx = _posx;
        this->posy = _posy;
        this->id = _id;
    }
    void manage(){
        key_t key=SVKEY;//key负责building和商家下订单
        key_t key2=SVKEY2;//key2负责通知骑手并抢订单
        key_t key3=SVKEY3;//key3负责通知骑手取单并骑手取单
        key_t key4=SVKEY4;//key4负责送餐
        printf("shop has started, waiting for requests\n");
        while(true){
            if((msqid=msgget(key,IPC_CREAT|0666))==-1){  //0666 means user has access to read,write and run
                perror("建立订餐消息队列");
                exit(1);
            }
            //收到building发来的order信息
            if(msgrcv(msqid,&reqmsg,sizeof(int),REQ,0)==-1){   //receive the msg from message queue
            perror("商家接收order");
            exit(1);
            }
            print("shop get order" << std::endl);
            if(id!=reqmsg.shopId)continue;
            
            //发布订单
            if((msqid=msgget(key2,IPC_CREAT|0666))==-1){  //0666 means user has access to read,write and run
                perror("建立发布订单消息队列");
                exit(1);
            }
            if(msgsnd(msqid,&reqmsg,sizeof(int),0)==-1){  //send the message to message queue
                perror("给骑手发送订单");
                exit(1);
            }
            if(msgrcv(msqid,&reqmsg,sizeof(int),REQ,0)==-1){   //receive the msg from message queue
                perror("收到骑手接单回应");
                exit(1);
            }  
            //接收抢单完成的消息（骑手发回信息）
            printf("order has received by riders, shop start to prepare for dishes\n");
            if((msqid=msgget(key3,IPC_CREAT|0666))==-1){  //0666 means user has access to read,write and run
                perror("建立骑手取餐消息队列");
                exit(1);
            }
            if(msgsnd(msqid,&reqmsg,sizeof(int),0)==-1){  //send the message to message queue
                perror("发送取单消息");
                exit(1);
            }
            //发出取餐消息后等待骑手回应
            if(msgrcv(msqid,&reqmsg,sizeof(int),REQ,0)==-1){   //receive the msg from message queue
                perror("收到骑手已取单回应");
                exit(1);
            }
            printf("dish has received by riders, start to send\n");
            //送餐
            if((msqid=msgget(key4,IPC_CREAT|0666))==-1){  //0666 means user has access to read,write and run
                perror("建立送餐消息队列");
                exit(1);
            }
            if(msgsnd(msqid,&reqmsg,sizeof(int),0)==-1){  //send the message to message queue
                perror("发送已送到消息");
                exit(1);
            }
        }
    }
};

struct Rider {
    int posx, posy, id, speed;
    std::deque<std::shared_ptr<Order>> orders;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool isNewOrder;
    int msqid;
    struct orderMsg reqmsg,replymsg,reqmsg2;
    struct ThreadArgs {
        Rider* rider;
        std::shared_ptr<Order> order;
    };

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
    void manage() {
        /*for (int i = 0; i < 5; ++i) {
            std::shared_ptr<Order> temp = std::make_shared<Order>();
            create_order(temp);
        }*/
        //骑手的循环
        while(1){
            key_t key2=SVKEY2;
            if((msqid=msgget(key2,IPC_CREAT|0666))==-1){  //0666 means user has access to read,write and run
                perror("建立抢单消息队列");
                exit(1);
            }
            if(msgrcv(msqid,&reqmsg,sizeof(int),REQ,0)==-1){   //receive the msg from message queue
                perror("收到商家的订单发布");
                exit(1);
            }
            //抢单
            replymsg.riderID=id;
            if(msgsnd(msqid,&replymsg,sizeof(int),0)==-1){  //send the message to message queue
                perror("发送抢单结果");
                exit(1);
            }
            //骑手被通知来取餐
            key_t key3=SVKEY3;
            if((msqid=msgget(key3,IPC_CREAT|0666))==-1){  //0666 means user has access to read,write and run
                perror("建立取餐消息队列");
                exit(1);
            }
            if(msgrcv(msqid,&reqmsg,sizeof(int),REQ,0)==-1){   //receive the msg from message queue
            perror("接收商家取餐完成");
            exit(1);
            }
            //收到取餐通知发消息去取餐
            if(msgsnd(msqid,&replymsg,sizeof(int),0)==-1){  //send the message to message queue
                perror("发送已取餐给商家");
                exit(1);
            }
            //开始送餐
            key_t key4=SVKEY4;
            if((msqid=msgget(key4,IPC_CREAT|0666))==-1){  //0666 means user has access to read,write and run
                perror("建立送餐与客户消息队列");
                exit(1);
            }
            if(msgsnd(msqid,&replymsg,sizeof(int),0)==-1){  //send the message to message queue
                perror("发送已送达消息给客户");
                exit(1);
            }
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
};

int main() {
    std::vector<Rider> riders(RIDER_NUM);
    std::vector<Building> buildings(BUILDING_NUM);
    std::vector<Shop> shops(SHOP_NUM);
    for (int i=0;i<BUILDING_NUM;++i){
        pid_t pid = fork();
        if(pid == 0){
            // Building::manage(buildings[i]);
            buildings[i].manage();
            exit(0);
        }
        else if(pid < 0){
            print("Failed to create buildings" << std::endl);
        }
    }

    for (int i = 0; i < SHOP_NUM; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            // Shop::manage(shops[i]);
            shops[i].manage();
            exit(0);
        } else if (pid < 0) {
            print("Failed to create shops" << std::endl);
        }
    }

    for (int i = 0; i < RIDER_NUM; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            // Rider::manage(riders[i]);
            riders[i].manage();
            exit(0);
        } else if (pid < 0) {
            print("Failed to create riders" << std::endl);
        }
    }

    for (int i = 0; i < RIDER_NUM+SHOP_NUM+BUILDING_NUM; ++i) {
        wait(NULL);
    }

    return 0;
}