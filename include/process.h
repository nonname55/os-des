#ifndef PRO_H
#define PRO_H

#include "header.h"
#include "macro.h"

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

extern struct Process* pro;
extern struct Que* que;
extern SHM_Data* shm;
extern int* system_time;

int create_shm(const char *path, int index, int size);
void init_shm(SHM_Data *shm);
void init_que();
void push_que(int id);
void rider_process(int id);
void restaurant_process(int id);
void user_process(int id);
void schedule();
void create_process();
void create_rider_process(int l, int r);
void create_restaurant_process(int l, int r);
void create_user_process(int l, int r);
void check_processes_status(pid_t sche_pid);

#endif