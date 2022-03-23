/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_NULLABLE_H
#define PMP_NULLABLE_H

namespace PMP
{
    class NullType
    {
    public:
        constexpr NullType() {}

        NullType const& operator=(NullType const&) = delete;

        constexpr bool operator==(NullType) { return true; }
        constexpr bool operator!=(NullType) { return false; }
    };

    constexpr NullType null = {};

    template<class T>
    class Nullable
    {
    public:
        constexpr Nullable() : _hasValue {false}, _value {}
        {
            //
        }

        constexpr Nullable(NullType) : _hasValue(), _value {}
        {
            //
        }

        constexpr Nullable(T const& contents)
         : _hasValue(true), _value(contents)
        {
            //
        }

        constexpr bool hasValue() const { return _hasValue; }
        constexpr bool isNull() const { return !_hasValue; }
        constexpr T value() const { return _value; }

        void setToNull() { _hasValue = false; }

        Nullable<T>& operator=(Nullable<T> const& nullable) = default;
        Nullable<T>& operator=(NullType) { setToNull(); return *this; };

        Nullable<T>& operator=(T const& value)
        {
            _value = value;
            _hasValue = true;
            return *this;
        }

        constexpr bool operator==(Nullable<T> const& other) const
        {
            if (!hasValue())
                return !other.hasValue();

            return other.hasValue() && value() == other.value();
        }

        constexpr bool operator!=(Nullable<T> const& other) const
        {
            return !(*this == other);
        }

    private:
        bool _hasValue;
        T _value;
    };

    template<class T>
    constexpr bool operator==(Nullable<T> const& nullable, NullType)
    {
        return nullable.isNull();
    }

    template<class T>
    constexpr bool operator!=(Nullable<T> const& nullable, NullType)
    {
        return !(nullable == null);
    }

    template<class T>
    constexpr bool operator==(NullType, Nullable<T> const& nullable)
    {
        return nullable.isNull();
    }

    template<class T>
    constexpr bool operator!=(NullType, Nullable<T> const& nullable)
    {
        return !(null == nullable);
    }
}
#endif
