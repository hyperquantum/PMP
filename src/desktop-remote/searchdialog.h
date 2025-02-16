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

#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Ui
{
    class SearchDialog;
}

namespace PMP
{
    class SearchData;

    class SearchDialog : public QDialog
    {
        Q_OBJECT

    public:
        SearchDialog(QWidget* parent, SearchData* searchData);
        ~SearchDialog();

    private:
        void onTextEdited();
        void onEditingFinished();
        void onTypingTimerTimeout();
        void setSearchText(QString text);

        Ui::SearchDialog* _ui;
        QTimer* _typingTimer;
        QString _searchText;
    };
}
#endif
