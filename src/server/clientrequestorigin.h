/*
    Copyright (C) 2019-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENTREQUESTORIGIN_H
#define PMP_CLIENTREQUESTORIGIN_H

#include <QMetaType>
#include <QtGlobal>

namespace PMP
{
    class ClientRequestOrigin
    {
    public:
        ClientRequestOrigin()
         : _connectionReference(0), _clientReference(0)
        {
            //
        }

        ClientRequestOrigin(uint connectionReference, uint clientReference)
         : _connectionReference(connectionReference), _clientReference(clientReference)
        {
            //
        }

        uint connectionReference() const { return _connectionReference; }
        uint clientReference() const { return _clientReference; }

        bool matchesConnection(uint connectionReference) const
        {
            return _connectionReference == connectionReference;
        }

        static const ClientRequestOrigin none;

    private:
        uint _connectionReference;
        uint _clientReference;
    };
}

Q_DECLARE_METATYPE(PMP::ClientRequestOrigin)

#endif
