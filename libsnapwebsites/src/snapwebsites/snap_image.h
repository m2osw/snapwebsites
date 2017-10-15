// Snap Websites Servers -- snap websites image handling
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

#include <QVector>
#include <QSharedPointer>

namespace snap
{

class snap_image_exception : public snap_exception
{
public:
    explicit snap_image_exception(char const *        whatmsg) : snap_exception("snap_image", whatmsg) {}
    explicit snap_image_exception(std::string const & whatmsg) : snap_exception("snap_image", whatmsg) {}
    explicit snap_image_exception(QString const &     whatmsg) : snap_exception("snap_image", whatmsg) {}
};

class snap_image_exception_no_buffer : public snap_image_exception
{
public:
    explicit snap_image_exception_no_buffer(char const *        whatmsg) : snap_image_exception(whatmsg) {}
    explicit snap_image_exception_no_buffer(std::string const & whatmsg) : snap_image_exception(whatmsg) {}
    explicit snap_image_exception_no_buffer(QString const &     whatmsg) : snap_image_exception(whatmsg) {}
};

class snap_image_exception_invalid_image : public snap_image_exception
{
public:
    explicit snap_image_exception_invalid_image(char const *        whatmsg) : snap_image_exception(whatmsg) {}
    explicit snap_image_exception_invalid_image(std::string const & whatmsg) : snap_image_exception(whatmsg) {}
    explicit snap_image_exception_invalid_image(QString const &     whatmsg) : snap_image_exception(whatmsg) {}
};





// whatever the input format, in memory we only manage RGBA images for
// whatever we do with images (flip, rotate, borders...)
struct snap_rgba
{
    unsigned char       f_red;
    unsigned char       f_green;
    unsigned char       f_blue;
    unsigned char       f_alpha;
};


class snap_image;

class snap_image_buffer_t
{
public:
                        snap_image_buffer_t(snap_image *owner);

    QString const &     get_mime_type() const;
    void                set_mime_type(QString const & mime_type);
    QString const &     get_format_version() const;
    void                set_format_version(QString const & format_version);
    QString const &     get_resolution_unit() const;
    void                set_resolution_unit(QString const & resolution_unit);
    int                 get_xres() const;
    void                set_xres(int xres);
    int                 get_yres() const;
    void                set_yres(int yres);
    int                 get_width() const;
    void                set_width(int width);
    int                 get_height() const;
    void                set_height(int height);
    int                 get_depth() const;
    void                set_depth(int depth);
    int                 get_bits() const;
    void                set_bits(int bits);

    unsigned char *     get_buffer() const;

private:
    snap_image *                        f_owner; // parent has a shared pointer to us, which is enough!
    QString                             f_mime_type;
    QString                             f_format_version;
    QString                             f_resolution_unit;
    int32_t                             f_xres = 0;
    int32_t                             f_yres = 0;
    int32_t                             f_width = 0;
    int32_t                             f_height = 0;
    int32_t                             f_depth = 0; // 1 or 3 or 4
    int32_t                             f_bits = 0;
    QSharedPointer<unsigned char>       f_buffer;
};
typedef QSharedPointer<snap_image_buffer_t>     smart_snap_image_buffer_t;
typedef QVector<smart_snap_image_buffer_t>      snap_image_buffer_vector_t;


class snap_image
{
public:
    bool                        get_info(QByteArray const & data);

    size_t                      get_size() const;
    smart_snap_image_buffer_t   get_buffer(int idx);

private:
    bool                        info_jpeg(unsigned char const * s, size_t l, unsigned char const * e);
    bool                        info_ico(unsigned char const * s, size_t l, unsigned char const * e);
    bool                        info_bmp(unsigned char const * s, size_t l, unsigned char const * e);
    bool                        info_png(unsigned char const * s, size_t l, unsigned char const * e);
    bool                        info_gif(unsigned char const * s, size_t l, unsigned char const * e);

    // each buffer represents one RGBA image
    snap_image_buffer_vector_t  f_buffers;
};



} // namespace snap
// vim: ts=4 sw=4 et
