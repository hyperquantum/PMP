/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_TRACKPROGRESSWIDGET_H
#define PMP_TRACKPROGRESSWIDGET_H

#include <QWidget>

namespace PMP {

    class TrackProgressWidget : public QWidget {
        Q_OBJECT

    public:
        explicit TrackProgressWidget(QWidget* parent = 0);
        ~TrackProgressWidget();

        QSize minimumSizeHint() const override;
        QSize sizeHint() const override;

    public slots:
        void setCurrentTrack(/*uint ID,*/ qint64 length);
        void setTrackPosition(qint64 position);

    Q_SIGNALS:
        void seekRequested(/*uint trackID,*/ qint64 position);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;

    private slots:


    private:
        //uint _trackID;
        qint64 _trackLength;
        qint64 _trackPosition;
    };
}
#endif
