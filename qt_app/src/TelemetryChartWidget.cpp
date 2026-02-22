#include "TelemetryChartWidget.hpp"
#include <QPainterPath>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// RealtimeLineChart
// ---------------------------------------------------------------------------
RealtimeLineChart::RealtimeLineChart(const QString& title, const QString& unit,
                                     double minVal, double maxVal,
                                     const QColor& lineColor, QWidget* parent)
    : QWidget(parent)
    , m_title(title), m_unit(unit)
    , m_minVal(minVal), m_maxVal(maxVal)
    , m_lineColor(lineColor)
{
    setMinimumHeight(150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void RealtimeLineChart::addDataPoint(double value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.push_back(value);
    if (static_cast<int>(m_data.size()) > MAX_POINTS) {
        m_data.pop_front();
    }
}

void RealtimeLineChart::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // Background
    QLinearGradient bgGrad(0, 0, 0, h);
    bgGrad.setColorAt(0, QColor(30, 30, 45));
    bgGrad.setColorAt(1, QColor(20, 20, 35));
    p.fillRect(rect(), bgGrad);

    // Margins
    const int marginL = 60, marginR = 15, marginT = 35, marginB = 25;
    const int chartW = w - marginL - marginR;
    const int chartH = h - marginT - marginB;
    const QRect chartRect(marginL, marginT, chartW, chartH);

    // Title
    p.setPen(QColor(220, 220, 240));
    QFont titleFont = font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(marginL, 5, chartW, 28, Qt::AlignLeft | Qt::AlignVCenter, m_title);

    // Last value badge
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_data.empty()) {
            double last = m_data.back();
            QColor badgeColor = m_lineColor;
            if (m_criticalThreshold > 0 && last > m_criticalThreshold)
                badgeColor = QColor(220, 50, 50);
            else if (m_warningThreshold > 0 && last > m_warningThreshold)
                badgeColor = QColor(240, 180, 30);

            QString valText = QString::number(last, 'f', 1) + m_unit;
            QFont valFont = font();
            valFont.setPointSize(12);
            valFont.setBold(true);
            p.setFont(valFont);
            p.setPen(badgeColor);
            p.drawText(marginL, 5, chartW, 28, Qt::AlignRight | Qt::AlignVCenter, valText);
        }
    }

    // Grid lines & Y axis labels
    p.setPen(QPen(QColor(60, 60, 80), 1, Qt::DotLine));
    QFont axisFont = font();
    axisFont.setPointSize(8);
    p.setFont(axisFont);

    int gridLines = 5;
    for (int i = 0; i <= gridLines; ++i) {
        double frac = static_cast<double>(i) / gridLines;
        int y = marginT + static_cast<int>(frac * chartH);
        p.setPen(QPen(QColor(50, 50, 70), 1, Qt::DotLine));
        p.drawLine(marginL, y, w - marginR, y);

        double val = m_maxVal - frac * (m_maxVal - m_minVal);
        p.setPen(QColor(140, 140, 170));
        p.drawText(0, y - 10, marginL - 8, 20, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(val, 'f', 0) + m_unit);
    }

    // Threshold lines
    auto drawThreshold = [&](double val, const QColor& color) {
        if (val < 0) return;
        double frac = 1.0 - (val - m_minVal) / (m_maxVal - m_minVal);
        int y = marginT + static_cast<int>(frac * chartH);
        if (y >= marginT && y <= marginT + chartH) {
            p.setPen(QPen(color, 1.5, Qt::DashLine));
            p.drawLine(marginL, y, w - marginR, y);
        }
    };
    drawThreshold(m_warningThreshold, QColor(240, 180, 30, 150));
    drawThreshold(m_criticalThreshold, QColor(220, 50, 50, 150));

    // Plot line
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_data.size() < 2) return;

    int n = static_cast<int>(m_data.size());
    double xStep = static_cast<double>(chartW) / (MAX_POINTS - 1);

    // Gradient fill under the line
    QPainterPath fillPath;
    QPointF firstPt;
    QPointF lastPt;

    for (int i = 0; i < n; ++i) {
        double frac = 1.0 - (m_data[i] - m_minVal) / (m_maxVal - m_minVal);
        frac = std::clamp(frac, 0.0, 1.0);
        double x = marginL + (MAX_POINTS - n + i) * xStep;
        double y = marginT + frac * chartH;
        QPointF pt(x, y);

        if (i == 0) {
            fillPath.moveTo(pt);
            firstPt = pt;
        } else {
            fillPath.lineTo(pt);
        }
        lastPt = pt;
    }

    // Close the fill path
    QPainterPath closedPath = fillPath;
    closedPath.lineTo(lastPt.x(), marginT + chartH);
    closedPath.lineTo(firstPt.x(), marginT + chartH);
    closedPath.closeSubpath();

    QLinearGradient fillGrad(0, marginT, 0, marginT + chartH);
    QColor fillTop = m_lineColor;
    fillTop.setAlpha(80);
    QColor fillBot = m_lineColor;
    fillBot.setAlpha(10);
    fillGrad.setColorAt(0, fillTop);
    fillGrad.setColorAt(1, fillBot);
    p.fillPath(closedPath, fillGrad);

    // The line itself with glow
    p.setPen(QPen(m_lineColor.lighter(120), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(fillPath);

    // Dot on last data point
    if (!m_data.empty()) {
        QColor dotColor = m_lineColor;
        if (m_criticalThreshold > 0 && m_data.back() > m_criticalThreshold)
            dotColor = QColor(220, 50, 50);
        else if (m_warningThreshold > 0 && m_data.back() > m_warningThreshold)
            dotColor = QColor(240, 180, 30);
        p.setPen(Qt::NoPen);
        p.setBrush(dotColor);
        p.drawEllipse(lastPt, 4, 4);
        // Glow
        QRadialGradient glow(lastPt, 12);
        glow.setColorAt(0, QColor(dotColor.red(), dotColor.green(), dotColor.blue(), 80));
        glow.setColorAt(1, Qt::transparent);
        p.setBrush(glow);
        p.drawEllipse(lastPt, 12, 12);
    }

    // Chart border
    p.setPen(QPen(QColor(60, 60, 80), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(chartRect);
}

// ---------------------------------------------------------------------------
// TelemetryChartWidget
// ---------------------------------------------------------------------------
TelemetryChartWidget::TelemetryChartWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(8, 8, 8, 8);

    m_cpuChart = new RealtimeLineChart("CPU Usage", "%", 0, 100,
                                        QColor(0, 180, 255), this);
    m_cpuChart->setWarningThreshold(75);
    m_cpuChart->setCriticalThreshold(90);

    m_memChart = new RealtimeLineChart("Memory Usage", "MB", 0, 32000,
                                        QColor(140, 80, 255), this);
    m_memChart->setWarningThreshold(8192);
    m_memChart->setCriticalThreshold(12288);

    m_gpuChart = new RealtimeLineChart("GPU Usage", "%", 0, 100,
                                        QColor(50, 220, 120), this);
    m_gpuChart->setWarningThreshold(80);
    m_gpuChart->setCriticalThreshold(95);

    layout->addWidget(m_cpuChart);
    layout->addWidget(m_memChart);
    layout->addWidget(m_gpuChart);

    // Refresh charts periodically
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        m_cpuChart->update();
        m_memChart->update();
        m_gpuChart->update();
    });
    m_refreshTimer->start(500); // 2 FPS
}

void TelemetryChartWidget::onLogMessage(const QString& /*formatted*/,
                                         const QString& appName,
                                         const QString& context,
                                         const QString& text,
                                         int /*severity*/, qint64 /*timestamp*/)
{
    if (appName != "Telemetry") return;

    // Extract numeric value from text like "Value: 42.5%" or "Value: 7343.76MB"
    static QRegularExpression valRe(R"(([0-9]*\.?[0-9]+))");
    QRegularExpressionMatch match = valRe.match(text);
    if (!match.hasMatch()) return;

    bool ok = false;
    double val = match.captured(1).toDouble(&ok);
    if (!ok) return;

    QString ctx = context.toLower();
    if (ctx == "cpu") {
        m_cpuChart->addDataPoint(val);
    } else if (ctx == "memory") {
        m_memChart->addDataPoint(val);
    } else if (ctx == "gpu") {
        m_gpuChart->addDataPoint(val);
    }
}
