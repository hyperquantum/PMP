/*
    Copyright (C) 2016-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/version.h"

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
                return file.mid(i + 1);
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
        QString generateOutputText(QtMsgType type, const QMessageLogContext& context,
                                   const QString& msg);

        QMutex _mutex;
        QTextStream _out;
    };

    void ConsoleLogger::logMessage(QtMsgType type, const QMessageLogContext& context,
                                   const QString& msg)
    {
        QString output = generateOutputText(type, context, msg);

        QMutexLocker lock(&_mutex);
        _out << output << flush;
    }

    QString ConsoleLogger::generateOutputText(QtMsgType type,
                                              const QMessageLogContext& context,
                                              const QString& msg)
    {
        QString time = QTime::currentTime().toString(Qt::ISODateWithMs);/* HH:mm:ss.zzz */
        QString sourcefile = stripSourcefilePath(context.file);

        QString locationText =
            sourcefile % ":" % QString::number(context.line).leftJustified(6, '-');

        QString output;
        switch (type) {
            case QtDebugMsg:
                return time % " [D] " % locationText % msg % "\n";

            case QtInfoMsg:
                return time % " [Info] " % locationText % msg % "\n";

            case QtWarningMsg:
                return time % " [Warning] " % locationText % msg % "\n";

            case QtCriticalMsg:
                return time % " [CRITICAL] " % locationText % msg % "\n";

            case QtFatalMsg:
                return time % " [FATAL] " % locationText % msg % "\n";
        }

        return time % " [???] " % locationText % msg % "\n";
    }

    /* ========================== TextFileLogger ========================== */

    class TextFileLogger : LoggerBase {
    public:
        TextFileLogger() : _mutex(QMutex::Recursive), _initialized(false), _appPid(0) {}

        bool init();
        bool initialized();

        void logMessage(QtMsgType type, const QMessageLogContext& context,
                        const QString& msg);

        void setFilenameTag(QString tag);

        void cleanupOldLogfiles();

    private:
        QString generateOutputText(QtMsgType type, const QMessageLogContext& context,
                                   const QString& msg);

        void writeToLogFile(QString const& output);

        void writeFileHeader(QFile& file);

        QMutex _mutex;
        bool _initialized;
        qint64 _appPid;
        QByteArray _byteOrderMark;
        QString _logDir;
        QString _tag;
    };

    bool TextFileLogger::initialized() {
        QMutexLocker lock(&_mutex);
        return _initialized;
    }

    bool TextFileLogger::init() {
        QMutexLocker lock(&_mutex);
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
        QMutexLocker lock(&_mutex);

        if (!QDir().mkpath(_logDir)) { /* could not create missing log directory */
            /* TODO : handle this */
            return;
        }

        QDate today = QDate::currentDate();
        QString logFile =
            _logDir + "/" + today.toString(Qt::ISODate)
                + (_tag.isEmpty() ? "" : ("-" + _tag))
                + "-P" + QString::number(_appPid)
                + ".txt";

        QFile file(logFile);
        bool existed = file.exists();

        bool opened =
            file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

        if (!opened) { /* could not create/open logfile */
            /* TODO : handle this */
            return;
        }

        if (!existed) {
            writeFileHeader(file);
        }

        file.write(output.toUtf8());
        file.close();
    }

    void TextFileLogger::writeFileHeader(QFile& file) {
#if defined(Q_OS_WIN)
         /* Windows needs a byte order mark */
        file.write(_byteOrderMark);
#endif

        /* write PMP version */
        QString firstLine =
                "# "
                "Party Music Player " PMP_VERSION_DISPLAY
                "\n";

        file.write(firstLine.toUtf8());
    }

    void TextFileLogger::logMessage(QtMsgType type, const QMessageLogContext& context,
                                    const QString& msg)
    {
        QString output = generateOutputText(type, context, msg);

        writeToLogFile(output);
    }

    QString TextFileLogger::generateOutputText(QtMsgType type,
                                               const QMessageLogContext& context,
                                               const QString& msg)
    {
        QString time = QTime::currentTime().toString(Qt::ISODateWithMs);/* HH:mm:ss.zzz */
        QString sourcefile = stripSourcefilePath(context.file);

        QString locationText =
            sourcefile % ":" % QString::number(context.line).leftJustified(6, '-');

        QString output;
        switch (type) {
            case QtDebugMsg:
                return time % " [D] " % locationText % msg % "\n";

            case QtInfoMsg:
                return time % " [I] " % locationText % msg % "\n";

            case QtWarningMsg:
                return time % " [Warning] " % locationText % msg % "\n";

            case QtCriticalMsg:
                return time % " [CRITICAL] " % locationText % msg % "\n";

            case QtFatalMsg:
                return time % " [FATAL] " % locationText % msg % "\n";
        }

        return time % " [???] " % locationText % msg % "\n";
    }

    void TextFileLogger::setFilenameTag(QString tag) {
        QMutexLocker lock(&_mutex);

        /* cut off an initial dash character, because we put one there automatically */
        if (tag.startsWith("-"))
            tag = tag.mid(1);

        /* cut off a dash character at the end, because we put one there automatically */
        if (tag.endsWith("-"))
            tag = tag.left(tag.size() - 1);

        _tag = tag;
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

    static ConsoleLogger globalConsoleLogger; /* global instance */
    static TextFileLogger globalTextFileLogger; /* global instance */

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

    void logToTextFileAndReducedConsole(QtMsgType type, const QMessageLogContext& context,
                                        const QString& msg)
    {
        if (type != QtDebugMsg)
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

    void Logging::enableTextFileOnlyLogging() {
        if (!globalTextFileLogger.init()) return;

        qInstallMessageHandler(logToTextFile);
    }

    void Logging::enableConsoleAndTextFileLogging(bool reducedConsoleOutput) {
        if (!globalTextFileLogger.init()) return;

        if (reducedConsoleOutput)
            qInstallMessageHandler(logToTextFileAndReducedConsole);
        else
            qInstallMessageHandler(logToTextFileAndConsole);
    }

    void Logging::enableConsoleOnlyLogging() {
        qInstallMessageHandler(logToConsole);
    }

    void Logging::setFilenameTag(QString suffix) {
        globalTextFileLogger.setFilenameTag(suffix);
    }

    void Logging::cleanupOldLogfiles() {
        globalTextFileLogger.cleanupOldLogfiles();
    }
}
