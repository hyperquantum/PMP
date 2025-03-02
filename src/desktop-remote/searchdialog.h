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

#ifndef PMP_SEARCHDIALOG_H
#define PMP_SEARCHDIALOG_H

#include "client/localhashid.h"

#include <QAbstractTableModel>
#include <QDialog>
#include <QList>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Ui
{
    class SearchDialog;
}

namespace PMP::Client
{
    class CollectionWatcher;
    class ServerInterface;
}

namespace PMP
{
    class SearchData;

    class SearchResultsTableModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        SearchResultsTableModel(QObject* parent,
                                Client::ServerInterface* serverInterface);

        void setTracksList(QList<Client::LocalHashId> tracks);

        int rowCount(const QModelIndex& parent = QModelIndex()) const;
        int columnCount(const QModelIndex& parent = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    private:
        QVariant trackData(Client::LocalHashId trackId, int column, int role) const;

        Client::CollectionWatcher* _collectionWatcher;
        QList<Client::LocalHashId> _tracks;
    };

    class SearchDialog : public QDialog
    {
        Q_OBJECT

    public:
        SearchDialog(QWidget* parent, SearchData* searchData,
                     Client::ServerInterface* serverInterface);
        ~SearchDialog();

    private:
        void onTextEdited();
        void onEditingFinished();
        void onTypingTimerTimeout();
        void setSearchText(QString text);

        Ui::SearchDialog* _ui;
        QTimer* _typingTimer;
        SearchData* _searchData;
        Client::CollectionWatcher* _collectionWatcher;
        SearchResultsTableModel* _searchResultsModel;
        QString _searchText;
    };
}
#endif
