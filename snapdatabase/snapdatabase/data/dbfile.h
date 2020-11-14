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
#include    "snapdatabase/data/dbtype.h"


// snapdev lib
//
#include    <snapdev/lockfile.h>


// boost lib
//
#include    <boost/bimap.hpp>



namespace snapdatabase
{



typedef uint64_t                    reference_t;
typedef std::vector<reference_t>    reference_vector_t;
typedef uint64_t                    oid_t;
typedef uint8_t *                   data_t;
typedef uint8_t const *             const_data_t;

constexpr reference_t               NULL_FILE_ADDR = static_cast<reference_t>(0);
constexpr oid_t                     NULL_OID = static_cast<oid_t>(0);

class table;
typedef std::shared_ptr<table>      table_pointer_t;


static_assert(sizeof(reference_t) == sizeof(oid_t), "the OID and references must fit in each other's variables");


class dbfile
    : public std::enable_shared_from_this<dbfile>
{
public:
    typedef std::shared_ptr<dbfile>             pointer_t;
    //typedef std::map<reference_t, data_t>   page_map_t;
    typedef boost::bimap<reference_t, data_t>  page_bimap_t;

                            dbfile(std::string const & path, std::string const & table_name, std::string const & filename);
                            dbfile(dbfile const & rhs) = delete;
                            ~dbfile();

    dbfile &                operator = (dbfile const & rhs) = delete;

    std::string             get_fullname() const;
    void                    set_table(table_pointer_t t);
    table_pointer_t         get_table() const;
    void                    close();
    static size_t           get_system_page_size();
    void                    set_page_size(size_t size);
    size_t                  get_page_size() const;
    void                    set_sparse(bool sparse);
    bool                    get_sparse() const;
    void                    set_type(dbtype_t type);
    dbtype_t                get_type() const;
    data_t                  data(reference_t offset);
    void                    release_data(data_t data);
    void                    sync(data_t data, bool immediate);
    size_t                  get_size() const;
    reference_t             append_free_block(reference_t const previous_block_offset);

private:
    int                     open_file();
    void                    write_data(void const * ptr, size_t size);

    table_pointer_t         f_table = table_pointer_t();
    std::string             f_path = std::string();
    std::string             f_table_name = std::string();
    std::string             f_filename = std::string();
    std::string             f_dirname = std::string();
    std::string             f_fullname = std::string();
    std::string             f_lock_filename = std::string();
    size_t                  f_page_size = 0;
    dbtype_t                f_type = dbtype_t::DBTYPE_UNKNOWN;
    pid_t                   f_pid = -1;
    int                     f_fd = -1;
    page_bimap_t            f_pages = page_bimap_t();
    bool                    f_sparse_file = false;
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
