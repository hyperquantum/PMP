/*
    Copyright (C) 2015-2016, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_TRIBOOL_H
#define PMP_TRIBOOL_H

namespace PMP {

    class TriBool {
    public:
        TriBool() : _value(0) { /* <-- default constructed value is 'unknown' */ }
        TriBool(bool initialValue) : _value(1 + (initialValue ? 1 : 0)) { /* */ }
        TriBool(TriBool const& other) : _value(other._value) { /* */ }

        explicit TriBool(int number) : _value(1 + (number != 0 ? 1 : 0)) { /* */ }

        template<class T> explicit TriBool(T const* pointer)
            : _value(1 + (pointer != nullptr ? 1 : 0))
        {
            //
        }

        void reset() {
            _value = 0;
        }

        bool isUnknown() const { return _value == 0; }
        bool isKnown() const { return _value != 0; }
        bool isTrue() const { return _value >= 2; }
        bool isFalse() const { return _value == 1; }

        bool toBool(bool resultIfUnknown = false) const {
            return (_value == 0) ? resultIfUnknown : (_value - 1);
        }

        bool isIdenticalTo(TriBool other) const { return _value == other._value; }

        TriBool operator ! () const {
            /*  0 -> 0
                1 -> 2
                2 -> 1 */
            return _value == 0 ? TriBool() : TriBool(_value == 1);
        }

        friend TriBool operator == (TriBool a, TriBool b);
        friend TriBool operator != (TriBool a, TriBool b);
        friend TriBool operator & (TriBool a, TriBool b);
        friend TriBool operator | (TriBool a, TriBool b);

    private:
        unsigned char _value; /* 0=unknown, 1=false, 2=true */
    };

    inline TriBool operator == (TriBool a, TriBool b) {
        if ((a._value | b._value) == 0) return TriBool();
        return a._value == b._value;
    }

    inline TriBool operator != (TriBool a, TriBool b) {
        if ((a._value | b._value) == 0) return TriBool();
        return a._value != b._value;
    }

    /*
    Bitwise tables for internal values:
            Bit-AND                 Bit-OR                Bit-XOR
          | 0 | 1 | 2 |          | 0 | 1 | 2 |          | 0 | 1 | 2 |
       ---+---+---+---|       ---+---+---+---'       ---+---+---+---'
        0 | 0 | 0 | 0 |        0 | 0 | 1 | 2 |        0 | 0 | 1 | 2 |
       ---+---+---+---|       ---+---+---+---'       ---+---+---+---'
        1 | 0 | 1 | 0 |        1 | 1 | 1 | 3 |        1 | 1 | 0 | 3 |
       ---+---+---+---|       ---+---+---+---'       ---+---+---+---'
        2 | 0 | 0 | 2 |        2 | 2 | 3 | 2 |        2 | 2 | 3 | 0 |
       ---+---+---+---'       ---+---+---+---'       ---+---+---+---'

    Arithmetic tables for internal values:
              SUM                DIFFERENCE
          | 0 | 1 | 2 |          | 0 | 1 | 2 |
       ---+---+---+---|       ---+---+---+---'
        0 | 0 | 1 | 2 |        0 | 0 | 1 | 2 |
       ---+---+---+---|       ---+---+---+---'
        1 | 1 | 2 | 3 |        1 | -1| 0 | 1 |
       ---+---+---+---|       ---+---+---+---'
        2 | 2 | 3 | 4 |        2 | -2| -1| 0 |
       ---+---+---+---'       ---+---+---+---'

    Truth tables for TriBool:
              AND                    OR
          | 0 | 1 | 2 |          | 0 | 1 | 2 |
       ---+---+---+---|       ---+---+---+---'
        0 | 0 | 1 | 0 |        0 | 0 | 0 | 2 |
       ---+---+---+---|       ---+---+---+---'
        1 | 1 | 1 | 1 |        1 | 0 | 1 | 2 |
       ---+---+---+---|       ---+---+---+---'
        2 | 0 | 1 | 2 |        2 | 2 | 2 | 2 |
       ---+---+---+---'       ---+---+---+---'
    */

    inline TriBool operator & (TriBool a, TriBool b) {
        if ((a._value | b._value) & 1) return false;
        return (a._value & b._value) ? true : TriBool();
    }

    inline TriBool operator | (TriBool a, TriBool b) {
        if ((a._value | b._value) & 2) return true;
        return (a._value & b._value) ? false : TriBool();
    }
}
#endif
