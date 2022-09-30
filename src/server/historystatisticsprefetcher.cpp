/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "historystatisticsprefetcher.h"

#include "hashidregistrar.h"
#include "history.h"
#include "users.h"

#include <QRandomGenerator>
#include <QtDebug>
#include <QTimer>

namespace PMP
{
    HistoryStatisticsPrefetcher::HistoryStatisticsPrefetcher(QObject* parent,
                                                         HashIdRegistrar* hashIdRegistrar,
                                                         History* history,
                                                         Users* users)
     : QObject(parent),
       _hashIdRegistrar(hashIdRegistrar),
       _history(history),
       _users(users),
       _timer(new QTimer(this))
    {
        connect(_timer, &QTimer::timeout,
                this, &HistoryStatisticsPrefetcher::timeout);

        _timer->start(1000);
    }

    void HistoryStatisticsPrefetcher::timeout()
    {
        if (_index >= 0 && _index < _hashIds.size())
        {
            prefetchHash(_hashIds[_index]);
            _index++;
        }
        else
        {
            prepareList();
        }
    }

    void HistoryStatisticsPrefetcher::prepareList()
    {
        _hashIds = _hashIdRegistrar->getAllIdsLoaded();

        if (_hashIds.isEmpty())
        {
            qDebug() << "HistoryStatisticsPrefetcher: hash list is empty";

            _index = -1;
            _timer->setInterval(qMin(10 * 60 * 1000 /* 10 min */,
                                     _timer->interval() * 2));
        }
        else
        {
            qDebug() << "HistoryStatisticsPrefetcher: hash list size is"
                     << _hashIds.size();

            _index = 0;
            _timer->setInterval(200);
        }
    }

    void HistoryStatisticsPrefetcher::prefetchHash(uint hashId)
    {
        bool started = _history->scheduleUserStatsFetchingIfMissing(hashId, /* user */ 0);

        if (started)
        {
            qDebug() << "HistoryStatisticsPrefetcher: started fetch for hash ID"
                     << hashId;
        }
    }
}
