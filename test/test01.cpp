#include <sys/msg.h>
#include <iostream>
#include <unistd.h>

struct Msg {
    long type;
    int x;
};

int main() {
    int msgqid = msgget(90, IPC_CREAT | 0666);
    Msg msg;
    msg.x = 0;
    while (true) {
        msgsnd(msgqid, &msg, sizeof(int), 0);
        std::cout << "send x = " << msg.x << std::endl;
        msg.x++;
        sleep(1);
    }
    
    return 0;
}