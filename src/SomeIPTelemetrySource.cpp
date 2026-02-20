#include "SomeIPTelemetrySource.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

namespace telemetry {

// Static members — these must be defined here so the linker can find them
std::unique_ptr<SomeIPTelemetrySourceImpl, SomeIPTelemetrySourceImpl::Deleter> SomeIPTelemetrySourceImpl::s_instance = nullptr;
std::once_flag SomeIPTelemetrySourceImpl::s_onceFlag;

// --- SomeIPTelemetrySourceImpl (Singleton) ---

SomeIPTelemetrySourceImpl& SomeIPTelemetrySourceImpl::getInstance() {
    std::call_once(s_onceFlag, []() {
        s_instance.reset(new SomeIPTelemetrySourceImpl());
    });
    return *s_instance;
}

SomeIPTelemetrySourceImpl::SomeIPTelemetrySourceImpl()
    : m_runtime(nullptr)
    , m_proxy(nullptr)
    , m_subscribed(false)
    , m_initialized(false)
{
}

SomeIPTelemetrySourceImpl::~SomeIPTelemetrySourceImpl() {
    shutdown();
}

bool SomeIPTelemetrySourceImpl::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true;
    }

    try {
        // Load the CommonAPI runtime, which bootstraps the SOME/IP middleware
        m_runtime = CommonAPI::Runtime::get();
        if (!m_runtime) {
            std::cerr << "[SomeIPTelemetry] Failed to get CommonAPI runtime" << std::endl;
            return false;
        }

        // Build the generated proxy for the GpuUsageData service
        m_proxy = m_runtime->buildProxy<v1::omnimetron::gpu::GpuUsageDataProxy>(
            "local",
            "omnimetron.gpu.GpuUsageData"
        );

        if (!m_proxy) {
            std::cerr << "[SomeIPTelemetry] Failed to build proxy" << std::endl;
            return false;
        }

        m_initialized = true;
        std::cout << "[SomeIPTelemetry] Client initialized successfully" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[SomeIPTelemetry] Initialization error: " << e.what() << std::endl;
        return false;
    }
}

bool SomeIPTelemetrySourceImpl::isAvailable() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_proxy && m_proxy->isAvailable();
}

bool SomeIPTelemetrySourceImpl::waitForAvailability(uint32_t timeoutMs) {
    if (!m_initialized) {
        if (!const_cast<SomeIPTelemetrySourceImpl*>(this)->initialize()) {
            return false;
        }
    }

    if (!m_proxy) {
        return false;
    }

    // Quick path: no need to poll if the service is already up
    if (m_proxy->isAvailable()) {
        return true;
    }

    // Poll in 100ms increments until the service appears or we hit the timeout
    auto startTime = std::chrono::steady_clock::now();
    uint32_t elapsedMs = 0;

    while (elapsedMs < timeoutMs || timeoutMs == 0) {
        if (m_proxy->isAvailable()) {
            std::cout << "[SomeIPTelemetry] Service became available after " 
                      << elapsedMs << "ms" << std::endl;
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto now = std::chrono::steady_clock::now();
        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    }

    std::cerr << "[SomeIPTelemetry] Timeout waiting for service availability" << std::endl;
    return false;
}

bool SomeIPTelemetrySourceImpl::requestGpuUsage(float& usage) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_proxy || !m_proxy->isAvailable()) {
        std::cerr << "[SomeIPTelemetry] Service not available for request" << std::endl;
        return false;
    }

    try {
        CommonAPI::CallStatus callStatus;
        float receivedUsage = 0.0f;

        // Block until the reply comes back from the service
        m_proxy->requestGpuUsageData(callStatus, receivedUsage);

        if (callStatus == CommonAPI::CallStatus::SUCCESS) {
            usage = receivedUsage;
            return true;
        } else {
            std::cerr << "[SomeIPTelemetry] Request failed with status: " 
                      << static_cast<int>(callStatus) << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        std::cerr << "[SomeIPTelemetry] Request error: " << e.what() << std::endl;
        return false;
    }
}

