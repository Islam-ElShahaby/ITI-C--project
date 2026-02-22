#include "ConfigEditorWidget.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QScrollArea>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDateTime>

// ---------------------------------------------------------------------------
// ToggleSwitch
// ---------------------------------------------------------------------------
ToggleSwitch::ToggleSwitch(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(50, 26);
    setCursor(Qt::PointingHandCursor);
}

void ToggleSwitch::setChecked(bool on)
{
    if (m_checked == on) return;
    m_checked = on;
    m_handlePos = on ? 1.0 : 0.0;
    update();
    emit toggled(m_checked);
}

void ToggleSwitch::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int radius = h / 2;

    // Track
    QColor trackColor = m_checked ? QColor(0, 180, 80) : QColor(80, 80, 100);
    p.setPen(Qt::NoPen);
    p.setBrush(trackColor);
    p.drawRoundedRect(0, 0, w, h, radius, radius);

    // Handle
    const int handleD = h - 4;
    double xOff = m_checked ? (w - handleD - 2) : 2;
    QRectF handleRect(xOff, 2, handleD, handleD);

    // Handle shadow
    p.setBrush(QColor(0, 0, 0, 40));
    p.drawEllipse(handleRect.adjusted(1, 1, 1, 1));

    // Handle face
    QLinearGradient handleGrad(handleRect.topLeft(), handleRect.bottomLeft());
    handleGrad.setColorAt(0, QColor(255, 255, 255));
    handleGrad.setColorAt(1, QColor(230, 230, 230));
    p.setBrush(handleGrad);
    p.drawEllipse(handleRect);
}

void ToggleSwitch::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        setChecked(!m_checked);
    }
}

// ---------------------------------------------------------------------------
// ConfigEditorWidget
// ---------------------------------------------------------------------------
ConfigEditorWidget::ConfigEditorWidget(const QString& configPath, QWidget* parent)
    : QWidget(parent), m_configPath(configPath)
{
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* scrollContent = new QWidget();
    auto* mainLayout = new QVBoxLayout(scrollContent);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // Header
    auto* headerLabel = new QLabel("⚙ Configuration");
    headerLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #e0e0f0; padding: 4px 0px;");
    mainLayout->addWidget(headerLabel);

    // --- Telemetry Section ---
    auto* telLabel = new QLabel("TELEMETRY SOURCES");
    telLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #888; letter-spacing: 1px; padding-top: 6px;");
    mainLayout->addWidget(telLabel);

    mainLayout->addWidget(createTelemetryGroup("CPU", m_cpuUI));
    mainLayout->addWidget(createTelemetryGroup("Memory", m_memUI));
    mainLayout->addWidget(createTelemetryGroup("GPU", m_gpuUI));

    // --- Sinks Section ---
    auto* sinksLabel = new QLabel("LOG SINKS");
    sinksLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #888; letter-spacing: 1px; padding-top: 6px;");
    mainLayout->addWidget(sinksLabel);

    mainLayout->addWidget(createSinkGroup("Console", m_consoleSinkUI, false));
    mainLayout->addWidget(createSinkGroup("File", m_fileSinkUI, true));

    // --- General Section ---
    auto* genGroup = new QGroupBox("General");
    genGroup->setStyleSheet(
        "QGroupBox { background: rgba(40,40,60,180); border: 1px solid #3a3a50; "
        "border-radius: 8px; margin-top: 14px; padding: 16px 12px 12px 12px; font-weight: bold; color: #c0c0d0; } "
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }");
    auto* genLayout = new QHBoxLayout(genGroup);
    genLayout->addWidget(new QLabel("Global Interval (ms):"));
    m_globalIntervalSpin = new QSpinBox();
    m_globalIntervalSpin->setRange(100, 60000);
    m_globalIntervalSpin->setSingleStep(100);
    genLayout->addWidget(m_globalIntervalSpin);
    mainLayout->addWidget(genGroup);

    // --- Save Button ---
    m_saveBtn = new QPushButton("💾  Save Configuration");
    m_saveBtn->setFixedHeight(42);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, "
        "stop:0 #00b4d8, stop:1 #0077b6); color: white; font-size: 14px; "
        "font-weight: bold; border: none; border-radius: 8px; } "
        "QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, "
        "stop:0 #48cae4, stop:1 #0096c7); } "
        "QPushButton:pressed { background: #005f8a; }");
    connect(m_saveBtn, &QPushButton::clicked, this, &ConfigEditorWidget::saveConfig);
    mainLayout->addWidget(m_saveBtn);

    // Status label
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #70d070; font-size: 11px; padding: 2px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    loadConfig();
}

