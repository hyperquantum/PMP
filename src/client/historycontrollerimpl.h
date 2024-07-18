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

#ifndef PMP_HISTORYCONTROLLERIMPL_H
#define PMP_HISTORYCONTROLLERIMPL_H

#include "historycontroller.h"

namespace PMP::Client
{
    class ServerConnection;

    class HistoryControllerImpl : public HistoryController
    {
        Q_OBJECT
    public:
        HistoryControllerImpl(ServerConnection* connection);

        void sendPlayerHistoryRequest(int limit) override;

        NewFuture<HistoryFragment, AnyResultMessageCode> getPersonalTrackHistory(
            LocalHashId hashId, uint userId,
            int limit, uint startId = 0) override;

    private Q_SLOTS:
        void connected();
        void connectionBroken();

    private:
        ServerConnection* _connection;
    };
}
#endif
