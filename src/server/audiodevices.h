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

#ifndef PMP_AUDIODEVICES_H
#define PMP_AUDIODEVICES_H

#include <QAudioDevice>
#include <QObject>

namespace PMP::Server
{
    class AudioDevices : public QObject
    {
        Q_OBJECT
    public:
        explicit AudioDevices(QObject* parent = nullptr);

        QAudioDevice defaultOutputDevice() const { return _defaultOutputDevice; }

    Q_SIGNALS:
        void defaultOutputDeviceChanged();

    private:
        QAudioDevice _defaultOutputDevice;
    };
}
#endif
