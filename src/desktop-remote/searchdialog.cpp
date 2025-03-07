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
#include "client/queuecontroller.h"
#include "client/queuehashesmonitor.h"
#include "client/serverinterface.h"

#include "colors.h"
#include "searching.h"
#include "trackinfodialog.h"

#include <QMenu>
#include <QSettings>
#include <QtDebug>
#include <QTimer>

using namespace PMP::Client;

namespace PMP
{
    SearchDialog::SearchDialog(QWidget* parent, SearchData* searchData,
                               ServerInterface* serverInterface,
                               QueueHashesMonitor* queueHashesMonitor,
                               UserForStatisticsDisplay* userForStatisticsDisplay)
     : QDialog(parent),
        _ui(new Ui::SearchDialog),
        _typingTimer(new QTimer(this)),
        _searchData(searchData),
        _serverInterface(serverInterface),
        _userStatisticsDisplay(userForStatisticsDisplay),
        _collectionWatcher(&serverInterface->collectionWatcher()),
        _searchResultsModel(new SearchResultsTableModel(this,
                                                        serverInterface,
                                                        queueHashesMonitor))
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

        connect(_ui->resultsTableView, &QTableView::customContextMenuRequested,
                this, &SearchDialog::resultsContextMenuRequested);

        connect(_ui->closeButton, &QPushButton::clicked,
                this, &SearchDialog::close);

        {
            QSettings settings(QCoreApplication::organizationName(),
                               QCoreApplication::applicationName());

            settings.beginGroup("searchdialog");

            restoreGeometry(settings.value("geometry").toByteArray());

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

        settings.setValue("geometry", saveGeometry());

        settings.setValue(
            "columnsstate", _ui->resultsTableView->horizontalHeader()->saveState()
        );

        delete _ui;
    }

    void SearchDialog::resultsContextMenuRequested(const QPoint& position)
    {
        qDebug() << "SearchDialog: contextmenu requested";

        auto index = _ui->resultsTableView->indexAt(position);
        if (!index.isValid()) return;

        auto trackId = _searchResultsModel->trackAt(index);
        if (trackId.isZero()) return;

        if (_resultsContextMenu)
            delete _resultsContextMenu;
        _resultsContextMenu = new QMenu(this);

        auto enqueueFrontAction =
            _resultsContextMenu->addAction(tr("Add to front of queue"));
        connect(
            enqueueFrontAction, &QAction::triggered,
            this,
            [this, trackId]()
            {
                qDebug() << "SearchDialog: context menu: enqueue (front) triggered";
                _serverInterface->queueController().insertQueueEntryAtFront(trackId);
            }
        );

        auto enqueueEndAction =
            _resultsContextMenu->addAction(tr("Add to end of queue"));
        connect(
            enqueueEndAction, &QAction::triggered,
            this,
            [this, trackId]()
            {
                qDebug() << "SearchDialog: context menu: enqueue (end) triggered";
                _serverInterface->queueController().insertQueueEntryAtEnd(trackId);
            }
        );

        _resultsContextMenu->addSeparator();

        auto trackInfoAction = _resultsContextMenu->addAction(tr("Track info"));
        connect(
            trackInfoAction, &QAction::triggered,
            this,
            [this, trackId]()
            {
                qDebug() << "SearchDialog: context menu: track info triggered";
                auto dialog = new TrackInfoDialog(this, _serverInterface,
                                                  _userStatisticsDisplay, trackId);
                connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
                dialog->open();
            }
        );

        auto popupPosition = _ui->resultsTableView->viewport()->mapToGlobal(position);
        _resultsContextMenu->popup(popupPosition);
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
                                                     ServerInterface* serverInterface,
                                                QueueHashesMonitor* queueHashesMonitor)
     : QAbstractTableModel(parent),
        _collectionWatcher(&serverInterface->collectionWatcher()),
        _queueHashesMonitor(queueHashesMonitor)
    {
        connect(_collectionWatcher, &CollectionWatcher::trackAvailabilityChanged,
                this, &SearchResultsTableModel::onTrackAvailabilityChanged);

        connect(_collectionWatcher, &CollectionWatcher::trackDataChanged,
                this, &SearchResultsTableModel::onTrackDataChanged);

        connect(_queueHashesMonitor, &QueueHashesMonitor::hashInQueuePresenceChanged,
                this, &SearchResultsTableModel::onHashInQueuePresenceChanged);
    }

    LocalHashId SearchResultsTableModel::trackAt(const QModelIndex& index) const
    {
        auto row = index.row();

        if (row < 0 || row >= _tracks.size())
            return {};

        return _tracks[row];
    }

    void SearchResultsTableModel::setTracksList(QList<LocalHashId> tracks)
    {
        if (_tracks.size() > 0)
        {
            beginRemoveRows({}, 0, _tracks.size() - 1);
            _tracks.clear();
            _hashIdToIndex.clear();
            endRemoveRows();
        }

        if (tracks.size() > 0)
        {
            beginInsertRows({}, 0, tracks.size() - 1);

            _tracks = tracks;

            for (int i = 0; i < tracks.size(); ++i)
            {
                _hashIdToIndex[tracks[i]] = i;
            }

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

    void SearchResultsTableModel::onTrackAvailabilityChanged(Client::LocalHashId hashId,
                                                             bool isAvailable)
    {
        int index = getTrackIndex(hashId);
        if (index < 0)
            return;

        markRowAsChanged(index);
    }

    void SearchResultsTableModel::onTrackDataChanged(Client::CollectionTrackInfo track)
    {
        int index = getTrackIndex(track.hashId());
        if (index < 0)
            return;

        // TODO : what if the new track data no longer satisfies the search query?

        markRowAsChanged(index);
    }

    void SearchResultsTableModel::onHashInQueuePresenceChanged(Client::LocalHashId hashId)
    {
        int index = getTrackIndex(hashId);
        if (index < 0)
            return;

        markLeftColumnAsChanged(index);
    }

    int SearchResultsTableModel::getTrackIndex(Client::LocalHashId trackId) const
    {
        auto it = _hashIdToIndex.constFind(trackId);
        if (it != _hashIdToIndex.constEnd())
            return it.value();

        return -1;
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
            case Qt::DecorationRole:
                if (column == 0)
                {
                    if (_queueHashesMonitor->isPresentInQueue(trackId))
                    {
                        return QIcon(":/mediabuttons/queue.svg");
                    }
                }
                break;
            case Qt::ForegroundRole:
                {
                    auto trackOrNull = _collectionWatcher->getTrackFromCache(trackId);
                    if (trackOrNull.isNull()) { return {}; }

                    auto track = trackOrNull.value();

                    if (!track.isAvailable())
                        return QBrush(Colors::instance().inactiveItemForeground);
                }
                break;
        }

        return {};
    }

    void SearchResultsTableModel::markLeftColumnAsChanged(int index)
    {
        Q_EMIT dataChanged(createIndex(index, 0), createIndex(index, 0));
    }

    void SearchResultsTableModel::markRowAsChanged(int index)
    {
        Q_EMIT dataChanged(createIndex(index, 0), createIndex(index, 4 - 1));
    }
}
