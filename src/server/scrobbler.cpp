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

#include "scrobbler.h"

#include "scrobblingprovider.h"

#include <QDebug>

namespace PMP {

    TrackToScrobble::TrackToScrobble() {
        //
    }

    TrackToScrobble::~TrackToScrobble() {
        //
    }

    ScrobblingDataSource::ScrobblingDataSource() {
        //
    }

    ScrobblingDataSource::~ScrobblingDataSource() {
        //
    }

    Scrobbler::Scrobbler(QObject* parent)
     : QObject(parent)
    {
        //
    }

    void Scrobbler::addProvider(ScrobblingProvider* provider) {
        if (provider->parent() != nullptr && provider->parent() != this) {
            qWarning() << "cannot add provider that already has a parent!";
            return;
        }

        provider->setParent(this);

        if (_providers.contains(provider)) return;

        _providers.append(provider);



    }

}
