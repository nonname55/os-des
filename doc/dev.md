# 1 守护进程

## 1.1 思路

在`main`函数中，使用`fork`创建一个子进程，将父进程退出，子进程利用`setsid()`脱离进程组，成为守护进程。守护进程利用c++文件操作写入日志、启动前后端、在前端异常退出时重启前端、监测子进程状态

## 1.2 实现

1. 创建守护进程

   ```cpp
   pid_t pid = fork();
   if (pid > 0) {
       exit(0);
   }
   setsid();
   ```

2. 监测子进程状态、写入日志

   ```cpp
   void log_info(const std::string& s) 
   {
       auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
       char buf[1024];
       strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&tm));
       std::ofstream file("daemonLog.txt", std::ios::app);
       file << buf << "\t\t" << s << std::endl;
       file.close();
   }
   ```

3. 启动前端。主要利用`execl`函数将创建的子进程替换为前端可执行程序

   ```cpp
   execl("../bin/MainWindow", "MainWindow", NULL);
   ```

4. 前端异常退出时重启。原理：当前端子进程点击关闭按钮正常退出时，通过消息队列给守护进程发送一个消息。如果在监测子进程状态时发现前端子进程已经退出，并且消息队列中并没有任何消息，说明前端是异常退出，需要重启。重启即重新创建前端子进程。

   ```cpp
   int mq_ret = MQ::read(MQ::create(FRONT_END_SVKEY), msg, 0, false);
   if (mq_ret == -1 && check_processes_status(frontpid, FRONT_PRO) != 0) {
       frontpid = fork();
       if (frontpid == 0) {
           setsid();
           int ret = execl("../bin/MainWindow", "MainWindow", NULL);
           if (ret == -1) {
               print("execl error" << std::endl);
           }
           exit(0);
       }
   }
   ```

# 2 进程调度

## 2.1 简介

本项目中共有三种进程：骑手、饭店、顾客。相同种类进程可能有多个（例：多个骑手）。不同种进程之间并行，相同种类进程之间串行。并行的实现是利用`setsid`使进程脱离进程组，在系统中独立运行。相同种类进程因为是串行，所以需要进程调度。

## 2.2 实现

### 2.2.1 API和进程通信方式

API：

```cpp
pthread_cond_signal(&(shm->userConds[i]));
pthread_cond_wait(&(shm->userCond), &(shm->userLock));
```

进程通信方式：共享内存

### 2.2.2 使用thread库进行同步互斥的原因

1. 希望采取绝对安全的方式创建、调度进程。

   不希望在一个进程没创建好的时候就去创建下一个进程

   不希望还有进程没创建，主进程就开始调度

   不希望唤醒信号发出但目标进程因为某些原因没有收到信号

2. 联系课内所学

### 2.2.3 使用共享内存的原因

要实现同步互斥需要互斥锁和条件变量，但进程间内存不共享，故必须使用共享内存访问同一个锁/条件变量。

### 2.2.4 原理

1. 申请共享内存，将所需信息存入共享内存

   部分代码片段：

   ```cpp
   int shmid[10];
   shmid[0] = shmget(0, sizeof(struct Process) * RIDER_NUM, IPC_CREAT | 0666);
   shmid[1] = shmget(1, sizeof(struct Process) * USER_NUM, IPC_CREAT | 0666);
   shmid[2] = shmget(2, sizeof(struct Process) * RESTAURANT_NUM, IPC_CREAT | 0666);
   riderPro = (struct Process*)shmat(shmid[0], NULL, 0);
   userPro = (struct Process*)shmat(shmid[1], NULL, 0);
   restPro = (struct Process*)shmat(shmid[2], NULL, 0);
   ```

2. 创建一个子进程，父进程阻塞，子进程将自己的信息加入到进程队列，唤醒父进程后子进程阻塞。父进程被唤醒，继续创建子进程。

   部分代码片段：

   ```cpp
   //父进程
   void create_rider_process(int l, int r) 
   {
       pid_t pid;
       for (int i = l; i <= r; ++i) {
           pid = fork();
           if (pid < 0) {
               std::cerr << "fork error" << std::endl;
               exit(-1);
           } else if (pid == 0) {
               //子进程把自己加入进程队列
               riderPro[i] = {i, random_int(1, 10), getpid()};
               pushRiderQue(i);
               //子进程执行函数，子进程创建好了
               rider_process(i);
               exit(0);
           } else if (pid > 0) {
               //创建一个子进程，父进程阻塞
               pthread_cond_wait(&(shm->riderCond), &(shm->riderLock));
           }
       }
   }
   
   //子进程
   void rider_process(int id) 
   {
       pthread_mutex_lock(&(shm->riderLock));
       while (true) {
           //唤醒父进程，阻塞自己
           pthread_cond_signal(&(shm->riderCond));
           pthread_cond_wait(&(shm->riderConds[id]), &(shm->riderLock));
           //父进程唤醒自己后，开始执行，执行完后再次进入循环，重复这个过程
   
           rider.self_id = id;
           print("骑手" << rider.self_id << "开始执行" << std::endl);
           rider.manage();
           print("骑手" << rider.self_id << "结束执行" << std::endl);
       }
   
       pthread_cond_signal(&(shm->riderCond));
       pthread_mutex_unlock(&(shm->riderLock));
   }
   ```

