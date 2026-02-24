#include "LogViewerWidget.hpp"
#include "LogTypes.hpp"
#include <QHBoxLayout>
#include <QScrollBar>

LogViewerWidget::LogViewerWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // Header
    auto* headerLabel = new QLabel("Log Output");
    headerLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #e0e0f0;");
    mainLayout->addWidget(headerLabel);

    // Toolbar
    auto* toolbar = new QHBoxLayout();

    m_severityFilter = new QComboBox();
    m_severityFilter->addItem("All Levels",     -1);
    m_severityFilter->addItem("INFO",         static_cast<int>(LogSeverity::INFO));
    m_severityFilter->addItem("WARNING",      static_cast<int>(LogSeverity::WARNING));
    m_severityFilter->addItem("ERROR",       static_cast<int>(LogSeverity::ERROR));
    m_severityFilter->addItem("CRITICAL",    static_cast<int>(LogSeverity::CRITICAL));
    m_severityFilter->setFixedWidth(140);
    m_severityFilter->setStyleSheet(
        "QComboBox { background: #2a2a3e; color: #d0d0e0; border: 1px solid #3a3a50; "
        "border-radius: 4px; padding: 4px 8px; } "
        "QComboBox::drop-down { border: none; } "
        "QComboBox QAbstractItemView { background: #2a2a3e; color: #d0d0e0; "
        "selection-background-color: #0077b6; }");
    toolbar->addWidget(m_severityFilter);

    m_searchBar = new QLineEdit();
    m_searchBar->setPlaceholderText("🔍 Filter logs...");
    m_searchBar->setStyleSheet(
        "QLineEdit { background: #2a2a3e; color: #d0d0e0; border: 1px solid #3a3a50; "
        "border-radius: 4px; padding: 4px 8px; }");
    toolbar->addWidget(m_searchBar);

    m_autoScrollCheck = new QCheckBox("Auto-scroll");
    m_autoScrollCheck->setChecked(true);
    m_autoScrollCheck->setStyleSheet("color: #b0b0c0;");
    toolbar->addWidget(m_autoScrollCheck);

    m_clearBtn = new QPushButton("Clear");
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    m_clearBtn->setStyleSheet(
        "QPushButton { background: #3a3a50; color: #d0d0e0; border: 1px solid #4a4a60; "
        "border-radius: 4px; padding: 4px 12px; } "
        "QPushButton:hover { background: #4a4a60; }");
    toolbar->addWidget(m_clearBtn);

    mainLayout->addLayout(toolbar);

    // Log area
    m_logArea = new QTextEdit();
    m_logArea->setReadOnly(true);
    m_logArea->setFont(QFont("Monospace", 9));
    m_logArea->setStyleSheet(
        "QTextEdit { background: #1a1a2e; color: #c8c8d8; border: 1px solid #3a3a50; "
        "border-radius: 6px; padding: 6px; } "
        "QScrollBar:vertical { background: #1a1a2e; width: 10px; border-radius: 5px; } "
        "QScrollBar::handle:vertical { background: #4a4a60; border-radius: 5px; min-height: 30px; } "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }");
    mainLayout->addWidget(m_logArea, 1);

    // Footer
    m_countLabel = new QLabel("0 entries");
    m_countLabel->setStyleSheet("color: #777; font-size: 10px;");
    mainLayout->addWidget(m_countLabel);

    // Connections
    connect(m_clearBtn, &QPushButton::clicked, this, &LogViewerWidget::clearLog);
    connect(m_severityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogViewerWidget::applyFilter);
    connect(m_searchBar, &QLineEdit::textChanged, this, &LogViewerWidget::applyFilter);
}

void LogViewerWidget::onLogMessage(const QString& formatted, const QString& /*appName*/,
                                    const QString& /*context*/, const QString& /*text*/,
                                    int severity, qint64 /*timestamp*/)
{
    // Store entry
    if (m_allEntries.size() >= MAX_ENTRIES) {
        m_allEntries.removeFirst();
    }
    m_allEntries.append({formatted, severity});
    m_totalCount++;

    // Check if it passes current filter
    int filterSev = m_severityFilter->currentData().toInt();
    QString searchText = m_searchBar->text();

    bool passFilter = (filterSev == -1 || severity >= filterSev);
    if (passFilter && !searchText.isEmpty()) {
        passFilter = formatted.contains(searchText, Qt::CaseInsensitive);
    }

    if (passFilter) {
        appendFormattedLine(formatted, severity);
        m_visibleCount++;
    }

    m_countLabel->setText(QString("%1 entries (%2 shown)")
                          .arg(m_allEntries.size()).arg(m_visibleCount));
}

void LogViewerWidget::appendFormattedLine(const QString& formatted, int severity)
{
    QColor color;
    switch (static_cast<LogSeverity>(severity)) {
        case LogSeverity::INFO:     color = QColor(100, 220, 140); break;
        case LogSeverity::WARNING:  color = QColor(240, 200, 60);  break;
        case LogSeverity::ERROR:    color = QColor(240, 80, 80);   break;
        case LogSeverity::CRITICAL: color = QColor(200, 40, 40);   break;
        default:                    color = QColor(200, 200, 220);  break;
    }

    m_logArea->setTextColor(color);
    m_logArea->append(formatted);

    if (m_autoScrollCheck->isChecked()) {
        QScrollBar* bar = m_logArea->verticalScrollBar();
        bar->setValue(bar->maximum());
    }
}

void LogViewerWidget::clearLog()
{
    m_logArea->clear();
    m_allEntries.clear();
    m_totalCount = 0;
    m_visibleCount = 0;
    m_countLabel->setText("0 entries");
}

void LogViewerWidget::applyFilter()
{
    m_logArea->clear();
    m_visibleCount = 0;

    int filterSev = m_severityFilter->currentData().toInt();
    QString searchText = m_searchBar->text();

    for (const auto& entry : m_allEntries) {
        bool pass = (filterSev == -1 || entry.severity >= filterSev);
        if (pass && !searchText.isEmpty()) {
            pass = entry.formatted.contains(searchText, Qt::CaseInsensitive);
        }
        if (pass) {
            appendFormattedLine(entry.formatted, entry.severity);
            m_visibleCount++;
        }
    }

    m_countLabel->setText(QString("%1 entries (%2 shown)")
                          .arg(m_allEntries.size()).arg(m_visibleCount));
}
