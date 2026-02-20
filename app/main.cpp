#include "Application.hpp"
#include <iostream>
#include <csignal>
#include <atomic>

Application* g_app = nullptr;

void signalHandler(int signal) {
    std::cout << "\n[Main] Received signal " << signal << ", shutting down..." << std::endl;
    if (g_app) {
        g_app->stop();
    }
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        Application app("config/app_config.json");
        g_app = &app;
        
        app.start();

    } catch (const std::exception& e) {
        std::cerr << "Application Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[Main] Shutdown complete" << std::endl;
    return 0;
}