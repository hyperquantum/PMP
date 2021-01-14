/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COMMAND_H
#define PMP_COMMAND_H

#include <QObject>

namespace PMP {

    class ClientServerInterface;

    class Command : public QObject
    {
        Q_OBJECT
    public:
        ~Command();

        virtual bool requiresAuthentication() const = 0;
        virtual void execute(ClientServerInterface* clientServerInterface) = 0;

    Q_SIGNALS:
        void executionSuccessful(QString output = "");
        void executionFailed(int resultCode, QString errorOutput);

    protected:
        explicit Command(QObject* parent = nullptr);
    };
}
#endif