QGroupBox* ConfigEditorWidget::createTelemetryGroup(const QString& name, TelemetrySectionUI& ui)
{
    auto* group = new QGroupBox(name);
    group->setStyleSheet(
        "QGroupBox { background: rgba(40,40,60,180); border: 1px solid #3a3a50; "
        "border-radius: 8px; margin-top: 14px; padding: 16px 12px 12px 12px; font-weight: bold; color: #c0c0d0; } "
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }");

    auto* vbox = new QVBoxLayout(group);

    // Enabled row
    auto* enabledRow = new QHBoxLayout();
    enabledRow->addWidget(new QLabel("Enabled"));
    ui.enabledSwitch = new ToggleSwitch();
    enabledRow->addStretch();
    enabledRow->addWidget(ui.enabledSwitch);
    vbox->addLayout(enabledRow);

    // Interval row
    auto* intervalRow = new QHBoxLayout();
    intervalRow->addWidget(new QLabel("Interval (ms):"));
    ui.intervalSpin = new QSpinBox();
    ui.intervalSpin->setRange(100, 60000);
    ui.intervalSpin->setSingleStep(100);
    ui.intervalSpin->setFixedWidth(100);
    intervalRow->addStretch();
    intervalRow->addWidget(ui.intervalSpin);
    vbox->addLayout(intervalRow);

    // Sinks row
    auto* sinksLabel = new QLabel("Route to sinks:");
    sinksLabel->setStyleSheet("color: #999; font-size: 10px;");
    vbox->addWidget(sinksLabel);

    auto* sinksRow = new QHBoxLayout();
    ui.consoleSink = new QCheckBox("Console");
    ui.fileSink    = new QCheckBox("File");
    ui.socketSink  = new QCheckBox("Socket");
    sinksRow->addWidget(ui.consoleSink);
    sinksRow->addWidget(ui.fileSink);
    sinksRow->addWidget(ui.socketSink);
    sinksRow->addStretch();
    vbox->addLayout(sinksRow);

    return group;
}

QGroupBox* ConfigEditorWidget::createSinkGroup(const QString& name, SinkSectionUI& ui, bool hasPath)
{
    auto* group = new QGroupBox(name + " Sink");
    group->setStyleSheet(
        "QGroupBox { background: rgba(40,40,60,180); border: 1px solid #3a3a50; "
        "border-radius: 8px; margin-top: 14px; padding: 16px 12px 12px 12px; font-weight: bold; color: #c0c0d0; } "
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }");

    auto* vbox = new QVBoxLayout(group);

    // Enabled row
    auto* enabledRow = new QHBoxLayout();
    enabledRow->addWidget(new QLabel("Enabled"));
    ui.enabledSwitch = new ToggleSwitch();
    enabledRow->addStretch();
    enabledRow->addWidget(ui.enabledSwitch);
    vbox->addLayout(enabledRow);

    if (hasPath) {
        auto* pathRow = new QHBoxLayout();
        pathRow->addWidget(new QLabel("Path:"));
        ui.pathEdit = new QLineEdit();
        ui.pathEdit->setPlaceholderText("e.g. app_log.log");
        pathRow->addWidget(ui.pathEdit);
        vbox->addLayout(pathRow);
    }

    return group;
}

