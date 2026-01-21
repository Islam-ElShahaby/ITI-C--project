#pragma once
#include <unistd.h>
#include <string>
#include <fcntl.h>

class SafeFile
{
private:
    int fd;
    std::string path;
public:
    SafeFile(const std::string& path, int flags = O_RDWR | O_CREAT);
    ~SafeFile();

    SafeFile(SafeFile&& other);
    SafeFile& operator=(SafeFile&& other);

    SafeFile(const SafeFile&) = delete;
    SafeFile& operator=(const SafeFile&) = delete;

    void write(const std::string& data);
    void read(std::string& data, size_t size);
};
