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

#ifndef PMP_USERFORSTATISTICSDISPLAY_H
#define PMP_USERFORSTATISTICSDISPLAY_H

#include "common/nullable.h"

#include <QObject>

namespace PMP::Client
{
    class ServerInterface;
}

namespace PMP
{
    class UserForStatisticsDisplay : public QObject
    {
        Q_OBJECT
    public:
        UserForStatisticsDisplay(QObject* parent,
                                 Client::ServerInterface* serverInterface);

        Nullable<quint32> userId() const;
        Nullable<bool> isPersonal() const;

        void setPersonal();
        void setPublic();

    Q_SIGNALS:
        void userChanged();

    private:
        Client::ServerInterface* _serverInterface;
        quint32 _userId { 0 };
        bool _unknown { true };
    };
}
#endif
