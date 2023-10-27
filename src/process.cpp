#include "../include/header.h"
#include "../include/process.h"
#include "../include/graph.h"
#include "../include/rider.h"
#include "../include/user.h"
#include "../include/restaurant.h"

struct Process *riderPro;
struct Process *userPro;
struct Process *restPro;
struct Que *riderQue;
struct Que *userQue;
struct Que *restQue;
SHM_Data *shm;
int *order_count;
int *system_time;

int create_shm(const char *path, int index, int size) 
{
    key_t key = ftok(path, index);
    int shmid = shmget(key, size, IPC_CREAT | 0666); 
    return shmid;
}

void init_shm(SHM_Data* shm) 
{
    pthread_mutexattr_t rider_mattr; 
    pthread_condattr_t rider_cattr; 
    pthread_mutexattr_t user_mattr; 
    pthread_condattr_t user_cattr; 
    pthread_mutexattr_t rest_mattr; 
    pthread_condattr_t rest_cattr; 
    pthread_mutexattr_init(&rider_mattr); 
    pthread_condattr_init(&rider_cattr);
    pthread_mutexattr_init(&user_mattr); 
    pthread_condattr_init(&user_cattr); 
    pthread_mutexattr_init(&rest_mattr); 
    pthread_condattr_init(&rest_cattr);  
    pthread_mutexattr_setpshared(&rider_mattr, PTHREAD_PROCESS_SHARED); 
    pthread_condattr_setpshared(&rider_cattr, PTHREAD_PROCESS_SHARED); 
    pthread_mutexattr_setpshared(&user_mattr, PTHREAD_PROCESS_SHARED); 
    pthread_condattr_setpshared(&user_cattr, PTHREAD_PROCESS_SHARED); 
    pthread_mutexattr_setpshared(&rest_mattr, PTHREAD_PROCESS_SHARED); 
    pthread_condattr_setpshared(&rest_cattr, PTHREAD_PROCESS_SHARED); 
    pthread_mutex_init(&shm->riderLock, &rider_mattr);
    pthread_mutex_init(&shm->userLock, &user_mattr);
    pthread_mutex_init(&shm->restLock, &rest_mattr);
    pthread_cond_init(&shm->riderCond, &rider_cattr);
    pthread_cond_init(&shm->userCond, &user_cattr);
    pthread_cond_init(&shm->restCond, &rest_cattr);
    for (int i = 0; i <= RIDER_NUM; ++i) {
        pthread_cond_init(&shm->riderConds[i], &rider_cattr);
    }
    for (int i = 0; i <= USER_NUM; ++i) {
        pthread_cond_init(&shm->userConds[i], &user_cattr);
    }
    for (int i = 0; i <= RESTAURANT_NUM; ++i) {
        pthread_cond_init(&shm->restConds[i], &rest_cattr);
    }
}

void pushRiderQue(int val) 
{
    (riderQue->q)[riderQue->tail] = val;
    (riderQue->tail) = ((riderQue->tail) + 1) % (P_NUM + 1);
}

void pushUserQue(int val) 
{
    (userQue->q)[userQue->tail] = val;
    (userQue->tail) = ((userQue->tail) + 1) % (P_NUM + 1);
}

void pushRestQue(int val) 
{
    (restQue->q)[restQue->tail] = val;
    (restQue->tail) = ((restQue->tail) + 1) % (P_NUM + 1);
}

void init_que() 
{
    riderQue->head = 0;
    riderQue->tail = 0;
    userQue->head = 0;
    userQue->tail = 0;
    restQue->head = 0;
    restQue->tail = 0;
}

void rider_process(int id) 
{
    pthread_mutex_lock(&(shm->riderLock));
    while (true) {
        pthread_cond_signal(&(shm->riderCond));
        pthread_cond_wait(&(shm->riderConds[id]), &(shm->riderLock));

        rider.self_id = id;
        print("骑手" << rider.self_id << "开始执行" << std::endl);
        rider.manage();
        print("骑手" << rider.self_id << "结束执行" << std::endl);
    }

    pthread_cond_signal(&(shm->riderCond));
    pthread_mutex_unlock(&(shm->riderLock));
}

