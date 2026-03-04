#include "GpuUsageService.hpp"
#include "CpuUsageService.hpp"
#include "MemoryUsageService.hpp"
#include "CpuTempService.hpp"
#include <CommonAPI/CommonAPI.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <signal.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    sigset_t waitset;
    sigemptyset(&waitset);
    sigaddset(&waitset, SIGINT);
    sigaddset(&waitset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &waitset, nullptr);

    std::cout << "===========================================" << std::endl;
    std::cout << " Telemetry Service (vSOME/IP)" << std::endl;
    std::cout << " CPU + Memory + GPU + CPU Temp" << std::endl;
    std::cout << "===========================================" << std::endl;

    try {
        auto runtime = CommonAPI::Runtime::get();
        if (!runtime) {
            std::cerr << "[TelemetryService] Failed to get CommonAPI runtime" << std::endl;
            return 1;
        }

        const std::string domain = "local";

        // --- Register GPU service ---
        auto gpuService = std::make_shared<service::GpuUsageService>();
        const std::string gpuInstance = "omnimetron.gpu.GpuUsageData";
        if (!runtime->registerService(domain, gpuInstance, gpuService)) {
            std::cerr << "[TelemetryService] Failed to register GPU service" << std::endl;
            return 1;
        }
        std::cout << "[TelemetryService] GPU service registered: " << gpuInstance << std::endl;

        // --- Register CPU service ---
        auto cpuService = std::make_shared<service::CpuUsageService>();
        const std::string cpuInstance = "omnimetron.cpu.CpuUsageData";
        if (!runtime->registerService(domain, cpuInstance, cpuService)) {
            std::cerr << "[TelemetryService] Failed to register CPU service" << std::endl;
            return 1;
        }
        std::cout << "[TelemetryService] CPU service registered: " << cpuInstance << std::endl;

        // --- Register Memory service ---
        auto memService = std::make_shared<service::MemoryUsageService>();
        const std::string memInstance = "omnimetron.memory.MemoryUsageData";
        if (!runtime->registerService(domain, memInstance, memService)) {
            std::cerr << "[TelemetryService] Failed to register Memory service" << std::endl;
            return 1;
        }
        std::cout << "[TelemetryService] Memory service registered: " << memInstance << std::endl;

        // --- Register CPU Temperature service ---
        auto cpuTempService = std::make_shared<service::CpuTempService>();
        const std::string cpuTempInstance = "omnimetron.cpu_temp.CpuTempData";
        if (!runtime->registerService(domain, cpuTempInstance, cpuTempService)) {
            std::cerr << "[TelemetryService] Failed to register CPU Temperature service" << std::endl;
            return 1;
        }
        std::cout << "[TelemetryService] CPU Temperature service registered: " << cpuTempInstance << std::endl;

        std::cout << std::endl;
        std::cout << "[TelemetryService] All services running. Waiting for client requests..." << std::endl;
        std::cout << "[TelemetryService] (Press Ctrl+C to stop)" << std::endl;

        // Wait for SIGINT or SIGTERM using sigwait (signal-safe, no UB)
        int sig = 0;
        sigwait(&waitset, &sig);
        std::cout << "\n[TelemetryService] Received signal " << sig << ", shutting down..." << std::endl;

        // Unregister all services
        runtime->unregisterService(domain, v1::omnimetron::gpu::GpuUsageData::getInterface(), gpuInstance);
        runtime->unregisterService(domain, v1::omnimetron::cpu::CpuUsageData::getInterface(), cpuInstance);
        runtime->unregisterService(domain, v1::omnimetron::memory::MemoryUsageData::getInterface(), memInstance);
        runtime->unregisterService(domain, v1::omnimetron::cpu_temp::CpuTempData::getInterface(), cpuTempInstance);
        std::cout << "[TelemetryService] All services unregistered" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[TelemetryService] Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[TelemetryService] Shutdown complete" << std::endl;
    return 0;
}
