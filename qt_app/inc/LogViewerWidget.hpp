#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

class LogViewerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogViewerWidget(QWidget* parent = nullptr);

public slots:
    void onLogMessage(const QString& formatted, const QString& appName,
                      const QString& context, const QString& text,
                      int severity, qint64 timestamp);

private slots:
    void clearLog();
    void applyFilter();

private:
    void appendFormattedLine(const QString& formatted, int severity);

    QTextEdit*     m_logArea;
    QComboBox*     m_severityFilter;
    QLineEdit*     m_searchBar;
    QPushButton*   m_clearBtn;
    QCheckBox*     m_autoScrollCheck;
    QLabel*        m_countLabel;

    int m_totalCount  = 0;
    int m_visibleCount = 0;

    // Stored for re-filtering
    struct LogEntry {
        QString formatted;
        int severity;
    };
    QVector<LogEntry> m_allEntries;
    static constexpr int MAX_ENTRIES = 10000;
};
