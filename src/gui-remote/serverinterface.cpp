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

#include "serverinterface.h"

#include "common/serverconnection.h"

#include "userdatafetcher.h"

namespace PMP {

    ServerInterface::ServerInterface(QObject* parent, ServerConnection* connection)
     : QObject(parent), _connection(connection), _userDataFetcher(nullptr)
    {
        //
    }

    UserDataFetcher* ServerInterface::getUserDataFetcher() {
        if (_userDataFetcher == nullptr) {
            _userDataFetcher = new UserDataFetcher(this, _connection);
        }

        return _userDataFetcher;
    }

}
