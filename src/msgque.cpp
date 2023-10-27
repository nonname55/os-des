#include "../include/header.h"
#include "../include/macro.h"
#include "../include/msgque.h"

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

bool MQ::hasMessage(int svkey) 
{
    int msgqid = create(svkey);
    struct msqid_ds buf;
    if (msgctl(msgqid, IPC_STAT, &buf) == -1) {
        std::cerr << "Failed to get message queue status" << std::endl;
        return false;
    }
    return buf.msg_qnum > 0;
}

void MQ::delete_mq(int svkey)
{
    msgctl(msgget(svkey, 0666), IPC_RMID, NULL);
}