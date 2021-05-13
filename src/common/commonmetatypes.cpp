/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "collectiontrackinfo.h"
#include "compatibilityui.h"
#include "filehash.h"
#include "playerhistorytrackinfo.h"
#include "playermode.h"
#include "playerstate.h"
#include "queueentrytype.h"
#include "serverhealthstatus.h"
#include "startstopeventstatus.h"
#include "userloginerror.h"

namespace PMP {

    /** Utility object to automatically do the qRegisterMetaType calls at program
     *  startup */
    class CommonMetatypesInit {
    protected:
        CommonMetatypesInit() {
            qRegisterMetaType<PMP::CollectionTrackInfo>();
            qRegisterMetaType<PMP::CompatibilityUiActionState>();
            qRegisterMetaType<PMP::CompatibilityUiState>();
            qRegisterMetaType<PMP::FileHash>();
            qRegisterMetaType<PMP::PlayerHistoryTrackInfo>();
            qRegisterMetaType<PMP::PlayerMode>();
            qRegisterMetaType<PMP::PlayerState>();
            qRegisterMetaType<PMP::QueueEntryType>();
            qRegisterMetaType<PMP::ServerHealthStatus>();
            qRegisterMetaType<PMP::StartStopEventStatus>();
            qRegisterMetaType<PMP::UserInterfaceLanguage>();
            qRegisterMetaType<PMP::UserLoginError>();
        }

    private:
        static CommonMetatypesInit GlobalVariable;
    };

    CommonMetatypesInit CommonMetatypesInit::GlobalVariable;

}
