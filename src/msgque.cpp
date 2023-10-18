#include "header.h"
#include "macro.h"

int create_msgque(int svkey) 
{
    key_t key = svkey;
    int msgqid = msgget(key, IPC_CREAT | 0666);
    if (msgqid == -1) {
        print(getpid() << " error create message que, svkey = " << svkey << std::endl);
        exit(-1);
    }
    return msgqid;
}

bool hasMessage(int svkey) 
{
    int msgqid = create_msgque(svkey);
    struct msqid_ds buf;
    if (msgctl(msgqid, IPC_STAT, &buf) == -1) {
        std::cerr << "Failed to get message queue status" << std::endl;
        return false;
    }
    return buf.msg_qnum > 0;
}