void ConfigEditorWidget::loadConfig()
{
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_statusLabel->setStyleSheet("color: #e05050; font-size: 11px;");
        m_statusLabel->setText("⚠ Failed to open config file");
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (doc.isNull()) {
        m_statusLabel->setStyleSheet("color: #e05050; font-size: 11px;");
        m_statusLabel->setText("⚠ JSON parse error: " + err.errorString());
        return;
    }

    QJsonObject root = doc.object();

    // Helper to load a telemetry section
    auto loadTelemetry = [&](const QString& key, TelemetrySectionUI& ui) {
        if (!root.contains("telemetry")) return;
        QJsonObject tel = root["telemetry"].toObject();
        if (!tel.contains(key)) return;
        QJsonObject sec = tel[key].toObject();

        ui.enabledSwitch->setChecked(sec.value("enabled").toBool(true));
        ui.intervalSpin->setValue(sec.value("interval").toInt(1000));

        QJsonArray sinksArr = sec.value("sinks").toArray();
        QStringList sinkList;
        for (auto v : sinksArr) sinkList << v.toString();
        ui.consoleSink->setChecked(sinkList.contains("console"));
        ui.fileSink->setChecked(sinkList.contains("file"));
        ui.socketSink->setChecked(sinkList.contains("socket"));
    };

    loadTelemetry("cpu", m_cpuUI);
    loadTelemetry("memory", m_memUI);
    loadTelemetry("gpu", m_gpuUI);

    // Sinks
    if (root.contains("sinks")) {
        QJsonObject sinks = root["sinks"].toObject();
        if (sinks.contains("console")) {
            m_consoleSinkUI.enabledSwitch->setChecked(
                sinks["console"].toObject().value("enabled").toBool(true));
        }
        if (sinks.contains("file")) {
            auto fobj = sinks["file"].toObject();
            m_fileSinkUI.enabledSwitch->setChecked(fobj.value("enabled").toBool(true));
            if (m_fileSinkUI.pathEdit)
                m_fileSinkUI.pathEdit->setText(fobj.value("path").toString());
        }
    }

    // General
    if (root.contains("general")) {
        m_globalIntervalSpin->setValue(
            root["general"].toObject().value("global_interval").toInt(1000));
    }

    m_statusLabel->setStyleSheet("color: #70d070; font-size: 11px;");
    m_statusLabel->setText("✓ Config loaded");
}

void ConfigEditorWidget::saveConfig()
{
    QJsonObject root;

    // Telemetry
    auto buildTelemetry = [](const TelemetrySectionUI& ui) -> QJsonObject {
        QJsonObject obj;
        obj["enabled"] = ui.enabledSwitch->isChecked();
        obj["interval"] = ui.intervalSpin->value();
        QJsonArray sinksArr;
        if (ui.consoleSink->isChecked()) sinksArr.append("console");
        if (ui.fileSink->isChecked())    sinksArr.append("file");
        if (ui.socketSink->isChecked())  sinksArr.append("socket");
        obj["sinks"] = sinksArr;
        return obj;
    };

    QJsonObject telObj;
    telObj["cpu"]    = buildTelemetry(m_cpuUI);
    telObj["memory"] = buildTelemetry(m_memUI);
    telObj["gpu"]    = buildTelemetry(m_gpuUI);
    root["telemetry"] = telObj;

    // Sinks
    QJsonObject sinksObj;
    {
        QJsonObject consoleObj;
        consoleObj["enabled"] = m_consoleSinkUI.enabledSwitch->isChecked();
        sinksObj["console"] = consoleObj;
    }
    {
        QJsonObject fileObj;
        fileObj["enabled"] = m_fileSinkUI.enabledSwitch->isChecked();
        if (m_fileSinkUI.pathEdit)
            fileObj["path"] = m_fileSinkUI.pathEdit->text();
        sinksObj["file"] = fileObj;
    }
    root["sinks"] = sinksObj;

    // General
    QJsonObject genObj;
    genObj["global_interval"] = m_globalIntervalSpin->value();
    root["general"] = genObj;

    QJsonDocument doc(root);
    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_statusLabel->setStyleSheet("color: #e05050; font-size: 11px;");
        m_statusLabel->setText("⚠ Failed to write config file");
        return;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    m_statusLabel->setStyleSheet("color: #70d070; font-size: 11px;");
    m_statusLabel->setText("✓ Saved at " + QDateTime::currentDateTime().toString("hh:mm:ss"));

    emit configSaved();
}
