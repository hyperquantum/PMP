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

#include "searchdialog.h"
#include "ui_searchdialog.h"

#include "common/util.h"

#include "client/collectionwatcher.h"
#include "client/serverinterface.h"

#include "searching.h"

#include <QSettings>
#include <QtDebug>
#include <QTimer>

using namespace PMP::Client;

namespace PMP
{
    SearchDialog::SearchDialog(QWidget* parent, SearchData* searchData,
                               ServerInterface* serverInterface)
     : QDialog(parent),
        _ui(new Ui::SearchDialog),
        _typingTimer(new QTimer(this)),
        _searchData(searchData),
        _collectionWatcher(&serverInterface->collectionWatcher()),
        _searchResultsModel(new SearchResultsTableModel(this, serverInterface))
    {
        _ui->setupUi(this);

        _typingTimer->setInterval(300);

        _collectionWatcher->enableCollectionDownloading();

        _ui->resultsTableView->setModel(_searchResultsModel);
        //_ui->resultsTableView->setDragEnabled(true);
        //_ui->resultsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _ui->resultsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

        _ui->resultsCountLabel->setVisible(false);

        connect(_ui->searchTextLineEdit, &QLineEdit::textEdited,
                this, &SearchDialog::onTextEdited);

        connect(_ui->searchTextLineEdit, &QLineEdit::editingFinished,
                this, &SearchDialog::onEditingFinished);

        connect(_typingTimer, &QTimer::timeout,
                this, &SearchDialog::onTypingTimerTimeout);

        connect(_ui->closeButton, &QPushButton::clicked,
                this, &SearchDialog::close);

        {
            QSettings settings(QCoreApplication::organizationName(),
                               QCoreApplication::applicationName());

            settings.beginGroup("searchdialog");

            _ui->resultsTableView->horizontalHeader()->restoreState(
                settings.value("columnsstate").toByteArray()
            );
        }
    }

    SearchDialog::~SearchDialog()
    {
        QSettings settings(QCoreApplication::organizationName(),
                           QCoreApplication::applicationName());

        settings.beginGroup("searchdialog");

        settings.setValue(
            "columnsstate", _ui->resultsTableView->horizontalHeader()->saveState()
        );

        delete _ui;
    }

    void SearchDialog::onTextEdited()
    {
        _typingTimer->start(); // start or restart
    }

    void SearchDialog::onEditingFinished()
    {
        _typingTimer->stop();

        setSearchText(_ui->searchTextLineEdit->text());
    }

    void SearchDialog::onTypingTimerTimeout()
    {
        _typingTimer->stop();

        setSearchText(_ui->searchTextLineEdit->text());
    }

    void SearchDialog::setSearchText(QString text)
    {
        if (_searchText == text)
            return;

        _searchText = text;

        qDebug() << "SearchDialog: search text has changed to:" << _searchText;

        SearchQuery query { _searchText };

        if (query.isEmpty())
        {
            _searchResultsModel->setTracksList({});
            _ui->resultsCountLabel->setVisible(false);
            return;
        }

        auto searchResults = _searchData->getAllMatchesForQuery(query);
        auto resultsCount = searchResults.size();

        qDebug() << "SearchDialog: got" << resultsCount << "results";

        _ui->resultsCountLabel->setText(tr("%n match(es) found", "", resultsCount));
        _ui->resultsCountLabel->setVisible(true);

        _searchResultsModel->setTracksList(searchResults);
    }

    /* ============== SearchResultsTableModel ============== */

    SearchResultsTableModel::SearchResultsTableModel(QObject* parent,
                                                     ServerInterface* serverInterface)
     : QAbstractTableModel(parent),
        _collectionWatcher(&serverInterface->collectionWatcher())
    {
        //
    }

    void SearchResultsTableModel::setTracksList(QList<LocalHashId> tracks)
    {
        if (_tracks.size() > 0)
        {
            beginRemoveRows({}, 0, _tracks.size() - 1);
            _tracks.clear();
            endRemoveRows();
        }

        if (tracks.size() > 0)
        {
            beginInsertRows({}, 0, tracks.size() - 1);
            _tracks = tracks;
            endInsertRows();
        }
    }

    int SearchResultsTableModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent)
        return _tracks.size();
    }

    int SearchResultsTableModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent)
        return 4; /* title, artist, length, album */
    }

    QVariant SearchResultsTableModel::headerData(int section, Qt::Orientation orientation,
                                                 int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case 0: return QString(tr("Title"));
            case 1: return QString(tr("Artist"));
            case 2: return QString(tr("Length"));
            case 3: return QString(tr("Album"));
            }
        }

        return {};
    }

    namespace
    {
        inline QVariant toQVariant(QFlags<Qt::Alignment::enum_type> alignment)
        {
            return static_cast<Qt::Alignment::Int>(alignment);
        }
    }

    QVariant SearchResultsTableModel::data(const QModelIndex& index, int role) const
    {
        if (index.row() < _tracks.size())
        {
            return trackData(_tracks[index.row()], index.column(), role);
        }

        return {};
    }

    QVariant SearchResultsTableModel::trackData(LocalHashId trackId, int column,
                                                int role) const
    {
        switch (role)
        {
            case Qt::TextAlignmentRole:
                switch (column)
                {
                    case 2: return toQVariant(Qt::AlignRight | Qt::AlignVCenter);
                }
                break;
            case Qt::DisplayRole:
                {
                    auto trackOrNull = _collectionWatcher->getTrackFromCache(trackId);
                    if (trackOrNull.isNull()) { return {}; }

                    auto track = trackOrNull.value();

                    switch (column)
                    {
                    case 0: return track.title();
                    case 1: return track.artist();
                    case 2:
                    {
                        qint64 lengthInMilliseconds = track.lengthInMilliseconds();
                        if (lengthInMilliseconds < 0) { return "?"; }

                        return Util::millisecondsToShortDisplayTimeText(
                            lengthInMilliseconds);
                    }
                    case 3: return track.album();
                    }
                }
                break;
        }

        return {};
    }

}
