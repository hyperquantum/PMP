/*
    Copyright (C) 2017-2026, Kevin André <hyperquantum@gmail.com>

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

#include "dragdroputils.h"

#include <QBuffer>
#include <QDataStream>
#include <QMimeData>

namespace PMP::DragDropUtils
{
    Nullable<QList<FileHash>> tryGetHashes(const QModelIndexList& indexes,
                    std::function<Nullable<FileHash> (QModelIndex)> indexToHashConversion)
    {
        QList<FileHash> hashes;

        int previousRow = -1;
        for (auto& index : indexes)
        {
            int row = index.row();
            if (row == previousRow) continue;
            previousRow = row;

            auto hashOrNull = indexToHashConversion(index);
            if (hashOrNull == null)
                return null;

            hashes.append(hashOrNull.value());
        }

        return hashes;
    }

    QMimeData* convertHashesToMimeData(const QList<FileHash>& hashes)
    {
        if (hashes.isEmpty())
            return nullptr;

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QDataStream stream(&buffer);
        stream.setVersion(QDataStream::Qt_5_2);

        stream << quint32(hashes.size());
        for (int i = 0; i < hashes.size(); ++i)
        {
            stream << quint64(hashes[i].length());
            stream << hashes[i].SHA1();
            stream << hashes[i].MD5();
        }

        buffer.close();

        QMimeData* data = new QMimeData();

        data->setData("application/x-pmp-filehash", buffer.data());
        return data;
    }
}
