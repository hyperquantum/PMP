/*
    Copyright (C) 2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "volumemediator.h"

#include "playercontroller.h"

#include <QtDebug>

namespace PMP::Client
{
    VolumeMediator::VolumeMediator(QObject *parent, PlayerController* playerController)
     : QObject(parent),
        _playerController(playerController),
        _volumeRequested(-1),
        _volumeReceived(playerController->volume())
    {
        qDebug() << "VolumeMediator: initial server volume:" << _volumeReceived;

        connect(_playerController, &PlayerController::volumeChanged,
                this, &VolumeMediator::volumeUpdateReceived);
    }

    Nullable<int> VolumeMediator::volume() const
    {
        if (_volumeRequested >= 0)
            return _volumeRequested;

        if (_volumeReceived >= 0)
            return _volumeReceived;

        return null;
    }

    void VolumeMediator::setVolume(int volume)
    {
        Q_ASSERT_X(volume >= 0, "VolumeMediator::setVolume", "volume is negative");
        Q_ASSERT_X(volume <= 100, "VolumeMediator::setVolume", "volume is over 100");

        qDebug() << "VolumeMediator::setVolume called with value" << volume;

        if (this->volume() == volume)
            return; /* no-op */

        if (volume == _volumeReceived && !_requestToSend)
        {
            _volumeRequested = volume;
            emitVolumeChanged();
            return;
        }

        _volumeRequested = volume;

        if (!_requestToSend)
        {
            _requestToSend = true;
            QTimer::singleShot(200, this, &VolumeMediator::sendVolumeChangeRequest);
        }

        emitVolumeChanged();
    }

    void VolumeMediator::sendVolumeChangeRequest()
    {
        qDebug() << "VolumeMediator: sending request for changing volume to"
                 << _volumeRequested;

        _requestToSend = false;
        _playerController->setVolume(_volumeRequested);
    }

    void VolumeMediator::volumeUpdateReceived()
    {
        auto volumeReceived = _playerController->volume();

        qDebug() << "VolumeMediator: received server volume:" << volumeReceived;

        _volumeReceived = volumeReceived;

        if (volumeReceived < 0)
            return; /* volume unknown */

        if (!_requestToSend)
        {
            if (volumeReceived != _volumeRequested)
            {
                _volumeRequested = volumeReceived;
                emitVolumeChanged();
            }

            return;
        }

        if (volumeReceived == _volumeRequested)
        {
            /* cancel request */
            _requestToSend = false;
            return;
        }

        /* do not signal the volume change; we will request another value anyway */
    }

    void VolumeMediator::emitVolumeChanged()
    {
        qDebug() << "VolumeMediator: emitting volumeChanged signal";
        Q_EMIT volumeChanged();
    }
}
