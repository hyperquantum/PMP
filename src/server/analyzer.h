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
#include "common/future.h"
#include "common/tagdata.h"

#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QSet>
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

        bool contains(FileHash hash) { return _hashes.contains(hash); }

        QVector<FileHash> const& allHashes() const { return _hashes; }

    private:
        QVector<FileHash> _hashes;
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

    class FileAnalysis
    {
    public:
        FileAnalysis() {}

        FileAnalysis(FileHashes const& hashes, FileInfo const& fileInfo,
                     AudioData const& audioData, TagData const& tagData)
         : _hashes(hashes), _fileInfo(fileInfo), _audioData(audioData), _tagData(tagData)
        {
            //
        }

        FileHashes const& hashes() const { return _hashes; }
        FileInfo const& fileInfo() const { return _fileInfo; }
        AudioData const& audioData() const { return _audioData; }
        TagData const& tagData() const { return _tagData; }

    private:
        FileHashes _hashes;
        FileInfo _fileInfo;
        AudioData _audioData;
        TagData _tagData;
    };

    class Analyzer : public QObject
    {
        Q_OBJECT
    public:
        Analyzer(QObject* parent);
        ~Analyzer();

        void enqueueFile(QString path);
        bool isFinished();

        Future<FileAnalysis, FailureType> analyzeFileAsync(QString path);
        ResultOrError<FileAnalysis, FailureType> analyzeFile(QString path);

    Q_SIGNALS:
        void fileAnalysisFailed(QString path);
        void fileAnalysisCompleted(QString path, PMP::FileAnalysis analysis);
        void finished();

    private:
        static ResultOrError<FileAnalysis, FailureType> analyzeFileInternal(
                                                                    Analyzer* analyzer,
                                                                    QString path,
                                                                    bool fromQueue);
        static FileHashes extractHashes(FileAnalyzer const& fileAnalyzer);
        static FileInfo extractFileInfo(QFileInfo& fileInfo);
        void onFileAnalysisFailed(QString path);
        void onFileAnalysisCompleted(QString path, FileAnalysis analysis);
        void markAsNoLongerInProgress(QString path, bool& allFinished);

        QThreadPool* _queueThreadPool;
        QThreadPool* _onDemandThreadPool;
        QMutex _lock;
        QSet<QString> _pathsInProgress;
        QHash<QString, Future<FileAnalysis, FailureType>> _onDemandInProgress;
    };
}

Q_DECLARE_METATYPE(PMP::FileHashes)
Q_DECLARE_METATYPE(PMP::FileInfo)
Q_DECLARE_METATYPE(PMP::FileAnalysis)

#endif
