#pragma once
#include "ILogSink.hpp"
#include "LogMessage.hpp"
#include <QObject>
#include <QString>

class QtSinkImpl : public QObject, public ILogSink
{
    Q_OBJECT
public:
    explicit QtSinkImpl(QObject* parent = nullptr);
    ~QtSinkImpl() override = default;

    void write(const LogMessage& msg) override;
    std::string getName() const override { return "qt"; }

signals:
    void newLogMessage(const QString& formatted, const QString& appName,
                       const QString& context, const QString& text,
                       int severity, qint64 timestamp);
};
