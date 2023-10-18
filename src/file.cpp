#include "file.h"

std::string workdir;
std::string logPath;

bool IsFile(const std::string& filePath) 
{
    struct stat st;
    if (0 == stat(filePath.c_str(), &st)) {
        if (st.st_mode & S_IFDIR) {
            return false; // 目录
        } else if (st.st_mode & S_IFREG) {
            return true; // 文件
        }
    }
    return false;
}

bool ReadFile(const std::string &filePath, std::vector<std::string> &data, bool lock) 
{
    std::string fileData;
    static constexpr long bufSize = 4096;
    if (!IsFile(filePath)) {
        return false;
    }
    FILE *pFile;
    if ((pFile = fopen(filePath.c_str(), "r")) == NULL) {
        return false;
    }
 
    // 共享锁/不阻塞
    if (lock && flock(fileno(pFile), LOCK_SH | LOCK_NB) != 0) {
        fclose(pFile);
        return false;
    }
 
    // 计算文件大小
    fseek(pFile, 0, SEEK_SET);
    long begin = ftell(pFile);
    fseek(pFile, 0, SEEK_END);
    long end = ftell(pFile);
    long fileSize = end - begin;
    fseek(pFile, 0, SEEK_SET); //重新指向文件头
 
    // 预分配内存空间
    fileData.reserve(fileSize + 1);
 
    // 读取文件内容
    char readBuf[bufSize + 1];
    long readSize = 0;
    while (readSize < fileSize) {
        long minRead = std::min(fileSize - readSize, bufSize);
        long len = fread(readBuf, 1, minRead, pFile);
        readSize += len;
        fileData.append(readBuf, len);
    }

    splitString(data, fileData, '\n');
 
    // 解锁
    if (lock && flock(fileno(pFile), LOCK_UN) != 0) {
        fclose(pFile);
        return false;
    }
 
    fclose(pFile);
    return true;
}

bool WriteFile(const std::string &filePath, const std::string &fileData, bool lock) 
{
    FILE *pFile;
    if ((pFile = fopen(filePath.c_str(), "a+")) == NULL) {
        return false;
    }
 
    // 互斥锁/不阻塞
    if (lock && flock(fileno(pFile), LOCK_EX) != 0) {
        fclose(pFile);
        return false;
    }
 
    fwrite(fileData.c_str(), 1, fileData.length(), pFile);
    fwrite("\n", 1, 1, pFile);
 
    // 解锁
    if (lock && flock(fileno(pFile), LOCK_UN) != 0) {
        fclose(pFile);
        return false;
    }
 
    fclose(pFile);
    return true;
}

void splitString(std::vector<std::string> &words, const std::string& str, char delim) 
{
    std::stringstream ss(str);
    std::string word;
    while (std::getline(ss, word, delim)) {
        words.push_back(word);
    }
}