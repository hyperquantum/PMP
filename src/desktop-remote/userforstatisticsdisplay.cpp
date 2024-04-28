/*
    Copyright (C) 2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "userforstatisticsdisplay.h"

#include "client/authenticationcontroller.h"
#include "client/playercontroller.h"
#include "client/serverinterface.h"

#include <QtDebug>

using namespace PMP::Client;

namespace PMP
{
    UserForStatisticsDisplayImpl::UserForStatisticsDisplayImpl(QObject *parent,
                                                         ServerInterface* serverInterface)
     : UserForStatisticsDisplay(parent),
        _serverInterface(serverInterface)
    {
        auto* playerController = &_serverInterface->playerController();

        auto mode = playerController->playerMode();
        if (mode == PlayerMode::Personal)
        {
            _userId = _serverInterface->authenticationController().userLoggedInId();
            _unknown = false;
        }
        else if (mode == PlayerMode::Public)
        {
            _userId = 0;
            _unknown = false;
        }
        else
        {
            _unknown = true;
        }

        qDebug() << "UserForStatisticsDisplay: user ID initialized to:" << _userId;

        connect(
            playerController, &PlayerController::playerModeChanged,
            this,
            [this](PlayerMode playerMode, quint32 personalModeUserId)
            {
                uint newUserId = 0;
                bool newUnknown = playerMode == PlayerMode::Unknown;

                if (playerMode == PlayerMode::Personal)
                    newUserId = personalModeUserId;

                if (newUserId == _userId && newUnknown == _unknown)
                    return; /* no change */

                _userId = newUserId;
                _unknown = newUnknown;

                qDebug() << "UserForStatisticsDisplay: mode changed: user ID changed to:"
                         << _userId;

                Q_EMIT userChanged();
            }
        );
    }

    Nullable<quint32> UserForStatisticsDisplayImpl::userId() const
    {
        if (_unknown)
            return null;

        return _userId;
    }

    Nullable<bool> UserForStatisticsDisplayImpl::isPersonal() const
    {
        if (_unknown)
            return null;

        return _userId > 0;
    }

    void UserForStatisticsDisplayImpl::setPersonal()
    {
        auto* authenticationController = &_serverInterface->authenticationController();
        auto loggedInUserId = authenticationController->userLoggedInId();

        if (_userId > 0 && _userId == loggedInUserId && !_unknown)
            return; /* no change */

        _userId = loggedInUserId;
        _unknown = false;

        qDebug() << "UserForStatisticsDisplay: set to personal: user ID changed to:"
                 << _userId;

        Q_EMIT userChanged();
    }

    void UserForStatisticsDisplayImpl::setPublic()
    {
        if (_userId == 0 && !_unknown)
            return; /* no change */

        _userId = 0;
        _unknown = false;

        qDebug() << "UserForStatisticsDisplay: set to public: user ID changed to:"
                 << _userId;

        Q_EMIT userChanged();
    }
}
