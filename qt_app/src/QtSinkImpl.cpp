#include "QtSinkImpl.hpp"
#include <magic_enum/magic_enum.hpp>
#include <sstream>

QtSinkImpl::QtSinkImpl(QObject* parent)
    : QObject(parent)
{
}

void QtSinkImpl::write(const LogMessage& msg)
{
    std::ostringstream os;
    os << msg;
    QString formatted = QString::fromStdString(os.str());
    QString appName   = QString::fromStdString(msg.appName);
    QString context   = QString::fromStdString(msg.context);
    QString text      = QString::fromStdString(msg.text);
    int severity      = static_cast<int>(msg.severity);
    qint64 ts         = static_cast<qint64>(msg.timestamp);

    emit newLogMessage(formatted, appName, context, text, severity, ts);
}