3. 父进程获得执行权限后，根据上述原理，子进程此时全部阻塞，父进程通过访问进程队列，决定好下一个要执行的子进程后，唤醒指定的子进程执行。

   这里给出按照队列顺序执行的示例代码：

   ```cpp
   void scheduleRider() 
   {
       for (int i = 1; i <= RIDER_NUM; ++i) {
           pthread_cond_signal(&(shm->riderConds[i]));//唤醒指定子进程
           pthread_cond_wait(&(shm->riderCond), &(shm->riderLock));
       }
   }
   ```

> 特殊说明：这里使用的是fork出来的进程而不是线程，thread库中的互斥锁和条件变量经过初始化后就可以对进程使用

# 3 线程调度

线程调度整体实现思路和进程调度大体相同。只不过线程间内存本身就是共享的，所以相比进程调度少了申请共享内存的步骤。

## 3.1 线程调度算法

### 3.1.1 原理

每个订单有一个要求送达时间，首先把当前骑手接的所有订单按照要求送达时间升序排列（越紧急的放越前面），然后计算出送达这个订单之前是否能在不超时的情况下送另一个次紧急的订单。最终得出下一步要送的订单（执行的线程）

### 3.1.2 代码

```cpp
int Rider::cal_next_order()
{
    int orders_size = orders.size();
    if (orders_size == 0)
        return -1;
    if (orders_size == 1) {
        return 0;
    }
    //按照要求送达时间升序排列
    std::sort(orders.begin(), orders.end(), 
        [](std::shared_ptr<Order> porder1, std::shared_ptr<Order> porder2) {
            return porder1->required_time < porder2->required_time;
        });
    if (cal_delivery_time(self_x, self_y, orders[0]) + (*system_time) >= orders[0]->required_time) {
        return 0;
    }
    //尝试寻找中转订单
    for (int i = 1; i < orders_size; ++i) {
        std::pair<int, int> transfer_pos;
        if (orders[i]->is_take)
            transfer_pos = {orders[i]->user_x, orders[i]->user_y};
        else
            transfer_pos = {orders[i]->rest_x, orders[i]->rest_y};
        int to_transfer_time = Graph::astar(self_x, self_y, transfer_pos.first, transfer_pos.second, Graph::graph);
        if (orders[i]->is_take)
            to_transfer_time += std::max(orders[i]->done_time - (*system_time) - to_transfer_time, 0);
        int sum_time = to_transfer_time + 
            cal_delivery_time(transfer_pos.first, transfer_pos.second, orders[0]);
        //送这个订单且最紧急的订单不会超时，那就先送这个订单
        if (sum_time <= orders[0]->required_time) {
            auto it = std::next(orders.begin(), i);
            std::rotate(orders.begin(), it, orders.end());
            break;
        }
    }
    return 0;
}
```

# 4 消息传递

最主要的消息传递方式是消息队列

## 4.1 API

```cpp
msgsnd(msgqid , &msg, msgsiz, 0);
msgrcv(msgqid, &msg, msgsiz, msgtype, IPC_NOWAIT);
msgget(key, IPC_CREAT | 0666);
msgctl(msgqid, IPC_STAT, &buf)
```

## 4.2 设计

1. 发现消息队列相关函数要传的参数很多，并且并不是每个参数每次都需要，所以可以对消息队列函数进行封装：

   创建消息队列：

   ```cpp
   int MQ::create(int svkey) 
   {
       key_t key = svkey;
       int msgqid = msgget(key, IPC_CREAT | 0666);
       if (msgqid == -1) {
           print(getpid() << " error create message que, svkey = " << svkey << std::endl);
           exit(-1);
       }
       return msgqid;
   }
   ```

   删除消息队列：

   ```cpp
   void MQ::delete_mq(int svkey)
   {
       msgctl(msgget(svkey, 0666), IPC_RMID, NULL);
   }
   ```

   收/发消息：

   ```cpp
   template <typename T>
   void write(int msgqid, T &msg) 
   {
       int msgsiz = sizeof(msg) - sizeof(long);
       int ret = msgsnd(msgqid , &msg, msgsiz, 0);
       if (ret == -1) {
           print(getpid() << " write msg que error at msgqid " << msgqid << std::endl);
           exit(-1);
       }
   }
   
   template <typename T>
   int read(int msgqid, T &msg, int msgtype, bool is_wait) 
   {
       int msgsiz = sizeof(msg) - sizeof(long);
       // int msg_flag = is_wait ? 0 : IPC_NOWAIT;
       int ret = msgrcv(msgqid, &msg, msgsiz, msgtype, IPC_NOWAIT);
       if (ret == -1) {
           // print("接收消息失败, msgqid=" << msgqid << std::endl);
       }
       return ret;
   }
   ```

   > 这里收发消息采用了C++的模板，可以在同一个函数中发送不同类型的消息

