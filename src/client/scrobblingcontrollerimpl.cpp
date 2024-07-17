/*
    Copyright (C) 2022-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "scrobblingcontrollerimpl.h"

#include "serverconnection.h"

namespace PMP::Client
{
    ScrobblingControllerImpl::ScrobblingControllerImpl(ServerConnection* connection)
     : ScrobblingController(connection),
       _connection(connection)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &ScrobblingControllerImpl::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &ScrobblingControllerImpl::connectionBroken
        );

        connect(
            _connection, &ServerConnection::scrobblingProviderInfoReceived,
            this,
            [this](ScrobblingProvider provider, ScrobblerStatus status, bool enabled)
            {
                if (provider == ScrobblingProvider::LastFm
                        && (_lastFmEnabled != enabled || _lastFmStatus != status))
                {
                    _lastFmEnabled = enabled;
                    _lastFmStatus = status;
                    Q_EMIT lastFmInfoChanged();
                }
            }
        );

        connect(
            _connection, &ServerConnection::scrobblingProviderEnabledChanged,
            this,
            [this](ScrobblingProvider provider, bool enabled)
            {
                if (provider == ScrobblingProvider::LastFm && _lastFmEnabled != enabled)
                {
                    _lastFmEnabled = enabled;
                    Q_EMIT lastFmInfoChanged();
                }
            }
        );

        connect(
            _connection, &ServerConnection::scrobblerStatusChanged,
            this,
            [this](ScrobblingProvider provider, ScrobblerStatus status)
            {
                if (provider == ScrobblingProvider::LastFm && _lastFmStatus != status)
                {
                    _lastFmStatus = status;
                    Q_EMIT lastFmInfoChanged();
                }
            }
        );

        if (_connection->isConnected())
            connected();
    }

    Nullable<bool> ScrobblingControllerImpl::lastFmEnabled() const
    {
        return _lastFmEnabled;
    }

    ScrobblerStatus ScrobblingControllerImpl::lastFmStatus() const
    {
        return _lastFmStatus;
    }

    NewSimpleFuture<AnyResultMessageCode> ScrobblingControllerImpl::authenticateLastFm(
                                                                QString usernameOrEmail,
                                                                QString password)
    {
        return _connection->authenticateScrobbling(ScrobblingProvider::LastFm,
                                                   usernameOrEmail,
                                                   password);
    }

    void ScrobblingControllerImpl::setLastFmScrobblingEnabled(bool enabled)
    {
        if (enabled)
            _connection->enableScrobblingForCurrentUser(ScrobblingProvider::LastFm);
        else
            _connection->disableScrobblingForCurrentUser(ScrobblingProvider::LastFm);
    }

    void ScrobblingControllerImpl::connected()
    {
        _connection->requestScrobblingProviderInfoForCurrentUser();
    }

    void ScrobblingControllerImpl::connectionBroken()
    {
        _lastFmEnabled = null;
        _lastFmStatus = ScrobblerStatus::Unknown;
        Q_EMIT lastFmInfoChanged();
    }
}
