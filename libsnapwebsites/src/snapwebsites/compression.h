// Snap Websites Server -- manage sessions for users, forms, etc.
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

#include "snapwebsites/snap_exception.h"

#include "snapwebsites/snap_string_list.h"

#include <unistd.h>


namespace snap
{
namespace compression
{

class compression_exception : public snap_exception
{
public:
    compression_exception(char const *        whatmsg) : snap_exception("compression", whatmsg) {}
    compression_exception(std::string const & whatmsg) : snap_exception("compression", whatmsg) {}
    compression_exception(QString const &     whatmsg) : snap_exception("compression", whatmsg) {}
};

class compression_exception_not_implemented : public compression_exception
{
public:
    compression_exception_not_implemented(char const *        whatmsg) : compression_exception(whatmsg) {}
    compression_exception_not_implemented(std::string const & whatmsg) : compression_exception(whatmsg) {}
    compression_exception_not_implemented(QString const &     whatmsg) : compression_exception(whatmsg) {}
};

class compression_exception_not_supported : public compression_exception
{
public:
    compression_exception_not_supported(char const *        whatmsg) : compression_exception(whatmsg) {}
    compression_exception_not_supported(std::string const & whatmsg) : compression_exception(whatmsg) {}
    compression_exception_not_supported(QString const &     whatmsg) : compression_exception(whatmsg) {}
};

class compression_exception_not_compatible : public compression_exception
{
public:
    compression_exception_not_compatible(char const *        whatmsg) : compression_exception(whatmsg) {}
    compression_exception_not_compatible(std::string const & whatmsg) : compression_exception(whatmsg) {}
    compression_exception_not_compatible(QString const &     whatmsg) : compression_exception(whatmsg) {}
};




// compression level is a percent (a number from 0 to 100)
typedef int         level_t;

// all compressors derive from this class
class compressor_t
{
public:
    static char const * BEST_COMPRESSION;
    static char const * NO_COMPRESSION;

                        compressor_t(char const * name);
    virtual             ~compressor_t();
    virtual char const *get_name() const = 0;
    virtual QByteArray  compress(QByteArray const & input, level_t level, bool text) = 0;
    virtual bool        compatible(QByteArray const & input) const = 0;
    virtual QByteArray  decompress(QByteArray const & input) = 0;
    virtual QByteArray  decompress(QByteArray const & input, size_t uncompressed_size) = 0;
};

class archiver_t
{
public:
    class file_t
    {
    public:
        enum class type_t
        {
            FILE_TYPE_REGULAR,
            FILE_TYPE_DIRECTORY
        };

        void                set_type(type_t type);
        void                set_data(QByteArray const & data);
        void                set_filename(QString const & filename);
        void                set_user(QString const & user, uid_t uid);
        void                set_group(QString const & group, gid_t gid);
        void                set_mode(mode_t mode);
        void                set_mtime(time_t mtime);

        type_t              get_type() const;
        QByteArray const &  get_data() const;
        QString const &     get_filename() const;
        QString const &     get_user() const;
        QString const &     get_group() const;
        uid_t               get_uid() const;
        gid_t               get_gid() const;
        mode_t              get_mode() const;
        time_t              get_mtime() const;

    private:
        type_t              f_type = type_t::FILE_TYPE_REGULAR;
        QByteArray          f_data;
        QString             f_filename;
        QString             f_user;
        QString             f_group;
        uid_t               f_uid;
        gid_t               f_gid;
        mode_t              f_mode;
        time_t              f_mtime;
    };

                            archiver_t(char const * name);
    virtual                 ~archiver_t();
    void                    set_archive(QByteArray const & input);
    QByteArray              get_archive() const;
    virtual char const *    get_name() const = 0;
    virtual void            append_file(file_t const & file) = 0;
    virtual bool            next_file(file_t & file) const = 0;
    virtual void            rewind_file() = 0;

protected:
    QByteArray              f_archive;
};

//void register_compressor(compressor_t * compressor_name); -- automatic at this point
snap_string_list    compressor_list();
compressor_t *      get_compressor(QString const & compressor_name);
QByteArray          compress(QString & compressor_name, QByteArray const & input, level_t level, bool text);
QByteArray          decompress(QString & compressor_name, QByteArray const & input);

snap_string_list    archiver_list();
archiver_t *        get_archiver(QString const & archiver_name);

} // namespace snap
} // namespace compression
// vim: ts=4 sw=4 et
