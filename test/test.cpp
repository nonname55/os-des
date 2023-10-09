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

#define SVKEY1 75
#define SVKEY2 76

struct orderMsg{
    long msg_type;
    int tarx, tary;
};

int main() {
    pid_t pid1 = fork();
    if (pid1 == 0) {
        key_t key = SVKEY1;
        int msgqid = msgget(key, IPC_CREAT | 0666);
        struct orderMsg reqmsg;
        reqmsg.msg_type = 1;
        reqmsg.tarx = 2;
        reqmsg.tary = 3;
        msgsnd(msgqid , &reqmsg, 2 * sizeof(int), 0);
        sleep(10);
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        key_t key = SVKEY1;
        int msgqid = msgget(key, IPC_CREAT | 0666);
        struct orderMsg reqmsg;
        msgrcv(msgqid, &reqmsg, 2 * sizeof(int), 1, 0);
        std::cout << reqmsg.tarx << ' ' << reqmsg.tary << std::endl;
        sleep(10);
        exit(0);
    }
    
    return 0;
}