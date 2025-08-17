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

#ifndef PMP_LABELSCONTROLLERIMPL_H
#define PMP_LABELSCONTROLLERIMPL_H

#include "labelscontroller.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>

namespace PMP::Client
{
    class ServerConnection;

    class LabelsControllerImpl : public LabelsController
    {
        Q_OBJECT
    public:
        explicit LabelsControllerImpl(ServerConnection* connection);

        SimpleFuture<AnyResultMessageCode> applyLabelToTrack(LocalHashId hashId,
                                                             QString label) override;
        SimpleFuture<AnyResultMessageCode> removeLabelFromTrack(LocalHashId hashId,
                                                                QString label) override;
        Future<QList<QString>, AnyResultMessageCode> getLabelNamesByTrack(
            LocalHashId hashId) override;
        Future<QHash<quint32,QString>, AnyResultMessageCode> getLabelNamesByIds(
            QList<quint32> labelIds) override;

    private Q_SLOTS:
        void onTrackLabelsChanged(LocalHashId hashId, QList<quint32> labelsAddedIds,
                                  QList<quint32> labelsRemovedIds);

    private:
        Future<QHash<quint32, QString>, AnyResultMessageCode>
            getLabelNamesFromIdsInternal(QList<quint32> labelIds);

        Future<SuccessType, AnyResultMessageCode> fetchMissingLabelNames(
                                                                QList<quint32> labelIds);
        QHash<quint32, QString> getLabelIdsToNamesMappingAssumingFetched(
                                                                QList<quint32> labelIds);

        ServerConnection* _connection;
        QHash<quint32, QString> _labelIdToName;
        QHash<QString, quint32> _labelNameToId;
        QHash<LocalHashId, QSet<quint32>> _hashToLabelIds;
    };
}
#endif
