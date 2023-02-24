/*
    Copyright (C) 2014-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "audiodata.h"
#include "filehash.h"
#include "playerhistorytrackinfo.h"
#include "playermode.h"
#include "playerstate.h"
#include "queueentrytype.h"
#include "queueindextype.h"
#include "serverhealthstatus.h"
#include "specialqueueitemtype.h"
#include "startstopeventstatus.h"
#include "tagdata.h"
#include "userloginerror.h"
#include "userregistrationerror.h"
#include "versioninfo.h"

namespace PMP
{
    /** Utility object to automatically do the qRegisterMetaType calls at program
     *  startup */
    class CommonMetatypesInit
    {
    protected:
        CommonMetatypesInit()
        {
            qRegisterMetaType<PMP::AudioData>();
            qRegisterMetaType<PMP::FileHash>();
            qRegisterMetaType<PMP::PlayerHistoryTrackInfo>();
            qRegisterMetaType<PMP::PlayerMode>();
            qRegisterMetaType<PMP::PlayerState>();
            qRegisterMetaType<PMP::QueueEntryType>();
            qRegisterMetaType<PMP::QueueIndexType>();
            qRegisterMetaType<PMP::ServerHealthStatus>();
            qRegisterMetaType<PMP::SpecialQueueItemType>();
            qRegisterMetaType<PMP::StartStopEventStatus>();
            qRegisterMetaType<PMP::TagData>();
            qRegisterMetaType<PMP::UserLoginError>();
            qRegisterMetaType<PMP::UserRegistrationError>();
            qRegisterMetaType<PMP::VersionInfo>();
        }

    private:
        static CommonMetatypesInit GlobalVariable;
    };

    CommonMetatypesInit CommonMetatypesInit::GlobalVariable;
}
