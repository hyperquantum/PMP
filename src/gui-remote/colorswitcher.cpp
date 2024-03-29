/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include <QFontMetricsF>
#include <QMouseEvent>
#include <QPainter>
#include <QtDebug>

namespace PMP
{
    ColorSwitcher::ColorSwitcher()
     : _colors({ QColor(Qt::white) }),
       _colorIndex(0)
    {
        setCursor(Qt::PointingHandCursor);
        setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    }

    void ColorSwitcher::setColors(QVector<QColor> colors)
    {
        setColors(colors, 0);
    }

    void ColorSwitcher::setColors(QVector<QColor> colors, int colorIndex)
    {
        if (colors.empty())
        {
            _colors = { QColor(Qt::white) };
            _colorIndex = 0;
        }
        else
        {
            _colors = colors;
            _colorIndex = qBound(0, colorIndex, colors.size() - 1);
        }

        update();

        Q_EMIT colorIndexChanged();
    }

    int ColorSwitcher::colorIndex() const
    {
        return _colorIndex;
    }

    void ColorSwitcher::setColorIndex(int colorIndex)
    {
        if (_colorIndex == colorIndex)
            return;

        _colorIndex = qBound(0, colorIndex, _colors.size() - 1);

        update();

        Q_EMIT colorIndexChanged();
    }

    QSize ColorSwitcher::minimumSizeHint() const
    {
        auto size = desiredSize();

        return QSize(size, size);
    }

    QSize ColorSwitcher::sizeHint() const
    {
        return minimumSizeHint();
    }

    void ColorSwitcher::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event)

        QPainter painter(this);

        QRect rect = this->rect().adjusted(+1, +1, -1, -1);

        painter.fillRect(rect, QBrush(_colors[_colorIndex]));

        painter.setPen(QPen(Colors::instance().widgetBorder));
        painter.drawRect(rect);
    }

    void ColorSwitcher::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            _colorIndex++;
            if (_colorIndex >= _colors.size())
                _colorIndex = 0;

            update();

            Q_EMIT colorIndexChanged();
        }
    }

    qreal ColorSwitcher::desiredSize() const
    {
        QFontMetricsF metrics { font() };

        return metrics.height();
    }
}
