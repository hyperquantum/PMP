/*
    Copyright (C) 2018-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/logging.h"
//#include "common/util.h"
#include "common/version.h"

//#include "server/database.h"
//#include "server/lastfmscrobblingbackend.h"
//#include "server/serversettings.h"
//#include "server/tokenencoder.h"

#include <QAudioDevice>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QDateTime>
#include <QHostAddress>
#include <QHostInfo>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QNetworkInterface>
#include <QSslSocket>
#include <QtDebug>
#include <QtGlobal>
#include <QThread>

using namespace PMP;
//using namespace PMP::Server;

void printOutputDevices(QTextStream& out)
{
    const auto audioDevices = QMediaDevices::audioOutputs();
    for (const QAudioDevice& device : audioDevices)
    {
        out << "Device ID: " << device.id() << Qt::endl;
        out << " Description: " << device.description() << Qt::endl;
        out << " Is default: " << (device.isDefault() ? "Yes" : "No") << Qt::endl;
    }
    out << Qt::endl;
}

QAudioDevice getDefaultAudioDevice()
{
    const auto audioDevices = QMediaDevices::audioOutputs();
    for (const QAudioDevice& device : audioDevices)
    {
        if (device.isDefault())
            return device;
    }

    return {};
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Simple test program");
    QCoreApplication::setApplicationVersion(PMP_VERSION_DISPLAY);
    QCoreApplication::setOrganizationName(PMP_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(PMP_ORGANIZATION_DOMAIN);

    /* set up logging */
    Logging::enableConsoleAndTextFileLogging(false);
    Logging::setFilenameTag("T"); /* T = Test */

    QTextStream out(stdout);
    qDebug() << "Qt version:" << qVersion();

    /*
    for (int i = 0; i < 10; ++i)
    {
        auto encoded = TokenEncoder::encodeToken("Flub");
        qDebug() << "encoded:" << encoded;

        auto decoded = TokenEncoder::decodeToken(encoded);
        qDebug() << "decoded:" << decoded;
    }
    */

    /*
    qDebug() << "Local hostname:" << QHostInfo::localHostName();

    for (const QHostAddress& address : QNetworkInterface::allAddresses())
    {
        qDebug() << address.toString();
    }
    */
    
    /*
    qDebug() << "SSL version:" << QSslSocket::sslLibraryVersionNumber();
    qDebug() << "SSL version string:" << QSslSocket::sslLibraryVersionString();
    */

    //return 0;

    //auto lastFm = new PMP::LastFmScrobblingBackend();

    /*
    QObject::connect(
        lastFm, &PMP::LastFmScrobblingBackend::receivedAuthenticationReply,
        &app, &QCoreApplication::quit
    );*/

    //lastFm->authenticateWithCredentials("xxxxxxxxxxxx", "xxxxxxxxxxxxxxx");

    //lastFm->setSessionKey("xxxxxxxxxxxxx");

    /*
    lastFm->doScrobbleCall(
        QDateTime(QDate(2018, 03, 05), QTime(15, 57), Qt::LocalTime),
        "Title",
        QString("Artist"),
        "Album",
        3 * 60 + 39
    );*/

    //return app.exec();

    /*
    DatabaseConnectionSettings databaseConnectionSettings;
    databaseConnectionSettings.hostname = "localhost";
    databaseConnectionSettings.username = "root";
    databaseConnectionSettings.password = "xxxxxxxxxxx";

    if (!Database::init(out, databaseConnectionSettings))
    {
        qWarning() << "could not initialize database";
        return 1;
    }

    auto database = Database::getDatabaseForCurrentThread();
    if (!database)
    {
        qWarning() << "could not get database connection for this thread";
        return 1;
    }

    bool ok;
    auto preferences = database->getUserDynamicModePreferences(1, &ok);
    if (!ok)
    {
        qWarning() << "failed to load user preferences";
        return 1;
    }

    qDebug() << "Enable dynamic mode:" << (preferences.dynamicModeEnabled ? "Y" : "N");
    qDebug() << "Non-repetition interval:"
             << preferences.trackRepetitionAvoidanceIntervalSeconds << "seconds";
    */

    /*
    auto d1 = QDateTime::currentDateTimeUtc();
    QThread::msleep(200);
    auto d2 = QDateTime::currentDateTimeUtc();
    QThread::msleep(200);
    auto d3 = QDateTime::currentDateTimeUtc();

    qDebug() << "d1-d2:" << d2.msecsTo(d1);
    qDebug() << "d2-d3:" << d3.msecsTo(d2);
    qDebug() << d1.toString(Qt::ISODateWithMs)
             << d2.toString(Qt::ISODateWithMs)
             << d3.toString(Qt::ISODateWithMs);

    auto msSinceEpoch = d1.toMSecsSinceEpoch();
    auto d1R = QDateTime::fromMSecsSinceEpoch(msSinceEpoch, Qt::UTC);

    qDebug() << d1R.toString(Qt::ISODateWithMs);
    */

    auto audioOutput1 = new QAudioOutput();
    auto audioOutput2 = new QAudioOutput();

    QObject::connect(
        audioOutput1, &QAudioOutput::deviceChanged,
        &app,
        [&out, audioOutput1]()
        {
            out << "output 1 device has changed to: " << audioOutput1->device().id() << Qt::endl;
        }
    );
    QObject::connect(
        audioOutput2, &QAudioOutput::deviceChanged,
        &app,
        [&out, audioOutput2]()
        {
            out << "output 2 device has changed to: " << audioOutput2->device().id() << Qt::endl;
        }
        );

    printOutputDevices(out);
    auto defaultDevice = getDefaultAudioDevice();
    out << "default device: " << defaultDevice.id() << Qt::endl << Qt::endl;

    QMediaDevices mediaDevices;
    QObject::connect(
        &mediaDevices, &QMediaDevices::audioOutputsChanged,
        &app,
        [&out, &defaultDevice, &audioOutput1]()
        {
            out << "<<< AUDIO OUTPUT DEVICES CHANGED >>>" << Qt::endl << Qt::endl;
            printOutputDevices(out);
            auto newDefaultDevice = getDefaultAudioDevice();

            if (newDefaultDevice.id() != defaultDevice.id())
            {
                out << "default device has changed to: " << newDefaultDevice.id() << Qt::endl;
                out << Qt::endl;

                defaultDevice = newDefaultDevice;
                audioOutput1->setDevice(newDefaultDevice);
            }
        }
    );

    auto track1 = "track1.flac";
    auto track2 = "track2.mp3";
    auto track3 = "track3.mp3";
    auto track4 = "track4.mp3";

    audioOutput1->setVolume(85);
    audioOutput2->setVolume(80);

    auto player1 = new QMediaPlayer;
    player1->setAudioOutput(audioOutput1);
    player1->setSource(QUrl::fromLocalFile(track3));

    auto player2 = new QMediaPlayer;
    player2->setAudioOutput(audioOutput2);

    out << "output 1 device: " << audioOutput1->device().id() << Qt::endl;
    out << "output 2 device: " << audioOutput2->device().id() << Qt::endl;
    out << Qt::endl;

    player1->play();

    return app.exec();
}
