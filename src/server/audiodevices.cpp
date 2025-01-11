/*
    Copyright (C) 2024, Kevin André <hyperquantum@gmail.com>

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

#include "audiodevices.h"

#include <QAudioDevice>
#include <QMediaDevices>
#include <QtDebug>

namespace PMP::Server
{
    AudioDevices::AudioDevices(QObject* parent)
        : QObject{parent}
    {
        auto* mediaDevices = new QMediaDevices(this);

        _defaultOutputDevice = QMediaDevices::defaultAudioOutput();

        qDebug() << "AudioDevices: initialized default audio output device to:"
                 << _defaultOutputDevice.description();

        connect(
            mediaDevices, &QMediaDevices::audioOutputsChanged,
            this,
            [this]()
            {
                QAudioDevice newDefaultDevice = QMediaDevices::defaultAudioOutput();

                if (newDefaultDevice != _defaultOutputDevice)
                {
                    qDebug() << "AudioDevices: default audio output device has changed;"
                             << "old:" << _defaultOutputDevice.description()
                             << "; new:" << newDefaultDevice.description();

                    _defaultOutputDevice = newDefaultDevice;
                    Q_EMIT defaultOutputDeviceChanged();
                }
            }
        );
    }
}
