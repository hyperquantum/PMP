/*
    Copyright (C) 2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_GENERALCONTROLLER_H
#define PMP_GENERALCONTROLLER_H

#include "requestid.h"
#include "resultmessageerrorcode.h"

#include <QObject>

namespace PMP
{
    class GeneralController : public QObject
    {
        Q_OBJECT
    public:
        virtual ~GeneralController() {}

        virtual qint64 clientClockTimeOffsetMs() const = 0;

        virtual RequestID reloadServerSettings() = 0;

    public Q_SLOTS:
        virtual void shutdownServer() = 0;

    Q_SIGNALS:
        void clientClockTimeOffsetChanged();
        void serverSettingsReloadResultEvent(ResultMessageErrorCode errorCode,
                                             RequestID requestId);

    protected:
        explicit GeneralController(QObject* parent) : QObject(parent) {}
    };
}
#endif