## 4.3 结构体属性

由于消息队列在发送时，要指定发送的消息大小，并且这个大小是不包括消息结构体一开始的long，所以相比自己数出来有多少int来计算大小（例：7 * sizeof(int) + 2 * sizeof(bool)），不如直接用sizeof(消息结构体)-sizeof(long)，但是c++的struct默认有内存对齐，也就是sizeof得到的结构体大小并不是正确的，所以可以通过设置结构体的属性取消其内存对齐，就可以非常方便的计算出消息的准确大小

```cpp
struct rider_info_struct {
    long type;
    int event_type;
    int rider_x, rider_y;
} __attribute__((packed));

msgsiz = sizeof(rider_info_struct) - sizeof(long)
```

# 5 文件处理&文件锁

在本项目中，需要文件处理的地方主要有：守护进程日志，恢复日志（用于重启时恢复信息）

主要介绍恢复日志

由于不同种进程间是并行的关系，并且它们都有各自的信息要写入恢复日志，这时就需要文件锁来保证共享资源的安全访问。

原理很简单，就是对文件读写前加锁，读写后解锁即可

## 5.1 API

```cpp
fopen(filePath.c_str(), "a+");
flock(fileno(pFile), LOCK_EX)
fwrite(fileData.c_str(), 1, fileData.length(), pFile);
fread(readBuf, 1, minRead, pFile);
```

## 5.2 示例代码

完整代码位于include/file.h

```cpp
// 共享锁/不阻塞
if (lock && flock(fileno(pFile), LOCK_SH | LOCK_NB) != 0) {
    fclose(pFile);
    return false;
}

// 计算文件大小
fseek(pFile, 0, SEEK_SET);
long begin = ftell(pFile);
fseek(pFile, 0, SEEK_END);
long end = ftell(pFile);
long fileSize = end - begin;
fseek(pFile, 0, SEEK_SET); //重新指向文件头

// 预分配内存空间
fileData.reserve(fileSize + 1);

// 读取文件内容
char readBuf[bufSize + 1];
long readSize = 0;
while (readSize < fileSize) {
    long minRead = std::min(fileSize - readSize, bufSize);
    long len = fread(readBuf, 1, minRead, pFile);
    readSize += len;
    fileData.append(readBuf, len);
}

splitString(data, fileData, '\n');

// 解锁
if (lock && flock(fileno(pFile), LOCK_UN) != 0) {
    fclose(pFile);
    return false;
}
```

# 6 最短路/较短路

选择A*算法

## 6.1 原因

1. 本项目的应用环境是一个二维地图，在这种条件下最适合使用bfs算法及其衍生、改进算法，A*就是其中一种
2. A*相比常规bfs、优先队列bfs的效率都高了非常多，大大缩短骑手找到路径的时间
3. A*相比复杂算法（蚁群算法、遗传算法……）实现上容易了很多
4. 虽然A*不一定得出最短路，但是得出最短路概率很高，且本程项目也不是要求一定要最短，只是一个较短路也完全可以接受

## 6.2 原理

A* 是一个启发式算法。基于优先队列，每次出队的是预估到达目的地长度最短的，这个预估到达长度是 当前点已经走过的距离+当前点到终点的曼哈顿距离

代码：

```cpp
struct Point {
    int x, y;
    int step, est;
    bool operator < (const Point& p) const {
        return step + est > p.step + p.est;
    }
};
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
```

# 7 同步互斥

主要体现在：

1. 对于所有骑手进程来说，商家发布的订单队列就是公共资源，为了避免访问冲突，需要采用互斥锁来控制。骑手之间是串行的，通过进程调度，控制每次访问订单队列的只有一个骑手
2. 进程队列。进程队列是为了方便主进程调度子进程（访问子进程优先级等内容），是每个子进程被创建后自行将信息存入进程队列。由于进程队列是共享内存，所以需要同步互斥，控制每次只有一个进程访问
3. 文件锁部分
