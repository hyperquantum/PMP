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

#include "searching.h"

#include <QtDebug>
#include <QTimer>

namespace PMP
{
    SearchDialog::SearchDialog(QWidget* parent, SearchData* searchData)
     : QDialog(parent),
        _ui(new Ui::SearchDialog),
        _typingTimer(new QTimer(this))
    {
        _ui->setupUi(this);

        _typingTimer->setInterval(300);

        connect(_ui->searchTextLineEdit, &QLineEdit::textEdited,
                this, &SearchDialog::onTextEdited);

        connect(_ui->searchTextLineEdit, &QLineEdit::editingFinished,
                this, &SearchDialog::onEditingFinished);

        connect(_typingTimer, &QTimer::timeout,
                this, &SearchDialog::onTypingTimerTimeout);

        connect(_ui->closeButton, &QPushButton::clicked,
                this, &SearchDialog::close);
    }

    SearchDialog::~SearchDialog()
    {
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

        // TODO
    }
}
