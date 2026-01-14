/*
    Copyright (C) 2018-2025, Kevin André <hyperquantum@gmail.com>

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
#include <QFile>
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
    QTextStream err(stderr);
    qDebug() << "Qt version:" << qVersion();

    auto audioOutput1 = new QAudioOutput();
    auto audioOutput2 = new QAudioOutput();

    QObject::connect(
        audioOutput1, &QAudioOutput::deviceChanged,
        &app,
        [&out, audioOutput1]()
        {
            out << "output 1 device has changed to:" << audioOutput1->device().id()
                << "(" << audioOutput1->device().description() << ")" << Qt::endl;
        }
    );
    QObject::connect(
        audioOutput2, &QAudioOutput::deviceChanged,
        &app,
        [&out, audioOutput2]()
        {
            out << "output 2 device has changed to:" << audioOutput2->device().id()
                << "(" << audioOutput2->device().description() << ")" << Qt::endl;
        }
    );

    printOutputDevices(out);
    auto defaultDevice = getDefaultAudioDevice();
    out << "default device: " << defaultDevice.id()
        << "(" << defaultDevice.description() << ")" << Qt::endl;

    out << Qt::endl;

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

    auto track = "track3.mp3";
    if (!QFile::exists(track))
    {
        err << "Error: file not found:" << track << Qt::endl;
        return 1;
    }

    audioOutput1->setVolume(85);
    audioOutput2->setVolume(80);

    auto player1 = new QMediaPlayer;
    player1->setAudioOutput(audioOutput1);
    player1->setSource(QUrl::fromLocalFile(track));

    auto player2 = new QMediaPlayer;
    player2->setAudioOutput(audioOutput2);

    out << "output 1 device: " << audioOutput1->device().id() << Qt::endl;
    out << "output 2 device: " << audioOutput2->device().id() << Qt::endl;
    out << Qt::endl;

    player1->play();

    return app.exec();
}
