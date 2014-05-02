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

#include "queueentry.h"

#include "common/filedata.h"

#include "resolver.h"

#include <QFileInfo>
#include <QtDebug>

namespace PMP {

    QueueEntry::QueueEntry(uint queueID, QString const& filename)
     : _queueID(queueID), _filename(filename), _haveFilename(true),
       _fileData(0), _hash(0)
    {
        //
    }

    QueueEntry::QueueEntry(uint queueID, FileData const& filedata)
     : _queueID(queueID), _haveFilename(false),
       _fileData(new FileData(filedata)), _hash(0)
    {
        //
    }

    QueueEntry::QueueEntry(uint queueID, HashID const& hash)
     : _queueID(queueID), _haveFilename(false),
       _fileData(0), _hash(new HashID(hash))
    {
        //
    }

    QueueEntry::~QueueEntry() {
        delete _fileData;
        delete _hash;
    }

    HashID const* QueueEntry::hash() const {
        FileData const* data = _fileData;
        if (data) { return &data->hash(); }

        HashID const* hash = _hash;
        //if (hash) { return hash; }

        return hash;
    }

    void QueueEntry::setFilename(QString const& filename) {
        _filename = filename;
        _haveFilename = true;
    }

    QString const* QueueEntry::filename() const {
        if (_haveFilename) {
            return &_filename;
        }

        return 0;
    }

    bool QueueEntry::checkValidFilename(Resolver& resolver, QString* outFilename) {
        qDebug() << "QueueEntry::checkValidFilename QID" << _queueID;

        if (!_haveFilename) {
            HashID const* fileHash = this->hash();
            if (fileHash == 0) {
                qDebug() << " no hash, cannot get filename";
                return false;
            }

            QString path = resolver.findPath(*fileHash);

            if (path.length() > 0) {
                _filename = path;
                _haveFilename = true;
                if (outFilename) { (*outFilename) = _filename; }
                qDebug() << " found filename: " << path;
                return true;
            }

            qDebug() << " no known filename";
            return false;
        }

        qDebug() << " have filename, need to verify it: " << _filename;

        QString name = _filename;
        QFileInfo file(name);

        if (file.isRelative()) {
            if (!file.makeAbsolute()) { return false; }
            _filename = file.absolutePath();
        }

        if (file.isFile() && file.isReadable()) {
            if (outFilename) { (*outFilename) = _filename; }
            return true;
        }

        return false;
    }

    void QueueEntry::checkTrackData(Resolver& resolver) {
        const FileData* data = _fileData;
        if (data) return;

        const HashID* hash = _hash;
        if (!hash) return;

        data = resolver.findData(*hash);
        //if (data) {
            _fileData = data;
        //}
    }

    int QueueEntry::lengthInSeconds() const {
        FileData const* data = _fileData;
        if (data) { return data->trackLength(); }

        return -1;
    }

    QString QueueEntry::artist() const {
        FileData const* data = _fileData;
        if (data) { return data->artist(); }

        return "";
    }

    QString QueueEntry::title() const {
        FileData const* data = _fileData;
        if (data) { return data->title(); }

        return "";
    }

}
