/*
    Copyright (C) 2014-2016, Kevin Andre <hyperquantum@gmail.com>

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
#include <QtDebug>

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
            painter.fillRect(rect, QBrush(QColor(220, 220, 220)));
            return;
        }

        painter.fillRect(rect, QBrush(QColor(210, 220, 255)));

        if (_trackPosition > 0) {
            auto position = qMin(_trackPosition, _trackLength);

            QRectF rect2(rect);
            rect2.adjust(+2, +2, -2, -2);
            int w = (position * rect2.width() + _trackLength / 2) / _trackLength;
            rect2.setWidth(w);
            painter.fillRect(rect2, QBrush(QColor(170, 190, 250)));
        }

        painter.setPen(QPen(QColor(170, 190, 250)));
        painter.drawRect(rect);
    }

    void TrackProgressWidget::mousePressEvent(QMouseEvent* event) {
        if (_trackLength > 0 && event->button() == Qt::LeftButton) {
            QRect rect = this->rect().adjusted(+1+2, +1+2, -1-2, -1-2);

            if (rect.contains(event->pos())) {
                int x = event->x();
                if (x < rect.left() || x > rect.x() + rect.width()) {
                    return;
                }

                qDebug() << "TrackProgressWidget::mousePressEvent  with x=" << x
                         << " and rectangle width" << rect.width();

                qint64 position =
                    ((x - rect.x()) * _trackLength + rect.width() / 2) / rect.width();
                qDebug() << " calculated position:" << position;

                int x_r =
                    (position * rect.width() + _trackLength / 2) / _trackLength + rect.x();
                qDebug() << " calculated x from calculated position would be:" << x_r;

                emit seekRequested(position);
            }
        }

        /* call default implementation */
        QWidget::mousePressEvent(event);
    }

}
