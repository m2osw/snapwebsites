// Snap Websites Server -- manage sessions for users, forms, etc.
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include "snapwebsites/snap_string_list.h"


// libexcept lib
//
#include "libexcept/exception.h"


// C lib
//
#include <unistd.h>



namespace snap
{
namespace compression
{

DECLARE_MAIN_EXCEPTION(compression_exception);

DECLARE_EXCEPTION(compression_exception, compression_exception_not_implemented);
DECLARE_EXCEPTION(compression_exception, compression_exception_not_supported);
DECLARE_EXCEPTION(compression_exception, compression_exception_not_compatible);



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
        QByteArray          f_data = QByteArray();
        QString             f_filename = QString();
        QString             f_user = QString();
        QString             f_group = QString();
        uid_t               f_uid = 0;
        gid_t               f_gid = 0;
        mode_t              f_mode = 0;
        time_t              f_mtime = 0;
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
    QByteArray              f_archive = QByteArray();
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
