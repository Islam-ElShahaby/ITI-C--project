#include "LogSinks.hpp"
#include "LogMessage.hpp"
#include <iostream>
#include <fstream>

void ConsoleSinkImpl::write(const LogMessage& msg) 
{   
    std::cout << msg << std::endl;
}

FileSinkImpl::FileSinkImpl(const std::string& path) : filePath(path) {}

void FileSinkImpl::write(const LogMessage& msg) 
{
    std::ofstream file(filePath, std::ios::app);
    if (file.is_open()) 
    {
        file << msg << "\n";
    }
}