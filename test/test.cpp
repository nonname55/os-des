#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<bits/stdc++.h>
#include<pthread.h>
#include<unistd.h>
#include<time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include<sys/wait.h>
#include<sys/types.h>
using namespace std;

#define P_NUM 30
#define TIME_SLICE 5
#define SCHED_OTHER 0
#define SCHED_FIFO 1
#define SCHED_RR 2
#define schedule_t int
struct PCB {
    int id;
    int rem;
    int priority;
    int arriveTime;
    pthread_t thread;
    schedule_t sche;
};
typedef struct SHM_Data {
    pthread_mutex_t mylock;
    pthread_cond_t mcond;
    pthread_cond_t cond_tids[P_NUM];
} SHM_Data;
struct Que{
    int head;
    int tail;
    int q[11];
};
int* sys_t;
struct PCB* pro;
struct Que* que;
SHM_Data *shm;

int create_shm(const char *path, int index, int size) {
    key_t key = ftok(path, index); // 生成key
    int shmid = shmget(key, size, IPC_CREAT | 0666); // 创建共享内存
    return shmid;
}

void my_sort(int pos) {
    vector<int> tem;
    int s = que[pos].head;
    int e = que[pos].tail;
    for (int i = s; i != e; i = (i + 1) % 11) {
        tem.push_back(que[pos].q[i]);
    }
    sort(tem.begin(), tem.end(), [](int aa, int bb) {
        if (pro[aa].arriveTime != pro[bb].arriveTime) return pro[aa].arriveTime < pro[bb].arriveTime;
        return pro[aa].id < pro[bb].id;
    });
    int p = 0;
    for (int i = s; i != e; i = (i + 1) % 11) {
        que[pos].q[i] = tem[p++];
    }
}

void init_shm(SHM_Data *shm) {
    pthread_mutexattr_t mattr; // 互斥锁属性
    pthread_condattr_t cattr; // 条件变量属性
    pthread_mutexattr_init(&mattr); // 初始化互斥锁属性
    pthread_condattr_init(&cattr); // 初始化条件变量属性
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED); // 设置互斥锁为进程间共享
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED); // 设置条件变量为进程间共享
    pthread_mutex_init(&shm->mylock, &mattr); // 初始化互斥锁
    pthread_cond_init(&shm->mcond, &cattr); // 初始化条件变量

    // pthread_condattr_t cattr1[P_NUM];
    for (int i = 0; i < P_NUM; ++i) {
        // pthread_condattr_init(cattr1 + i);
        pthread_cond_init(&shm->cond_tids[i], &cattr);
    }
}

void t_func(void* arg) {
    pthread_mutex_lock(&shm->mylock);
    PCB* pinfo = (PCB*) arg;
    while (true) {       
    pthread_cond_signal(&shm->mcond);

            
    pthread_cond_wait(&shm->cond_tids[pinfo->id], &shm->mylock);
        cout << getpid() << "\t\t\t" ;
        if(pinfo->sche == SCHED_OTHER)
            cout << "SCHED_OTHER";
        else if(pinfo->sche == SCHED_FIFO)
            cout << "SCHED_FIFO";
        else if(pinfo->sche == SCHED_RR)
            cout << "SCHED_RR";
        cout <<"\t\t" << pinfo->priority << "\t\t\t";
        int exeTime = 0;
        if (pinfo->sche == SCHED_OTHER || pinfo->sche == SCHED_RR) {
            while (exeTime < pinfo->rem && exeTime < TIME_SLICE) {
                for (int i = 2; i >= 0; --i) {
                    for (int j = que[i].head; j != que[i].tail; j = (j + 1) % 11) {
                        if (i > pinfo->priority && *sys_t == pro[que[i].q[j]].arriveTime) {
                            goto FINISH;
                        }
                    }
                }
                exeTime++;
                (*sys_t)++;
            }
        } else if(pinfo->sche == SCHED_FIFO){
            while (exeTime < pinfo->rem) {
                for (int i = 2; i >= 0; --i) {
                    for (int j = que[i].head; j != que[i].tail; j = (j + 1) % 11) {
                        if (i > pinfo->priority && *sys_t == pro[que[i].q[j]].arriveTime) {
                            goto FINISH;
                        }
                    }
                }
                exeTime++;
                (*sys_t)++;
            }
        }
FINISH:
        pinfo->rem -= exeTime;
        cout << exeTime << "\t\t" << pinfo->rem <<"\t\t\t";
        if ((pinfo->sche == SCHED_OTHER || pinfo->sche == SCHED_RR)) {
            if (pinfo->rem == 0) {
                cout << "Terminated" << endl;
            } else {
                if (exeTime == TIME_SLICE) {
                    cout << "Timeslice runs out" << endl;
                } else {
                    cout << "Preemted" << endl;
                }
            }
        } else {
            if (pinfo->rem == 0) {
                cout << "Terminated" << endl;
            } else {
                cout << "Preemted" << endl;
            }
        }
        
        pthread_cond_signal(&shm->mcond);
        if (pinfo->rem == 0) {
            break;
        }
    }

    pthread_mutex_unlock(&shm->mylock);
}

