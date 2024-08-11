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

#ifndef PMP_ANALYZER_H
#define PMP_ANALYZER_H

#include "common/audiodata.h"
#include "common/future.h"

#include "fileanalysis.h"

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
}

namespace PMP::Server
{
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
        void fileAnalysisCompleted(QString path, PMP::Server::FileAnalysis analysis);
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
#endif
