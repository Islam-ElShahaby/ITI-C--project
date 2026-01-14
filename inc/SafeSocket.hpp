#pragma once
#include <string>

class SafeSocket
{
private:
    int fd;
public:
    SafeSocket(); 
    ~SafeSocket();

    SafeSocket(SafeSocket&& other);
    SafeSocket& operator=(SafeSocket&& other);

    // Delete Copy (Rule of 5)
    SafeSocket(const SafeSocket&) = delete;
    SafeSocket& operator=(const SafeSocket&) = delete;

    void connect(const std::string& socketPath);
    void send(const std::string& data);
    void receive(std::string& data);
};