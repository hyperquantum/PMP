/*
    Copyright (C) 2025-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_LABELSCONTROLLER_H
#define PMP_LABELSCONTROLLER_H

#include "common/future.h"
#include "common/resultmessageerrorcode.h"

#include "client/labelidandname.h"
#include "client/localhashid.h"

#include <QObject>
#include <QString>

namespace PMP::Client
{
    class LabelsController : public QObject
    {
        Q_OBJECT
    public:
        virtual ~LabelsController() {}

        virtual SimpleFuture<AnyResultMessageCode> applyLabelToTrack(LocalHashId hashId,
                                                                     QString label) = 0;
        virtual SimpleFuture<AnyResultMessageCode> removeLabelFromTrack(
                                                                     LocalHashId hashId,
                                                                     QString label) = 0;

        virtual Future<QList<QString>, AnyResultMessageCode> getLabelNamesByTrack(
                                                                LocalHashId hashId) = 0;
        virtual Future<QList<LabelIdAndName>, AnyResultMessageCode> getLabelsByTrack(
                                                                LocalHashId hashId) = 0;

        virtual Future<QHash<quint32,QString>, AnyResultMessageCode> getLabelNamesByIds(
                                                            QList<quint32> labelIds) = 0;

        virtual Future<QList<QString>, AnyResultMessageCode> getActiveLabelNames() = 0;
        virtual Future<QList<LabelIdAndName>, AnyResultMessageCode> getActiveLabels() = 0;

    Q_SIGNALS:
        void trackLabelsChanged(LocalHashId hashId);

    protected:
        explicit LabelsController(QObject* parent) : QObject(parent) {}
    };
}
#endif
