/*
    Copyright (C) 2015-2025, Kevin André <hyperquantum@gmail.com>

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

namespace PMP
{
    class TriBool
    {
    public:
        static const TriBool unknown;

        constexpr TriBool() : _value(0) {} /* default constructed value is 'unknown' */
        constexpr TriBool(bool initialValue) : _value(1 + (initialValue ? 1 : 0)) { }
        constexpr TriBool(TriBool const& other) = default;

        constexpr explicit TriBool(int number) : _value(1 + (number != 0 ? 1 : 0)) { }

        template<class T> constexpr explicit TriBool(T const* pointer)
            : _value(1 + (pointer != nullptr ? 1 : 0))
        {
            //
        }

        void reset()
        {
            _value = 0;
        }

        constexpr bool isUnknown() const { return _value == 0; }
        constexpr bool isKnown() const { return _value != 0; }
        constexpr bool isTrue() const { return _value >= 2; }
        constexpr bool isFalse() const { return _value == 1; }

        constexpr bool toBool(bool resultIfUnknown = false) const
        {
            return (_value == 0) ? resultIfUnknown : (_value - 1);
        }

        constexpr bool isIdenticalTo(TriBool other) const
        {
            return _value == other._value;
        }

        constexpr TriBool& operator=(TriBool const& other) = default;

        constexpr TriBool& operator &= (TriBool other)
        {
            if (isFalse() || other.isFalse())
                _value = 1; // set to false
            else if (isTrue() && other.isTrue())
                _value = 2; // set to true
            else
                _value = 0; // set to unknown

            return *this;
        }

        constexpr TriBool& operator |= (TriBool other)
        {
            if (isTrue() || other.isTrue())
                _value = 2; // set to true
            else if (isFalse() && other.isFalse())
                _value = 1; // set to false
            else
                _value = 0; // set to unknown

            return *this;
        }

        constexpr TriBool operator ! () const
        {
            if (isUnknown())
                return TriBool(); // return unknown

            return TriBool(isFalse());
        }

        friend constexpr TriBool operator == (TriBool a, TriBool b);
        friend constexpr TriBool operator != (TriBool a, TriBool b);
        friend constexpr TriBool operator & (TriBool a, TriBool b);
        friend constexpr TriBool operator | (TriBool a, TriBool b);

    private:
        unsigned char _value; /* 0=unknown, 1=false, 2=true */
    };

    constexpr inline TriBool operator == (TriBool a, TriBool b)
    {
        if (a.isUnknown() || b.isUnknown())
            return TriBool();

        return a.toBool() == b.toBool();
    }

    constexpr inline TriBool operator != (TriBool a, TriBool b)
    {
        return !(a == b);
    }

    constexpr inline TriBool operator & (TriBool a, TriBool b)
    {
        a &= b;
        return a;
    }

    constexpr inline TriBool operator | (TriBool a, TriBool b)
    {
        a |= b;
        return a;
    }
}
#endif
