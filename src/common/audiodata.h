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

#ifndef PMP_AUDIODATA_H
#define PMP_AUDIODATA_H

#include <QString>

namespace PMP {

    class AudioData {
    public:
        enum FileFormat {
            UnknownFormat = 0,
            MP3 = 1,
            WMA = 2
        };

        AudioData();
        AudioData(FileFormat format, quint64 trackLengthMilliseconds);

        bool isComplete() const {
            return _format != UnknownFormat && _trackLengthMilliseconds >= 0;
        }

        FileFormat format() const { return _format; }
        void setFormat(FileFormat format) { _format = format; }

        quint64 trackLengthMilliseconds() const { return _trackLengthMilliseconds; }
        int trackLengthSeconds() const { return _trackLengthMilliseconds / 1000; }

        void setTrackLengthMilliseconds(quint64 length) {
            _trackLengthMilliseconds = length;
        }

        void setTrackLengthSeconds(int length) {
            _trackLengthMilliseconds = length * 1000;
        }

        static QString millisecondsToTimeString(quint64 lengthMilliseconds);

    private:
        FileFormat _format;
        quint64 _trackLengthMilliseconds;
    };
}
#endif
