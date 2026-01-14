#include "LogSinks.hpp"
#include "LogMessage.hpp"
#include <iostream>
#include <sstream>
#include <fcntl.h>

void ConsoleSinkImpl::write(const LogMessage& msg) 
{   
    std::cout << msg << std::endl;
}

FileSinkImpl::FileSinkImpl(const std::string& path) : filePath(path) 
{
    safeFile = std::make_unique<SafeFile>(path, O_RDWR | O_CREAT | O_APPEND);
}

void FileSinkImpl::write(const LogMessage& msg) 
{
    std::stringstream ss;
    ss << msg << "\n";
    if (safeFile) {
        safeFile->write(ss.str());
    }
}

SocketSinkImpl::SocketSinkImpl(const std::string& path) : socketPath(path)
{
    try {
        safeSocket.connect(socketPath);
    } catch (const std::exception& e) {
        std::cerr << "Socket Sink Connection Warning: " << e.what() << std::endl;
    }
}

void SocketSinkImpl::write(const LogMessage& msg)
{
    std::stringstream ss;
    ss << msg << "\n";
    try {
        safeSocket.send(ss.str());
    } catch (const std::exception& e) {
        std::cerr << "Socket Sink Error: " << e.what() << std::endl;
    }
}