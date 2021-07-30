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

#ifndef PMP_COMPATIBILITYUICONTROLLERCOLLECTION_H
#define PMP_COMPATIBILITYUICONTROLLERCOLLECTION_H

#include <QHash>
#include <QObject>

namespace PMP
{
    class CompatibilityUiController;
    class ServerInterface;

    class IndexationUiController;
    class TestUiController;

    class CompatibilityUiControllerCollection : public QObject
    {
        Q_OBJECT
    public:
        CompatibilityUiControllerCollection(QObject* parent,
                                            ServerInterface* serverInterface);

        void activateIndexationController();
        void activateTestController();

        QList<int> getControllerIds() const { return _controllersById.keys(); }

        CompatibilityUiController* getControllerById(int id) const
        {
            return _controllersById.value(id, nullptr);
        }

    Q_SIGNALS:
        void textChanged(int interfaceId);
        void stateChanged(int interfaceId);
        void actionCaptionChanged(int interfaceId, int actionId);
        void actionStateChanged(int interfaceId, int actionId);
        void actionSuccessful(int interfaceId, int actionId, uint clientReference);
        void actionFailed(int interfaceId, int actionId, uint clientReference);

    private:
        void activateController(CompatibilityUiController* controller);
        void connectSignals(CompatibilityUiController* controller);

        ServerInterface* _serverInterface;
        IndexationUiController* _indexationController;
        TestUiController* _testController;
        QHash<int, CompatibilityUiController*> _controllersById;
    };
}
#endif
