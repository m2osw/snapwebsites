// Snap Websites Servers -- handle file content (i.e. read all / write all)
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include "snapwebsites/file_content.h"

// snapwebsites lib
//
#include "snapwebsites/log.h"
#include "snapwebsites/mkdir_p.h"

// C lib
//
#include <unistd.h>

// C++ lib
//
#include <fstream>

// include last
//
#include <snapdev/poison.h>


namespace snap
{


/** \brief Initialize a content file.
 *
 * The constructor initialize the file content object with a filename.
 * The filename is used by the read_all() and write_all() functions.
 *
 * If the file_content is setup to be a temporary file, then the
 * destructor also makes use of the filename to delete the file
 * at that time. By default a file is not marked as temporary.
 *
 * \exception file_content_exception_invalid_parameter
 * The \p filename parameter cannot be an empty string.
 *
 * \exception file_content_exception_io_error
 * The function checks whether all the folders exist. If not then the
 * file can't be create or read so there is no valid file_content()
 * possible with that path. This exception only occurs if the
 * \p create_missing_directories is true and the creation of any
 * of the directories fails.
 *
 * \param[in] filename  The name of the file to read and write.
 * \param[in] create_missing_directories  Whether to create missing directories
 *            as found in the path (see mkdir_p()).
 * \param[in] temporary  Whether the file is temporary.
 *
 * \sa mkdir_p()
 */
file_content::file_content(std::string const & filename, bool create_missing_directories, bool temporary)
    : f_filename(filename)
    , f_temporary(temporary)
{
    if(f_filename.empty())
    {
        throw file_content_exception_invalid_parameter("the filename of a file_content object cannot be the empty string");
    }

    if(create_missing_directories)
    {
        int const r(mkdir_p(QString::fromUtf8(f_filename.c_str()), true));
        if(r != 0)
        {
            throw file_content_exception_io_error("the full path to filename for a file_content object could not be created");
        }
    }
}


/** \brief Clean up as required.
 *
 * If the file_content was marked as temporary, then the destructor
 * deletes the file on disk before returning.
 *
 * If the file does not exist and it was marked as temporary, the
 * deletion will fail silently. Otherwise you get a warning in the
 * logs with more details about the error (i.e. permission, bad
 * filename, etc.)
 */
file_content::~file_content()
{
    // TODO: add support for temporary files, unlink() it here
    if(f_temporary)
    {
        if(unlink(f_filename.c_str()) != 0)
        {
            if(errno != ENOENT)
            {
                int const e(errno);
                SNAP_LOG_WARNING("could not delete file \"")(f_filename)("\", got errno: ")(e)(", ")(strerror(e))(".");
            }
        }
    }
}


/** \brief Get the filename.
 *
 * This function returns a copy of the filename used by this file_content
 * object. Note that the filename cannot be modified.
 *
 * \return A reference to this file content filename.
 */
std::string const & file_content::get_filename() const
{
    return f_filename;
}


/** \brief Check whether this file exists.
 *
 * This function checks whether the file exists on disk. If the read_all()
 * function returns false and this functio nreturns true, then you probably
 * do not have permission to read the file or it is a directory.
 *
 * \return true if the file exists and can be read.
 */
bool file_content::exists() const
{
    return access(f_filename.c_str(), R_OK) == 0;
}


/** \brief Read the entire file in a string.
 *
 * This function reads the entire file in memory and saves it in a
 * string. The file can be binary in which case remember that c_str()
 * and other similar function will not work right.
 *
 * The function may return false if the file could not be opened.
 *
 * \return true if the file could be read, false otherwise.
 *
 * \sa write_all()
 */
bool file_content::read_all()
{
    // try to open the file
    //
    std::ifstream in;
    in.open(f_filename, std::ios::in | std::ios::binary);
    if(!in.is_open())
    {
        // cannot open
        SNAP_LOG_WARNING("could not open \"")(f_filename)("\" for reading.");
        return false;
    }

    // get the size
    //
    in.seekg(0, std::ios::end);
    std::ifstream::pos_type const size(in.tellg());
    in.seekg(0, std::ios::beg);

    if(std::ifstream::pos_type(-1) == size)
    {
        // cannot get the size
        SNAP_LOG_WARNING("tellg() failed against \"")(f_filename)("\".");
        return false;
    }

    // resize the buffer accoringly
    //
    try
    {
        f_content.resize(size, '\0');
    }
    catch(std::bad_alloc const & e)
    {
        SNAP_LOG_WARNING("could not allocate ")(size)(" bytes to read \"")(f_filename)("\".");
        return false;
    }

    // read the data
    //
    in.read(&f_content[0], size);
    if(in.fail()) // note: eof() will be true so good() will return false
    {
        SNAP_LOG_WARNING("could not read ")(size)(" bytes from \"")(f_filename)("\".");
        return false;
    }

    return true;
}


/** \brief Write the content to the file.
 *
 * This function writes the file content data to the file. If a new
 * filename is specified, the content is saved there instead. Note
 * that the filename does not get modified, but it allows for creating
 * a backup before making changes and save the new file:
 *
 * \code
 *      file_content fc("filename.here");
 *      fc.read_all();
 *      fc.write_all("filename.bak");  // create backup
 *      std::string content(fc.get_content());
 *      ... // make changes on 'content'
 *      fc.set_content(content);
 *      fc.write_all();
 * \endcode
 *
 * \warning
 * If you marked the file_content object as managing a temporary file
 * and specify a filename here which is not exactly equal to the
 * filename passed to the constructor, then the file you are writing
 * now will not be deleted automatically.
 *
 * \param[in] filename  The name to use or an empty string (or nothing)
 *                      to use the filename defined in the constructor.
 *
 * \return true if the file was saved successfully.
 *
 * \sa set_content()
 * \sa read_all()
 */
bool file_content::write_all(std::string const & filename)
{
    // select filename
    //
    std::string const name(filename.empty() ? f_filename : filename);

    // try to open the file
    //
    std::ofstream out;
    out.open(name, std::ios::trunc | std::ios::out | std::ios::binary);
    if(!out.is_open())
    {
        // cannot open
        SNAP_LOG_WARNING("could not open \"")(name)("\" for writing.");
        return false;
    }

    // write the data
    //
    out.write(&f_content[0], f_content.length());
    if(out.fail()) // note: eof() will be true so good() will return false
    {
        SNAP_LOG_WARNING("could not write ")(f_content.length())(" bytes to \"")(name)("\".");
        return false;
    }

    return true;
}


/** \brief Change the content with \p new_content.
 *
 * This function replaces the current file content with new_content.
 *
 * If \p new_content is empty, then the file will become empty on a
 * write().
 *
 * \param[in] new_content  The new content of the file.
 *
 * \sa get_content()
 * \sa write()
 */
void file_content::set_content(std::string const & new_content)
{
    f_content = new_content;
}


/** \brief Get a constant reference to the content.
 *
 * This function gives you access to the existing content of the file.
 * The content is considered valid only if you called the read() function
 * first, although it is not mandatory (i.e. in case you are creating a
 * new file, then you do not need to call read() first.)
 *
 * \return A constant reference to this file content data.
 */
std::string const & file_content::get_content() const
{
    return f_content;
}




} // namespace snap
// vim: ts=4 sw=4 et
