//
// File:        server_status.cpp
// Object:      Handle the status object, including saving/reading to file.
//
// Copyright:   Copyright (c) 2016-2017 Made to Order Software Corp.
//              All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// ourselves
//
#include "snapmanager/server_status.h"

// our lib
//
#include "snapmanager/manager.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/snap_string_list.h>

// C lib
//
#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

// last entry
//
#include <snapwebsites/poison.h>

namespace snap_manager
{

namespace
{

/** \brief The magic at expected on the first line of a status file.
 *
 * This string defines the magic string expected on the first line of
 * the file.
 *
 * \note
 * Note that our reader ignores \r characters so this is not currently
 * a 100% exact match, but since only our application is expected to
 * create / read these files, we are not too concerned.
 *
 * v1 -- very first version
 * v2 -- changed format to include error level
 * v3 -- added error level [highlight]
 */
char const g_status_file_magic[] = "Snap! Status v3";

}
// no name namespace


/** \brief Initializes the status file with the specified filename.
 *
 * This function saves the specified \p filename to this status file
 * object. It does not attempt to open() the file. Use the actual
 * open() function for that and make sure to check whether it
 * succeeds.
 *
 * \param[in] filename  The name of the file to read.
 */
server_status::server_status(QString const & filename)
    : f_filename(filename)
{
}


/** \brief Initialize the status with the data path and server name.
 *
 * In this case you do not have the exact file yet so you pass in the
 * \p cluster_status_path, which by default is /var/snapwebsites/cluster-status,
 * and the name of the server to which we add .db.
 *
 * \param[in] cluster_status_path  The path to the directory were server info is saved.
 * \param[in] server  The server this data is linked to.
 */
server_status::server_status(QString const & cluster_status_path, QString const & server)
    : f_filename(QString("%1/%2.db").arg(cluster_status_path).arg(server))
{
}


/** \brief Clean up the status file.
 *
 * This destructor makes sure that the status file is closed before
 * the status_file object goes away.
 */
server_status::~server_status()
{
    close();
}


/** \brief Insert a status in the list of statuses of this server.
 *
 * This function inserts a status to this server_status object. The
 * status is inserted by plugin and field name so it can be retrieved
 * that way too.
 *
 * \param[in] status  The new status to be added.
 */
void server_status::set_field(status_t const & status)
{
    QString const key(QString("%1::%2").arg(status.get_plugin_name()).arg(status.get_field_name()));
    f_statuses[key] = status;
}


/** \brief Retrieve a field.
 *
 * This function checks whether a field exists and if so returns its
 * value. The function returns an empty string whether the field value
 * is empty or the field is not set.
 *
 * To know whether a field is defined, you can use the get_field_state()
 * function first. If that function returns STATUS_STATE_UNDEFINED, then
 * you know that the field is not set.
 *
 * \param[in] plugin_name  The name of the plugin generating that value.
 * \param[in] field_name  The name of the field to be retrieved.
 *
 * \return The value of the specified field.
 */
QString server_status::get_field(QString const & plugin_name, QString const & field_name) const
{
    QString const key(QString("%1::%2").arg(plugin_name).arg(field_name));
    auto const it(f_statuses.find(key));
    if(it == f_statuses.end())
    {
        return QString();
    }
    return it->second.get_value();
}


/** \brief Retrieve a field state.
 *
 * This function checks whether a field exists and if so returns its
 * value. The function returns an empty string whether the field value
 * is empty or the field is not set.
 *
 * \param[in] plugin_name  The name of the plugin generating that value.
 * \param[in] field_name  The name of the field to be retrieved.
 *
 * \return The state of this field: undefined, debug, info, warning,
 *         error, fatal error.
 */
status_t::state_t server_status::get_field_state(QString const & plugin_name, QString const & field_name) const
{
    QString const key(QString("%1::%2").arg(plugin_name).arg(field_name));
    auto const it(f_statuses.find(key));
    if(it == f_statuses.end())
    {
        return status_t::state_t::STATUS_STATE_UNDEFINED;
    }
    return it->second.get_state();
}


status_t const & server_status::get_field_status(QString const & plugin_name, QString const & field_name) const
{
    QString const key(QString("%1::%2").arg(plugin_name).arg(field_name));
    auto const it(f_statuses.find(key));
    if(it == f_statuses.end())
    {
        throw snapmanager_exception_undefined(QString("get_field_status() called to get unknown field %1").arg(key));
    }
    return it->second;
}


/** \brief Return the number of statuses.
 *
 * This function returns the current number of statuses that were added
 * to this object.
 *
 * If creating a new server_status object (i.e. retrieve_status() signals),
 * then the number is exactly what the user added.
 *
 * If checking the size after loading a server_status object, then the
 * number of statuses includes the header which are duplicates of other
 * fields.
 *
 * \warning
 * While creating a status, no header statuses are created in the file.
 * These are articifially created by the write() function. However, these
 * headers get loaded by read_all() and are in additional to the \em normal
 * fields.
 *
 * \return The number of statuses currently defined.
 */
int server_status::size() const
{
    return f_statuses.size();
}


/** \brief Get a constant reference to the map of statuses.
 *
 * This function gives you direct access to all the statuses. Note that
 * in most cases using the get_field() and get_field_state() functions
 * is enough to determine the necessary data you may need from the
 * outside.
 *
 * \return A reference to the statuses.
 */
status_t::map_t const & server_status::get_statuses() const
{
    return f_statuses;
}


/** \brief Count the number of warnings defined in the statuses.
 *
 * This function counts the number of statuses that have their state
 * set to STATUS_STATE_WARNING.
 *
 * That value gets saved in the file header as field warning_count.
 *
 * \return The number of fields that represent a warning.
 */
size_t server_status::count_warnings() const
{
    size_t result(0);

    for(auto const & s : f_statuses)
    {
        if(s.second.get_state() == status_t::state_t::STATUS_STATE_WARNING)
        {
            ++result;
        }
    }

    return result;
}


/** \brief Count the number of errors defined in the statuses.
 *
 * This function counts the number of statuses that have their state
 * set to STATUS_STATE_ERROR or STATUS_STATE_FATAL_ERROR.
 *
 * That value gets saved in the file header as field error_count.
 *
 * \return The number of fields that represent an error.
 */
size_t server_status::count_errors() const
{
    size_t result(0);

    for(auto const & s : f_statuses)
    {
        if(s.second.get_state() == status_t::state_t::STATUS_STATE_ERROR
        || s.second.get_state() == status_t::state_t::STATUS_STATE_FATAL_ERROR)
        {
            ++result;
        }
    }

    return result;
}


/** \brief Convert the status data to a string.
 *
 * In order to send the status data from one snapmanagerdaemon to another,
 * the data needs to be converted to a string which this function does.
 *
 * The resulting string does not include a magic header.
 *
 * \return A string representing all the statuses.
 */
QString server_status::to_string() const
{
    QString result;

    for(auto const & s : f_statuses)
    {
        result += s.second.to_string() + "\n";
    }

    return result;
}


/** \brief Convert a string to a set of statuses.
 *
 * This function clears any existing statuses and replace them by
 * the statuses defined in the \p status parameter string. The
 * parsing is done per line by the status_t::from_string()
 * function.
 *
 * \param[in] status  The status to convert this this server_status object.
 *
 * \return true if the parsing went well, otherwise it returns false and
 *         only statuses that were valid before the error are defined.
 */
bool server_status::from_string(QString const & status)
{
    f_statuses.clear();

    snap::snap_string_list const lines(status.split("\n", QString::SkipEmptyParts));

    for(auto const & l : lines)
    {
        status_t s;
        if(!s.from_string(l))
        {
            return false;
        }
        QString const key(QString("%1::%2").arg(s.get_plugin_name()).arg(s.get_field_name()));
        f_statuses[key] = s;
    }

    return true;
}


/** \brief Retrieve the filename of the host being managed.
 *
 * This function returns the filename as used by the read_all(),
 * read_header(), and write() functions.
 *
 * \return The name of the status file.
 */
QString const & server_status::get_filename() const
{
    return f_filename;
}


/** \brief Check whether the file had errors.
 *
 * If an error occurs while reading a file, this flag will be set to
 * true.
 *
 * The flag is false by default and gets reset to false when close()
 * gets called.
 *
 * \return true if an error occurred while reading the file.
 */
bool server_status::has_error() const
{
    return f_has_error;
}


/** \brief Read all the data from the input file.
 *
 * This function reads all the data from the input file.
 *
 * If the file was already read and is still opened, then the read
 * continues. If already at the end of the file, nothing happens.
 * This allows you to first call read_header(), then call read_all()
 * to read the remaining fields from the file.
 *
 * To re-read the same file, create a new server_status object and
 * call read_all() on that new instance.
 *
 * \return true if the read does not generate an error.
 */
bool server_status::read_all()
{
    if(f_fd == -1)
    {
        if(f_filename.isEmpty())
        {
            SNAP_LOG_ERROR("no filename specified to save snapmanagerdamon status.");
            f_has_error = true;
            return false;
        }

        if(!open_for_read())
        {
            return false;
        }
    }

    QString line;
    for(;;)
    {
        bool const r(readline(line));
        if(!line.isEmpty())
        {
            status_t s;
            if(!s.from_string(line))
            {
                return false;
            }
            set_field(s);
            if(!r)
            {
                return true;
            }
        }
        else if(!r)
        {
            return !f_has_error;
        }
    }
    snap::NOTREACHED();
}


/** \brief Open and read the file headers.
 *
 * This function reads the file header only and then returns. If more
 * fields are available, they can be read by calling the read_all()
 * function.
 *
 * \return true if the read worked without error.
 */
bool server_status::read_header()
{
    // if the file descriptor is already opened then we at least got
    // the header
    //
    if(f_fd != -1)
    {
        return true;
    }

    if(f_filename.isEmpty())
    {
        SNAP_LOG_ERROR("no filename specified to save snapmanagerdaemon status.");
        f_has_error = true;
        return false;
    }

    // open the file if possible
    //
    if(!open_for_read())
    {
        return false;
    }

    // read data until the plugin_name is not "header" anymore
    char const * const header(get_name(name_t::SNAP_NAME_MANAGER_STATUS_FILE_HEADER));
    QString line;
    for(;;)
    {
        bool const r(readline(line));
        if(!line.isEmpty())
        {
            status_t s;
            if(!s.from_string(line))
            {
                return false;
            }
            // whether it is a header field, we have to save it otherwise
            // it would be load (even with a read_all() afterward!)
            //
            set_field(s);
            if(!r
            || s.get_plugin_name() != header)
            {
                // EOF or not header anymore
                return true;
            }
        }
        else if(!r)
        {
            return true;
        }
    }
    snap::NOTREACHED();
}


/** \brief Write the status information to file.
 *
 * This function saves the current status information to file.
 */
bool server_status::write()
{
    class auto_close
    {
    public:
        auto_close(server_status & s, bool & has_error)
            : f_server_status_ref(s)
            , f_has_error_ref(has_error)
        {
        }

        ~auto_close()
        {
            if(!f_success)
            {
                f_server_status_ref.close();
                f_has_error_ref = true;
            }
        }

        void no_errors()
        {
            f_success = true;
        }

    private:
        bool            f_success = false;
        server_status & f_server_status_ref;
        bool &          f_has_error_ref;
    };
    auto_close ensure_close(*this, f_has_error);

    close();

    if(f_filename.isEmpty())
    {
        SNAP_LOG_ERROR("no filename specified to save snapmanagerdamon status.");
        return false;
    }

    // open the file
    //
    f_fd = ::open(f_filename.toUtf8().data(), O_CREAT | O_WRONLY | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if(f_fd < 0)
    {
        f_fd = -1;
        SNAP_LOG_ERROR("could not open file \"")
                      (f_filename)
                      ("\" to save snapmanagerdamon status.");
        return false;
    }

    // make sure no write occur while we read the file
    //
    if(::flock(f_fd, LOCK_EX) != 0)
    {
        SNAP_LOG_ERROR("could not lock file \"")
                      (f_filename)
                      ("\" to write snapmanagerdamon status.");
        return false;
    }

    // now that the file is locked, make sure it is empty by
    // truncating it before writing anything to it
    //
    if(ftruncate(f_fd, 0) != 0)
    {
        SNAP_LOG_ERROR("could not truncate file \"")
                      (f_filename)
                      ("\" to write snapmanagerdamon status.");
        return false;
    }

    // transform to a FILE * so that way we benefit from the
    // caching mechanism without us having to re-implement such
    //
    f_file = fdopen(f_fd, "wb");
    if(f_file == nullptr)
    {
        SNAP_LOG_ERROR("could not allocate a FILE* for file \"")
                      (f_filename)
                      ("\" to write snapmanagerdamon status.");
        return false;
    }

    // write the file magic
    if(fwrite(g_status_file_magic, sizeof g_status_file_magic - 1, 1, f_file) != 1)
    {
        SNAP_LOG_ERROR("could not write magic to FILE* for file \"")
                      (f_filename)
                      ("\".");
        return false;
    }
    if(fwrite("\n", 1, 1, f_file) != 1)
    {
        SNAP_LOG_ERROR("could not write new line after magic in \"")
                      (f_filename)
                      ("\".");
        return false;
    }

    // write the header data first
    //
    bool found(false);
    for(auto const & s : f_statuses)
    {
        if(s.second.get_plugin_name() == "header")
        {
            found = true;
            QString const status(s.second.to_string());
            QByteArray const status_utf8(status.toUtf8());
            if(fwrite(status_utf8.data(), status_utf8.size(), 1, f_file) != 1)
            {
                SNAP_LOG_ERROR("could not write status data header to \"")
                              (f_filename)
                              ("\".");
                return false;
            }
            if(fwrite("\n", 1, 1, f_file) != 1)
            {
                SNAP_LOG_ERROR("could not write new line after status header in \"")
                              (f_filename)
                              ("\".");
                return false;
            }
        }
        else if(found)
        {
            // fields are put in a map and thus it is ordered, so after
            // the last "header" there is no more "header" fields
            break;
        }
    }

    // then write the other data
    //
    for(auto const & s : f_statuses)
    {
        if(s.second.get_plugin_name() != "header")
        {
            QString const status(s.second.to_string());
            QByteArray const status_utf8(status.toUtf8());
            if(fwrite(status_utf8.data(), status_utf8.size(), 1, f_file) != 1)
            {
                SNAP_LOG_ERROR("could not write status data to \"")
                              (f_filename)
                              ("\".");
                return false;
            }
            if(fwrite("\n", 1, 1, f_file) != 1)
            {
                SNAP_LOG_ERROR("could not write new line after status in \"")
                              (f_filename)
                              ("\".");
                return false;
            }
        }
    }

    // close the file, if we were to leave it open the read_all() and
    // read_header() functions would assume it was the read-only one.
    // Also, we have to use fclose() to make sure the FILE* buffers
    // actually get saved to the file and to free the memory.
    //
    close();

    // we do not want the RAII close() because it also sets the
    // f_has_error field to true!
    //
    ensure_close.no_errors();

    return true;
}


/** \brief Close the currently opened file if any.
 *
 * This function makes sure that the status file is closed. This
 * automatically unlocks the file so other processes now have
 * access to the data.
 *
 * This function also has the side effect of resetting the
 * has-error flag to false.
 */
void server_status::close()
{
    if(f_file != nullptr)
    {
        // fclose() gets rid of the FILE * object and also closes
        // the f_fd descriptor in one go (i.e. fdopen() does not
        // call dup() when it creates the FILE * object.)
        //
        fclose(f_file);
        f_file = nullptr;
        f_fd = -1;
    }
    else if(f_fd != -1)
    {
        // Note: there is no need for an explicit unlock, the close()
        //       has the same effect on that file
        //::flock(f_fd, LOCK_UN);
        ::close(f_fd);
    }
    f_has_error = false;
}


/** \brief Open this status file.
 *
 * This function actually tries to open the status file. The function
 * makes sure to lock the file. The lock blocks until it is obtained.
 *
 * If the file does not exist, is not accessible (permissions denied),
 * or cannot be locked, then the function returns false.
 *
 * Also, if the first line is not a valid status file magic string,
 * then the function returns false also.
 *
 * \note
 * On an error, this function already logs information so the caller
 * does not have to do that again.
 *
 * \return true if the file was successfully opened.
 */
bool server_status::open_for_read()
{
    close();

    // open the file
    //
    f_fd = ::open(f_filename.toUtf8().data(), O_RDONLY | O_CLOEXEC, 0);
    if(f_fd < 0)
    {
        f_fd = -1;
        SNAP_LOG_ERROR("could not open file \"")
                      (f_filename)
                      ("\" to read snapmanagerdamon status.");
        f_has_error = true;
        return false;
    }

    // make sure no write occur while we read the file
    //
    if(::flock(f_fd, LOCK_SH) != 0)
    {
        close();
        SNAP_LOG_ERROR("could not lock file \"")
                      (f_filename)
                      ("\" to read snapmanagerdamon status.");
        f_has_error = true;
        return false;
    }

    // transform to a FILE * so that way we benefit from the
    // caching mechanism without us having to re-implement such
    //
    f_file = fdopen(f_fd, "rb");
    if(f_file == nullptr)
    {
        close();
        SNAP_LOG_ERROR("could not allocate a FILE* for file \"")
                      (f_filename)
                      ("\" to read snapmanagerdamon status.");
        f_has_error = true;
        return false;
    }

    // read the first line, it has to be the proper file magic
    {
        QString line;
        if(!readline(line))
        {
            close();
            SNAP_LOG_ERROR("an error occurred while trying to read the first line of status file \"")
                          (f_filename)
                          ("\".");
            f_has_error = true;
            return false;
        }
        if(line != g_status_file_magic)
        {
            close();
            SNAP_LOG_ERROR("status file \"")
                          (f_filename)
                          ("\" does not start with the expected magic. Found: \"")
                          (line)
                          ("\", expected: \"")
                          (g_status_file_magic)
                          ("\".");
            f_has_error = true;
            return false;
        }
    }

    return true;
}


/** \brief Read one line from the input file.
 *
 * This function reads one line of data from the input file and saves
 * it in \p result.
 *
 * If the end of the file is reached or an error occurs, then the
 * function returns false. Otherwise it returns true.
 *
 * \note
 * If an error occurs, the \p result parameter is set to the empty string.
 *
 * \param[out] result  The resulting line of input read.
 *
 * \return true if a line we read, false otherwise.
 */
bool server_status::readline(QString & result)
{
    std::string line;

    for(;;)
    {
        char buf[2];
        int const r(::fread(buf, sizeof(buf[0]), 1, f_file));
        if(r != 1)
        {
            // reached end of file?
            if(feof(f_file))
            {
                // we reached the EOF
                result = QString::fromUtf8(line.c_str());
                return false;
            }
            // there was an error
            int const e(errno);
            SNAP_LOG_ERROR("an error occurred while reading status file. Error: ")
                  (e)
                  (", ")
                  (strerror(e));
            f_has_error = true;
            result = QString();
            return false; // simulate an EOF so we stop the reading loop
        }
        if(buf[0] == '\n')
        {
            result = QString::fromUtf8(line.c_str());
            return true;
        }
        // ignore any '\r'
        if(buf[0] != '\r')
        {
            buf[1] = '\0';
            line += buf;
        }
    }
    snap::NOTREACHED();
}


/** \brief Read one variable from the status file.
 *
 * This function reads the next variable from the status file.
 *
 * \param[out] name  The name of the newly read parameter.
 * \param[out] value  The value of this parameter.
 *
 * \return true if a variable was found and false otherwise.
 */
bool server_status::readvar(QString & name, QString & value)
{
    // reset the out variables
    //
    name.clear();
    value.clear();

    // read next line of data
    //
    QString line;
    if(!readline(line))
    {
        return false;
    }

    // search for the first equal (between name and value)
    //
    int const pos(line.indexOf('='));
    if(pos < 1)
    {
        SNAP_LOG_ERROR("invalid line in \"")
                      (f_filename)
                      ("\", it has no \"name=...\".");
        f_has_error = true;
        return false;
    }

    name = line.mid(0, pos);
    value = line.mid(pos + 1);

    return true;
}


} // namespace snap_manager
// vim: ts=4 sw=4 et
