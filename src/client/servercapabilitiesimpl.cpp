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

#include "servercapabilitiesimpl.h"

namespace PMP::Client
{
    ServerCapabilitiesImpl::ServerCapabilitiesImpl()
     : _serverProtocolNumber(0)
    {
        //
    }

    void ServerCapabilitiesImpl::setServerProtocolNumber(int serverProtocolNumber)
    {
        _serverProtocolNumber = serverProtocolNumber;
    }

    bool ServerCapabilitiesImpl::supportsSendingVersionInfo() const
    {
        return _serverProtocolNumber >= 22;
    }

    bool ServerCapabilitiesImpl::supportsReloadingServerSettings() const
    {
        return _serverProtocolNumber >= 15;
    }

    bool ServerCapabilitiesImpl::supportsDelayedStart() const
    {
        return _serverProtocolNumber >= 20;
    }

    bool ServerCapabilitiesImpl::supportsQueueEntryDuplication() const
    {
        return _serverProtocolNumber >= 9;
    }

    bool ServerCapabilitiesImpl::supportsDynamicModeWaveTermination() const
    {
        return _serverProtocolNumber >= 14;
    }

    bool ServerCapabilitiesImpl::supportsInsertingBreaksAtAnyIndex() const
    {
        return _serverProtocolNumber >= 17;
    }

    bool ServerCapabilitiesImpl::supportsInsertingBarriers() const
    {
        return _serverProtocolNumber >= 18;
    }
}
