/*
    Copyright (C) 2014-2024, Kevin Andre <hyperquantum@gmail.com>

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

//#include "common/concurrent.h"
#include "common/fileanalyzer.h"
#include "common/newconcurrent.h"

#include <QtDebug>
#include <QThreadPool>

namespace PMP::Server
{
    Analyzer::Analyzer(QObject* parent)
     : QObject(parent),
       _queueThreadPool(new QThreadPool(this)),
       _onDemandThreadPool(new QThreadPool(this))
    {
        /* single thread only, because it's mostly I/O */
        _queueThreadPool->setMaxThreadCount(1);
        _onDemandThreadPool->setMaxThreadCount(1);
    }

    Analyzer::~Analyzer()
    {
        _queueThreadPool->clear();
        _onDemandThreadPool->clear();

        _queueThreadPool->waitForDone();
        _onDemandThreadPool->waitForDone();
    }

    void Analyzer::enqueueFile(QString path)
    {
        QMutexLocker lock(&_lock);

        if (_pathsInProgress.contains(path))
            return;

        _pathsInProgress << path;

        auto future =
            NewConcurrent::runOnThreadPool<FileAnalysis, FailureType>(
                _queueThreadPool,
                [this, path]()
                {
                    return analyzeFileInternal(this, path, true);
                }
            );

        future.handleOnEventLoop(
            this,
            [this, path](ResultOrError<FileAnalysis, FailureType> outcome)
            {
                if (outcome.succeeded())
                    onFileAnalysisCompleted(path, outcome.result());
                else
                    onFileAnalysisFailed(path);
            }
        );
    }

    bool Analyzer::isFinished()
    {
        QMutexLocker lock(&_lock);
        return _pathsInProgress.empty();
    }

    NewFuture<FileAnalysis, FailureType> Analyzer::analyzeFileAsync(QString path)
    {
        QMutexLocker lock(&_lock);

        auto it = _onDemandInProgress.find(path);
        if (it != _onDemandInProgress.end())
            return it.value();

        qDebug() << "Analyzer: starting background job for:" << path;
        auto future =
            NewConcurrent::runOnThreadPool<FileAnalysis, FailureType>(
                _onDemandThreadPool,
                [this, path]()
                {
                    auto result = analyzeFile(path);

                    qDebug() << "Analyzer: job finished for:" << path;

                    QMutexLocker lock(&_lock);
                    _onDemandInProgress.remove(path);

                    return result;
                }
            );

        _onDemandInProgress.insert(path, future);

        return future;
    }

    ResultOrError<FileAnalysis, FailureType> Analyzer::analyzeFile(QString path)
    {
        auto maybeResult = analyzeFileInternal(this, path, false);

        if (maybeResult.succeeded())
            Q_EMIT fileAnalysisCompleted(path, maybeResult.result());

        return maybeResult;
    }

    ResultOrError<FileAnalysis, FailureType> Analyzer::analyzeFileInternal(
                                                                    Analyzer* analyzer,
                                                                    QString path,
                                                                    bool fromQueue)
    {
        QFileInfo firstQFileInfo(path);
        FileInfo firstFileInfo = extractFileInfo(firstQFileInfo);

        FileAnalyzer fileAnalyzer(firstQFileInfo);
        fileAnalyzer.analyze();

        QFileInfo secondQFileInfo(path);
        FileInfo secondFileInfo = extractFileInfo(secondQFileInfo);

        if (firstFileInfo != secondFileInfo) /* file was changed? */
        {
            if (secondQFileInfo.exists())
            {
                qDebug() << "Analyzer: file seems to have changed, will retry later:"
                         << path;
                if (fromQueue)
                {
                    QMutexLocker lock(&analyzer->_lock);
                    analyzer->_pathsInProgress.remove(path);
                }
                analyzer->enqueueFile(path); /* try again later */
            }
            else
            {
                qDebug() << "Analyzer: file seems to have been deleted:" << path;
            }

            return failure;
        }

        if (!fileAnalyzer.analysisDone()) /* something went wrong */
        {
            qDebug() << "Analyzer: file analysis failed:" << path;
            return failure;
        }

        auto audioData = fileAnalyzer.audioData();
        if (audioData.trackLengthMilliseconds() > std::numeric_limits<qint32>::max())
        {
            /* file too long, probably not music anyway */
            qDebug() << "Analyzer: file audio too long:" << path;
            return failure;
        }

        auto hashes = extractHashes(fileAnalyzer);
        auto tagData = fileAnalyzer.tagData();

        FileAnalysis analysis(hashes, secondFileInfo, audioData, tagData);
        return analysis;
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
        qDebug() << "Analyzer: failed to analyze" << path;

        bool isFinished;
        markAsNoLongerInProgress(path, isFinished);

        Q_EMIT fileAnalysisFailed(path);

        if (isFinished)
            Q_EMIT finished();
    }

    void Analyzer::onFileAnalysisCompleted(QString path, FileAnalysis analysis)
    {
        qDebug() << "Analyzer: completed analysis of" << path;
        if (analysis.hashes().multipleHashes())
        {
            auto hashes = analysis.hashes().allHashes();
            qDebug() << "Analyzer: multiple hashes found for" << path;
            for (int i = 0; i < hashes.size(); ++i)
            {
                qDebug() << (i + 1) << "/" << hashes.size() << ":" << hashes[i];
            }
        }

        bool isFinished;
        markAsNoLongerInProgress(path, isFinished);

        Q_EMIT fileAnalysisCompleted(path, analysis);

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
