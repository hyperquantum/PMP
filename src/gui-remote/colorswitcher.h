/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COLORSWITCHER_H
#define PMP_COLORSWITCHER_H

#include <QColor>
#include <QVector>
#include <QWidget>

namespace PMP {

    class ColorSwitcher : public QWidget {
        Q_OBJECT
    public:
        ColorSwitcher();

        void setColors(QVector<QColor> colors);
        void setColors(QVector<QColor> colors, int colorIndex);

        int colorIndex() const;
        void setColorIndex(int colorIndex);

        QSize minimumSizeHint() const override;
        QSize sizeHint() const override;

    Q_SIGNALS:
        void colorIndexChanged();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;

    private:
        QVector<QColor> _colors;
        int _colorIndex;
    };
}
#endif
