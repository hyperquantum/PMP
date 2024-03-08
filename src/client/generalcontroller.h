/*
    Copyright (C) 2021-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/future.h"
#include "common/resultmessageerrorcode.h"
#include "common/serverhealthstatus.h"
#include "common/versioninfo.h"

#include <QObject>

namespace PMP::Client
{
    class GeneralController : public QObject
    {
        Q_OBJECT
    public:
        virtual ~GeneralController() {}

        virtual ServerHealthStatus serverHealth() const = 0;

        virtual qint64 clientClockTimeOffsetMs() const = 0;

        virtual SimpleFuture<AnyResultMessageCode> startFullIndexation() = 0;
        virtual SimpleFuture<AnyResultMessageCode> reloadServerSettings() = 0;

        virtual Future<VersionInfo, ResultMessageErrorCode> getServerVersionInfo() = 0;

    public Q_SLOTS:
        virtual void shutdownServer() = 0;

    Q_SIGNALS:
        void serverHealthChanged();
        void clientClockTimeOffsetChanged();

    protected:
        explicit GeneralController(QObject* parent) : QObject(parent) {}
    };
}
#endif
