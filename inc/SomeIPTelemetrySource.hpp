#pragma once

#include "ITelemetrySource.hpp"
#include <CommonAPI/CommonAPI.hpp>
#include <v1/omnimetron/gpu/GpuUsageDataProxy.hpp>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

namespace telemetry {

/**
 * @brief SomeIP Telemetry Source Implementation (Singleton Pattern)
 * 
 * This class provides a single instance client for communicating with a 
 * vSOME/IP GPU usage data service using CommonAPI. It supports both
 * synchronous method request/response and event-based communication.
 * 
 * Design Pattern: Singleton
 * - Ensures only one instance exists throughout the application lifetime
 * - Thread-safe initialization using double-checked locking
 */
class SomeIPTelemetrySourceImpl {
    // Custom deleter for use with unique_ptr (allows access to private destructor)
    struct Deleter {
        void operator()(SomeIPTelemetrySourceImpl* ptr) const {
            delete ptr;
        }
    };
    friend struct Deleter;
    
public:
    // Delete copy and move operations (Singleton pattern)
    SomeIPTelemetrySourceImpl(const SomeIPTelemetrySourceImpl&) = delete;
    SomeIPTelemetrySourceImpl& operator=(const SomeIPTelemetrySourceImpl&) = delete;
    SomeIPTelemetrySourceImpl(SomeIPTelemetrySourceImpl&&) = delete;
    SomeIPTelemetrySourceImpl& operator=(SomeIPTelemetrySourceImpl&&) = delete;

    /**
     * @brief Get the singleton instance
     * @return Reference to the singleton instance
     */
    static SomeIPTelemetrySourceImpl& getInstance();

    /**
     * @brief Initialize the CommonAPI runtime and build the proxy
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Check if the service proxy is available
     * @return true if the service is available
     */
    bool isAvailable() const;

    /**
     * @brief Wait for the service to become available (blocking)
     * @param timeoutMs Maximum time to wait in milliseconds (0 = indefinite)
     * @return true if service became available, false on timeout
     */
    bool waitForAvailability(uint32_t timeoutMs = 5000);

    /**
     * @brief Request GPU usage data using synchronous method call
     * @param usage Output parameter for the GPU usage percentage (0-100)
     * @return true if request successful, false otherwise
     */
    bool requestGpuUsage(float& usage);

    /**
     * @brief Request GPU usage data using asynchronous method call
     * @param callback Callback function to receive the result
     */
    void requestGpuUsageAsync(std::function<void(bool success, float usage)> callback);

    /**
     * @brief Subscribe to GPU usage data change events (broadcasts)
     * @param handler Event handler callback
     */
    void subscribeToEvents(std::function<void(float usage)> handler);

    /**
     * @brief Unsubscribe from GPU usage data change events
     */
    void unsubscribeFromEvents();

    /**
     * @brief Shutdown the client and release resources
     */
    void shutdown();

private:
    // Private constructor (Singleton pattern)
    SomeIPTelemetrySourceImpl();
    ~SomeIPTelemetrySourceImpl();

    // CommonAPI components
    std::shared_ptr<CommonAPI::Runtime> m_runtime;
    std::shared_ptr<v1::omnimetron::gpu::GpuUsageDataProxy<>> m_proxy;
    
    // Event subscription
    CommonAPI::Event<float>::Subscription m_eventSubscription;
    bool m_subscribed;
    
    // State
    std::atomic<bool> m_initialized;
    mutable std::mutex m_mutex;		

    // Singleton instance (uses custom Deleter)
    static std::unique_ptr<SomeIPTelemetrySourceImpl, Deleter> s_instance;
    static std::once_flag s_onceFlag;
};


/**
 * @brief Adapter for SomeIPTelemetrySourceImpl to ITelemetrySource interface
 * 
 * Design Pattern: Adapter
 * - Adapts the SomeIPTelemetrySourceImpl interface to match the ITelemetrySource
 *   interface expected by the rest of the application
 * - Allows seamless integration with existing telemetry infrastructure
 */
class SomeIPTelemetrySourceAdapter : public ITelemetrySource {
public:
    /**
     * @brief Construct adapter wrapping the singleton SomeIPTelemetrySourceImpl
     * @param useEvents If true, use event-based communication; otherwise use request/response
     */
    explicit SomeIPTelemetrySourceAdapter(bool useEvents = false);
    
    ~SomeIPTelemetrySourceAdapter() override = default;

    /**
     * @brief Open the telemetry source (initialize and wait for service)
     * @return true if source opened successfully
     */
    bool openSource() override;

    /**
     * @brief Read telemetry data from the source
     * @param out Output string containing formatted GPU usage data
     * @return true if read successful
     */
    bool readSource(std::string& out) override;

private:
    SomeIPTelemetrySourceImpl& m_impl;
    bool m_useEvents;
    float m_lastEventValue;
    std::atomic<bool> m_eventReceived;
    mutable std::mutex m_eventMutex;
};

} // namespace telemetry
