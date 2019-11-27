// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapdatabase
// contact@m2osw.com
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


/** \file
 * \brief Database file implementation.
 *
 * Each table uses one or more files. Each file is handled by a dbfile
 * object and a corresponding set of blocks.
 */

// self
//
#include    "snapdatabase/file/hash.h"

#include    "snapdatabase/exception.h"

// C lib
//
#include    <string.h>

// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{


constexpr hash_t        END_OF_BUFFER = static_cast<hash_t>(-1);


/** \brief Init the hash with the specified seed.
 *
 * The hash class starts with the specified seed. By changing the seed
 * you can reuse the same class as if you were using several different
 * hash functions. This is how we create multiple hashes for bloom
 * filters.
 *
 * \param[in] seed  The seed to start with.
 */
hash::hash(hash_t seed)
    : f_hash(seed)
{
}


hash_t hash::get_byte() const
{
    if(f_temp_size > 0)
    {
        --f_temp_size;
        return f_temp[f_temp_size];
    }

    if(f_size > 0)
    {
        uint8_t const v(*f_buffer);
        ++f_buffer;
        --f_size;
        return v;
    }

    return END_OF_BUFFER;
}


hash_t hash::peek_byte(int pos) const
{
    if(static_cast<std::size_t>(pos) < f_temp_size)
    {
        // bytes are in reverse order in f_temp
        //
        return f_temp[f_temp_size - pos - 1];
    }
    pos -= f_temp_size;

    if(static_cast<std::size_t>(pos) < f_size)
    {
        return f_buffer[pos];
    }

    return 0;
}


size_t hash::size() const
{
    return f_temp_size + f_size;
}


void hash::get_64bits(hash_t & v1, hash_t & v2)
{
    if(f_temp_size == 0 && f_size >= 8)
    {
        // faster this way
        //
        v1 = (f_buffer[0] << 24)
           + (f_buffer[1] << 16)
           + (f_buffer[2] <<  8)
           + (f_buffer[3] <<  0);

        v2 = (f_buffer[4] << 24)
           + (f_buffer[5] << 16)
           + (f_buffer[6] <<  8)
           + (f_buffer[7] <<  0);

        f_buffer += 8;
        f_size -= 8;

        return;
    }

    if(size() >= 8)
    {
        v1 = (get_byte() << 24)
           + (get_byte() << 16)
           + (get_byte() <<  8)
           + (get_byte() <<  0);

        v2 = (get_byte() << 24)
           + (get_byte() << 16)
           + (get_byte() <<  8)
           + (get_byte() <<  0);

        return;
    }

    throw snapdatabase_logic_error("called get_64bits() when the input buffer is less than 64 bits");
}


void hash::peek_64bits(hash_t & v1, hash_t & v2) const
{
    switch(size())
    {
    case 7:
        v1 = (get_byte() << 24)
           + (get_byte() << 16)
           + (get_byte() <<  8)
           + (get_byte() <<  0);

        v2 = (get_byte() << 16)
           + (get_byte() <<  8)
           + (get_byte() <<  0);
        break;

    case 6:
        v1 = (get_byte() << 24)
           + (get_byte() << 16)
           + (get_byte() <<  8)
           + (get_byte() <<  0);

        v2 = (get_byte() <<  8)
           + (get_byte() <<  0);
        break;

    case 5:
        v1 = (get_byte() << 24)
           + (get_byte() << 16)
           + (get_byte() <<  8)
           + (get_byte() <<  0);

        v2 = (get_byte() <<  0);
        break;

    case 4:
        v1 = (get_byte() << 24)
           + (get_byte() << 16)
           + (get_byte() <<  8)
           + (get_byte() <<  0);

        v2 = 0;
        break;

    case 3:
        v1 = (get_byte() << 16)
           + (get_byte() <<  8)
           + (get_byte() <<  0);

        v2 = 0;
        break;

    case 2:
        v1 = (get_byte() <<  8)
           + (get_byte() <<  0);

        v2 = 0;
        break;

    case 1:
        v1 = (get_byte() <<  0);

        v2 = 0;
        break;

    case 0:
        v1 = 0;
        v2 = 0;
        break;

    default:
        throw snapdatabase_logic_error("called peek_64bits() when the input buffer is 64 bits or more");

    }
}


// hash function taken from: https://github.com/ArashPartow/bloom
// and modified to work incrementally.
//
void hash::add(uint8_t const * v, std::size_t buffer_size)
{
    f_buffer = v;
    f_size = buffer_size;

    while(size() >= 8)
    {
        hash_t v1(0);
        hash_t v2(0);
        get_64bits(v1, v2);

        f_hash ^= (f_hash <<  7) ^ (v1 * (f_hash >> 3))
             ^ (~((f_hash << 11) + (v2 ^ (f_hash >> 5))));
    }

    if(f_temp_size > 0 && f_size > 0)
    {
        memmove(f_temp + f_size, f_temp, f_temp_size);
    }

    f_temp_size += f_size;
    for(int in(0); f_size > 0; ++in)
    {
        --f_size;
        f_temp[in] = f_buffer[f_size];
    }
}


hash_t hash::get() const
{
    hash_t h(f_hash);

    std::size_t sz(size());
    if(sz > 0)
    {
        hash_t loop(0);
        hash_t v1;
        hash_t v2;
        peek_64bits(v1, v2);

        if(sz >= 4)
        {
            h ^= ~((h << 11) + (v1 ^ (h >> 5)));
            ++loop;

            sz -= 4;
            v1 = v2;
        }

        if(sz >= 3)
        {
            v2 = v1 >> 8;
            if(loop != 0)
            {
                h ^=    (h <<  7) ^  v2 * (h >> 3);
            }
            else
            {
                h ^= (~((h << 11) + (v2 ^ (h >> 5))));
            }
            ++loop;

            sz = 1;
            v1 &= 255;
        }
        else if(sz == 2)
        {
            if(loop != 0)
            {
                h ^=    (h <<  7) ^  v1 * (h >> 3);
            }
            else
            {
                h ^= (~((h << 11) + (v1 ^ (h >> 5))));
            }
            //++loop; -- not necessary, we won't reuse it

            sz = 0;
        }

        if(sz > 0)
        {
            h += (v1 ^ (h * 0xA5A5A5A5)) + loop;
        }
    }

    return h;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