void restaurant_process(int id) 
{
    pthread_mutex_lock(&(shm->restLock));
    while (true) {
        pthread_cond_signal(&(shm->restCond));
        pthread_cond_wait(&(shm->restConds[id]), &(shm->restLock));

        // auto validPos = graph.getValidPos();
        // restaurant = {validPos.first, validPos.second, id};
        restaurant.self_id = id;
        restaurant.manage();
    }

    pthread_cond_signal(&(shm->restCond));
    pthread_mutex_unlock(&(shm->restLock));
}

void user_process(int id) 
{
    pthread_mutex_lock(&(shm->userLock));
    while (true) {
        pthread_cond_signal(&(shm->userCond));
        pthread_cond_wait(&(shm->userConds[id]), &(shm->userLock));

        // auto validPos = graph.getValidPos();
        // user = {validPos.first, validPos.second, id};
        user.self_id = id;
        user.manage();
    }

    pthread_cond_signal(&(shm->userCond));
    pthread_mutex_unlock(&(shm->userLock));
}

void scheduleRider() 
{
    for (int i = 1; i <= RIDER_NUM; ++i) {
        pthread_cond_signal(&(shm->riderConds[i]));
        pthread_cond_wait(&(shm->riderCond), &(shm->riderLock));
    }
}

void scheduleUser() 
{
    for (int i = 1; i <= USER_NUM; ++i) {
        pthread_cond_signal(&(shm->userConds[i]));
        pthread_cond_wait(&(shm->userCond), &(shm->userLock));
    }
}

void scheduleRest() 
{
    for (int i = 1; i <= RESTAURANT_NUM; ++i) {
        pthread_cond_signal(&(shm->restConds[i]));
        pthread_cond_wait(&(shm->restCond), &(shm->restLock));
    }
}

void create_process() 
{
    init_que();
    create_user_process(USERL, USERH);
    create_restaurant_process(RESTAURANTL, RESTAURANTH);
    create_rider_process(RIDERL, RIDERH);
}

void create_rider_process(int l, int r) 
{
    pid_t pid;
    for (int i = l; i <= r; ++i) {
        pid = fork();
        if (pid < 0) {
            std::cerr << "fork error" << std::endl;
            exit(-1);
        } else if (pid == 0) {
            print("create rider" << i << ' ' << RIDER_NUM << std::endl);
            riderPro[i] = {i, random_int(1, 10), getpid()};
            pushRiderQue(i);
            rider_process(i);
            exit(0);
        } else if (pid > 0) {
            pthread_cond_wait(&(shm->riderCond), &(shm->riderLock));
        }
    }
}

void create_user_process(int l, int r) 
{
    pid_t pid;
    for (int i = l; i <= r; ++i) {
        pid = fork();
        if (pid < 0) {
            std::cerr << "fork error" << std::endl;
            exit(-1);
        } else if (pid == 0) {
            userPro[i] = {i, random_int(1, 10), getpid()};
            pushUserQue(i);
            user_process(i);
            exit(0);
        } else if (pid > 0) {
            pthread_cond_wait(&(shm->userCond), &(shm->userLock));
        }
    }
}

void create_restaurant_process(int l, int r) 
{
    pid_t pid;
    for (int i = l; i <= r; ++i) {
        pid = fork();
        if (pid < 0) {
            std::cerr << "fork error" << std::endl;
            exit(-1);
        } else if (pid == 0) {
            restPro[i] = {i, random_int(1, 10), getpid()};
            pushRestQue(i);
            restaurant_process(i);
            exit(0);
        } else if (pid > 0) {
            pthread_cond_wait(&(shm->restCond), &(shm->restLock));
        }
    }
}

void check_processes_status(pid_t sche_pid) 
{
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