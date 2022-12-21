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

#ifndef PMP_SERVERCAPABILITIES_H
#define PMP_SERVERCAPABILITIES_H

namespace PMP::Client
{
    class ServerCapabilities
    {
    public:
        virtual ~ServerCapabilities() {}

        virtual bool supportsSendingVersionInfo() const = 0;
        virtual bool supportsReloadingServerSettings() const = 0;
        virtual bool supportsDelayedStart() const = 0;
        virtual bool supportsQueueEntryDuplication() const = 0;
        virtual bool supportsDynamicModeWaveTermination() const = 0;
        virtual bool supportsInsertingBreaksAtAnyIndex() const = 0;
        virtual bool supportsInsertingBarriers() const = 0;

    protected:
        ServerCapabilities() {}
    };
}
#endif
