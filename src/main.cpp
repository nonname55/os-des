#include "header.h"
#include "macro.h"
#include "msgque.h"
#include "graph.h"
#include "process.h"
#include "order.h"
#include "rider.h"
#include "restaurant.h"
#include "user.h"
#include "file.h"

int main() 
{
    getPath(workdir);
    logPath = workdir + "output/log.txt";

    pid_t pid = fork();
    if (pid > 0) {
        exit(0);
    }
    setsid();
    umask(022);
    std::string outputDir = workdir + "output";
    const std::filesystem::path path = std::filesystem::path(outputDir);
    if (!(std::filesystem::exists(path) && std::filesystem::is_directory(path))) {
        mkdir(outputDir.c_str(), 0777);
    }
    chdir(outputDir.c_str());
    // int fd = open("/dev/null", O_RDWR);
    outputDir += "/output.txt";
    int fd = open(outputDir.c_str(), O_RDWR | O_TRUNC);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    int shmid[10];
    shmid[0] = shmget(0, sizeof(struct Process) * RIDER_NUM, IPC_CREAT | 0666);
    shmid[1] = shmget(1, sizeof(struct Process) * USER_NUM, IPC_CREAT | 0666);
    shmid[2] = shmget(2, sizeof(struct Process) * RESTAURANT_NUM, IPC_CREAT | 0666);
    shmid[3] = shmget(3, sizeof(Que), IPC_CREAT | 0666);
    shmid[4] = shmget(4, sizeof(Que), IPC_CREAT | 0666);
    shmid[5] = shmget(5, sizeof(Que), IPC_CREAT | 0666);
    shmid[6] = shmget(6, sizeof(int), IPC_CREAT | 0666);
    shmid[7] = shmget(7, sizeof(int), IPC_CREAT | 0666);
    int shmId = create_shm(".", 1, sizeof(SHM_Data));
    riderPro = (struct Process*)shmat(shmid[0], NULL, 0);
    userPro = (struct Process*)shmat(shmid[1], NULL, 0);
    restPro = (struct Process*)shmat(shmid[2], NULL, 0);
    riderQue = (struct Que*)shmat(shmid[3], NULL, 0);
    userQue = (struct Que*)shmat(shmid[4], NULL, 0);
    restQue = (struct Que*)shmat(shmid[5], NULL, 0);
    order_count = (int*)shmat(shmid[6], NULL, 0);
    system_time = (int*)shmat(shmid[7], NULL, 0);
    shm = (SHM_Data*)shmat(shmId, NULL, 0);
    init_shm(shm);

    *order_count = 0;
    *system_time = 0;

    MQ::delete_mq(INFO_DESC_SVKEY);
    MQ::delete_mq(USER_TO_REST);
    MQ::delete_mq(REST_TO_RIDER);
    MQ::delete_mq(RIDER_INFO_BACK_SVKEY);
    MQ::delete_mq(RIDER_INFO_FRONT_SVKEY);

    pid_t scheRider = fork();
    if (scheRider == 0) {
        pthread_mutex_lock(&shm->riderLock);
        
        setsid();
        create_rider_process(1, RIDER_NUM);
        while (true) {
            scheduleRider();
            sleep(2);
        }

        pthread_mutex_unlock(&shm->riderLock);
    } else if (scheRider < 0) {
        print("schedule rider fork error");
    }

    pid_t scheUser = fork();
    if (scheUser == 0) {
        pthread_mutex_lock(&shm->userLock);
        
        setsid();
        create_user_process(1, USER_NUM);
        while (true) {
            scheduleUser();
            sleep(5);
        }

        pthread_mutex_unlock(&shm->userLock);
    } else if (scheUser < 0) {
        print("schedule user fork error");
    }

    pid_t scheRest = fork();
    if (scheRest == 0) {
        pthread_mutex_lock(&shm->restLock);
        
        setsid();
        create_restaurant_process(1, RESTAURANT_NUM);
        while (true) {
            scheduleRest();
            sleep(2);
        }

        pthread_mutex_unlock(&shm->restLock);
    } else if (scheRest < 0) {
        print("schedule restaurant fork error");
    }

    while (true) {
        check_processes_status(scheRider);
        check_processes_status(scheUser);
        check_processes_status(scheRest);
        sleep(1);
        ++(*system_time);
    }

    while (true) {
        check_processes_status(scheRider);
        check_processes_status(scheUser);
        check_processes_status(scheRest);
        sleep(1);
        ++(*system_time);
    }

    return 0;
}



