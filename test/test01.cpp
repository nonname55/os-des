#include <iostream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct Msg {
    long type;
    int x;
};

int create_msgque(int svkey) {
    key_t key = svkey;
    int msgqid = msgget(key, IPC_CREAT | 0666);
    if (msgqid == -1) {
        std:: cerr << " error create message que, svkey = " << svkey << std::endl;
        exit(-1);
    }
    return msgqid;
}

bool hasMessage(int svkey) {
    int msgqid = create_msgque(svkey);
    struct msqid_ds buf;
    if (msgctl(msgqid, IPC_STAT, &buf) == -1) {
        std::cerr << "Failed to get message queue status" << std::endl;
        return false;
    }
    return buf.msg_qnum > 0;
}

int main() {
    if (hasMessage(88)) {
        std::cout << "yes\n";
    } else {
        std::cout << "no\n";
    }
    Msg msg;
    msg.type = 1;
    msg.x = 1;
    int msgqid = create_msgque(88);
    msgsnd(msgqid, &msg, sizeof(int), 0);

    if (hasMessage(88)) {
        std::cout << "yes\n";
    } else {
        std::cout << "no\n";
    }
    
    return 0;
}