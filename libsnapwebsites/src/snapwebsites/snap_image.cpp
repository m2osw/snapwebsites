// Snap Websites Server -- snap basic image handling
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

#include "snapwebsites/snap_image.h"

#include "snapwebsites/not_reached.h"
#include "snapwebsites/log.h"

#include "snapwebsites/poison.h"

namespace snap
{


namespace
{

uint32_t chunk_name(unsigned char a, unsigned char b, unsigned char c, unsigned char d) __attribute__ ((const));

/** \brief Transform 4 characters in a chunk name.
 *
 * Several different formats make use of chunk names (i.e. PNG, TIFF, IFF,
 * WAVE.) This macro is used to transform 4 bytes in a name that can
 * quickly be compared using uint32_t variables.
 *
 * Note that the characters do not need to be valid ASCII. Even '\0' is
 * acceptable.
 *
 * The order in which you specify the characters does not matter, although
 * you may want to put them in a sensical order. (i.e. the PNG format uses
 * a chunk name IHDR, so it makes more sense to write the characters in
 * that order!)
 *
 * \param[in] a  The first character.
 * \param[in] b  The second character.
 * \param[in] c  The third character.
 * \param[in] d  The fourth character.
 */
uint32_t chunk_name(unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
    return (a << 24) | (b << 16) | (c << 8) | d;
}

}
// empty namespace



snap_image_buffer_t::snap_image_buffer_t(snap_image *owner)
    : f_owner(owner)
    //, f_buffer(nullptr) -- auto-init
{
}

QString const & snap_image_buffer_t::get_mime_type() const
{
    return f_mime_type;
}

void snap_image_buffer_t::set_mime_type(QString const& mime_type)
{
    f_mime_type = mime_type;
}

QString const& snap_image_buffer_t::get_format_version() const
{
    return f_format_version;
}

void snap_image_buffer_t::set_format_version(QString const& format_version)
{
    f_format_version = format_version;
}

QString const& snap_image_buffer_t::get_resolution_unit() const
{
    return f_resolution_unit;
}

void snap_image_buffer_t::set_resolution_unit(QString const& resolution_unit)
{
    f_resolution_unit = resolution_unit;
}

int snap_image_buffer_t::get_xres() const
{
    return f_xres;
}

void snap_image_buffer_t::set_xres(int xres)
{
    f_xres = xres;
}

int snap_image_buffer_t::get_yres() const
{
    return f_yres;
}

void snap_image_buffer_t::set_yres(int yres)
{
    f_yres = yres;
}

int snap_image_buffer_t::get_width() const
{
    return f_width;
}

void snap_image_buffer_t::set_width(int width)
{
    f_width = width;
}

int snap_image_buffer_t::get_height() const
{
    return f_height;
}

void snap_image_buffer_t::set_height(int height)
{
    f_height = height;
}

int snap_image_buffer_t::get_depth() const
{
    return f_depth;
}

void snap_image_buffer_t::set_depth(int depth)
{
    f_depth = depth;
}

int snap_image_buffer_t::get_bits() const
{
    return f_bits;
}

void snap_image_buffer_t::set_bits(int bits)
{
    f_bits = bits;
}

unsigned char * snap_image_buffer_t::get_buffer() const
{
    return f_buffer.data();
}





/** \brief Read an JPEG header for its information.
 *
 * This function reads the JPEG header information and saves it in a buffer.
 *
 * Source: http://www.ijg.org/files/
 *
 * \param[in] s  The start of the buffer.
 * \param[in] l  The size of the buffer.
 * \param[in] e  The end of the buffer (s + l).
 *
 * \return true if the JPEG was read successfully.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool snap_image::info_jpeg(unsigned char const *s, size_t l, unsigned char const *e)
{
    // create a buffer "on the stack"; add it to the list of buffers only
    // if successful
    smart_snap_image_buffer_t buffer(new snap_image_buffer_t(this));

    buffer->set_mime_type("image/jpeg");

    // bytes 18/19 are the thumbnail and if present it follows as an RGB
    // color bitmap of: 3 x thumbnail(width) x thumbnail(height)
    int flags(0);
    unsigned char const *q(s + 2);
    for(;;)
    {
        if(q + 4 >= e || q[0] != 0xFF || q[1] < 0xC0)
        {
            // lost track... get out!
            return false;
        }
        int len(q[2] * 256 + q[3]);
        // XXX: determine whether C1 to CF should all be accepted here
        switch(q[1])
        {
        case 0xC0: // SOF0
        case 0xC1: // SOF1
        case 0xC2: // SOF2
        case 0xC3: // SOF3
        case 0xC5: // SOF5
        case 0xC6: // SOF6
        case 0xC7: // SOF7
        case 0xC9: // SOF9
        case 0xCA: // SOF10
        case 0xCB: // SOF11
        case 0xCD: // SOF13
        case 0xCE: // SOF14
        case 0xCF: // SOF15
            if(len < 10)
            {
                // we expect at least 6 bytes after the length
                return false;
            }
            buffer->set_bits(q[4] * q[9]); // usually 8 or 24
            buffer->set_width(q[5] * 256 + q[6]);
            buffer->set_height(q[7] * 256 + q[8]);
            buffer->set_depth(q[9]); // 1 or 3
            flags |= 1;
            break;

        case 0xE0: // APP0 (Not always present, i.e. Exif is APP1 or 0xE1)
            // verify length and marker name (JFIF\0)
            if(len < 16
            || q[4] != 'J' || q[5] != 'F' || q[6] != 'I' || q[7] != 'F' || q[8] != '\0')
            {
                // we expected at least 16 bytes
                break;
            }
            // usually 1.01 or 1.02
            buffer->set_format_version(QString("%1.%2").arg(static_cast<int>(q[9])).arg(static_cast<int>(q[10]), 2, 10, QChar('0')));

            buffer->set_resolution_unit(q[11] == 0 ? "" : (q[11] == 1 ? "inch" : "cm"));
            buffer->set_xres(q[12] * 256 + q[13]); // also called density
            buffer->set_yres(q[14] * 256 + q[15]);
            flags |= 2;
            break;

        case 0xDA: // start of data
            // TODO: add support to skip the image (read all the data up
            //       to 0xFF 0xD9) and see whether another image is present
            //       in the file (as far as I know "animated JPEG" are JPEG
            //       images concatenated one after another)
            if((flags & 1) != 0)
            {
                f_buffers.push_back(buffer);
                return true;
            }
            return false;

        }
        q += 2 + len;
    }
    NOTREACHED();
}
#pragma GCC diagnostic pop


/** \brief Read an ICO header for its information.
 *
 * This function reads the ICO header information and saves it in a buffer.
 * The ICO format is used for the favicon.
 *
 * Source: http://en.wikipedia.org/wiki/ICO_%28file_format%29
 * Source: https://en.wikipedia.org/wiki/BMP_file_format
 *
 * \param[in] s  The start of the buffer.
 * \param[in] l  The size of the buffer.
 * \param[in] e  The end of the buffer (s + l).
 *
 * \return true if the ICO was read successfully.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool snap_image::info_ico(unsigned char const *s, size_t l, unsigned char const *e)
{
    // number of images
    int const max_images(s[4] + s[5] * 256);

    for(int i(0); i < max_images; ++i)
    {
        unsigned char const *q(s + 6 + i * 16);
        // width and height are taken from the BMP if 0x0
        // also we can verify that it matches to the BMP anyway
        uint32_t width(q[0]);
        uint32_t height(q[1]);
        // ignore palette, planes, bits per pixel
        uint32_t size(q[8] + q[9] * 256 + q[10] * 65536 + q[11] * 16777216);
        uint32_t offset(q[12] + q[13] * 256 + q[14] * 65536 + q[15] * 16777216);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
        if(offset + size > l || size < 40)
        {
            // invalid offset/size (out of bounds)
            return false;
        }
#pragma GCC diagnostic pop

        unsigned char const *b(s + offset);
        if(b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G')
        {
            // We can get the info simply by calling info_png()!
            if(!info_png(s + offset, size, s + offset + size))
            {
                return false;
            }
        }
        else
        {
            // TBD is that bitmap info header always 40 bytes?
            if(b[0] != 0x28 || b[1] != 0 || b[2] != 0 || b[3] != 0)
            {
                // invalid BITMAPINFOHEADER size
                return false;
            }
            uint32_t bitmap_width(b[4] + b[5] * 256 + b[6] * 65536 + b[7] * 16777216);
            uint32_t bitmap_height(b[8] + b[9] * 256 + b[10] * 65536 + b[11] * 16777216);
            // Weird! The bitmap sizes may be larger or equal, just not smaller
            if((width  != 0 && bitmap_width  < width)
            || (height != 0 && bitmap_height < height))
            {
                // sizes between ICO and BITMAPINFOHEADER to do not correspond!
                return false;
            }
            smart_snap_image_buffer_t buffer(new snap_image_buffer_t(this));

            buffer->set_mime_type("image/x-icon");
            buffer->set_format_version("1.0");
            buffer->set_width(width != 0 ? width : bitmap_width);
            buffer->set_height(height != 0 ? height : bitmap_height);
            int bits(b[14] + b[15] * 256);
            buffer->set_bits(bits);
            buffer->set_resolution_unit("m");
            buffer->set_xres(b[24] + b[25] * 256 + b[26] * 65536 + b[27] * 16777216);
            buffer->set_yres(b[28] + b[29] * 256 + b[30] * 65536 + b[31] * 16777216);

            // TODO this is not correct, we'd need to know whether we use a palette or not
            if(bits == 8)
            {
                buffer->set_depth(1);
            }
            else if(bits == 24)
            {
                buffer->set_depth(3);
            }
            else if(bits == 32)
            {
                buffer->set_depth(4);
            }

            f_buffers.push_back(buffer);
        }
    }

    // got all the images successfully
    return true;
}
#pragma GCC diagnostic pop


/** \brief Read an BMP header for its information.
 *
 * This function reads the BMP header information and saves it in a buffer.
 *
 * Source: http://msdn.microsoft.com/en-us/library/windows/desktop/dd183374(v=vs.85).aspx
 * Source: http://msdn.microsoft.com/en-us/library/windows/desktop/dd183376(v=vs.85).aspx
 * Source: https://en.wikipedia.org/wiki/BMP_file_format
 *
 * \param[in] s  The start of the buffer.
 * \param[in] l  The size of the buffer.
 * \param[in] e  The end of the buffer (s + l).
 *
 * \return true if the BMP was read successfully.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool snap_image::info_bmp(unsigned char const *s, size_t l, unsigned char const *e)
{
    uint32_t headerinfosize(s[14] + s[15] * 256 + s[16] * 65536 + s[17] * 16777216);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
    if(headerinfosize < 40) // v1.0 is 40 bytes
    {
#pragma GCC diagnostic pop
        // invalid BITMAPINFOHEADER size
        return false;
    }

    smart_snap_image_buffer_t buffer(new snap_image_buffer_t(this));
    buffer->set_mime_type("image/bmp");

    uint32_t width(s[18] + s[19] * 256 + s[20] * 65536 + s[21] * 16777216);
    uint32_t height(s[22] + s[23] * 256 + s[24] * 65536 + s[25] * 16777216);
#ifdef DEBUG
SNAP_LOG_TRACE() << "BMP size (" << width << "x" << height << ")";
#endif
    if(width == 0 || height == 0)
    {
        // size just cannot be zero
        return false;
    }
    buffer->set_width(width);
    buffer->set_height(height);

    switch(headerinfosize)
    {
    case 40:
        buffer->set_format_version("1.0");
        break;

    // TBD v2 and v3 exist?

    case 108:
        buffer->set_format_version("4.0");
        break;

    case 124:
        buffer->set_format_version("5.0");
        break;

    default:
        // do not like others for now
        return false;

    }
    int bits(s[28] + s[29] * 256);
    buffer->set_bits(bits);

    // TODO this is not correct, we'd need to know whether we use a palette or not
    if(bits == 8)
    {
        buffer->set_depth(1);
    }
    else if(bits == 24)
    {
        buffer->set_depth(3);
    }
    else if(bits == 32)
    {
        buffer->set_depth(4);
    }

    buffer->set_resolution_unit("m");
    buffer->set_xres(s[38] + s[39] * 256 + s[40] * 65536 + s[41] * 16777216);
    buffer->set_yres(s[42] + s[43] * 256 + s[44] * 65536 + s[45] * 16777216);

    f_buffers.push_back(buffer);

    // got the image successfully
    return true;
}
#pragma GCC diagnostic pop


/** \brief Read a PNG header for its information.
 *
 * This function reads the PNG header information and saves it in a buffer.
 *
 * Source: http://www.w3.org/TR/PNG/
 *
 * \param[in] s  The start of the buffer.
 * \param[in] l  The size of the buffer.
 * \param[in] e  The end of the buffer (s + l).
 *
 * \return true if the PNG was read successfully.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool snap_image::info_png(unsigned char const *s, size_t l, unsigned char const *e)
{
    smart_snap_image_buffer_t buffer(new snap_image_buffer_t(this));
    buffer->set_mime_type("image/png");
    buffer->set_format_version("1.0");

    int color_format(-1);
    unsigned char const *q(s + 8);
    for(;;)
    {
        if(q + 12 > e)
        {
            return false;
        }
        uint32_t size(q[0] * 16777216 + q[1] * 65536 + q[2] * 256 + q[3]);
        // TODO test the CRC
        uint32_t name(chunk_name(q[4], q[5], q[6], q[7]));
        if(name == chunk_name('I','H','D','R') && size == 13)
        {
            buffer->set_width(q[8] * 16777216 + q[9] * 65536 + q[10] * 256 + q[11]);
            buffer->set_height(q[12] * 16777216 + q[13] * 65536 + q[14] * 256 + q[15]);

            color_format = q[17];
            switch(color_format)
            {
            case 0: // Y
                buffer->set_bits(q[16]);
                buffer->set_depth(1);
                break;

            case 2: // RGB
                buffer->set_bits(q[16] * 3);
                buffer->set_depth(3);
                break;

            case 3: // Palettized RGB
                buffer->set_bits(q[16] * 3);
                buffer->set_depth(3);
                break;

            case 4: // YA
                buffer->set_bits(q[16] * 2);
                buffer->set_depth(2);
                break;

            case 6: // RGBA
                buffer->set_bits(q[16] * 4);
                buffer->set_depth(4);
                break;

            default:
                // not supported
                return false;

            }
        }
        else if(name == chunk_name('t','R','N','S') && color_format == 3)
        {
            // well there is an alpha channel, so we have Palettized RGBA
            buffer->set_bits(buffer->get_bits() / 3 * 4);
            buffer->set_depth(4);
        }
        else if(name == chunk_name('p','H','Y','s'))
        {
            buffer->set_xres(q[8] * 16777216 + q[9] * 65536 + q[10] * 256 + q[11]);
            buffer->set_yres(q[12] * 16777216 + q[13] * 65536 + q[14] * 256 + q[15]);
            buffer->set_resolution_unit(q[16] == 1 ? "m" : "");
        }
        else if(name == chunk_name('I','D','A','T'))
        {
            f_buffers.push_back(buffer);

            // there could be multiple images, create a new buffer
            smart_snap_image_buffer_t next_image(new snap_image_buffer_t(*buffer));
            buffer = next_image;
        }
        else if(name == chunk_name('I','E','N','D'))
        {
            return true;
        }

        q += size + 12;
    }
}
#pragma GCC diagnostic pop



/** \brief Read a GIF header for its information.
 *
 * This function reads the GIF header information and saves it in a buffer.
 *
 * Source: http://www.w3.org/Graphics/GIF/spec-gif89a.txt
 *
 * \param[in] s  The start of the buffer.
 * \param[in] l  The size of the buffer.
 * \param[in] e  The end of the buffer (s + l).
 *
 * \return true if the GIF was read successfully.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool snap_image::info_gif(unsigned char const *s, size_t l, unsigned char const *e)
{
    smart_snap_image_buffer_t buffer(new snap_image_buffer_t(this));

    buffer->set_mime_type("image/gif");
    buffer->set_format_version(QString::fromLatin1(reinterpret_cast<char const *>(s) + 3, 3));

    buffer->set_width(s[6] + s[7] * 256);
    buffer->set_height(s[8] + s[9] * 256);

    //int const flags(s[10]);
    //int bits(((flags & 0x0E) >> 1) + 1);
    buffer->set_bits(8 * 3);
    buffer->set_depth(3); // GIFs are always RGB

    int const aspect(s[12]);
    if(aspect != 0)
    {
        buffer->set_xres(aspect + 15);
        buffer->set_yres(64);
    }

    f_buffers.push_back(buffer);

    // TODO create one buffer per image; unfortunately, I do not think we
    //      can safely skip the image data without parsing it all...

    return true;
}
#pragma GCC diagnostic pop



bool snap_image::get_info(QByteArray const& data)
{
    // this function is a very fast way to detect the image MIME type
    // and extract the header information (mainly the width and height
    // parameters.)
    size_t const l(data.size());
    if(l == 0)
    {
        return false;
    }
    unsigned char const *s(reinterpret_cast<unsigned char const *>(data.constData()));
    unsigned char const *e(s + l);

    // PNG starts with a clearly recognizable magic
    if(l >= 30 && s[0] == 0x89 && s[1] == 'P' && s[2] == 'N' && s[3] == 'G'
    && s[4] == 0x0D && s[5] == 0x0A && s[6] == 0x1A && s[7] == 0x0A)
    {
        return info_png(s, l, e);
    }

    // GIF starts with a clearly recognizable magic
    // (support other versions than 87a and 89a?)
    if(l >= 30
    && s[0] == 'G' && s[1] == 'I' && s[2] == 'F'
    && s[3] == '8' && (s[4] == '9' || s[4] == '7') && s[5] == 'a')
    {
        return info_gif(s, l, e);
    }

    // JPEG start with FF D8 then other markers may be in any order
    if(l >= 30 && s[0] == 0xFF && s[1] == 0xD8) // SOI marker (start of image)
    {
        return info_jpeg(s, l, e);
    }

    // Microsoft Bitmaps start with BM
    if(l >= 14 + 40 && s[0] == 'B' && s[1] == 'M')
    {
        return info_bmp(s, l, e);
    }

    // MS-Windows ICO files do not have a real magic in their header
    if(l >= 46
    && s[0] == 0x00 && s[1] == 0x00
    && s[2] == 0x01 && s[3] == 0x00
    && (s[4] + s[5] * 256) > 0)
    {
        return info_ico(s, l, e);
    }

    // no match...
    return false;
}


size_t snap_image::get_size() const
{
    return f_buffers.size();
}


smart_snap_image_buffer_t snap_image::get_buffer(int idx)
{
    return f_buffers[idx];
}



} // namespace snap

// vim: ts=4 sw=4 et
