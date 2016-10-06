/*
    Copyright (C) 2014-2016, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP {

    AudioData::AudioData()
     : _format(UnknownFormat), _trackLengthMilliseconds(-1)
    {
        //
    }

    AudioData::AudioData(FileFormat format, qint64 trackLengthMilliseconds)
     : _format(format), _trackLengthMilliseconds(trackLengthMilliseconds)
    {
        //
    }

    QString AudioData::millisecondsToTimeString(qint64 lengthMilliseconds) {
        int partialSeconds = lengthMilliseconds % 1000;
        int totalSeconds = lengthMilliseconds / 1000;

        int sec = totalSeconds % 60;
        int min = (totalSeconds / 60) % 60;
        int hrs = totalSeconds / 3600;

        return QString::number(hrs).rightJustified(2, '0')
                + ":" + QString::number(min).rightJustified(2, '0')
                + ":" + QString::number(sec).rightJustified(2, '0')
                + "." + QString::number(partialSeconds).rightJustified(3, '0');
    }
}
