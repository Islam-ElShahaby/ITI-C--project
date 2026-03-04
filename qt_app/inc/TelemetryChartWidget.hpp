#pragma once
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <deque>
#include <mutex>
#include <atomic>
#include <QString>

// A single real-time line chart painted with QPainter
class RealtimeLineChart : public QWidget
{
    Q_OBJECT
public:
    explicit RealtimeLineChart(const QString& title, const QString& unit,
                                double minVal, double maxVal,
                                const QColor& lineColor,
                                QWidget* parent = nullptr);

    void addDataPoint(double value);
    void clearData();
    void setWarningThreshold(double val) { m_warningThreshold = val; }
    void setCriticalThreshold(double val) { m_criticalThreshold = val; }
    void setAutoScale(bool on) { m_autoScale = on; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_title;
    QString m_unit;
    double m_minVal;
    double m_maxVal;
    double m_warningThreshold = -1;
    double m_criticalThreshold = -1;
    QColor m_lineColor;
    bool m_autoScale = false;

    static constexpr int MAX_POINTS = 120; // ~2 minutes at 1s interval
    std::deque<double> m_data;
    mutable std::mutex m_mutex;
};

// Widget that contains three charts stacked vertically
class TelemetryChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TelemetryChartWidget(const QString& configPath, QWidget* parent = nullptr);

    RealtimeLineChart* cpuChart()      { return m_cpuChart; }
    RealtimeLineChart* memoryChart()   { return m_memChart; }
    RealtimeLineChart* gpuChart()      { return m_gpuChart; }
    RealtimeLineChart* cpuTempChart()  { return m_cpuTempChart; }

public slots:
    void onLogMessage(const QString& formatted, const QString& appName,
                      const QString& context, const QString& text,
                      int severity, qint64 timestamp);
    void onConfigSaved();

private:
    void reloadEnabledFlags();

    QString m_configPath;
    std::atomic<bool> m_cpuEnabled{true};
    std::atomic<bool> m_memEnabled{true};
    std::atomic<bool> m_gpuEnabled{true};
    std::atomic<bool> m_cpuTempEnabled{true};

    RealtimeLineChart* m_cpuChart;
    RealtimeLineChart* m_memChart;
    RealtimeLineChart* m_gpuChart;
    RealtimeLineChart* m_cpuTempChart;
    QTimer* m_refreshTimer;
};
