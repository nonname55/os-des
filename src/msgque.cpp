#include "header.h"
#include "macro.h"

int create_msgque(int svkey) {
    key_t key = svkey;
    int msgqid = msgget(key, IPC_CREAT | 0666);
    if (msgqid == -1) {
        print(getpid() << " error create message que, svkey = " << svkey << std::endl);
        exit(-1);
    }
    return msgqid;
}

