/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_ANALYZER_H
#define PMP_ANALYZER_H

#include "common/audiodata.h"
#include "common/filehash.h"
#include "common/tagdata.h"

#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QFileInfo)
QT_FORWARD_DECLARE_CLASS(QThreadPool)

namespace PMP
{
    class FileAnalyzer;

    class FileHashes
    {
    public:
        FileHashes()
         : _hashes { FileHash() }
        {
            //
        }

        FileHashes(FileHash mainHash)
         : _hashes { mainHash }
        {
            //
        }

        FileHashes(FileHash mainHash, FileHash alternativeHash)
         : _hashes {mainHash, alternativeHash}
        {
            //
        }

        FileHash main() const { return _hashes[0]; }
        bool multipleHashes() const { return _hashes.size() > 1; }

        QVector<FileHash> const& allHashes() const { return _hashes; }

    private:
        const QVector<FileHash> _hashes;
    };

    class FileInfo
    {
    public:
        FileInfo() : _size(-1) {}

        FileInfo(QString path, qint64 size, QDateTime lastModifiedUtc)
         : _path(path), _size(size), _lastModifiedUtc(lastModifiedUtc)
        {
            //
        }

        QString path() const { return _path; }
        qint64 size() const { return _size; }
        QDateTime lastModifiedUtc() const { return _lastModifiedUtc; }

        bool equals(FileInfo const& other) const
        {
            return path() == other.path()
                && size() == other.size()
                && lastModifiedUtc() == other.lastModifiedUtc();
        }

    private:
        QString _path;
        qint64 _size;
        QDateTime _lastModifiedUtc;
    };

    inline bool operator == (FileInfo const& first, FileInfo const& second)
    {
        return first.equals(second);
    }

    inline bool operator != (FileInfo const& first, FileInfo const& second)
    {
        return !(first == second);
    }

    class Analyzer : public QObject
    {
        Q_OBJECT
    public:
        Analyzer(QObject* parent);
        ~Analyzer();

        void enqueueFile(QString path);

        bool isFinished();

    Q_SIGNALS:
        void fileAnalysisFailed(QString path);
        void fileAnalysisCompleted(QString path, PMP::FileHashes hashes,
                                   PMP::FileInfo fileInfo, PMP::AudioData audioData,
                                   PMP::TagData tagData);
        void finished();

    private:
        static void analyzeFile(Analyzer* analyzer, QString path);
        static FileHashes extractHashes(FileAnalyzer const& fileAnalyzer);
        static FileInfo extractFileInfo(QFileInfo& fileInfo);
        void onFileAnalysisFailed(QString path);
        void onFileAnalysisCompleted(QString path, FileHashes hashes, FileInfo fileInfo,
                                     AudioData audioData, TagData tagData);
        void markAsNoLongerInProgress(QString path, bool& allFinished);

        QThreadPool* _threadPool;
        QMutex _lock;
        QSet<QString> _pathsInProgress;
    };

}

Q_DECLARE_METATYPE(PMP::FileHashes)
Q_DECLARE_METATYPE(PMP::FileInfo)

#endif
