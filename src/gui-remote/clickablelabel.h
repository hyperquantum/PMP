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

#ifndef PMP_CLICKABLELABEL_H
#define PMP_CLICKABLELABEL_H

#include <QLabel>

namespace PMP
{

    class ClickableLabel : public QLabel
    {
        Q_OBJECT
    public:
        explicit ClickableLabel(QWidget* parent = nullptr,
                                Qt::WindowFlags f = Qt::WindowFlags());
        ~ClickableLabel();

        static ClickableLabel* replace(QLabel*& existingLabel);

    Q_SIGNALS:
        void clicked();

    protected:
        void mousePressEvent(QMouseEvent* event) override;
    };
}
#endif
