/*
    Copyright (C) 2025, Kevin André <hyperquantum@gmail.com>

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

#include "labels.h"

#include "common/concurrent.h"
#include "common/containerutil.h"

#include "database.h"

#include <QDateTime>

namespace PMP::Server
{
    Labels::Labels(QObject *parent)
     : QObject(parent)
    {
        QMutexLocker lock(&_mutex);

        loadFromDatabase();
    }

    SimpleFuture<Result> Labels::applyLabelToTrack(uint trackHashId, const QString& label)
    {
        if (!isValidPotentialName(label))
            return FutureResult(Error::labelNameInvalid());

        auto future =
            Concurrent::runOnThreadPool<Result>(
                globalThreadPool,
                [this, trackHashId, label]() -> Result
                {
                    auto db = Database::getDatabaseForCurrentThread();
                    if (!db) return Error::databaseUnvailable();

                    QMutexLocker lock(&_mutex);

                    auto labelId = _nameToId.value(label, 0);
                    if (labelId == 0)
                    {
                        auto idOrFailure = db->insertLabel(label);
                        if (idOrFailure.failed())
                            return Error::internalError(); // TODO : find a better error

                        labelId = idOrFailure.result();
                        _nameToId.insert(label, labelId);
                    }

                    auto it = _labelDataByLabelId.find(labelId);
                    if (it == _labelDataByLabelId.end())
                    {
                        it = _labelDataByLabelId.insert(labelId, { .name = label });
                    }

                    LabelData* labelData = &it.value();

                    if (labelData->hashes.contains(trackHashId))
                        return NoOp();

                    auto connectResult = db->connectLabelToHash(labelId, trackHashId);
                    if (connectResult.failed())
                        return Error::internalError(); // TODO : find a better error

                    labelData->inActiveUse = true;
                    labelData->hashes << trackHashId;
                    _labelsByHashId[trackHashId] << labelId;

                    Q_EMIT trackLabelsAdded(trackHashId, { labelId });

                    return Success();
                }
            );

        return future;
    }

    SimpleFuture<Result> Labels::removeLabelFromTrack(uint trackHashId,
                                                      const QString& label)
    {
        if (!isValidPotentialName(label))
            return FutureResult(Error::labelNameInvalid());

        auto future =
            Concurrent::runOnThreadPool<Result>(
                globalThreadPool,
                [this, trackHashId, label]() -> Result
                {
                    auto db = Database::getDatabaseForCurrentThread();
                    if (!db) return Error::databaseUnvailable();

                    QMutexLocker lock(&_mutex);

                    auto labelId = _nameToId.value(label, 0);
                    if (labelId == 0)
                        return NoOp();

                    auto it = _labelDataByLabelId.find(labelId);
                    if (it == _labelDataByLabelId.end())
                    {
                        qWarning() << "Labels: no data found for label with ID"
                                   << labelId;
                        return Error::internalError();
                    }

                    LabelData* labelData = &it.value();

                    if (!labelData->hashes.contains(trackHashId))
                        return NoOp();

                    auto disconnectResult =
                        db->disconnectLabelFromHash(labelId, trackHashId);
                    if (disconnectResult.failed())
                        return Error::internalError(); // TODO : find a better error

                    labelData->hashes.remove(trackHashId);
                    _labelsByHashId[trackHashId].remove(labelId);

                    Q_EMIT trackLabelsRemoved(trackHashId, { labelId });

                    return Success();
                }
            );

        return future;
    }

    QList<quint32> Labels::getLabelsInActiveUse()
    {
        QMutexLocker lock(&_mutex);

        QList<quint32> result;
        result.reserve(_labelDataByLabelId.size()); // probably more than we really need

        for (auto it = _labelDataByLabelId.constBegin();
             it != _labelDataByLabelId.constEnd();
             ++it)
        {
            if (it->inActiveUse)
                result.append(it.key());
        }

        return result;
    }

    QList<quint32> Labels::getLabelsOfTrack(uint trackHashId)
    {
        QMutexLocker lock(&_mutex);

        return ContainerUtil::toList(_labelsByHashId[trackHashId]);
    }

    ResultOrError<QHash<quint32, QString>, Result> Labels::getLabelNames(
                                                                QList<quint32> labelIds)
    {
        QMutexLocker lock(&_mutex);

        QHash<quint32, QString> result;
        result.reserve(labelIds.size());

        for (auto labelId : labelIds)
        {
            auto it = _labelDataByLabelId.constFind(labelId);
            if (it == _labelDataByLabelId.constEnd())
                return Error::labelIdNotFound(labelId);

            result.insert(labelId, it.value().name);
        }

        return result;
    }

    bool Labels::isValidPotentialName(const QString& name)
    {
        if (name.isEmpty())
            return false;

        auto toLowercase = name.toLower();
        if (toLowercase != name)
            return false; // label names must be lowercase

        if (!name.front().isLetterOrNumber())
            return false; // label name must start with letter or number

        if (!name.back().isLetterOrNumber())
            return false; // label name must end with letter or number

        QChar previousChar = '\0';
        for (auto& character : name)
        {
            if (character == '-')
            {
                if (previousChar == character)
                    return false; // dashes are supposed to be used as separators only
            }
            else if (character.isLetterOrNumber())
            {
                // OK
            }
            else
            {
                return false; // invalid character
            }

            previousChar = character;
        }

        return true;
    }

    void Labels::loadFromDatabase()
    {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db) return;

        // load labels
        {
            auto labelsOrFailure = db->getLabels();
            if (labelsOrFailure.failed())
            {
                qWarning() << "failed to load labels!";
                return;
            }

            for (auto const& label : labelsOrFailure.result())
            {
                _labelDataByLabelId.insert(label.id, { .name = label.name});
                _nameToId.insert(label.name, label.id);
            }
        }

        // load label <-> hash associations
        {
            auto associationsOrFailure = db->getHashLabelAssociations();
            if (associationsOrFailure.failed())
            {
                qWarning() << "failed to load label/hash associations!";
                return;
            }

            for (auto const& hashLabelRecord : associationsOrFailure.result())
            {
                auto& labelData = _labelDataByLabelId[hashLabelRecord.labelId];
                labelData.hashes << hashLabelRecord.hashId;

                _labelsByHashId[hashLabelRecord.hashId] << hashLabelRecord.labelId;
            }
        }

        // determine which labels are in active use
        {
            int currentYear = QDateTime::currentDateTimeUtc().date().year();

            auto activeLabelIdsOrFailure = db->getLabelsInActiveUse(currentYear - 1);
            if (activeLabelIdsOrFailure.failed())
            {
                qWarning() << "failed to load list labels in active use!";
                return;
            }

            for (auto labelId : activeLabelIdsOrFailure.result())
            {
                _labelDataByLabelId[labelId].inActiveUse = true;
            }
        }
    }
}
