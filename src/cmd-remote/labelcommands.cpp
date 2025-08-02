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

#include "labelcommands.h"

#include "client/labelscontroller.h"
#include "client/localhashidrepository.h"
#include "client/serverinterface.h"

using namespace PMP::Client;

namespace PMP
{
    LabelAddCommand::LabelAddCommand(QString labelName, const FileHash& hash)
     : _name(labelName), _hash(hash)
    {
        //
    }

    void LabelAddCommand::run(ServerInterface* serverInterface)
    {
        auto hashId = serverInterface->hashIdRepository()->getOrRegisterId(_hash);

        auto future =
            serverInterface->labelsController().applyLabelToTrack(hashId, _name);

        setCommandExecutionResultFuture(future);
    }
}
