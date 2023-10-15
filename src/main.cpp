#include "header.h"
#include "macro.h"
#include "msgque.h"
#include "graph.h"
#include "process.h"
#include "order.h"
#include "rider.h"
#include "restaurant.h"
#include "user.h"

int main() {
    std::string workdir;
    getPath(workdir);

    pid_t pid = fork();
    if (pid > 0) {
        exit(0);
    }
    setsid();
    umask(022);
    std::string outputDir = workdir + "output";
    const std::filesystem::path path = std::filesystem::path(outputDir);
    if (!(std::filesystem::exists(path) && std::filesystem::is_directory(path))) {
        int re = mkdir(outputDir.c_str(), 0777);
    }
    chdir(outputDir.c_str());
    // int fd = open("/dev/null", O_RDWR);
    outputDir += "/output.txt";
    int fd = open(outputDir.c_str(), O_RDWR | O_TRUNC);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    int shmid[6];
    shmid[0] = shmget(0, sizeof(struct Process) * P_NUM, IPC_CREAT | 0666);
    shmid[1] = shmget(1, sizeof(Que), IPC_CREAT | 0666);
    shmid[2] = shmget(2, sizeof(int), IPC_CREAT | 0666);
    int shmId = create_shm(".", 1, sizeof(SHM_Data));
    pro = (struct Process*)shmat(shmid[0], NULL, 0);
    que = (struct Que*)shmat(shmid[1], NULL, 0);
    system_time = (int*)shmat(shmid[2], NULL, 0);
    shm = (SHM_Data*)shmat(shmId, NULL, 0);
    init_shm(shm);

    pid_t sche_pid = fork();
    if (sche_pid == 0) {
        pthread_mutex_lock(&shm->mylock);
        msgctl(msgget(SVKEY1, 0666), IPC_RMID, NULL);
        msgctl(msgget(SVKEY2, 0666), IPC_RMID, NULL);
        msgctl(msgget(SVKEY3, 0666), IPC_RMID, NULL);
        msgctl(msgget(SVKEY4, 0666), IPC_RMID, NULL);
        msgctl(msgget(SVKEY5, 0666), IPC_RMID, NULL);
        msgctl(msgget(SVKEY6, 0666), IPC_RMID, NULL);
        msgctl(msgget(SVKEY7, 0666), IPC_RMID, NULL);
        create_process();
        while (true) {
            schedule();
            sleep(1);
        }

        pthread_mutex_unlock(&shm->mylock);
    } else if (sche_pid > 0) {
        while (true) {
            check_processes_status(sche_pid);
            sleep(2);
        }
    } else {
        print("schedule fork error");
    }

    return 0;
}



