#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <sys/wait.h>

void log_info(const std::string& s){
    auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[1024];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&tm));
    std::ofstream file("log.txt", std::ios::app);
    file << buf << "\t\t" << s << std::endl;
    file.close();
}

void create_rider_process() {
    pid_t pid = fork();
    if (pid == 0) {
        log_info("rider " + std::to_string(getpid()));
        exit(0);
    } else if (pid > 0) {
        
    }
}

int main() {
    pid_t pid = fork();
    if (pid > 0) {
        exit(0);
    }
    setsid();
    umask(022);
    std::string folder = std::string(getenv("HOME")).append("/OS-Des");
    const std::filesystem::path path = std::filesystem::path(folder);
    if (!(std::filesystem::exists(path) && std::filesystem::is_directory(path))) {
        int re = mkdir(folder.c_str(), 0777);
        if (re == 0) {
            std::cout << "创建程序主目录成功" << std::endl;
        } else {
            std::cout << "创建程序主目录失败" << std::endl;
        }
    }
    chdir(folder.c_str());
    int fd = open("/dev/null", O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    pid_t child_pid = fork();
    if (child_pid > 0) {
        int status;
        pid_t spid;
        while (true) {
            spid = waitpid(child_pid, &status, WNOHANG);
            if (spid == 0) {
                log_info(std::string("子进程正在运行, pid: "));
            } else {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    log_info("子进程已经结束");
                }
            }
            sleep(1);
        }
    } else if (child_pid == 0) {
        log_info(std::string("子进程启动:").append(std::to_string(getpid())));
        sleep(10);
        exit(0);
    } else {
        log_info("fork error");
        exit(-1);
    }

    return 0;
}