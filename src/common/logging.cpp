/*
    Copyright (C) 2016-2017, Kevin Andre <hyperquantum@gmail.com>

    This file is part of PMP (Party Music Player).

    PMP is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    PMP is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along
    with PMP.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "logging.h"

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QTextStream>
#include <QtGlobal>
#include <QTime>

namespace PMP {

    /* ========================== LoggerBase ========================== */

    class LoggerBase {
    protected:
        static QString stripSourcefilePath(QString file);
    };

    /*! transform "/long/path/name/src/common/xyz.cpp" to "common/xyz.cpp" */
    QString LoggerBase::stripSourcefilePath(QString file) {
        /* file refers to a source file not present on the machine we are running on, */
        /* so we will use string operations instead of QFileInfo */

        /* writing a loop ourselves is faster than calling lastIndexOf() multiple times */
        int lastOne = -1;
        for (int i = file.size() - 1; i >= 0; --i) {
            if (file[i] != '/' && file[i] != '\\') continue; /* not a separator */

            if (lastOne >= 0) {
                /* we found the second last one */
                return file.mid( i + 1);
            }

            lastOne = i; /* we found the last one */
        }

        if (lastOne >= 0) return file.mid(lastOne + 1);
        return file;
    }

    /* ========================== ConsoleLogger ========================== */

    class ConsoleLogger : LoggerBase {
    public:
        ConsoleLogger() : _mutex(QMutex::Recursive), _out(stdout) {}

        void logMessage(QtMsgType type, const QMessageLogContext& context,
                        const QString& msg);

    private:
        QMutex _mutex;
        QTextStream _out;
    };

    void ConsoleLogger::logMessage(QtMsgType type, const QMessageLogContext& context,
                                   const QString& msg)
    {
        QString time = QTime::currentTime().toString(Qt::ISODate); /* HH:mm:ss */
        QString sourcefile = stripSourcefilePath(context.file);

        QString locationText =
            sourcefile % ":" % QString::number(context.line).leftJustified(6, '-');

        QString output;
        switch (type) {
            case QtDebugMsg:
                output = time % " [D] " % locationText % msg % "\n";
                break;
            /* QtInfoMsg is only available in Qt 5.5 and later
            case QtInfoMsg:
                output = time % " [I] " % locationText % msg % "\n";
                break;
            */
            case QtWarningMsg:
                output = time % " [Warning] " % locationText % msg % "\n";
                break;
            case QtCriticalMsg:
                output = time % " [CRITICAL] " % locationText % msg % "\n";
                break;
            case QtFatalMsg:
                output = time % " [FATAL] " % locationText % msg % "\n";
                break;
            default:
                output = time % " [???] " % locationText % msg % "\n";
                break;
        }

        QMutexLocker lock(&_mutex);
        _out << output << flush;
    }

    /* ========================== TextFileLogger ========================== */

    class TextFileLogger : LoggerBase {
    public:
        TextFileLogger() : _initialized(false), _appPid(0) {}

        bool init();
        bool initialized() { return _initialized; }

        void logMessage(QtMsgType type, const QMessageLogContext& context,
                        const QString& msg);

        void cleanupOldLogfiles();

    private:
        void writeToLogFile(QString const& output);

        bool _initialized;
        qint64 _appPid;
        QByteArray _byteOrderMark;
        QString _logDir;
    };

    bool TextFileLogger::init() {
        _initialized = false;
        _appPid = QCoreApplication::applicationPid();

        char bom[3] = { char(0xEF), char(0xBB), char(0xBF)}; /* UTF-8 byte order mark */
        _byteOrderMark = QByteArray(bom, 3);

        if (QDir::temp().mkpath("PMP-logs")) {
            _logDir = QDir::temp().absolutePath() + "/PMP-logs";
        }
        else {
            return false;
        }

        _initialized = true;
        return true;
    }

    void TextFileLogger::writeToLogFile(const QString& output) {
        QDate today = QDate::currentDate();

        QString logFile =
            _logDir + "/" + today.toString(Qt::ISODate)
                + "-P" + QString::number(_appPid) + ".txt";

        QFile file(logFile);
        bool existed = file.exists();

        bool opened =
            file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

        if (!opened) { /* could not create/open logfile */
            /* TODO : handle this */
            return;
        }

        if (!existed) {
#if defined(Q_OS_WIN)
            file.write(_byteOrderMark); /* Windows needs a byte order mark */
#endif
            /* TODO: write PMP version */
            file.write("\n", 1);
        }

        file.write(output.toUtf8());
        file.close();
    }

    void TextFileLogger::logMessage(QtMsgType type, const QMessageLogContext& context,
                                    const QString& msg)
    {
        QString time = QTime::currentTime().toString(Qt::ISODate); /* HH:mm:ss */
        QString sourcefile = stripSourcefilePath(context.file);

        QString locationText =
            sourcefile % ":" % QString::number(context.line).leftJustified(6, '-');

        QString output;
        switch (type) {
            case QtDebugMsg:
                output = time % " [D] " % locationText % msg % "\n";
                break;
            /* QtInfoMsg is only available in Qt 5.5 and later
            case QtInfoMsg:
                output = time % " [I] " % locationText % msg % "\n";
                break;
            */
            case QtWarningMsg:
                output = time % " [Warning] " % locationText % msg % "\n";
                break;
            case QtCriticalMsg:
                output = time % " [CRITICAL] " % locationText % msg % "\n";
                break;
            case QtFatalMsg:
                output = time % " [FATAL] " % locationText % msg % "\n";
                break;
            default:
                output = time % " [???] " % locationText % msg % "\n";
                break;
        }

        writeToLogFile(output);
    }

    void TextFileLogger::cleanupOldLogfiles() {
        if (!_initialized) return;

        QDir dir(_logDir);
        if (!dir.exists()) { return; }

        QDateTime threshhold = QDateTime(QDate::currentDate().addDays(-6));
        QRegularExpression regex("^\\d{4}-\\d{2}-\\d{2}-");

        auto files =
            dir.entryInfoList(
                QDir::Files | QDir::NoSymLinks | QDir::Readable | QDir::Writable
            );

        Q_FOREACH(auto file, files) {
            if (file.lastModified() >= threshhold) continue;
            if (file.suffix() != "txt") continue;
            if (!regex.match(file.baseName()).hasMatch()) continue;

            //qDebug() << "deleting old logfile:" << file.fileName();
            QFile::remove(file.absoluteFilePath());
        }
    }

    /* ========================== Logging ========================== */

    ConsoleLogger globalConsoleLogger; /* global instance */
    TextFileLogger globalTextFileLogger; /* global instance */

    void logToTextFile(QtMsgType type, const QMessageLogContext& context,
                       const QString& msg)
    {
        globalTextFileLogger.logMessage(type, context, msg);

        if (type == QtFatalMsg) { abort(); }
    }

    void logToTextFileAndConsole(QtMsgType type, const QMessageLogContext& context,
                                 const QString& msg)
    {
        globalConsoleLogger.logMessage(type, context, msg);
        globalTextFileLogger.logMessage(type, context, msg);

        if (type == QtFatalMsg) { abort(); }
    }

    void logToConsole(QtMsgType type, const QMessageLogContext& context,
                      const QString& msg)
    {
        globalConsoleLogger.logMessage(type, context, msg);

        if (type == QtFatalMsg) { abort(); }
    }

    void Logging::enableTextFileLogging(bool alsoPrintToStdOut) {
        if (!globalTextFileLogger.init()) return;

        if (alsoPrintToStdOut)
            qInstallMessageHandler(logToTextFileAndConsole);
        else
            qInstallMessageHandler(logToTextFile);
    }

    void Logging::enableConsoleOnlyLogging() {
        qInstallMessageHandler(logToConsole);
    }

    void Logging::cleanupOldLogfiles() {
        globalTextFileLogger.cleanupOldLogfiles();
    }
}
