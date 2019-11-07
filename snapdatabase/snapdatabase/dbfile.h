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
#pragma once


/** \file
 * \brief Database file header.
 *
 * The block base class handles the loading of the block in memory using
 * mmap() and gives information such as its type and location.
 */

// snapdatabase lib
//
#include    "snapdatabase/dbtype.h"

// snapdev lib
//
#include    <snapdev/lockfile.h>

// boost lib
//
#include    <boost/multi_index_container.hpp>
#include    <boost/multi_index/sequenced_index.hpp>
#include    <boost/multi_index/ordered_index.hpp>
#include    <boost/multi_index/identity.hpp>



namespace snapdatabase
{



typedef uint64_t            file_addr_t;
typedef uint8_t *           data_t;

constexpr file_addr_t       NULL_FILE_ADDR = static_cast<file_addr_t>(0);


class dbfile
    : protected std::enable_shared_from_this<dbfile>
{
public:
    //typedef std::map<file_addr_t, data_t>   page_map_t;

    struct page_t
    {
        file_addr_t     f_addr;
        data_t          f_data;
    };

    typedef boost::multi_index_container<
                  page_t
                , indexed_by<
                      boost::ordered_unique<page_t, page_addr_t, &page_t::f_addr>
                    , boost::ordered_unique<page_t, data_t, &page_t::f_data>
                >
        > page_map_t;

                            dbfile(std::string const & path, std::string const & table_name, std::string const & filename);
                            dbfile(dbfile const & rhs) = delete;
                            ~dbfile();

    dbfile &                operator = (dbfile const & rhs) = delete;

    static size_t           get_system_page_size();
    void                    set_page_size(size_t size);
    size_t                  get_page_size() const;
    void                    set_type(dbtype_t type);
    dbtype_t                get_type() const;
    data_t                  data(file_addr_t offset) const;
    void                    release_data(data_t data) const;
    size_t                  get_size() const;

private:
    int                     open_file();

    std::string             f_path = std::string();
    std::string             f_table_name = std::string();
    std::string             f_filename = std::string();
    std::string             f_dirname = std::string();
    size_t                  f_page_size = 0;
    dbtype_t                f_type = dbtype_t::DBTYPE_UNKNOWN;
    pid_t                   f_pid = -1;
    int                     f_fd = -1;
    page_map_t              f_pages = page_map_t();
    bool                    f_spares_file = false;
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
