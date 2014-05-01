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

    void Resolver::registerData(const FileData& data) {
        _tagCache.insert(data.hash(), new FileData(data));
    }

    void Resolver::registerFile(const FileData& file, const QString& filename) {
        registerData(file);

        if (filename.length() > 0) {
            QFileInfo info(filename);

            if (!info.isRelative() || info.makeAbsolute()) {
                _pathCache.insert(file.hash(), info.filePath());
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
            int score = 0;

            QString title = tag->title();
            QString artist = tag->artist();

            if (title.length() > 0) {
                score += 100000;
                score += 8 * title.length();
            }

            if (artist.length() > 0) {
                score += 80000;
                score += artist.length();
            }

            if (score <= resultScore) { continue; }

            result = tag;
            resultScore = score;
        }

        return result;
    }

}
