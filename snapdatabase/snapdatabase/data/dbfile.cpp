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
#include    "snapdatabase/data/dbfile.h"

#include    "snapdatabase/block/block_free_block.h"
#include    "snapdatabase/data/dbtype.h"
#include    "snapdatabase/exception.h"
#include    "snapdatabase/file/file_snap_database_table.h"
#include    "snapdatabase/data/structure.h"
#include    "snapdatabase/database/table.h"


// snapdev lib
//
#include    <snapdev/not_used.h>


// C lib
//
#include    <sys/mman.h>
#include    <sys/stat.h>


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{


namespace
{


constexpr char const *          g_table_extension = ".snapdb";
constexpr char const *          g_global_lock_filename = "global.lock";


std::string generate_table_dir(std::string const & path, std::string const & table_name)
{
    std::string dirname(path);
    if(!dirname.empty())
    {
        dirname += '/';
    }
    dirname += table_name;

    struct stat s;
    if(::stat(dirname.c_str(), &s) != 0)
    {
        snapdev::NOT_USED(mkdir(dirname.c_str(), S_IRWXU));

        if(::stat(dirname.c_str(), &s) != 0)
        {
            throw io_error(
                  "System could not properly create directory \""
                + dirname
                + "\" to handle table \""
                + table_name
                + "\".");
        }
    }

    if(!S_ISDIR(s.st_mode))
    {
        throw io_error(
              "\""
            + dirname
            + "\" must be a directory.");
    }

    return dirname;
}


}
// no name namespace



dbfile::dbfile(std::string const & path, std::string const & table_name, std::string const & filename)
    : f_path(path)
    , f_table_name(table_name)
    , f_filename(filename)
    , f_dirname(generate_table_dir(path, table_name))
    , f_fullname(f_dirname + "/" + f_filename + g_table_extension)
    , f_lock_filename(f_dirname + "/" + g_global_lock_filename)
    , f_pid(getpid())
{
}


dbfile::~dbfile()
{
    close();
}


std::string dbfile::get_fullname() const
{
    return f_fullname;
}


void dbfile::set_table(table::pointer_t t)
{
    f_table = t;
}


table::pointer_t dbfile::get_table() const
{
    return f_table;
}


void dbfile::close()
{
    if(f_fd != -1)
    {
        ::close(f_fd);
        f_fd = -1;
    }
}


size_t dbfile::get_system_page_size()
{
    static long const sc_page_size(sysconf(_SC_PAGE_SIZE));
    return sc_page_size;
}


void dbfile::set_page_size(size_t page_size)
{
    if(f_page_size != 0)
    {
        throw snapdatabase_logic_error("The size of a page in a dbfile can only be set once.");
    }

    // make sure it is at least one system page in size and a multiple of
    // the system page so we can easily mmap() our blocks
    //
    size_t const system_page_size(get_system_page_size());
    size_t const count((page_size + system_page_size - 1) / system_page_size);
    if(count <= 1)
    {
        f_page_size = system_page_size;
    }
    else
    {
        f_page_size = count * system_page_size;
    }
}


size_t dbfile::get_page_size() const
{
    if(f_page_size == 0)
    {
        throw snapdatabase_logic_error("The dbfile page size is not yet defined.");
    }

    return f_page_size;
}


void dbfile::set_sparse(bool sparse)
{
    f_sparse_file = sparse;
}


bool dbfile::get_sparse() const
{
    return f_sparse_file;
}


void dbfile::set_type(dbtype_t type)
{
    if(f_type != dbtype_t::DBTYPE_UNKNOWN)
    {
        throw snapdatabase_logic_error("The dbfile type is already defined.");
    }
    if(type == dbtype_t::DBTYPE_UNKNOWN)
    {
        throw snapdatabase_logic_error("The dbfile type cannot be set to dbtype_t::DBTYPE_UNKNOWN.");
    }

    f_type = type;
}


dbtype_t dbfile::get_type() const
{
    return f_type;
}


int dbfile::open_file()
{
    // already open?
    //
    if(f_fd != -1)
    {
        return f_fd;
    }

    size_t const page_size(get_page_size());

    // we need to have a global lock in case the file was not yet created
    //
    snapdev::lockfile global_lock(f_lock_filename, snapdev::lockfile::mode_t::LOCKFILE_EXCLUSIVE);
    global_lock.lock();

    // first attempt a regular open because once a file was created, this
    // works every time
    //
    f_fd = open(f_fullname.c_str(), O_RDWR | O_CLOEXEC | O_NOATIME | O_NOFOLLOW);
    if(f_fd == -1)
    {
        if(errno != ENOENT)
        {
            int const e(errno);
            throw io_error(
                  "System could not open file \""
                + f_fullname
                + "\" (errno: "
                + std::to_string(e)
                + ", "
                + strerror(e)
                + ".");
        }

        f_fd = open(f_fullname.c_str(), O_RDWR | O_CLOEXEC | O_NOATIME | O_NOFOLLOW | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if(f_fd == -1)
        {
            // nothing more we can do, whatever the error, fail
            //
            // (note we have a global lock so we should not have a problem
            // with the O_EXCL flag)
            //
            int const e(errno);
            throw io_error(
                  "System could not open file \""
                + f_fullname
                + "\" (errno: "
                + std::to_string(e)
                + ", "
                + strerror(e)
                + "\".");
        }

        // in this one case we are in creation mode which means we
        // create the header block, which is important because it has
        // the special offset of 0 and we use that block to allocate
        // other blocks
        //
        version_t v(STRUCTURE_VERSION_MAJOR, STRUCTURE_VERSION_MINOR);

        file_snap_database_table::pointer_t sdbt(std::static_pointer_cast<file_snap_database_table>(
                    f_table->allocate_new_block(dbtype_t::FILE_TYPE_SNAP_DATABASE_TABLE)));

        sdbt->set_first_free_block(page_size);
        sdbt->set_block_size(page_size);
        sdbt->set_file_version(v);
        sdbt->set_last_oid(1);
        sdbt->sync(false);
    }

    return f_fd;
}


data_t dbfile::data(reference_t offset)
{
    int fd(open_file());

    size_t const sz(get_page_size());

    reference_t const page_offset(offset % sz);
    reference_t const page_start(offset - page_offset);

    auto it(f_pages.left.find(page_start));
    if(it != f_pages.left.end())
    {
        return it->second;
    }

static int count(0);
++count;
std::cerr << ">>> data() calling mmap() for the " << count << " times\n";

    data_t ptr(reinterpret_cast<data_t>(mmap(
          nullptr
        , sz
        , PROT_READ | PROT_WRITE
        , MAP_SHARED
        , fd
        , page_start)));

    if(ptr == nullptr)
    {
        throw io_error(
                  "mmap() failed on \""
                + f_filename
                + "\" at offset "
                + std::to_string(offset)
                + ".");
    }

    f_pages.insert(page_bimap_t::value_type(page_start, ptr));

    return ptr + page_offset;
}


void dbfile::release_data(data_t data)
{
    size_t const sz(get_page_size());

    intptr_t const data_ptr(reinterpret_cast<intptr_t>(data));
    intptr_t const page_ptr(data_ptr - data_ptr % sz);
    auto it(f_pages.right.find(reinterpret_cast<data_t>(page_ptr)));
    if(it == f_pages.right.end())
    {
        throw page_not_found(
                  "page "
                + std::to_string(page_ptr)
                + " not found. It cannot be unmapped.");
    }
    f_pages.right.erase(it);

    munmap(reinterpret_cast<data_t>(page_ptr), sz);
}


void dbfile::sync(data_t data, bool immediate)
{
    size_t const sz(get_page_size());

    intptr_t const data_ptr(reinterpret_cast<intptr_t>(data));
    intptr_t const page_ptr(data_ptr - data_ptr % sz);

    msync(reinterpret_cast<data_t>(page_ptr)
        , sz
        , (immediate ? MS_SYNC : MS_ASYNC) | MS_INVALIDATE);
}


size_t dbfile::get_size() const
{
    if(f_fd == -1)
    {
        throw file_not_opened(
                  "file is not yet opened, get_size() can't be called.");
    }

    struct stat s;
    if(::fstat(f_fd, &s) == -1)
    {
        throw io_error(
                  "stat() failed on \""
                + f_filename
                + "\".");
    }

    return s.st_size;
}


reference_t dbfile::append_free_block(reference_t const previous_block_offset)
{
    if(f_fd == -1)
    {
        throw file_not_opened(
                  "file is not yet opened, append_free_block() can't be called.");
    }

    reference_t const p(lseek(f_fd, 0, SEEK_END));
    if(p == static_cast<reference_t>(-1))
    {
        close();
        throw io_error(
                  "lseek() failed on \""
                + f_filename
                + "\".");
    }

    dbtype_t const magic(dbtype_t::BLOCK_TYPE_FREE_BLOCK);
    write_data(&magic, sizeof(magic));
    version_t const version(0, 1);
    auto const v(version.to_binary());
    write_data(&v, sizeof(v));
    write_data(&previous_block_offset, sizeof(previous_block_offset));
    if(!f_sparse_file)
    {
        // make sure to write the rest too so for sure it's not sparse
        //
        std::vector<uint8_t> zeroes(get_page_size() - sizeof(magic) - sizeof(previous_block_offset));
        write_data(zeroes.data(), zeroes.size());
    }
    else
    {
        // this is what makes the file sparse
        //
        // (note that really happens only when
        // `get_page_size() > get_system_page_size()`)
        //
        if(ftruncate(f_fd, p + get_page_size()) != 0)
        {
            int const e(errno);
            close();
            throw io_error(
                  "System could not properly truncate the file \""
                + f_filename
                + "\" "
                + std::to_string(e)
                + ".");
        }
    }

    return p;
}


/** \brief Grow the file.
 *
 * We use this function to grow the file with a full page of data.
 *
 * \exception io_error
 * On an error, the function raises this exception and closes the file.
 *
 * \param[in] ptr  Pointer to the block of data to write to the file.
 * \param[in] size  The number of bytes in the block of data to write.
 */
void dbfile::write_data(void const * ptr, size_t size)
{
    if(f_fd == -1)
    {
        throw file_not_opened(
                  "file is not yet opened, write_data() can't be called.");
    }

    int const sz(write(f_fd, ptr, size));
    if(static_cast<size_t>(sz) != size)
    {
        close();
        throw io_error(
              "System could not properly write to file \""
            + f_filename
            + "\".");
    }
}


std::string to_string(dbtype_t type)
{
    switch(type)
    {
    case dbtype_t::DBTYPE_UNKNOWN:
        return std::string("Unknown");

    case dbtype_t::FILE_TYPE_SNAP_DATABASE_TABLE:
        return std::string("Snap Database Type (SDBT)");

    case dbtype_t::FILE_TYPE_EXTERNAL_INDEX:
        return std::string("External Index File (INDX)");

    case dbtype_t::FILE_TYPE_BLOOM_FILTER:
        return std::string("Bloom Filter File (BLMF)");

    case dbtype_t::BLOCK_TYPE_BLOB:
        return std::string("Blob Block (BLOB)");

    case dbtype_t::BLOCK_TYPE_DATA:
        return std::string("Data Block (DATA)");

    case dbtype_t::BLOCK_TYPE_ENTRY_INDEX:
        return std::string("Entry Index Block (EIDX)");

    case dbtype_t::BLOCK_TYPE_FREE_BLOCK:
        return std::string("Free Block (FREE)");

    case dbtype_t::BLOCK_TYPE_FREE_SPACE:
        return std::string("Free Space Block (FSPC)");

    case dbtype_t::BLOCK_TYPE_INDEX_POINTERS:
        return std::string("Index Pointer Block (IDXP)");

    case dbtype_t::BLOCK_TYPE_INDIRECT_INDEX:
        return std::string("Indirect Index Block (INDR)");

    case dbtype_t::BLOCK_TYPE_PRIMARY_INDEX:
        return std::string("Primary Index Block (PIDX)");

    case dbtype_t::BLOCK_TYPE_SECONDARY_INDEX:
        return std::string("Secondary Index Block (SIDX)");

    case dbtype_t::BLOCK_TYPE_SCHEMA:
        return std::string("Schema Block (SCHM)");

    case dbtype_t::BLOCK_TYPE_SCHEMA_LIST:
        return std::string("Schema List Block (SCHL)");

    case dbtype_t::BLOCK_TYPE_TOP_INDEX:
        return std::string("Top Index Block (TIDX)");

    case dbtype_t::BLOCK_TYPE_TOP_INDIRECT_INDEX:
        return std::string("Top Indirect Index Block (TIND)");

    }

    return std::string("Invalid");
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
