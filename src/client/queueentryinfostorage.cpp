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

#include "queueentryinfostorage.h"

namespace PMP::Client
{
    QueueEntryInfo::QueueEntryInfo(quint32 queueId)
     : _queueId(queueId), _type(QueueEntryType::Unknown), _lengthMilliseconds(-1)
    {
        //
    }

    bool QueueEntryInfo::needFilename() const
    {
        if (_type != QueueEntryType::Track)
            return false;

        return title().trimmed().isEmpty() || artist().trimmed().isEmpty();
    }

    void QueueEntryInfo::setHash(QueueEntryType type, LocalHashId hashId)
    {
        _type = type;
        _hashId = hashId;
    }

    void QueueEntryInfo::setInfo(QueueEntryType type, qint64 lengthInMilliseconds,
                                 QString const& title, QString const& artist)
    {
        _type = type;
        _lengthMilliseconds = lengthInMilliseconds;
        _title = title;
        _artist = artist;
    }

    bool QueueEntryInfo::setPossibleFilenames(QList<QString> const& names)
    {
        if (names.empty()) return false;

        int shortestLength = names[0].length();
        int longestLength = names[0].length();

        int limit = 20;
        for (auto& name : names)
        {
            if (name.length() < shortestLength) shortestLength = name.length();
            else if (name.length() > longestLength) longestLength = name.length();

            if (--limit <= 0) break;
        }

        /* Avoid a potential overflow, don't add shortest and longest, the result does not
           need to be exact.
           Also, if there are only two possibilities, we favor the longest. */
        int middleLength = (shortestLength + 1) / 2 + (longestLength + 1) / 2 + 1;

        QString middle = names[0];

        limit = 10;
        for (auto& name : names)
        {
            int diff = std::abs(name.length() - middleLength);
            int oldDiff = std::abs(middle.length() - middleLength);

            if (diff < oldDiff) middle = name;

            if (--limit <= 0) break;
        }

        if (_informativeFilename.trimmed() == "" && _informativeFilename != middle)
        {
            _informativeFilename = middle;
            return true;
        }

        return false;
    }
}
