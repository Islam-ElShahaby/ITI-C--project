#include "SafeSocket.hpp"
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <cstring>

SafeSocket::SafeSocket() {
    fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    
    if (fd == -1) {
        throw std::runtime_error("Failed to create socket.");
    }
}

SafeSocket::~SafeSocket() {
    if (fd != -1) {
        ::close(fd);
    }
}

SafeSocket::SafeSocket(SafeSocket&& other) {
    fd = other.fd;
    other.fd = -1;
}

SafeSocket& SafeSocket::operator=(SafeSocket&& other) {
    if (this != &other) {
        if (fd != -1) {
            ::close(fd);
        }
        fd = other.fd;
        other.fd = -1;
    }
    return *this;
}

void SafeSocket::connect(const std::string& socketPath) {
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        throw std::runtime_error("Failed to connect to UDS path: " + socketPath);
    }
}

void SafeSocket::send(const std::string& data) {
    if (::write(fd, data.c_str(), data.size()) == -1) {
        throw std::runtime_error("Failed to send data.");
    }
}

void SafeSocket::receive(std::string& data) {
    std::vector<char> buffer(1024);
    ssize_t bytes = ::read(fd, buffer.data(), buffer.size());
    
    if (bytes == -1) {
        throw std::runtime_error("Failed to receive data.");
    }
    data.assign(buffer.data(), bytes);
}