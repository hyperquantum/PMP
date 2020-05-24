/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "queue.h"
#include "resolver.h"

#include <QFileInfo>
#include <QtDebug>

namespace PMP {

    QueueEntry::QueueEntry(PlayerQueue* parent, QString const& filename)
     : QObject(parent),
       _queueID(parent->getNextQueueID()), _new(true), _kind(QueueEntryKind::Track),
       _filename(filename), _haveFilename(true),
       _fetchedTagData(false), _fileFinderBackoff(0), _fileFinderFailedCount(0)
    {
        //
    }

    QueueEntry::QueueEntry(PlayerQueue* parent, FileHash hash, TagData const& tags)
     : QObject(parent),
       _queueID(parent->getNextQueueID()), _new(true), _kind(QueueEntryKind::Track),
       _hash(hash), _haveFilename(false),
       _fetchedTagData(true), _tagData(tags),
       _fileFinderBackoff(0), _fileFinderFailedCount(0)
    {
        //
    }

    QueueEntry::QueueEntry(PlayerQueue* parent, FileHash hash)
     : QObject(parent),
       _queueID(parent->getNextQueueID()), _new(true), _kind(QueueEntryKind::Track),
       _hash(hash), _haveFilename(false),
       _fetchedTagData(false),
       _fileFinderBackoff(0), _fileFinderFailedCount(0)
    {
        //
    }

    QueueEntry::QueueEntry(PlayerQueue* parent, QueueEntry const* existing)
     : QObject(parent),
       _queueID(parent->getNextQueueID()), _new(true), _kind(existing->_kind),
       _hash(existing->_hash), _audioInfo(existing->_audioInfo),
       _filename(existing->_filename), _haveFilename(existing->_haveFilename),
       _fetchedTagData(existing->_fetchedTagData), _tagData(existing->_tagData),
       _fileFinderBackoff(existing->_fileFinderBackoff),
       _fileFinderFailedCount(existing->_fileFinderFailedCount)
    {
        //
    }

    QueueEntry::QueueEntry(PlayerQueue* parent, QueueEntryKind kind)
     : QObject(parent),
       _queueID(parent->getNextQueueID()), _new(true), _kind(kind),
       _haveFilename(false), _fetchedTagData(false),
       _fileFinderBackoff(0), _fileFinderFailedCount(0)
    {
        //
    }

    QueueEntry* QueueEntry::createBreak(PlayerQueue* parent) {
        return new QueueEntry(parent, QueueEntryKind::Break);
    }

    QueueEntry::~QueueEntry() {
        //
    }

    void QueueEntry::markAsNotNewAnymore() {
        _new = false;
    }

    FileHash const* QueueEntry::hash() const {
        if (_hash.empty()) { return nullptr; }

        return &_hash;
    }

    bool QueueEntry::checkHash(Resolver& resolver) {
        if (!_hash.empty()) return true; /* already got it */

        if (!_haveFilename) {
            qDebug() << "PROBLEM: QueueEntry" << _queueID
                     << "does not have either hash nor filename";
            return false;
        }

        _hash = resolver.analyzeAndRegisterFile(_filename);

        if (_hash.empty()) {
            qDebug() << "PROBLEM: QueueEntry" << _queueID
                     << ": analysis of file failed:" << _filename;
            return false;
        }

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

        return nullptr;
    }

    bool QueueEntry::checkValidFilename(Resolver& resolver, bool fast,
                                        QString* outFilename)
    {
        qDebug() << "QueueEntry::checkValidFilename QID" << _queueID;
        if (!isTrack()) return false;

        FileHash const* fileHash = this->hash();

        if (_haveFilename) {
            qDebug() << " have filename, need to verify it: " << _filename;

            if (fileHash) {
                if (resolver.pathStillValid(*fileHash, _filename)) {
                    if (outFilename) { (*outFilename) = _filename; }
                    return true;
                }

                qDebug() << "  filename is no longer valid";
                _haveFilename = false;
                _filename = "";
            }
            else {
                QString name = _filename;
                QFileInfo file(name);

                if (file.isRelative()) {
                    if (!file.makeAbsolute()) {
                        qDebug() << "  failed to make path absolute";
                        return false;
                    }
                    _filename = file.absolutePath();
                }

                if (file.isFile() && file.isReadable()) {
                    if (outFilename) { (*outFilename) = _filename; }
                    return true;
                }

                qDebug() << "  filename is no longer valid, and we have no hash";
                return false;
            }
        }

        /* we don't have a valid filename */

        if (!fileHash) {
            qDebug() << " no hash, cannot get filename";
            return false;
        }

        QString path = resolver.findPath(*fileHash, fast);

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

    qint64 QueueEntry::lengthInMilliseconds() const {
        return _audioInfo.trackLengthMilliseconds();
    }

    QString QueueEntry::artist() const {
        return _tagData.artist();
    }

    QString QueueEntry::title() const {
        return _tagData.title();
    }

    void QueueEntry::setStartedNow() {
        _started = QDateTime::currentDateTimeUtc();
    }

    void QueueEntry::setEndedNow() {
        /* set 'started' equal to 'ended' if 'started' hasn't been set yet */
        if (_started.isNull()) {
            _started = QDateTime::currentDateTimeUtc();
            _ended = _started;
            return;
        }

        _ended = QDateTime::currentDateTimeUtc();
    }
}
