/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#include "resolver.h"

#include "common/filedata.h"

#include <QFileInfo>
#include <QtDebug>

namespace PMP {

    Resolver::Resolver() {
        //
    }

    void Resolver::registerFile(const FileData* file, const QString& filename) {
        if (file != 0) {
            _tagCache.insert(file->hash(), file);
        }

        if (filename.length() > 0) {
            QFileInfo info(filename);

            if (!info.isRelative() || info.makeAbsolute()) {
                _pathCache.insert(file->hash(), info.filePath());
            }
        }
    }

    QString Resolver::findPath(const HashID& hash) {
        QList<QString> paths = _pathCache.values(hash);
        qDebug() << "Resolver::findPath for hash " << hash.dumpToString();
        qDebug() << " candidates for hash: " << paths.count();

        QString path;
        foreach(path, paths) {
            qDebug() << " candidate: " << path;
            QFileInfo info(path);

            if (info.isFile() && info.isReadable()) {
                return path;
            }
        }

        return ""; /* not found */
    }

    const FileData* Resolver::findData(const HashID& hash) {
        QList<const FileData*> tags = _tagCache.values(hash);

        /* try to return a match with complete tags */
        const FileData* result = 0;
        int resultScore = -1;
        const FileData* tag;
        foreach (tag, tags) {
            int score = (tag->title().length() > 0) ? 3 : 0;
            score += (tag->artist().length() > 0) ? 2 : 0;

            if (score <= resultScore) { continue; }

            result = tag;
            resultScore = score;
        }

        return result;
    }

}