void SomeIPTelemetrySourceImpl::requestGpuUsageAsync(std::function<void(bool success, float usage)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_proxy || !m_proxy->isAvailable()) {
        if (callback) {
            callback(false, 0.0f);
        }
        return;
    }

    try {
        // Fire the request and let CommonAPI call our lambda when the reply arrives
        m_proxy->requestGpuUsageDataAsync(
            [callback](const CommonAPI::CallStatus& status, const float& usage) {
                if (callback) {
                    callback(status == CommonAPI::CallStatus::SUCCESS, usage);
                }
            }
        );
    } catch (const std::exception& e) {
        std::cerr << "[SomeIPTelemetry] Async request error: " << e.what() << std::endl;
        if (callback) {
            callback(false, 0.0f);
        }
    }
}

void SomeIPTelemetrySourceImpl::subscribeToEvents(std::function<void(float usage)> handler) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_proxy) {
        std::cerr << "[SomeIPTelemetry] Cannot subscribe: proxy not initialized" << std::endl;
        return;
    }

    if (m_subscribed) {
        // Clean up the old subscription before registering a new one
        unsubscribeFromEvents();
    }

    try {
        // Hook into the broadcast event that the service fires whenever the GPU usage changes
        m_eventSubscription = m_proxy->getNotifyGpuUsageDataChangeEvent().subscribe(
            [handler](const float& usage) {
                if (handler) {
                    handler(usage);
                }
            }
        );
        m_subscribed = true;
        std::cout << "[SomeIPTelemetry] Subscribed to GPU usage events" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[SomeIPTelemetry] Subscribe error: " << e.what() << std::endl;
    }
}

void SomeIPTelemetrySourceImpl::unsubscribeFromEvents() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_subscribed && m_proxy) {
        try {
            m_proxy->getNotifyGpuUsageDataChangeEvent().unsubscribe(m_eventSubscription);
            m_subscribed = false;
            std::cout << "[SomeIPTelemetry] Unsubscribed from GPU usage events" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[SomeIPTelemetry] Unsubscribe error: " << e.what() << std::endl;
        }
    }
}

void SomeIPTelemetrySourceImpl::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_subscribed) {
        unsubscribeFromEvents();
    }

    m_proxy.reset();
    m_runtime.reset();
    m_initialized = false;

    std::cout << "[SomeIPTelemetry] Client shutdown complete" << std::endl;
}

// --- SomeIPTelemetrySourceAdapter (Adapter) ---

SomeIPTelemetrySourceAdapter::SomeIPTelemetrySourceAdapter(bool useEvents)
    : m_impl(SomeIPTelemetrySourceImpl::getInstance())
    , m_useEvents(useEvents)
    , m_lastEventValue(0.0f)
    , m_eventReceived(false)
{
}

bool SomeIPTelemetrySourceAdapter::openSource() {
    // Start the singleton and wait until the GPU service is reachable
    if (!m_impl.initialize()) {
        return false;
    }

    if (!m_impl.waitForAvailability(5000)) {
        return false;
    }

    // If using event mode, register a listener that stores the latest value for us
    if (m_useEvents) {
        m_impl.subscribeToEvents([this](float usage) {
            std::lock_guard<std::mutex> lock(m_eventMutex);
            m_lastEventValue = usage;
            m_eventReceived = true;
        });
    }

    return true;
}

bool SomeIPTelemetrySourceAdapter::readSource(std::string& out) {
    float usage = 0.0f;
    bool success = false;

    if (m_useEvents) {
        // Return whatever the last broadcast event delivered
        std::lock_guard<std::mutex> lock(m_eventMutex);
        if (m_eventReceived) {
            usage = m_lastEventValue;
            success = true;
        } else {
            // No event has come in yet, so fall back to a direct request
            success = m_impl.requestGpuUsage(usage);
        }
    } else {
        // Poll the service directly via request/response
        success = m_impl.requestGpuUsage(usage);
    }

    if (success) {
        if (usage < 0.0f) {
            out = "GPU Load: Unavailable";
        } else {
            std::ostringstream oss;
            oss << "GPU Load: " << std::fixed << std::setprecision(1) << usage << "%";
            out = oss.str();
        }
        return true;
    }

    return false;
}

} // namespace telemetry
