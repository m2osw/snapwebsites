// Snap Websites Servers -- iterator to iterate a C string in reverse
// Copyright (C) 2011-2017  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
// Based on: http://stackoverflow.com/questions/236129/split-a-string-in-c#1493195
//
#pragma once

#include <iterator>

namespace snap
{

/** \brief Iterate a standard C string in reverse.
 *
 * This function let you iterate a standard C string, such as "12345",
 * in reverse order. The begin() and end() functions return a reverse
 * iterator (watch out!) We do not offer a rbeing() and rend() since
 * in most cases, using the string pointers as is would have to exact
 * same effect.
 *
 * \note
 * There may already be something in STL to be able to do that without
 * a specific implementation, if so I have not found it yet. This
 * class satisfy various needs such as finding the last character
 * that do not match a set of characters:
 *
 * \code
 *   std::string match(":,/");
 *   reverse_cstring rstr(start, start + strlen(start));
 *   auto last = std::find_if_not(
 *                      rstr.begin(),
 *                      rstr.end(),
 *                      [](char const c)
 *                      {
 *                          return match.find(c) != std::string::npos;
 *                      });
 *   // last.get() == "first character found in 'match' at the end part of string"
 *   // for example, in "/,:test:,/" would return a pointer on the last ':'
 * \endcode
 *
 * \warning
 * This function uses the specified start and end pointer of a C string
 * that must be valid as long as these iterators are used.
 */
template <typename T>
class reverse_cstring
{
public:

    class iterator
        : public std::iterator<std::random_access_iterator_tag, T>
    {
    public:
        // Iterator traits
        //
        typedef std::ptrdiff_t      difference_type;
        typedef T                   value_type;
        typedef T *                 pointer;
        typedef T &                 reference;

        // ForwardIterator
        //
        reference operator * ()
        {
            return i_[-1];
        }

        value_type operator * () const
        {
            return i_[-1];
        }

        iterator operator ++ (int)
        {
            iterator const copy(*this);
            --i_;
            return copy;
        }

        iterator & operator ++ ()
        {
            --i_;
            return *this;
        }

        // BidirectionalIterator
        //
        iterator operator -- (int)
        {
            iterator const copy(*this);
            ++i_;
            return copy;
        }

        iterator & operator -- ()
        {
            ++i_;
            return *this;
        }

        // RandomAccessIterator
        //
        iterator & operator += (int n) const
        {
            return i_ - n;
        }

        iterator operator + (int n) const
        {
            return iterator(i_ - n);
        }

        friend iterator operator + (int n, iterator & rhs)
        {
            return iterator(rhs.i_ - n);
        }

        iterator & operator -= (int n) const
        {
            i_ += n;
            return *this;
        }

        iterator operator - (int n) const
        {
            return iterator(i_ + n);
        }

        difference_type operator - (iterator const & rhs) const
        {
            return rhs.i_ - i_;
        }

        reference operator [] (int idx)
        {
            return i_[-idx - 1];
        }

        value_type operator [] (int idx) const
        {
            return i_[-idx - 1];
        }

        bool operator < (iterator const & rhs) const
        {
            return i_ > rhs.i_;
        }

        bool operator > (iterator const & rhs) const
        {
            return i_ < rhs.i_;
        }

        bool operator <= (iterator const & rhs) const
        {
            return i_ >= rhs.i_;
        }

        bool operator >= (iterator const & rhs) const
        {
            return i_ <= rhs.i_;
        }

        // Other
        //
        bool operator == (iterator const & rhs) const
        {
            return i_ == rhs.i_;
        }

        bool operator != (iterator const & rhs) const
        {
            return i_ != rhs.i_;
        }

        T * get() const
        {
            return i_;
        }

     private:
         friend class reverse_cstring;

         iterator(T * start)
             : i_(start)
         {
         }

         T *    i_;
    };

    reverse_cstring(T * start, T * end)
        : s_(start)
        , e_(end)
    {
    }

    iterator begin() const
    {
        return iterator(e_);
    }

    iterator end() const
    {
        return iterator(s_);
    }

    std::size_t size() const
    {
        return e_ - s_;
    }

private:
    T *       s_;
    T *       e_;
};


} // namespace snap
// vim: ts=4 sw=4 et
