/*
    Copyright (C) 2021-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_GENERALCONTROLLERIMPL_H
#define PMP_GENERALCONTROLLERIMPL_H

#include "generalcontroller.h"

#include "common/lazypromisedvalue.h"

namespace PMP
{
    class ServerConnection;

    class GeneralControllerImpl : public GeneralController
    {
        Q_OBJECT
    public:
        explicit GeneralControllerImpl(ServerConnection* connection);

        ServerHealthStatus serverHealth() const override;

        qint64 clientClockTimeOffsetMs() const override;

        SimpleFuture<ResultMessageErrorCode> reloadServerSettings() override;

        Future<VersionInfo, ResultMessageErrorCode> getServerVersionInfo() override;

    public Q_SLOTS:
        void shutdownServer() override;

    private Q_SLOTS:
        void connected();
        void connectionBroken();
        void serverHealthReceived();
        void receivedClientClockTimeOffset(qint64 clientClockTimeOffsetMs);

    private:
        ServerConnection* _connection;
        qint64 _clientClockTimeOffsetMs { 0 };
        ServerHealthStatus _serverHealthStatus;
        LazyPromisedValue<VersionInfo, ResultMessageErrorCode> _serverVersionInfo;
    };
}
#endif
