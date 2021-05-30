/*
    Copyright (C) 2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COMPATIBILITYINTERFACEVIEWCREATOR_H
#define PMP_COMPATIBILITYINTERFACEVIEWCREATOR_H

#include <QObject>

namespace PMP
{
    class CompatibilityInterface;

    class CompatibilityInterfaceViewCreator : public QObject
    {
        Q_OBJECT
    public:
        ~CompatibilityInterfaceViewCreator() {}

        virtual void createViewForInterface(CompatibilityInterface* interface) = 0;

    protected:
        CompatibilityInterfaceViewCreator(QObject* parent) : QObject(parent) {}
    };
}
#endif
