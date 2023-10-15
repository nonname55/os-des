#include "util.h"

void log_info(const std::string& s) {
    auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[1024];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&tm));
    std::ofstream file("log.txt", std::ios::app);
    file << buf << "\t\t" << s << std::endl;
    file.close();
}

int random_int(int l, int h) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(l, h);
    return distrib(gen);
}

void getPath(std::string& s) {
    s = std::filesystem::current_path();
    size_t pos = s.rfind("/");
    if (s.back() == 'n' && pos != std::string::npos) {
        s.erase(pos + 1);
    }
    if (s.back() != '/') {
        s.push_back('/');
    }
}