#pragma once
#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QString>
#include <map>
#include <vector>

// A custom iOS-style toggle switch
class ToggleSwitch : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
public:
    explicit ToggleSwitch(QWidget* parent = nullptr);
    bool isChecked() const { return m_checked; }
    void setChecked(bool on);
    QSize sizeHint() const override { return QSize(50, 26); }
    QSize minimumSizeHint() const override { return QSize(50, 26); }

signals:
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    bool m_checked = false;
    double m_handlePos = 0.0;   // 0.0 = off, 1.0 = on
};

struct TelemetrySectionUI {
    ToggleSwitch*    enabledSwitch = nullptr;
    QSpinBox*        intervalSpin  = nullptr;
    QCheckBox*       consoleSink   = nullptr;
    QCheckBox*       fileSink      = nullptr;
    QCheckBox*       socketSink    = nullptr;
};

struct SinkSectionUI {
    ToggleSwitch*    enabledSwitch = nullptr;
    QLineEdit*       pathEdit      = nullptr;  // only for file/socket sinks
};

class ConfigEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigEditorWidget(const QString& configPath, QWidget* parent = nullptr);

signals:
    void configSaved();

private slots:
    void loadConfig();
    void saveConfig();

private:
    QGroupBox* createTelemetryGroup(const QString& name, TelemetrySectionUI& ui);
    QGroupBox* createSinkGroup(const QString& name, SinkSectionUI& ui, bool hasPath);

    QString m_configPath;

    // Telemetry sections
    TelemetrySectionUI m_cpuUI;
    TelemetrySectionUI m_memUI;
    TelemetrySectionUI m_gpuUI;

    // Sink sections
    SinkSectionUI m_consoleSinkUI;
    SinkSectionUI m_fileSinkUI;

    // General
    QSpinBox* m_globalIntervalSpin = nullptr;

    QPushButton* m_saveBtn = nullptr;
    QLabel*      m_statusLabel = nullptr;
};
