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

#include <QFileInfo>

namespace PMP {

    QueueEntry::QueueEntry(QString const& filename)
     : _filename(filename), _haveFilename(true), _fileData(0)
    {
        //
    }

    QueueEntry::QueueEntry(FileData const& filedata)
     : _filename(filedata.filename()), _haveFilename(true),
        _fileData(new FileData(filedata))
    {
        //
    }

    QueueEntry::~QueueEntry() {
        delete _fileData;
    }

    HashID const* QueueEntry::hash() const {
        FileData const* data = _fileData;
        if (data) { return &data->hash(); }

        return 0;
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

    bool QueueEntry::checkValidFilename(QString* outFilename) {
        if (!_haveFilename) { return false; }

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

    int QueueEntry::lengthInSeconds() const {
        FileData const* data = _fileData;
        if (data) { return data->lengthInSeconds(); }

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
