#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <deque>
#include <random>
#include <algorithm>

int random_int(int l, int h) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(l, h);
    return distrib(gen);
}

namespace PCB {
    void child_process();
    void create_process();
    void schedule();

    struct Process {
        int pid;
        int priority;

        Process() {}
        Process(
            int _pid,
            int _priority
        ) {
            pid = _pid;
            priority = _priority;
        }
    };
    std::deque<Process> processQue;

    void child_process() {
        std::cout << "child start" << std::endl;
        kill(getpid(), SIGSTOP);
        std::cout << "child wake up my pid:" << getpid() << std::endl;
    }

    void create_process() {
        pid_t pid = fork();
        if (pid == 0) {
            child_process();
            exit(0);
        } else if (pid > 0) {
            int status;
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFSTOPPED(status));
            processQue.emplace_back(pid, random_int(1, 10));
        }
    }

    void schedule() {
        std::sort(processQue.begin(), processQue.end(), [](auto p1, auto p2) {
            return p1.priority > p2.priority;
        });
        for (const auto& x : processQue) {
            std::cout << x.pid << ' ' << x.priority << std::endl;
        }
        for (const auto& x : processQue) {
            kill(x.pid, SIGCONT);
        }
    }
}

int main() {
    for (int i = 0; i < 10; ++i) {
        PCB::create_process();
    }
    PCB::schedule();
    
    return 0;
}