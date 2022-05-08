// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

// eventdispatcher lib
//
#include    <eventdispatcher/communicator.h>

#include    <eventdispatcher/tcp_bio_client.h>


// libexcept lib
//
#include    <libexcept/exception.h>



namespace snap
{

DECLARE_MAIN_EXCEPTION(snap_lock_exception);

DECLARE_EXCEPTION(snap_lock_exception, snap_lock_failed_exception);
DECLARE_EXCEPTION(snap_lock_exception, snap_lock_not_initialized);





// the lock internal implementation
namespace details
{
class lock_connection;
}


/** \brief Inter-computer snap_lock.
 *
 * \todo
 * We may want to rethink about the name because we already have a snap_lock
 * inside our snap_thread.
 */
class snap_lock
{
public:
    typedef std::shared_ptr<snap_lock>      pointer_t;
    typedef int32_t                         timeout_t;

    static constexpr timeout_t    SNAP_LOCK_DEFAULT_TIMEOUT = 5;        // in seconds
    static constexpr timeout_t    SNAP_LOCK_MINIMUM_TIMEOUT = 3;        // in seconds
    static constexpr timeout_t    SNAP_UNLOCK_MINIMUM_TIMEOUT = 60;     // in seconds
    static constexpr timeout_t    SNAP_UNLOCK_USES_LOCK_TIMEOUT = -1;   // use lock_duration as the unlock_duration
    static constexpr timeout_t    SNAP_MAXIMUM_OBTENTION_TIMEOUT = 60 * 60;  // limit obtension timeout to this value
    static constexpr timeout_t    SNAP_MAXIMUM_TIMEOUT = 7 * 24 * 60 * 60;   // no matter what limit all timeouts to this value (7 days)

                        snap_lock(
                              std::string const & object_name
                            , timeout_t lock_duration = -1
                            , timeout_t lock_obtention_timeout = -1
                            , timeout_t unlock_duration = SNAP_UNLOCK_USES_LOCK_TIMEOUT);

    static void         initialize_lock_duration_timeout(timeout_t timeout);
    static timeout_t    current_lock_duration_timeout();

    static void         initialize_lock_obtention_timeout(timeout_t timeout);
    static timeout_t    current_lock_obtention_timeout();

    static void         initialize_unlock_duration_timeout(timeout_t timeout);
    static timeout_t    current_unlock_duration_timeout();

    static void         initialize_snapcommunicator(
                              std::string const & addr
                            , int port
                            , ed::mode_t mode = ed::mode_t::MODE_PLAIN);

    bool                lock(
                              std::string const & object_name
                            , timeout_t lock_duration = -1
                            , timeout_t lock_obtention_timeout = -1
                            , timeout_t unlock_duration = SNAP_UNLOCK_USES_LOCK_TIMEOUT);
    void                unlock();

    time_t              get_timeout_date() const;
    bool                is_locked() const;
    bool                lock_timedout() const;

private:
    std::shared_ptr<details::lock_connection>    f_lock_connection = std::shared_ptr<details::lock_connection>();
};


class raii_lock_duration_timeout
{
public:
    raii_lock_duration_timeout(snap_lock::timeout_t temporary_lock_timeout);
    ~raii_lock_duration_timeout();

private:
    snap_lock::timeout_t  f_save_timeout = -1;
};


class raii_lock_obtention_timeout
{
public:
    raii_lock_obtention_timeout(snap_lock::timeout_t temporary_lock_timeout);
    ~raii_lock_obtention_timeout();

private:
    snap_lock::timeout_t  f_save_timeout = -1;
};


} // namespace snap
// vim: ts=4 sw=4 et
