#pragma once
#include <QMainWindow>
#include <QSplitter>

class ConfigEditorWidget;
class LogViewerWidget;
class TelemetryChartWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(const QString& configPath, QWidget* parent = nullptr);

    LogViewerWidget*       logViewer()  { return m_logViewer; }
    TelemetryChartWidget*  charts()     { return m_charts; }
    ConfigEditorWidget*    configEditor() { return m_configEditor; }

private:
    void applyGlobalStyle();

    ConfigEditorWidget*    m_configEditor;
    LogViewerWidget*       m_logViewer;
    TelemetryChartWidget*  m_charts;
    QSplitter*             m_mainSplitter;
};
