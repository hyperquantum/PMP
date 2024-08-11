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

#ifndef PMP_HISTORYCONTROLLER_H
#define PMP_HISTORYCONTROLLER_H

#include "common/future.h"
#include "common/playerhistorytrackinfo.h"
#include "common/resultmessageerrorcode.h"

#include "historyentry.h"

#include <QObject>
#include <QVector>

namespace PMP::Client
{
    class HistoryController : public QObject
    {
        Q_OBJECT
    public:
        virtual ~HistoryController() {}

        virtual void sendPlayerHistoryRequest(int limit) = 0;

        virtual Future<HistoryFragment, AnyResultMessageCode> getPersonalTrackHistory(
            LocalHashId hashId, uint userId,
            int limit, uint startId = 0) = 0;

    Q_SIGNALS:
        void receivedPlayerHistoryEntry(PMP::PlayerHistoryTrackInfo track);
        void receivedPlayerHistory(QVector<PMP::PlayerHistoryTrackInfo> tracks);

    protected:
        explicit HistoryController(QObject* parent) : QObject(parent) {}
    };
}
#endif
