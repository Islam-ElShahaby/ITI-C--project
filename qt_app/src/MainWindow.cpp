#include "MainWindow.hpp"
#include "ConfigEditorWidget.hpp"
#include "LogViewerWidget.hpp"
#include "TelemetryChartWidget.hpp"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QApplication>

MainWindow::MainWindow(const QString& configPath, QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("OmniMetron — Config & Logs");
    resize(1600, 900);

    applyGlobalStyle();

    // Create widgets
    m_configEditor = new ConfigEditorWidget(configPath, this);
    m_logViewer    = new LogViewerWidget(this);
    m_charts       = new TelemetryChartWidget(configPath, this);

    // When config is saved, update chart enabled flags
    connect(m_configEditor, &ConfigEditorWidget::configSaved,
            m_charts, &TelemetryChartWidget::onConfigSaved);

    // Right side: tabs for Logs and Charts
    auto* rightTabs = new QTabWidget();
    rightTabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #4a3660; border-top: none; background: #251e30; } "
        "QTabBar::tab { background: #322840; color: #b0b0c0; border: 1px solid #4a3660; "
        "border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; "
        "padding: 8px 20px; margin-right: 2px; font-weight: bold; } "
        "QTabBar::tab:selected { background: #251e30; color: #9d4edd; } "
        "QTabBar::tab:hover { background: #403050; }");
    rightTabs->addTab(m_charts,    "Telemetry Charts");
    rightTabs->addTab(m_logViewer, "Log Output");

    // Main splitter: Config left, Tabs right
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_configEditor);
    m_mainSplitter->addWidget(rightTabs);
    m_mainSplitter->setStretchFactor(0, 2);   // config: 2 parts
    m_mainSplitter->setStretchFactor(1, 3);   // logs/charts: 3 parts
    m_mainSplitter->setStyleSheet(
        "QSplitter::handle { background: #4a3660; width: 3px; }");

    setCentralWidget(m_mainSplitter);
}

void MainWindow::applyGlobalStyle()
{
    qApp->setStyleSheet(R"(
        * {
            font-family: 'Inter', 'Segoe UI', 'Roboto', sans-serif;
        }
        QMainWindow {
            background: #1d1626;
        }
        QWidget {
            background: #251e30;
            color: #d0d0e0;
        }
        QLabel {
            background: transparent;
            color: #c0c0d0;
        }
        QGroupBox {
            background: rgba(50,30,70,180);
            border: 1px solid #4a3660;
            border-radius: 8px;
            margin-top: 14px;
            padding: 16px 12px 12px 12px;
            font-weight: bold;
            color: #c0c0d0;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 4px;
        }
        QSpinBox {
            background: #322840;
            color: #d0d0e0;
            border: 1px solid #4a3660;
            border-radius: 4px;
            padding: 3px 6px;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            background: #4a3660;
            border: none;
            width: 16px;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background: #5c4575;
        }
        QLineEdit {
            background: #322840;
            color: #d0d0e0;
            border: 1px solid #4a3660;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QCheckBox {
            color: #b0b0c0;
            spacing: 6px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 4px;
            border: 2px solid #5c4575;
            background: #322840;
        }
        QCheckBox::indicator:checked {
            background: #9d4edd;
            border-color: #7b2cbf;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QToolTip {
            background: #322840;
            color: #d0d0e0;
            border: 1px solid #4a3660;
            padding: 4px;
        }
    )");
}
