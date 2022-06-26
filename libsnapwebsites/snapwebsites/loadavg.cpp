// Load Balancing -- class used to handle the load average file
// Copyright (c) 2017-2019  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "snapwebsites/loadavg.h"


// snaplogger lib
//
#include <snaplogger/message.h>


// libaddr lib
//
#include <libaddr/addr.h>


// snapdev lib
//
#include <snapdev/raii_generic_deleter.h>


// C++ lib
//
#include <algorithm>
#include <memory>


// C lib
//
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


// last include
//
#include <snapdev/poison.h>



namespace snap
{


namespace
{


std::string g_filename;




int const LOADAVG_VERSION = 1;

struct loadavg_magic
{
    char                    f_name[4]{'L', 'A', 'V', 'G'};  // 'LAVG'
    uint16_t                f_version = LOADAVG_VERSION;    // 1+ representing the version
};


} // no name namespace


bool loadavg_file::load()
{
    // open the file
    //
    snapdev::raii_fd_t safe_fd(open(g_filename.c_str(), O_RDONLY));
    if(!safe_fd)
    {
        return false;
    }

    // lock the file in share mode (multiple read, no writes)
    //
    if(flock(safe_fd.get(), LOCK_SH) != 0)
    {
        return false;
    }

    // verify the magic
    //
    loadavg_magic magic;
    if(read(safe_fd.get(), &magic, sizeof(magic)) != sizeof(magic))
    {
        return false;
    }
    if(magic.f_name[0] != 'L'
    || magic.f_name[1] != 'A'
    || magic.f_name[2] != 'V'
    || magic.f_name[3] != 'G'
    || magic.f_version != LOADAVG_VERSION)
    {
        return false;
    }

    // load each item
    //
    for(;;)
    {
        loadavg_item item;
        ssize_t const r(read(safe_fd.get(), &item, sizeof(item)));
        if(r < 0)
        {
            return false;
        }
        if(r == 0)
        {
            // we got EOF
            break;
        }
        f_items.push_back(item);
    }

    // it worked
    //
    return true;
}


bool loadavg_file::save() const
{
    // open the file
    //
    snapdev::raii_fd_t safe_fd(open(g_filename.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    if(!safe_fd)
    {
        return false;
    }

    // lock the file in share mode (multiple read, no writes)
    //
    if(flock(safe_fd.get(), LOCK_EX) != 0)
    {
        return false;
    }

    // write the magic each time (in case the version changed
    // or we are creating a new file)
    //
    loadavg_magic magic;
    if(write(safe_fd.get(), &magic, sizeof(magic)) != sizeof(magic))
    {
        return false;
    }

    // write each item
    //
    for(auto const & item : f_items)
    {
        ssize_t const r(write(safe_fd.get(), &item, sizeof(item)));
        if(r < 0)
        {
            return false;
        }
    }

    // it worked
    //
    return true;
}


void loadavg_file::add(loadavg_item const & new_item)
{
    auto const & it(std::find_if(
            f_items.begin(),
            f_items.end(),
            [new_item](auto const & item)
            {
                return item.f_address == new_item.f_address;
            }));

    if(it == f_items.end())
    {
        f_items.push_back(new_item);
    }
    else
    {
        // replace existing item with new avg and timestamp
        it->f_timestamp = new_item.f_timestamp;
        it->f_avg = new_item.f_avg;
    }
}


/** \brief Remove old entries from the list of items.
 *
 * This function checks each item. If one has a date which is too
 * old (i.e. less than now minus \p how_old), then it gets removed
 * from the list. The computer may get re-added later.
 *
 * Assuming everything works as expected, a computer that stops
 * sending us the LOADAVG message is considered hanged in some
 * way so we do not want to send it any additional work.
 *
 * In most cases, you want to use the following code to find
 * the least busy system to connect to:
 *
 * \code
 *      snap::loadavg_file avg;
 *      avg.load();
 *      if(avg.remove_old_entries(10))
 *      {
 *          avg.save();
 *      }
 *      snap::loadavg_item const * item(avg.find_least_busy());
 * \endcode
 *
 * \param[in] how_old  The number of seconds after which an entry
 *                     is considered too old to be kept around.
 *
 * \return true if one or more items were removed.
 */
bool loadavg_file::remove_old_entries(int how_old)
{
    size_t const size(f_items.size());
    time_t const now(time(nullptr) - how_old);
    f_items.erase(std::remove_if(
            f_items.begin(),
            f_items.end(),
            [now](auto const & item)
            {
                return item.f_timestamp < now;
            }),
            f_items.end());
    return f_items.size() != size;
}


/** \brief Retrieve an entry using its IP address.
 *
 * This function searches for an item using the specified IP address.
 *
 * \param[in] addr  The address used to search the item.
 *
 * \return nullptr if no item matched, the pointer of the item if one
 *         was found with the proper information.
 */
loadavg_item const * loadavg_file::find(struct sockaddr_in6 const & addr) const
{
    auto const & it(std::find_if(
            f_items.begin(),
            f_items.end(),
            [addr](auto const & item)
            {
                return item.f_address == addr;
            }));

    if(it == f_items.end())
    {
        return nullptr;
    }

    return &*it;
}


/** \brief Search for least busy server.
 *
 * This function searches the list of servers and returns the one
 * which has the smallest load average amount.
 *
 * If you want to make sure only fresh data is considered, you
 * probably want to call the remove_old_entries() function first.
 *
 * Note that the function will always return an item if there is
 * at least one registered with a mostly current average load.
 * If somehow all the servers get removed (too old, unregistered,
 * etc.) then the function will return a null pointer.
 *
 * \return The least busy server or nullptr if no server is available.
 *
 * \sa remove_old_entries()
 */
loadavg_item const * loadavg_file::find_least_busy() const
{
    auto const & it(std::min_element(
            f_items.begin(),
            f_items.end(),
            [](auto const & a, auto const & b)
            {
                return a.f_avg < b.f_avg;
            }));

    if(it == f_items.end())
    {
        return nullptr;
    }

    return &*it;
}


} // namespace snap
// vim: ts=4 sw=4 et
