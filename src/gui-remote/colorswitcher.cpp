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

#include "colorswitcher.h"

#include "colors.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtDebug>

namespace PMP {

    ColorSwitcher::ColorSwitcher()
     : _colors({ QColor(Qt::white) }),
       _colorIndex(0)
    {
        setCursor(Qt::PointingHandCursor);
        setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    }

    void ColorSwitcher::setColors(QVector<QColor> colors) {
        setColors(colors, 0);
    }

    void ColorSwitcher::setColors(QVector<QColor> colors, int colorIndex) {
        if (colors.empty()) {
            _colors = { QColor(Qt::white) };
            _colorIndex = 0;
        }
        else {
            _colors = colors;
            _colorIndex = qBound(0, colorIndex, colors.size() - 1);
        }

        update();

        Q_EMIT colorIndexChanged();
    }

    int ColorSwitcher::colorIndex() const {
        return _colorIndex;
    }

    void ColorSwitcher::setColorIndex(int colorIndex) {
        if (_colorIndex == colorIndex)
            return;

        _colorIndex = qBound(0, colorIndex, _colors.size() - 1);

        update();

        Q_EMIT colorIndexChanged();
    }

    QSize ColorSwitcher::minimumSizeHint() const {
        return QSize(16, 16);
    }

    QSize ColorSwitcher::sizeHint() const {
        return QSize(18, 18);
    }

    void ColorSwitcher::paintEvent(QPaintEvent* event) {
        Q_UNUSED(event)

        QPainter painter(this);

        QRect rect = this->rect();

        painter.fillRect(rect, QBrush(_colors[_colorIndex]));

        painter.setPen(QPen(Colors::instance().widgetBorder));
        painter.drawRect(rect.adjusted(0, 0, -1, -1));
    }

    void ColorSwitcher::mousePressEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            _colorIndex++;
            if (_colorIndex >= _colors.size())
                _colorIndex = 0;

            update();

            Q_EMIT colorIndexChanged();
        }
    }
}
