#include <signal.h>  
#include <unistd.h>
#include <iostream>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/wait.h>

void signalHandler(int signum, siginfo_t *info, void *context) {
  pid_t pid = info->si_pid;
  std::cout << "Received signal " << signum   
            << " from child " << pid << "\n";
}

void log_info(const std::string& s) {
    auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[1024];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&tm));
    std::ofstream file("log.txt", std::ios::app);
    file << buf << "\t\t" << s << std::endl;
    file.close();
}

int main() {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        for (int i = 0; i < 10; ++i) {
            log_info("child");
            sleep(1);
        }
    } else if (pid > 0) {
        while (true) {
            int status;
            int ret = waitpid(pid, &status, WNOHANG);
            if (ret == 0) {
                log_info("child is running");
            } else {
                log_info("child is end");
            }
            sleep(1);
        }
    }
    return 0;
}