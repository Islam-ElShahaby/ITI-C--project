#include "SafeFile.hpp"
#include <stdexcept>
#include <fcntl.h>
#include <vector>

SafeFile::SafeFile(const std::string& path, int flags) {
    fd = ::open(path.c_str(), flags, 0666);
    if (fd == -1) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    this->path = path;
}
    
SafeFile::~SafeFile() {
    if (fd != -1) {
        ::close(fd);
    }
}

SafeFile::SafeFile(SafeFile&& other) {
    fd = other.fd;
    path = std::move(other.path);
    other.fd = -1;
}

SafeFile& SafeFile::operator=(SafeFile&& other) {
    if (this != &other) {
        if (fd != -1) {
            ::close(fd);
        }
        fd = other.fd;
        path = std::move(other.path);
        other.fd = -1;
    }
    return *this;
}

void SafeFile::write(const std::string& data) {
    if (::write(fd, data.c_str(), data.size()) == -1) {
        throw std::runtime_error("Failed to write to file: " + path);
    }
}

void SafeFile::read(std::string& data, size_t size) {
    std::vector<char> buffer(size);
    ssize_t bytes_read = ::read(fd, buffer.data(), size);
    if (bytes_read == -1) {
        throw std::runtime_error("Failed to read from file: " + path);
    }
    data.assign(buffer.data(), bytes_read);
}
