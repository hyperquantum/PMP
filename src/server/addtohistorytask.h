/*
    Copyright (C) 2016-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_ADDTOHISTORYTASK_H
#define PMP_ADDTOHISTORYTASK_H

#include "playerhistoryentry.h"

#include <QObject>
#include <QRunnable>
#include <QSharedPointer>

namespace PMP
{
    class Resolver;

    class AddToHistoryTask : public QObject, public QRunnable
    {
        Q_OBJECT
    public:
        AddToHistoryTask(Resolver* resolver, QSharedPointer<PlayerHistoryEntry> entry);

        void run();

    Q_SIGNALS:
        void updatedHashUserStats(uint hashID, quint32 user,
                                  QDateTime previouslyHeard, qint16 score);

    private:
        Resolver* _resolver;
        QSharedPointer<PlayerHistoryEntry> _entry;
    };
}
#endif
