/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "trackprogresswidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

namespace PMP {

    TrackProgressWidget::TrackProgressWidget(QWidget* parent)
     : QWidget(parent), /*_trackID(0),*/ _trackLength(-1), _trackPosition(-1)
    {
        //
    }

    TrackProgressWidget::~TrackProgressWidget() {
        //
    }

    QSize TrackProgressWidget::minimumSizeHint() const {
        return QSize(0, 18);
    }

    QSize TrackProgressWidget::sizeHint() const {
        return QSize(256, 18);
    }

    void TrackProgressWidget::setCurrentTrack(/*uint ID,*/ qint64 length) {
        //_trackID = ID;
        _trackLength = length;
        update();
    }

    void TrackProgressWidget::setTrackPosition(qint64 position) {
        _trackPosition = position;
        update();
    }

    void TrackProgressWidget::paintEvent(QPaintEvent* event) {
        QPainter painter(this);

        painter.setRenderHint(QPainter::Antialiasing);

        QRect rect = this->rect().adjusted(+1, +1, -1, -1);

        if (_trackLength <= 0) {
            painter.fillRect(rect, QBrush(QColor(200, 200, 200)));
            return;
        }

        painter.fillRect(rect, QBrush(QColor(20, 40, 140)));

        if (_trackPosition > 0) {
            QRectF rect2(rect);
            int x = _trackPosition * (rect2.width() - 1) / _trackLength;
            rect2.setWidth(x);
            painter.fillRect(rect2, QBrush(QColor(20, 240, 20)));
        }

        painter.setPen(QPen(Qt::gray));
        painter.drawRect(rect);
    }

    void TrackProgressWidget::mousePressEvent(QMouseEvent* event) {
        if (_trackLength > 0 && event->button() == Qt::LeftButton) {
            QRect rect = this->rect().adjusted(+1, +1, -1, -1);

            if (rect.contains(event->pos())) {
                int x = event->x();
                qint64 position = (x - rect.x()) * _trackLength / rect.width();
                emit seekRequested(position);
            }
        }

        /* call default implementation */
        QWidget::mousePressEvent(event);
    }

}
