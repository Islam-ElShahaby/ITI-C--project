#include "MainWindow.hpp"
#include "ConfigEditorWidget.hpp"
#include "LogViewerWidget.hpp"
#include "TelemetryChartWidget.hpp"
#include "QtSinkImpl.hpp"
#include "Application.hpp"
#include <QApplication>
#include <QThread>
#include <csignal>
#include <iostream>

static Application* g_app = nullptr;

static void signalHandler(int signal) {
    std::cout << "\n[QtApp] Received signal " << signal << ", shutting down..." << std::endl;
    if (g_app) {
        g_app->stop();
    }
    QApplication::quit();
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("OmniMetron");
    app.setOrganizationName("ITI");

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    const QString configPath = "config/app_config.json";

    // Create the main window
    MainWindow mainWin(configPath);

    // Create the Application (logging + telemetry core)
    Application coreApp(configPath.toStdString());
    g_app = &coreApp;

    // Create the Qt sink and inject it into the LogManager
    // (We access via a public method we'll add, or via the builder approach)
    auto* qtSink = new QtSinkImpl();

    // Connect the Qt sink to the GUI widgets
    QObject::connect(qtSink, &QtSinkImpl::newLogMessage,
                     mainWin.logViewer(), &LogViewerWidget::onLogMessage,
                     Qt::QueuedConnection);
    QObject::connect(qtSink, &QtSinkImpl::newLogMessage,
                     mainWin.charts(), &TelemetryChartWidget::onLogMessage,
                     Qt::QueuedConnection);

    // Add the Qt sink to the core app's logger
    coreApp.addSink(std::unique_ptr<ILogSink>(qtSink));

    mainWin.show();

    // Run the telemetry loop in a background thread
    std::thread telemetryThread([&coreApp]() {
        coreApp.start();
    });

    int result = app.exec();

    // When Qt event loop exits, stop the core app
    coreApp.stop();
    if (telemetryThread.joinable()) {
        telemetryThread.join();
    }

    return result;
}
