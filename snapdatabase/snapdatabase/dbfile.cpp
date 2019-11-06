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
#include    "snapdatabase/dbfile.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{


namespace
{


constexpr char *        g_global_lock_filename = "global.lock";


std::string generate_table_dir(std::string const & path, std::string const & name)
{
    std::string dirname(path);
    if(!dirname.empty())
    {
        dirname += '/';
    }
    dirname += name;

    struct stat s;
    if(stat(dirname.c_str(), &s) != 0)
    {
        snap::NOTUSED(mkdir(dirname.c_str(), S_IRWXU));

        if(stat(dirname.c_str(), &s) != 0)
        {
            throw io_error(
                  "System could not properly create directory \""
                + dirname
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
    , f_pid(getpid())
{
}


dbfile::~dbfile()
{
    close();
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

    f_type = type
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

    size_t const page_size(dbfile::get_page_size());

    std::string const fullname(f_dirname + "/" + f_filename + ".snapdb");

    // we need to have a global lock in case the file was not yet created
    //
    snap::lockfile global_lock(f_dirname + "/" + g_global_lock_filename, snap::lockfile::mode_t::LOCKFILE_EXCLUSIVE);
    global_lock.lock();

    // first attempt a regular open because once a file was created, this
    // works every time
    //
    f_fd = open(fullname.c_str(), O_RDWR | O_CLOEXEC | O_NOATIME | O_NOFOLLOW);
    if(f_fd == -1)
    {
        f_fd = open(fullname.c_str(), O_RDWR | O_CLOEXEC | O_NOATIME | O_NOFOLLOW | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if(f_fd == -1)
        {
            // nothing more we can do
            //
            throw io_error(
                  "System could not open file \""
                + fullname
                + "\".");
        }
        else
        {
            // in this one case we are in creation mode which means we
            // create the header block, which is important because it has
            // the special offset of 0
            //
            auto write_block = [&](structure & s)
            {
                int const sz(write(f_fd, s.data(), page_size));
                if(sz != page_size)
                {
                    close(f_fd);
                    f_fd = -1;
                    unlink(fullname.c_str());
                    throw io_error(
                          "System could not properly write to file \""
                        + fullname
                        + "\".");
                }
            };

            block_free_block::allocate_new_block(shared_from_this());

            version_t struct_version(STRUCTURE_VERSION_MAJOR, STRUCTURE_VERSION_MINOR);

            structure header(g_header_description);
            header.set_uinteger("magic",             dbtype_t::FILE_TYPE_SPDB);
            header.set_uinteger("version",           struct_version.to_binary());
            header.set_uinteger("block_size",        page_size);
            header.set_uinteger("free_block",        page_size);
            write_block(header);

            file_addr_t offset(page_size * 2);
            for(int idx(0); idx < 14; ++idx, offset += page_size)
            {
                structure free_block(g_free_block_description);
                free_block.set_uinteger("magic",             dbtype_t::BLOCK_TYPE_FREE_BLOCK);
                free_block.set_uinteger("next_free_block",   offset);
                write_block(free_block);
            }

            // the last FREE block has no "next_free_block"
            {
                structure free_block(g_free_block_description);
                free_block.set_uinteger("magic",             dbtype_t::BLOCK_TYPE_FREE_BLOCK);
                write_block(free_block);
            }
        }
    }

    return f_fd;
}


data_t dbfile::page_offset(file_addr_t offset) const
{
    return offset % get_page_size();
}


data_t dbfile::data(file_addr_t offset) const
{
    int fd(open_file());

    size_t const sz(get_page_size());

    page_offset = offset % sz;
    start = offset - page_offset;

    auto it(f_pages.find(start));
    if(it == f_pages.end())
    {
        return it.second;
    }

    data_t ptr(mmap(
          nullptr
        , get_page_size()
        , PROT_READ | PROT_WRITE
        , MAP_SHARED
        , fd
        , start));

    page_t p;
    p.f_addr = offset;
    p.f_data = ptr;

    f_pages.insert(p);

    return ptr;
}


void dbfile::release_data(data_t ptr)
{
    intptr_t data_ptr(reinterpret_cast<intptr_t>(ptr));
    size_t const sz(get_page_size());
    intptr_t page_ptr(data_ptr - data_ptr % sz);
    auto it(f_pages.find(page_ptr));
    if(it == f_pages.end())
    {
        throw page_not_found(
                  "page "
                + std::to_string(page_ptr)
                + " not found. It can be unmapped.");
    }
    f_pages.erase(it);

    munmap(ptr, get_page_size());
}


size_t dbfile::get_size()
{
    if(f_fd == -1)
    {
        throw file_not_opened(
                  "file is not yet opened, get_size() can't be called.");
    }

    struct st s;
    if(stat(f_d, &s) == -1)
    {
        throw io_error(
                  "stat() failed on \""
                + f_filename
                + "\".");
    }

    return st.st_size;
}


file_addr_t dbfile::append_free_block(file_addr_t const previous_block_offset)
{
    if(f_fd == -1)
    {
        throw file_not_opened(
                  "file is not yet opened, get_size() can't be called.");
    }

    file_addr_t const p(lseek(f_fd, 0, SEEK_END);
    if(p == -1)
    {
        throw file_not_opened(
                  "lseek() failed on \""
                + f_filename
                + "\".");
    }

    if(f_spares_file)
    {
        // in this case we write the minimum for a chance to keep some
        // data at 0x00 so the file is sparse
        //
        dbtype_t magic(dbtype_t::BLOCK_TYPE_FREE_BLOCK);
        write_data(&magic, sizeof(magic));
        write_data(&previous_block_offset, sizeof(previous_block_offset));
    }
    else
    {
        structure free_block(g_free_block_description);
        free_block.set_uinteger("magic",             dbtype_t::BLOCK_TYPE_FREE_BLOCK);
        free_block.set_uinteger("next_free_block",   previous_block_offset);
        write_data(free_block.data(), f_page_size);
    }
}


void dbfile::write_data(void const * ptr, size_t size)
{
    int const sz(write(f_fd, ptr, size));
    if(sz != size)
    {
        close(f_fd);
        f_fd = -1;
        //unlink(fullname.c_str());
        throw io_error(
              "System could not properly write to file \""
            + f_filename
            + "\".");
    }
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
