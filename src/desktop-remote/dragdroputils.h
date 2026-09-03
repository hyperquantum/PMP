/*
    Copyright (C) 2025-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_DRAGDROPUTILS_H
#define PMP_DRAGDROPUTILS_H

#include "common/filehash.h"
#include "common/nullable.h"

#include <QAbstractItemModel>
#include <QList>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QMimeData)

namespace PMP::DragDropUtils
{
    Nullable<QList<FileHash>> tryGetHashes(
            const QModelIndexList& indexes,
            std::function<Nullable<FileHash> (QModelIndex)> indexToHashConversion);

    QMimeData* convertHashesToMimeData(const QList<FileHash>& hashes);
}
#endif
