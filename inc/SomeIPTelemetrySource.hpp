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

// A singleton client that talks to the vSOME/IP GPU usage data service via CommonAPI.
// Supports both synchronous request/response and event-based (broadcast) communication.
// Only one instance is ever created — use getInstance() to access it.
class SomeIPTelemetrySourceImpl {
    // Custom deleter so unique_ptr can call the private destructor
    struct Deleter {
        void operator()(SomeIPTelemetrySourceImpl* ptr) const {
            delete ptr;
        }
    };
    friend struct Deleter;
    
public:
    // Singleton means no copying or moving — there is exactly one instance for the process lifetime
    SomeIPTelemetrySourceImpl(const SomeIPTelemetrySourceImpl&) = delete;
    SomeIPTelemetrySourceImpl& operator=(const SomeIPTelemetrySourceImpl&) = delete;
    SomeIPTelemetrySourceImpl(SomeIPTelemetrySourceImpl&&) = delete;
    SomeIPTelemetrySourceImpl& operator=(SomeIPTelemetrySourceImpl&&) = delete;

    // Returns the one and only instance, creating it on the first call
    static SomeIPTelemetrySourceImpl& getInstance();

    // Connects to the CommonAPI runtime and builds the service proxy.
    // Safe to call multiple times — does nothing if already initialized.
    bool initialize();

    // Returns true if the service proxy is currently reachable
    bool isAvailable() const;

    // Blocks until the service shows up, or until timeoutMs milliseconds have passed.
    // Pass 0 to wait indefinitely.
    bool waitForAvailability(uint32_t timeoutMs = 5000);

    // Sends a synchronous request to the service and fills `usage` with the GPU percentage.
    // Returns false if the service is unavailable or the call fails.
    bool requestGpuUsage(float& usage);

    // Sends an asynchronous request; the callback receives (success, usage) when the reply arrives
    void requestGpuUsageAsync(std::function<void(bool success, float usage)> callback);

    // Subscribes to the GPU usage broadcast event; `handler` is called whenever a new value is published
    void subscribeToEvents(std::function<void(float usage)> handler);

    // Stops receiving broadcast events
    void unsubscribeFromEvents();

    // Releases the proxy and runtime and marks the instance as uninitialized
    void shutdown();

private:
    // Private so callers must go through getInstance()
    SomeIPTelemetrySourceImpl();
    ~SomeIPTelemetrySourceImpl();

    // CommonAPI runtime and the generated proxy for the GPU service
    std::shared_ptr<CommonAPI::Runtime> m_runtime;
    std::shared_ptr<v1::omnimetron::gpu::GpuUsageDataProxy<>> m_proxy;
    
    // Tracks the current event subscription handle
    CommonAPI::Event<float>::Subscription m_eventSubscription;
    bool m_subscribed;
    
    // Guards initialization and proxy access from multiple threads
    std::atomic<bool> m_initialized;
    mutable std::mutex m_mutex;		

    // Singleton storage and a once_flag to ensure the constructor runs exactly once
    static std::unique_ptr<SomeIPTelemetrySourceImpl, Deleter> s_instance;
    static std::once_flag s_onceFlag;
};


// Adapts SomeIPTelemetrySourceImpl to the ITelemetrySource interface so the rest of the
// application can treat it like any other telemetry source without knowing about SOME/IP.
class SomeIPTelemetrySourceAdapter : public ITelemetrySource {
public:
    // Set useEvents=true to get GPU values from broadcast events instead of polling via request/response
    explicit SomeIPTelemetrySourceAdapter(bool useEvents = false);
    
    ~SomeIPTelemetrySourceAdapter() override = default;

    // Initializes the underlying singleton and waits for the service to become available
    bool openSource() override;

    // Fetches the latest GPU usage and writes a formatted string (e.g. "GPU Load: 42.3%") into `out`
    bool readSource(std::string& out) override;

private:
    SomeIPTelemetrySourceImpl& m_impl;
    bool m_useEvents;
    float m_lastEventValue;
    std::atomic<bool> m_eventReceived;
    mutable std::mutex m_eventMutex;
};

} // namespace telemetry
