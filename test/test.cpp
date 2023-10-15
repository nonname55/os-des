#include <filesystem>
#include <string>
#include <iostream>

int main() {
    std::string path = std::filesystem::current_path();
    std::cout << path << std::endl;
    
    return 0;
}