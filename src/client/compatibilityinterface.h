/*
    Copyright (C) 2021-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COMPATIBILITYINTERFACE_H
#define PMP_COMPATIBILITYINTERFACE_H

#include "common/compatibilityui.h"

#include <QObject>
#include <QVector>

namespace PMP::Client
{
    class CompatibilityInterfaceAction : public QObject
    {
        Q_OBJECT
    public:
        virtual ~CompatibilityInterfaceAction() {}

        virtual int id() const = 0;
        virtual CompatibilityUiActionState state() const = 0;
        virtual QString caption() const = 0;

    public Q_SLOTS:
        virtual void triggerAction() = 0;

    Q_SIGNALS:
        void stateChanged();
        void captionChanged();

    protected:
        CompatibilityInterfaceAction(QObject* parent) : QObject(parent) {}
    };

    class CompatibilityInterface : public QObject
    {
        Q_OBJECT
    public:
        virtual ~CompatibilityInterface() {}

        virtual int id() const = 0;
        virtual CompatibilityUiState state() const = 0;
        virtual QString title() const = 0;
        virtual QString caption() const = 0;
        virtual QString description() const = 0;

        virtual QVector<int> getActionIds() const = 0;
        virtual CompatibilityInterfaceAction* getAction(int actionId) const = 0;

    Q_SIGNALS:
        void stateChanged();
        void textChanged();

    protected:
        CompatibilityInterface(QObject* parent) : QObject(parent) {}
    };
}
#endif
