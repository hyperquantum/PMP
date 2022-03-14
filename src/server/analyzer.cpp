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

#include "analyzer.h"

#include "common/fileanalyzer.h"

#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>

namespace PMP
{
    Analyzer::Analyzer(QObject* parent)
     : QObject(parent),
       _threadPool(new QThreadPool(this))
    {
        /* single thread only, because it's mostly I/O */
        _threadPool->setMaxThreadCount(1);
    }

    Analyzer::~Analyzer()
    {
        _threadPool->clear();
        _threadPool->waitForDone();
    }

    void Analyzer::enqueueFile(QString path)
    {
        QMutexLocker lock(&_lock);

        if (_pathsInProgress.contains(path))
            return;

        _pathsInProgress << path;

        QtConcurrent::run(_threadPool, &analyzeFile, this, path);
    }

    bool Analyzer::isFinished()
    {
        QMutexLocker lock(&_lock);
        return _pathsInProgress.empty();
    }

    void Analyzer::analyzeFile(Analyzer* analyzer, QString path)
    {
        QFileInfo firstQFileInfo(path);
        FileInfo firstFileInfo = extractFileInfo(firstQFileInfo);

        FileAnalyzer fileAnalyzer(firstQFileInfo);
        fileAnalyzer.analyze();

        QFileInfo secondQFileInfo(path);
        FileInfo secondFileInfo = extractFileInfo(secondQFileInfo);

        if (firstFileInfo != secondFileInfo) /* file was changed? */
        {
            qDebug() << "file seems to have changed:" << path;
            if (secondQFileInfo.exists())
            {
                analyzer->_pathsInProgress.remove(path);
                analyzer->enqueueFile(path); /* try again later */
            }
            else
                analyzer->onFileAnalysisFailed(path); /* file no longer exists */

            return;
        }

        if (!fileAnalyzer.analysisDone()) /* something went wrong */
        {
            qDebug() << "file analysis failed:" << path;
            analyzer->onFileAnalysisFailed(path);
            return;
        }

        auto audioData = fileAnalyzer.audioData();
        if (audioData.trackLengthMilliseconds() > std::numeric_limits<qint32>::max())
        {
            /* file too long, probably not music anyway */
            qDebug() << "file audio too long:" << path;
            analyzer->onFileAnalysisFailed(path);
            return;
        }

        auto hashes = extractHashes(fileAnalyzer);
        auto tagData = fileAnalyzer.tagData();

        analyzer->onFileAnalysisCompleted(path, hashes, secondFileInfo, audioData,
                                          tagData);
    }

    FileHashes Analyzer::extractHashes(const FileAnalyzer& fileAnalyzer)
    {
        FileHash mainHash = fileAnalyzer.hash();
        FileHash legacyHash = fileAnalyzer.legacyHash();

        if (!legacyHash.isNull())
            return FileHashes { mainHash, legacyHash };

        return FileHashes { mainHash };
    }

    FileInfo Analyzer::extractFileInfo(QFileInfo& fileInfo)
    {
        return FileInfo(fileInfo.absoluteFilePath(),
                        fileInfo.size(),
                        fileInfo.lastModified().toUTC());
    }

    void Analyzer::onFileAnalysisFailed(QString path)
    {
        bool isFinished;
        markAsNoLongerInProgress(path, isFinished);

        Q_EMIT fileAnalysisFailed(path);

        if (isFinished)
            Q_EMIT finished();
    }

    void Analyzer::onFileAnalysisCompleted(QString path, FileHashes hashes,
                                           FileInfo fileInfo, AudioData audioData,
                                           TagData tagData)
    {
        bool isFinished;
        markAsNoLongerInProgress(path, isFinished);

        Q_EMIT fileAnalysisCompleted(path, hashes, fileInfo, audioData, tagData);

        if (isFinished)
            Q_EMIT finished();
    }

    void Analyzer::markAsNoLongerInProgress(QString path, bool& allFinished)
    {
        QMutexLocker lock(&_lock);
        _pathsInProgress.remove(path);
        allFinished = _pathsInProgress.empty();
    }
}
