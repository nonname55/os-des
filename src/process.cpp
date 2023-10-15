#include "header.h"
#include "process.h"
#include "graph.h"
#include "rider.h"
#include "user.h"
#include "restaurant.h"

struct Process* pro;
struct Que* que;
SHM_Data* shm;
int* system_time;

int create_shm(const char *path, int index, int size) {
    key_t key = ftok(path, index);
    int shmid = shmget(key, size, IPC_CREAT | 0666); 
    return shmid;
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

void push_que(int val) {
    (que->q)[que->tail] = val;
    (que->tail) = ((que->tail) + 1) % (P_NUM + 1);
}

void init_que() {
    que->head = 0;
    que->tail = 0;
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

void schedule() {
    for (int i = 0; i < P_NUM; ++i) {
        // if (!(i >= RIDERL && i <= RIDERH)) continue;
        pthread_cond_signal(&(shm->cond_tids[i]));
        pthread_cond_wait(&(shm->cond_tids[i]), &(shm->mylock));
    }
}

void create_process() {
    init_que();
    create_user_process(USERL, USERH);
    create_restaurant_process(RESTAURANTL, RESTAURANTH);
    create_rider_process(RIDERL, RIDERH);
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