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
    pthread_mutex_t riderLock;
    pthread_cond_t riderCond;
    pthread_cond_t riderConds[RIDER_NUM + 1];

    pthread_mutex_t userLock;
    pthread_cond_t userCond;
    pthread_cond_t userConds[USER_NUM + 1];

    pthread_mutex_t restLock;
    pthread_cond_t restCond;
    pthread_cond_t restConds[RIDER_NUM + 1];
} SHM_Data;
struct Que {
    int head;
    int tail;
    int q[P_NUM + 1];
};

extern struct Process* riderPro;
extern struct Process* userPro;
extern struct Process* restPro;
extern struct Que* riderQue;
extern struct Que* userQue;
extern struct Que* restQue;
extern SHM_Data* shm;
extern int* system_time;

int create_shm(const char *path, int index, int size);
void init_shm(SHM_Data *shm);
void init_que();
void pushRiderQue(int id);
void pushUserQue(int id);
void pushRestQue(int id);
void rider_process(int id);
void restaurant_process(int id);
void user_process(int id);
void scheduleRider();
void scheduleUser();
void scheduleRest();
void create_process();
void create_rider_process(int l, int r);
void create_restaurant_process(int l, int r);
void create_user_process(int l, int r);
void check_processes_status(pid_t sche_pid);

#endif