/*
    Copyright (C) 2016, Kevin Andre <hyperquantum@gmail.com>

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
#include <QStringBuilder>
#include <QtGlobal>
#include <QTime>

namespace PMP {

    /* ========================== TextFileLogger ========================== */

    class TextFileLogger {
    public:
        TextFileLogger() : _initialized(false), _appPid(0) {}

        bool init();

        void logMessage(QtMsgType type, const QMessageLogContext& context,
                        const QString& msg);

    private:
        static QString stripSourcefilePath(QString file);

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

    void TextFileLogger::logMessage(QtMsgType type, const QMessageLogContext& context,
                                    const QString& msg)
    {
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

        QString time = QTime::currentTime().toString(Qt::ISODate); /* HH:mm:ss */
        QString sourcefile = stripSourcefilePath(context.file);

        auto locationText =
            sourcefile % ":" % QString::number(context.line).leftJustified(6, '-');

        switch (type) {
            case QtDebugMsg:
                file.write(
                    QString(time % " [D] " % locationText % msg % "\n").toUtf8()
                );
                file.close();
                break;
            /* QtInfoMsg is only available in Qt 5.5 and later
            case QtInfoMsg:
                file.write(
                    QString(time % " [I] " % locationText % msg % "\n").toUtf8()
                );
                file.close();
                break;
            */
            case QtWarningMsg:
                file.write(
                    QString(time % " [Warning] " % locationText % msg % "\n").toUtf8()
                );
                file.close();
                break;
            case QtCriticalMsg:
                file.write(
                    QString(time % " [CRITICAL] " % locationText % msg % "\n").toUtf8()
                );
                file.close();
                break;
            case QtFatalMsg:
                file.write(
                    QString(time % " [FATAL] " % locationText % msg % "\n").toUtf8()
                );
                file.close();
                abort();
        }
    }

    /*! transform "/long/path/name/src/common/xyz.cpp" to "common/xyz.cpp" */
    QString TextFileLogger::stripSourcefilePath(QString file) {
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

    /* ========================== Logging ========================== */

    TextFileLogger globalTextFileLogger; /* global instance */

    void logToTextFile(QtMsgType type, const QMessageLogContext& context,
                       const QString& msg)
    {
        globalTextFileLogger.logMessage(type, context, msg);
    }

    void Logging::enableTextFileLogging() {
        if (!globalTextFileLogger.init()) return;

        qInstallMessageHandler(logToTextFile);
    }

}
