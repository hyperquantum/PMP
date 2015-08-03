/*
    Copyright (C) 2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "directoriesindexationtask.h"

#include "common/filedata.h"

#include "fileanalysistask.h"
#include "resolver.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QtDebug>
#include <QThreadPool>

namespace PMP {

    DirectoriesIndexationTask::DirectoriesIndexationTask(Resolver* resolver,
                                                         QStringList directories)
     : _resolver(resolver), _directories(directories)
    {
        //
    }

    void DirectoriesIndexationTask::run() {
        qDebug() << "Indexation started";

        int fileCount = 0;

        Q_FOREACH(QString musicPath, _directories) {
            QDirIterator it(musicPath, QDirIterator::Subdirectories); /* no symlinks */

            while (it.hasNext()) {
                QFileInfo entry(it.next());
                if (!entry.isFile()) continue;

                if (!FileData::supportsExtension(entry.suffix())) continue;

                QString path = entry.absoluteFilePath();

                //qDebug() << "starting background analysis of" << path;

                FileAnalysisTask* task = new FileAnalysisTask(path);
                connect(
                    task, &FileAnalysisTask::finished,
                    _resolver, &Resolver::analysedFile
                );
                QThreadPool::globalInstance()->start(task);
                fileCount++;
            }
        }

        qDebug() << "Directory traversal complete; music file count:" << fileCount;
    }

}
