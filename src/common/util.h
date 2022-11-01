/*
    Copyright (C) 2016-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_UTIL_H
#define PMP_UTIL_H

#include <QByteArray>
#include <QChar>
#include <QDateTime>
#include <QString>

namespace PMP
{
    enum class DurationUnit
    {
        Seconds,
        Minutes,
        Hours,
        Days,
        Weeks,
        Months,
        Years,
    };

    class SimpleDuration
    {
    public:
        SimpleDuration() : _amount(0), _unit(DurationUnit::Seconds) {}

        SimpleDuration(int amount, DurationUnit unit)
         : _amount(amount), _unit(unit)
        {
            //
        }

        int amount() { return _amount; }
        DurationUnit unit() { return _unit; }

    private:
        int _amount;
        DurationUnit _unit;
    };

    class TextAndUpdateInterval
    {
    public:
        TextAndUpdateInterval(QString const& text, int intervalMs)
         : _text(text), _intervalMs(intervalMs)
        {
            //
        }

        QString text() const { return _text; }
        int intervalMs() const { return _intervalMs; }

    private:
        QString _text;
        int _intervalMs;
    };

    class Util
    {
    public:
        static unsigned getRandomSeed();

        static QString secondsToHoursMinuteSecondsText(qint32 totalSeconds);
        static QString millisecondsToShortDisplayTimeText(qint64 milliseconds);
        static QString millisecondsToLongDisplayTimeText(qint64 milliseconds);

        static QString getCountdownTimeText(qint64 millisecondsRemaining);
        static int getCountdownUpdateIntervalMs(qint64 millisecondsRemaining);

        static QString getHowLongAgoText(SimpleDuration howLongAgo);

        static SimpleDuration getHowLongAgoDuration(int secondsAgo);
        static SimpleDuration getHowLongAgoDuration(QDateTime pastTime, QDateTime now);
        static SimpleDuration getHowLongAgoDuration(QDateTime pastTime);

        static int getHowLongAgoUpdateIntervalMs(int secondsAgo);

        static TextAndUpdateInterval getHowLongAgoInfo(int secondsAgo);
        static TextAndUpdateInterval getHowLongAgoInfo(QDateTime pastTime, QDateTime now);
        static TextAndUpdateInterval getHowLongAgoInfo(QDateTime pastTime);

        static QString getCopyrightLine(bool mustBeAscii = true);

        static QByteArray generateZeroedMemory(int byteCount);

        static int compare(uint i1, uint i2)
        {
            if (i1 < i2) return -1;
            if (i1 > i2) return 1;
            return 0;
        }

        static int compare(int i1, int i2)
        {
            if (i1 < i2) return -1;
            if (i1 > i2) return 1;
            return 0;
        }

    private:
        Util();
    };
}
#endif
