#ifndef MSGQUE_H
#define MSGQUE_H

#include "macro.h"
#include "header.h"

#define SVKEY1 75
#define SVKEY2 76
#define SVKEY3 77
#define SVKEY4 78
#define SVKEY5 79
#define SVKEY6 80
#define SVKEY7 81
#define MQSIZ 7 * sizeof(int)

struct orderMsg {
    long msg_type;
    int tarx, tary, userId, orderId, buildingId, restaurantId, riderID;
};

int create_msgque(int svkey);
template<typename T>
void write_msgque(int msgqid, T& msg, int msgsiz) {
    int ret = msgsnd(msgqid , &msg, msgsiz, 0);
    if (ret == -1) {
        print(getpid() << " write msg que error at msgqid " << msgqid << std::endl);
        exit(-1);
    }
}
template<typename T>
int read_msgque(int msgqid, T& msg, int msgsiz, int msgtype) {
    int ret = msgrcv(msgqid, &msg, msgsiz, msgtype,IPC_NOWAIT);
    if (ret == -1) {
        // print("read msg que error at msg qid " << msgqid << std::endl);
    }
    return ret;
}

#endif