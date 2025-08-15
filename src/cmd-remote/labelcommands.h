/*
    Copyright (C) 2025, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_LABELCOMMANDS_H
#define PMP_LABELCOMMANDS_H

#include "commandbase.h"

#include "common/filehash.h"

#include <QString>

namespace PMP
{
    class LabelAddCommand : public CommandBase
    {
        Q_OBJECT
    public:
        LabelAddCommand(QString labelName, FileHash const& hash);

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        QString _name;
        FileHash _hash;
    };

    class LabelRemoveCommand : public CommandBase
    {
        Q_OBJECT
    public:
        LabelRemoveCommand(QString labelName, FileHash const& hash);

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        QString _name;
        FileHash _hash;
    };

    class LabelListCommand : public CommandBase
    {
        Q_OBJECT
    public:
        LabelListCommand(FileHash const& hash);

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        void printLabelNames(QList<QString> labelNames);

        FileHash _hash;
    };
}
#endif
