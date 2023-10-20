#include <sys/msg.h>
#include <random>
#include <unistd.h>

struct rider_info_struct {
    long type;
    int event_type;
    int rider_x, rider_y;
} __attribute__((packed));

int random_int(int l, int h) 
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(l, h);
    return distrib(gen);
}

int main() {
    

    int msqid = msgget(81, IPC_CREAT | 0666);
    while (true) {
        rider_info_struct ri;
        ri = {random_int(1, 2), 1, 0, 0};
        msgsnd(msqid, &ri, 12, 0);
        sleep(1);
    }
    
    return 0;
}