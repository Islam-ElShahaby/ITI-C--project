/**
 * @file service_main.cpp
 * @brief GPU Usage Data Service Application
 * 
 * This application runs the GpuUsageData service that responds to
 * client requests and broadcasts GPU usage changes.
 */

#include "GpuUsageService.hpp"
#include <CommonAPI/CommonAPI.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running(true);

void signalHandler(int signal) {
    std::cout << "\n[GpuService] Received signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Set up signal handler for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=====================================" << std::endl;
    std::cout << " GPU Usage Data Service (vSOME/IP)" << std::endl;
    std::cout << "=====================================" << std::endl;

    try {
        // Get CommonAPI runtime
        auto runtime = CommonAPI::Runtime::get();
        if (!runtime) {
            std::cerr << "[GpuService] Failed to get CommonAPI runtime" << std::endl;
            return 1;
        }

        // Create service instance
        auto gpuService = std::make_shared<service::GpuUsageService>();

        // Register the service
        // Domain: "local", Instance: "omnimetron.gpu.GpuUsageData"
        const std::string domain = "local";
        const std::string instance = "omnimetron.gpu.GpuUsageData";

        if (!runtime->registerService(domain, instance, gpuService)) {
            std::cerr << "[GpuService] Failed to register service" << std::endl;
            return 1;
        }

        std::cout << "[GpuService] Service registered successfully" << std::endl;
        std::cout << "[GpuService] Domain: " << domain << std::endl;
        std::cout << "[GpuService] Instance: " << instance << std::endl;
        std::cout << "[GpuService] Waiting for client requests..." << std::endl;
        std::cout << std::endl;

        // Main loop: periodically broadcast GPU usage updates
        int broadcastIntervalMs = 2000;  // 2 seconds
        auto lastBroadcast = std::chrono::steady_clock::now();

        while (g_running) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBroadcast).count();

            if (elapsed >= broadcastIntervalMs) {
                // Get current GPU usage and broadcast
                float usage = gpuService->getCurrentGpuUsage();
                gpuService->broadcastGpuUsage(usage);
                lastBroadcast = now;
            }

            // Small sleep to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Cleanup
        runtime->unregisterService(domain, v1::omnimetron::gpu::GpuUsageData::getInterface(), instance);
        std::cout << "[GpuService] Service unregistered" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[GpuService] Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[GpuService] Shutdown complete" << std::endl;
    return 0;
}
