/*
    Copyright (C) 2022-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include <QtGlobal>

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
    class alignas(T) Nullable
    {
    public:
        constexpr Nullable() : _hasValue { false }
        {
            //
        }

        constexpr Nullable(NullType) : _hasValue { false }
        {
            //
        }

        constexpr Nullable(T const& contents)
         : _hasValue(true)
        {
            new (reinterpret_cast<T*>(&_valueStorage)) T(contents);
        }

        Nullable(Nullable<T> const& other)
         : _hasValue(other.hasValue())
        {
            if (other.hasValue())
            {
                new (reinterpret_cast<T*>(&_valueStorage)) T(other.value());
            }
        }

        ~Nullable()
        {
            setToNull();
        }

        constexpr bool hasValue() const { return _hasValue; }
        constexpr bool isNull() const { return !_hasValue; }

        T& value()
        {
            Q_ASSERT_X(hasValue(), "Nullable::value()", "nullable has no value");
            return *reinterpret_cast<T*>(&_valueStorage);
        }

        T const& value() const
        {
            Q_ASSERT_X(hasValue(), "Nullable::value()", "nullable has no value");
            return *reinterpret_cast<T const*>(&_valueStorage);
        }

        constexpr T valueOr(T const& alternative) const
        {
            return hasValue() ? value() : alternative;
        }

        void setToNull()
        {
            if (_hasValue)
            {
                value().~T();
                _hasValue = false;
            }
        }

        Nullable<T>& operator=(NullType) { setToNull(); return *this; };

        Nullable<T>& operator=(Nullable<T> const& other)
        {
            if (this == &other)
                return *this;

            setToNull();

            if (other._hasValue)
            {
                new (reinterpret_cast<T*>(&_valueStorage)) T(other.value());
                _hasValue = true;
            }

            return *this;
        }

    private:
        alignas(T) quint8 _valueStorage[sizeof(T)] { 0 };
        bool _hasValue;
    };

    template<class T>
    constexpr bool operator==(Nullable<T> const& first, Nullable<T> const& second)
    {
        if (first.hasValue() != second.hasValue())
            return false;

        return first.hasValue() && first.value() == second.value();
    }

    template<class T>
    constexpr bool operator!=(Nullable<T> const& first, Nullable<T> const& second)
    {
        return !(first == second);
    }

    template<class T>
    constexpr bool operator==(Nullable<T> const& first, T const& second)
    {
        return first.hasValue() && first.value() == second;
    }

    template<class T>
    constexpr bool operator!=(Nullable<T> const& first, T const& second)
    {
        return !(first == second);
    }

    template<class T>
    constexpr bool operator==(T const& first, Nullable<T> const& second)
    {
        return second.hasValue() && second.value() == first;
    }

    template<class T>
    constexpr bool operator!=(T const& first, Nullable<T> const& second)
    {
        return !(first == second);
    }

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

    template <class T>
    constexpr Nullable<T> nullOf() { return {}; }
}
#endif
