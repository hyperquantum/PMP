/*
    Copyright (C) 2018, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SCROBBLINGPROVIDER_H
#define PMP_SCROBBLINGPROVIDER_H

#include <QObject>

namespace PMP {

    enum ScrobblingProviderState {
        NotInitialized = 0,
        NeedAuthentication,
        Ready,
        TemporarilyUnavailable,
        PermanentFatalError,
    };

    class ScrobblingProvider : public QObject {
        Q_OBJECT
    public:
        ScrobblingProvider();
        virtual ~ScrobblingProvider();

        ScrobblingProviderState state() const { return _state; }

    Q_SIGNALS:
        void stateChanged(ScrobblingProviderState newState);

    public slots:

    protected slots:
        void setState(ScrobblingProviderState newState);

    private:
        ScrobblingProviderState _state;
    };
}
#endif