void c_func() {
    int i;
    pid_t pid;
    
    srand((unsigned int)time(0));
    que[0].head=0;
    que[1].head=0;
    que[2].head=0;
    que[0].tail=0;
    que[1].tail=0;
    que[2].tail=0;
    for (i = 0; i < P_NUM / 3; i++) {
        pid = fork();
        rand();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            pro[i] = {i, 1 + rand() % 10, 0, rand() % 140, 0, rand()%3};
            que[0].q[que[0].tail] = i;
            que[0].tail = (que[0].tail + 1) % 11;
            t_func(&pro[i]);
            exit(0);
        } 
        if (pid > 0) {
            pthread_cond_wait(&shm->mcond, &shm->mylock);
        }
    }
    for (i = P_NUM / 3; i < 2 * (P_NUM / 3); ++i) {
        pid = fork();
        rand();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            pro[i] = {i, 1 + rand() % 10, 10, rand() % 140, 0, 1+rand()%2};
            que[1].q[que[1].tail] = i;
            que[1].tail = (que[1].tail + 1) % 11;
            t_func(&pro[i]);
            
            exit(0);
        }
        if (pid > 0) {
            pthread_cond_wait(&shm->mcond, &shm->mylock);
        }
    }
    for (i = 2 * (P_NUM / 3); i < 3 * (P_NUM / 3); ++i) {
        pid = fork();
        rand();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            pro[i] = {i, 1 + rand() % 10, 20, rand() % 140, 0, 1+rand()%2};
            que[2].q[que[2].tail] = i;
            que[2].tail = (que[2].tail + 1) % 11;
            t_func(&pro[i]);
            exit(0);
        } 
        if (pid > 0) {
            pthread_cond_wait(&shm->mcond, &shm->mylock);
        }
    }
}
int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int shmid[6];
    shmid[0] = shmget(0, sizeof(int), IPC_CREAT | 0666);
    shmid[1] = shmget(1, sizeof(struct PCB)*P_NUM, IPC_CREAT | 0666);
    shmid[2] = shmget(2, sizeof(Que) * 3, IPC_CREAT | 0666);
    int shmId = create_shm(".", 1, sizeof(SHM_Data));

    sys_t = (int*) shmat(shmid[0], NULL, 0);
    pro = (struct PCB*) shmat(shmid[1], NULL, 0);
    que = (struct Que*) shmat(shmid[2], NULL, 0);
    shm = (SHM_Data *)shmat(shmId, NULL, 0); 
    init_shm(shm);

    pthread_mutex_lock(&shm->mylock);

    cout << "processId\t\tstrategy\t\tpriority\t\texeTime\t\tremainTime\t\tresult" << endl;

    c_func();

    for (int i = 0; i < 3; ++i) {
        my_sort(i);
    }

    int cnt = 0;
    while (true) {
        bool flag = false;
        for (int it = 2; it >= 0; it--) {
            for (int i = que[it].head; i != que[it].tail; i = (i + 1) % 11) {
                if (pro[que[it].q[i]].arriveTime <= *sys_t) {
                    flag = true;
                    pthread_cond_signal(&shm->cond_tids[que[it].q[i]]);
                    int pre_t = *sys_t;
                    pthread_cond_wait(&shm->mcond, &shm->mylock);
                    if (pro[que[it].q[i]].rem == 0) {
                        que[it].tail = que[it].tail > 0 ? que[it].tail - 1 : 10;
                        for (int j = i; j != que[it].tail; j = (j + 1) % 11) {
                            que[it].q[j] = que[it].q[(j + 1) % 11];
                        }
                        cnt++;
                        goto FINISH;
                    }

                    if (pro[que[it].q[i]].sche == SCHED_OTHER || pro[que[it].q[i]].sche == SCHED_RR) {
                        if (*sys_t - pre_t == TIME_SLICE) {
                            que[it].q[que[it].tail] = que[it].q[i];
                            for (int j = i; j != que[it].tail; j = (j + 1) % 11) {
                                que[it].q[j] = que[it].q[(j + 1) % 11];
                            }
                        }
                    } 
                    goto FINISH;
                }
            }
        }
FINISH:
        if (!flag) (*sys_t)++;
        if (cnt == P_NUM) break;
    }


    return 0;
}