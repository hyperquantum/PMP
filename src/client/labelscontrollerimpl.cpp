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

#include "labelscontrollerimpl.h"

#include "serverconnection.h"

namespace PMP::Client
{
    LabelsControllerImpl::LabelsControllerImpl(ServerConnection* connection)
     : LabelsController(connection),
       _connection{connection}
    {
        //
    }

    SimpleFuture<AnyResultMessageCode> LabelsControllerImpl::applyLabelToTrack(
        LocalHashId hashId, QString label)
    {
        return _connection->applyLabelToTrack(hashId, label);
    }

    SimpleFuture<AnyResultMessageCode> LabelsControllerImpl::removeLabelFromTrack(
        LocalHashId hashId, QString label)
    {
        return _connection->removeLabelFromTrack(hashId, label);
    }
}
