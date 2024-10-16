/*
    Copyright (C) 2022-2024, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef SERVERCAPABILITIESIMPL_H
#define SERVERCAPABILITIESIMPL_H

#include "servercapabilities.h"

namespace PMP::Client
{
    class ServerCapabilitiesImpl : public ServerCapabilities
    {
    public:
        ServerCapabilitiesImpl();

        void setServerProtocolNumber(int serverProtocolNumber);

        bool supportsSendingVersionInfo() const override;
        bool supportsReloadingServerSettings() const override;
        bool supportsDelayedStart() const override;
        bool supportsQueueEntryDuplication() const override;
        bool supportsDynamicModeWaveTermination() const override;
        bool supportsInsertingBreaksAtAnyIndex() const override;
        bool supportsInsertingBarriers() const override;
        bool supportsAlbumArtist() const override;
        bool supportsRequestingPersonalTrackHistory() const override;
        bool supportsRequestingIndividualTrackInfo() const override;

    private:
        int _serverProtocolNumber;
    };
}
#endif
