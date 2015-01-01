/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "queue.h"
#include "resolver.h"

#include <QFileInfo>
#include <QtDebug>

namespace PMP {

    QueueEntry::QueueEntry(Queue* parent, QString const& filename)
     : QObject(parent),
       _queueID(parent->getNextQueueID()), _filename(filename), _haveFilename(true),
       _fetchedTagData(false)
    {
        //
    }

    QueueEntry::QueueEntry(Queue* parent, FileData const& filedata)
     : QObject(parent),
       _queueID(parent->getNextQueueID()), _hash(filedata.hash()), _haveFilename(false),
       _fetchedTagData(true), _tagData(filedata.tags())
    {
        //
    }

    QueueEntry::QueueEntry(Queue* parent, HashID const& hash)
     : QObject(parent),
       _queueID(parent->getNextQueueID()), _hash(hash), _haveFilename(false),
       _fetchedTagData(false)
    {
        //
    }

    QueueEntry::~QueueEntry() {
        //
    }

    HashID const* QueueEntry::hash() const {
        if (_hash.empty()) { return 0; }

        return &_hash;
    }

    bool QueueEntry::checkHash(Resolver& resolver) {
        if (!_hash.empty()) return true; /* already got it */

        if (!_haveFilename) {
            qDebug() << "PROBLEM: QueueEntry" << _queueID << "does not have either hash nor filename";
            return false;
        }

        FileData data = FileData::analyzeFile(_filename);

        if (!data.isValid()) {
            qDebug() << "PROBLEM: QueueEntry" << _queueID << ": analysis of file failed:" << _filename;
            return false;
        }

        _hash = data.hash();
        resolver.registerFile(data, _filename);
        return true;
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

        /* TODO: verify more information, like known last modification date, file size... */

        if (file.isFile() && file.isReadable()) {
            if (outFilename) { (*outFilename) = _filename; }
            return true;
        }

        return false;
    }

    void QueueEntry::checkAudioData(Resolver& resolver) {
        if (_hash.empty()) return;

        if (!_audioInfo.isComplete()) {
            _audioInfo = resolver.findAudioData(_hash);
        }
    }

    void QueueEntry::checkTrackData(Resolver& resolver) {
        if (_hash.empty()) return;

        checkAudioData(resolver);

        if (_fetchedTagData) return;

        const TagData* tag = resolver.findTagData(_hash);
        if (tag) {
            _tagData = *tag;
            _fetchedTagData = true;
        }
    }

    int QueueEntry::lengthInSeconds() const {
        return _audioInfo.trackLength();
    }

    QString QueueEntry::artist() const {
        return _tagData.artist();
    }

    QString QueueEntry::title() const {
        return _tagData.title();
    }
}
