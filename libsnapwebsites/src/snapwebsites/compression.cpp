// Snap Websites Server -- compress (decompress) data
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

#include "snapwebsites/compression.h"

#include "snapwebsites/log.h"
#include "snapwebsites/not_used.h"

#include <QMap>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <zlib.h>
#pragma GCC diagnostic pop

#include "snapwebsites/poison.h"


namespace snap
{
namespace compression
{

namespace
{
typedef QMap<QString, compressor_t *>   compressor_map_t;
typedef QMap<QString, archiver_t *>   archiver_map_t;

// IMPORTANT NOTE:
// This list only makes use of bare pointers for many good reasons.
// (i.e. all compressors are defined statitcally, not allocated)
// Do not try to change it! Thank you.
compressor_map_t * g_compressors;

// IMPORTANT NOTE:
// This list only makes use of bare pointers for many good reasons.
// (i.e. all archivers are defined statitcally, not allocated)
// Do not try to change it! Thank you.
archiver_map_t * g_archivers;

int bound_level(int level, int min, int max)
{
    return level < min ? min : (level > max ? max : level);
}

}


/** \brief Special compressor name to get the best compression available.
 *
 * Whenever we send a page on the Internet, we can compress it with zlib
 * (gzip, really). However, more and more, browsers are starting to support
 * other compressors. For example, Chrome supports "sdch" (a vcdiff
 * compressor) and FireFox is testing with lzma.
 *
 * Using the name "best" for the compressor will test with all available
 * compressions and return the smallest result whatever it is.
 */
char const *compressor_t::BEST_COMPRESSION = "best";


/** \brief Special compressor name returned in some cases.
 *
 * When trying to compress a buffer, there are several reasons why the
 * compression may "fail". When that happens the result is the same
 * as the input, meaning that the data is not going to be compressed
 * at all.
 *
 * You should always verify whether the compression worked by testing
 * the compressor_name variable on return.
 */
char const *compressor_t::NO_COMPRESSION = "none";


/** \brief Register the compressor.
 *
 * Whenever you implement a compressor, the constructor must call
 * this constructor with the name of the compressor. Remember that
 * the get_name() function is NOT ready from this constructor which
 * is why we require you to specify the name in the constructor.
 *
 * This function registers the compressor in the internal list of
 * compressors and then returns.
 *
 * \param[in] name  The name of the compressor.
 */
compressor_t::compressor_t(char const * name)
{
    if(g_compressors == nullptr)
    {
        g_compressors = new compressor_map_t;
    }
    (*g_compressors)[name] = this;
}


/** \brief Clean up the compressor.
 *
 * This function unregisters the compressor. Note that it is expected
 * that compressors get destroyed on exit only as they are expected
 * to be implemented and defined statically.
 */
compressor_t::~compressor_t()
{
    // TBD we probably don't need this code...
    //     it is rather slow so why waste our time on exit?
    //for(compressor_map_t::iterator
    //        it(g_compressors->begin());
    //        it != g_compressors->end();
    //        ++it)
    //{
    //    if(*it == this)
    //    {
    //        g_compressors->erase(it);
    //        break;
    //    }
    //}
    //delete g_compressors;
    //g_compressors = nullptr;
}


/** \brief Return a list of available compressors.
 *
 * In case you have more than one Accept-Encoding this list may end up being
 * helpful to know whether a compression is available or not.
 *
 * \return A list of all the available compressors.
 */
snap_string_list compressor_list()
{
    snap_string_list list;
    for(compressor_map_t::const_iterator
            it(g_compressors->begin());
            it != g_compressors->end();
            ++it)
    {
        list.push_back((*it)->get_name());
    }
    return list;
}


/** \brief Return a pointer to the named compressor.
 *
 * This function checks whether a compressor named \p compressor_name
 * exists, if so it gets returned, otherwise a null pointer is returned.
 *
 * \param[in] compressor_name  The name of the concerned compressor.
 *
 * \return A pointer to a compressor_t object or nullptr.
 */
compressor_t * get_compressor(QString const & compressor_name)
{
    if(g_compressors != nullptr)
    {
        if(g_compressors->contains(compressor_name))
        {
            return (*g_compressors)[compressor_name];
        }
    }

    return nullptr;
}


/** \brief Compress the input buffer.
 *
 * This function compresses the input buffer and returns the result
 * in a copy.
 *
 * IMPORTANT NOTE:
 *
 * There are several reasons why the compressor may refuse compressing
 * your input buffer and return the input as is. When this happens the
 * name of the compressor is changed to NO_COMPRESSION.
 *
 * \li The input is empty.
 * \li The input buffer is too small for that compressor.
 * \li The level is set to a value under 5%.
 * \li The buffer is way too large and allocating the compression buffer
 *     failed (this should never happen on a serious server!)
 * \li The named compressor does not exist.
 *
 * Again, if the compression fails for whatever reason, the compressor_name
 * is set to NO_COMPRESSION. You have to make sure to test that name on
 * return to know what worked and what failed.
 *
 * \param[in,out] compressor_name  The name of the compressor to use.
 * \param[in] input  The input buffer which has to be compressed.
 * \param[in] level  The level of compression (0 to 100).
 * \param[in] text  Whether the input is text, set to false if not sure.
 *
 * \return A byte array with the compressed input data.
 */
QByteArray compress(QString& compressor_name, const QByteArray& input, level_t level, bool text)
{
    // clamp the level, just in case
    //
    if(level < 0)
    {
        level = 0;
    }
    else if(level > 100)
    {
        level = 100;
    }

    // nothing to compress if empty or too small a level
    if(input.size() == 0 || level < 5)
    {
#ifdef DEBUG
SNAP_LOG_TRACE("nothing to compress");
#endif
        compressor_name = compressor_t::NO_COMPRESSION;
        return input;
    }

    if(compressor_name == compressor_t::BEST_COMPRESSION)
    {
        QByteArray best;
        for(compressor_map_t::const_iterator
                it(g_compressors->begin());
                it != g_compressors->end();
                ++it)
        {
            if(best.size() == 0)
            {
                best = (*it)->compress(input, level, text);
                compressor_name = (*it)->get_name();
            }
            else
            {
                QByteArray test((*it)->compress(input, level, text));
                if(test.size() < best.size())
                {
                    best.swap(test);
                    compressor_name = (*it)->get_name();
                }
            }
        }
        return best;
    }

    if(!g_compressors->contains(compressor_name))
    {
        // compressor is not available, return input as is...
        compressor_name = compressor_t::NO_COMPRESSION;
#ifdef DEBUG
SNAP_LOG_TRACE("compressor not found?!");
#endif
        return input;
    }

    // avoid the compression if the result is larger or equal to the input!
    QByteArray const result((*g_compressors)[compressor_name]->compress(input, level, text));
    if(result.size() >= input.size())
    {
        compressor_name = compressor_t::NO_COMPRESSION;
#ifdef DEBUG
SNAP_LOG_TRACE("compression is larger?!");
#endif
        return input;
    }

    return result;
}


/** \brief Decompress a buffer.
 *
 * This function checks the specified input buffer and decompresses it if
 * a compressor recognized its magic signature.
 *
 * If none of the compressors were compatible then the input is returned
 * as is. The compressor_name is set to NO_COMPRESSION in this case. This
 * does not really mean the buffer is not compressed, although it is likely
 * correct.
 *
 * \param[out] compressor_name  Receives the name of the compressor used
 *                              to decompress the input data.
 * \param[in] input  The input to decompress.
 *
 * \return The decompressed buffer.
 */
QByteArray decompress(QString & compressor_name, QByteArray const & input)
{
    // nothing to decompress if empty
    if(input.size() > 0)
    {
        for(compressor_map_t::const_iterator
                it(g_compressors->begin());
                it != g_compressors->end();
                ++it)
        {
            if((*it)->compatible(input))
            {
                compressor_name = (*it)->get_name();
                return (*it)->decompress(input);
            }
        }
    }

    compressor_name = compressor_t::NO_COMPRESSION;
    return input;
}


/** \brief Implementation of the GZip compressor.
 *
 * This class defines the gzip compressor which compresses and decompresses
 * data using the gzip file format.
 *
 * \note
 * This implementation makes use of the zlib library to do all the
 * compression and decompression work.
 */
class gzip_t : public compressor_t
{
public:
    gzip_t() : compressor_t("gzip")
    {
    }

    virtual char const * get_name() const
    {
        return "gzip";
    }

    virtual QByteArray compress(QByteArray const & input, level_t level, bool text)
    {
        // clamp the level, just in case
        //
        if(level < 0)
        {
            level = 0;
        }
        else if(level > 100)
        {
            level = 100;
        }

        // transform the 0 to 100 level to the standard 1 to 9 in zlib
        int const zlib_level(bound_level((level * 2 + 25) / 25, Z_BEST_SPEED, Z_BEST_COMPRESSION));
        // initialize the zlib stream
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        // deflateInit2 expects the input to be defined
        strm.avail_in = input.size();
        strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(input.data()));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        int ret(deflateInit2(&strm, zlib_level, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY));
#pragma GCC diagnostic pop
        if(ret != Z_OK)
        {
            // compression failed, return input as is
            return input;
        }

        // initialize the gzip header
        gz_header header;
        memset(&header, 0, sizeof(header));
        header.text = text;
        header.time = time(nullptr);
        header.os = 3;
        header.comment = const_cast<Bytef *>(reinterpret_cast<Bytef const *>("Snap! Websites"));
        //header.hcrc = 1; -- would that be useful?
        ret = deflateSetHeader(&strm, &header);
        if(ret != Z_OK)
        {
            deflateEnd(&strm);
            return input;
        }

        // prepare to call the deflate function
        // (to do it in one go!)
        // TODO check the size of the input buffer, if really large
        //      (256Kb?) then break this up in multiple iterations
        QByteArray result;
        result.resize(static_cast<int>(deflateBound(&strm, strm.avail_in)));
        strm.avail_out = result.size();
        strm.next_out = reinterpret_cast<Bytef *>(result.data());

        // compress in one go
        ret = deflate(&strm, Z_FINISH);
        if(ret != Z_STREAM_END)
        {
            deflateEnd(&strm);
            return input;
        }
        // lose the extra size returned by deflateBound()
        result.resize(result.size() - strm.avail_out);
        deflateEnd(&strm);
        return result;
    }

    virtual bool compatible(QByteArray const & input) const
    {
        // the header is at least 10 bytes
        // the magic code (identification) is 0x1F 0x8B
        return input.size() >= 10 && input[0] == 0x1F && input[1] == static_cast<char>(0x8B);
    }

    virtual QByteArray decompress(QByteArray const & input)
    {
        // initialize the zlib stream
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        // inflateInit2 expects the input to be defined
        strm.avail_in = input.size();
        strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(input.data()));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        int ret(inflateInit2(&strm, 15 + 16));
#pragma GCC diagnostic pop
        if(ret != Z_OK)
        {
            // decompression failed, return input as is assuming it was not
            // compressed maybe...
            return input;
        }

        // Unfortunately the zlib support for the gzip header does not help
        // us getting the ISIZE which is saved as the last 4 bytes of the
        // files (frankly?!)
        //
        // initialize the gzip header
        //gz_header header;
        //memset(&header, 0, sizeof(header));
        //ret = inflateGetHeader(&strm, &header);
        //if(ret != Z_OK)
        //{
        //    inflateEnd(&strm);
        //    return input;
        //}
        // The size is saved in the last 4 bytes in little endian
        size_t uncompressed_size(input[strm.avail_in - 4]
                | (input[strm.avail_in - 3] << 8)
                | (input[strm.avail_in - 2] << 16)
                | (input[strm.avail_in - 1] << 24));

        // prepare to call the inflate function
        // (to do it in one go!)
        QByteArray result;
        result.resize(static_cast<int>(uncompressed_size));
        strm.avail_out = result.size();
        strm.next_out = reinterpret_cast<Bytef *>(result.data());

        // decompress in one go
        ret = inflate(&strm, Z_FINISH);
        if(ret != Z_STREAM_END)
        {
            inflateEnd(&strm);
            return input;
        }
        inflateEnd(&strm);
        return result;
    }

    virtual QByteArray decompress(QByteArray const & input, size_t uncompressed_size)
    {
        NOTUSED(input);
        NOTUSED(uncompressed_size);
        throw compression_exception_not_implemented("gzip decompress() with the uncompressed_size parameter is not implemented.");
    }

} g_gzip; // create statically


class deflate_t : public compressor_t
{
public:
    deflate_t() : compressor_t("deflate")
    {
    }

    virtual const char *get_name() const
    {
        return "deflate";
    }

    virtual QByteArray compress(QByteArray const & input, level_t level, bool text)
    {
        NOTUSED(text);

        // transform the 0 to 100 level to the standard 1 to 9 in zlib
        int const zlib_level(bound_level((level * 2 + 25) / 25, Z_BEST_SPEED, Z_BEST_COMPRESSION));
        // initialize the zlib stream
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        // deflateInit2 expects the input to be defined
        strm.avail_in = input.size();
        strm.next_in = const_cast<Bytef *>(reinterpret_cast<Bytef const *>(input.data()));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        int ret(deflateInit2(&strm, zlib_level, Z_DEFLATED, 15, 9, Z_DEFAULT_STRATEGY));
#pragma GCC diagnostic pop
        if(ret != Z_OK)
        {
            // compression failed, return input as is
            return input;
        }

        // prepare to call the deflate function
        // (to do it in one go!)
        // TODO check the size of the input buffer, if really large
        //      (256Kb?) then break this up in multiple iterations
        QByteArray result;
        result.resize(static_cast<int>(deflateBound(&strm, strm.avail_in)));
        strm.avail_out = result.size();
        strm.next_out = reinterpret_cast<Bytef *>(result.data());

        // compress in one go
        ret = deflate(&strm, Z_FINISH);
        if(ret != Z_STREAM_END)
        {
            deflateEnd(&strm);
            return input;
        }
        // lose the extra size returned by deflateBound()
        result.resize(result.size() - strm.avail_out);
        deflateEnd(&strm);
        return result;
    }

    virtual bool compatible(QByteArray const & input) const
    {
        NOTUSED(input);

        // there is no magic header in this one...
        return false;
    }

    virtual QByteArray decompress(QByteArray const & input)
    {
        // the decompress function for "deflate" requires the size in
        // our case so this function is not implemented for now...
        NOTUSED(input);
        throw compression_exception_not_implemented("gzip decompress() with the uncompressed_size parameter is not implemented.");
    }

    virtual QByteArray decompress(QByteArray const & input, size_t uncompressed_size)
    {
        // by default we cannot reach this function, if we get called, then
        // the caller specifically wanted to call us, in such a case we
        // expect the size of the uncompressed data to be specified...

        // initialize the zlib stream
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        // inflateInit expects the input to be defined
        strm.avail_in = input.size();
        strm.next_in = const_cast<Bytef *>(reinterpret_cast<Bytef const *>(input.data()));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        int ret(inflateInit(&strm));
#pragma GCC diagnostic pop
        if(ret != Z_OK)
        {
            // compression failed, return input as is
            return input;
        }

        // Unfortunately the zlib support for the gzip header does not help
        // us getting the ISIZE which is saved as the last 4 bytes of the
        // files (frankly?!)
        //
        // initialize the gzip header
        //gz_header header;
        //memset(&header, 0, sizeof(header));
        //ret = inflateGetHeader(&strm, &header);
        //if(ret != Z_OK)
        //{
        //    inflateEnd(&strm);
        //    return input;
        //}
        // The size is saved in the last 4 bytes in little endian
        //size_t uncompressed_size(input[strm.avail_in - 4]
        //        | (input[strm.avail_in - 3] << 8)
        //        | (input[strm.avail_in - 2] << 16)
        //        | (input[strm.avail_in - 1] << 24));

        // prepare to call the inflate function
        // (to do it in one go!)
        QByteArray result;
        result.resize(static_cast<int>(uncompressed_size));
        strm.avail_out = result.size();
        strm.next_out = reinterpret_cast<Bytef *>(result.data());

        // decompress in one go
        ret = inflate(&strm, Z_FINISH);
        inflateEnd(&strm);
        if(ret != Z_STREAM_END)
        {
            return input;
        }
        return result;
    }

} g_deflate; // create statically






void archiver_t::file_t::set_type(type_t type)
{
    f_type = type;
}


void archiver_t::file_t::set_data(QByteArray const& data)
{
    f_data = data;
}


void archiver_t::file_t::set_filename(QString const& filename)
{
    f_filename = filename;
}


void archiver_t::file_t::set_user(QString const& user, uid_t uid)
{
    f_user = user;
    f_uid = uid;
}


void archiver_t::file_t::set_group(QString const& group, gid_t gid)
{
    f_group = group;
    f_gid = gid;
}


void archiver_t::file_t::set_mode(mode_t mode)
{
    f_mode = mode;
}


void archiver_t::file_t::set_mtime(time_t mtime)
{
    f_mtime = mtime;
}


archiver_t::file_t::type_t archiver_t::file_t::get_type() const
{
    return f_type;
}


QByteArray const & archiver_t::file_t::get_data() const
{
    return f_data;
}


QString const & archiver_t::file_t::get_filename() const
{
    return f_filename;
}


QString const & archiver_t::file_t::get_user() const
{
    return f_user;
}


QString const & archiver_t::file_t::get_group() const
{
    return f_group;
}


uid_t archiver_t::file_t::get_uid() const
{
    return f_uid;
}


gid_t archiver_t::file_t::get_gid() const
{
    return f_gid;
}


mode_t archiver_t::file_t::get_mode() const
{
    return f_mode;
}


time_t archiver_t::file_t::get_mtime() const
{
    return f_mtime;
}


archiver_t::archiver_t(char const * name)
    //: f_archive() -- auto-init
{
    if(g_archivers == nullptr)
    {
        g_archivers = new archiver_map_t;
    }
    (*g_archivers)[name] = this;
}


archiver_t::~archiver_t()
{
    // TBD we probably do not need this code...
    //     it is rather slow so why waste our time on exit?
    //for(archiver_map_t::iterator
    //        it(g_archivers->begin());
    //        it != g_archivers->end();
    //        ++it)
    //{
    //    if(*it == this)
    //    {
    //        g_archivers->erase(it);
    //        break;
    //    }
    //}
    //delete g_archivers;
    //g_archivers = nullptr;
}


void archiver_t::set_archive(QByteArray const & input)
{
    f_archive = input;
}


QByteArray archiver_t::get_archive() const
{
    return f_archive;
}



class tar : public archiver_t
{
public:
    tar()
        : archiver_t("tar")
        //, f_pos(0) -- auto-init
    {
    }

    virtual char const * get_name() const
    {
        return "tar";
    }

    virtual void append_file(file_t const & file)
    {
        QByteArray utf8;

        // INITIALIZE HEADER
        std::vector<char> header;
        header.resize(512, 0);
        std::string const ustar("ustar ");
        std::copy(ustar.begin(), ustar.end(), header.begin() + 257);
        header[263] = ' '; // version " \0"
        header[264] = '\0';

        // FILENAME
        QString fn(file.get_filename());
        int l(fn.length());
        if(l > 100)
        {
            // TODO: add support for longer filenames
            throw compression_exception_not_compatible("this file cannot be added to a tar archive at this point (filename too long)");
        }
        utf8 = fn.toUtf8();
        std::copy(utf8.data(), utf8.data() + utf8.size(), header.begin());

        // MODE, UID, GID, MTIME
        append_int(&header[100], file.get_mode(),   7, 8, '0');
        append_int(&header[108], file.get_uid(),    7, 8, '0');
        append_int(&header[116], file.get_gid(),    7, 8, '0');
        append_int(&header[136], static_cast<int>(file.get_mtime()), 11, 8, '0');

        // USER/GROUP NAMES
        utf8 = file.get_user().toUtf8();
        if(utf8.length() > 32)
        {
            throw compression_exception_not_compatible("this file cannot be added to a tar archive at this point (user name too long)");
        }
        std::copy(utf8.data(), utf8.data() + utf8.size(), header.begin() + 265);

        utf8 = file.get_group().toUtf8();
        if(utf8.length() > 32)
        {
            throw compression_exception_not_compatible("this file cannot be added to a tar archive at this point (group name too long)");
        }
        std::copy(utf8.data(), utf8.data() + utf8.size(), header.begin() + 265);

        // TYPE, SIZE
        switch(file.get_type())
        {
        case file_t::type_t::FILE_TYPE_REGULAR:
            header[156] = '0'; // regular (tar type)
            append_int(&header[124], file.get_data().size(), 11, 8, '0');
            break;

        case file_t::type_t::FILE_TYPE_DIRECTORY:
            // needs to be zero in ASCII
            header[156] = '5'; // directory (tar type)
            append_int(&header[124], 0, 11, 8, '0');
            break;

        //default: ... we could throw but here the compile fails if we
        //             are missing some types
        }

        uint32_t checksum(tar_check_sum(&header[0]));
        if(checksum > 32767)
        {
            // no null in this case (very rare if at all possible)
            append_int(&header[148], checksum, 7, 8, '0');
        }
        else
        {
            append_int(&header[148], checksum, 6, 8, '0');
        }
        header[155] = ' ';

        f_archive.append(&header[0], static_cast<int>(header.size()));

        switch(file.get_type())
        {
        case file_t::type_t::FILE_TYPE_REGULAR:
            f_archive.append(file.get_data());
            {
                // padding to next 512 bytes
                uint32_t size(file.get_data().size());
                size &= 511;
                if(size != 0)
                {
                    std::vector<char> pad;
                    pad.resize(512 - size, 0);
                    f_archive.append(&pad[0], static_cast<int>(pad.size()));
                }
            }
            break;

        default:
            // no data for that type
            break;

        }
    }

    virtual bool next_file(file_t & file) const
    {
        // any more files?
        // (make sure there is at least a header for now)
        if(f_pos + 512 > f_archive.size())
        {
            return false;
        }

        // read the header
        std::vector<char> header;
        std::copy(f_archive.data() + f_pos, f_archive.data() + f_pos + 512, &header[0]);

        // MAGIC
        if(header[257] != 'u' || header[258] != 's' || header[259] != 't' || header[260] != 'a'
        || header[261] != 'r' || (header[262] != ' ' && header[262] != '\0'))
        {
            // if no MAGIC we may have empty blocks (which are legal at the
            // end of the file)
            for(int i(0); i < 512; ++i)
            {
                if(header[i] != '\0')
                {
                    throw compression_exception_not_compatible(QString("ustar magic code missing at position %1").arg(f_pos));
                }
            }
            f_pos += 512;
            // TODO: test all the following blocks as they all should be null
            //       (as you cannot find an empty block within the tarball)
            return false;
        }

        uint32_t const file_checksum(read_int(&header[148], 8, 8));
        uint32_t const comp_checksum(tar_check_sum(&header[0]));
        if(file_checksum != comp_checksum)
        {
            throw compression_exception_not_compatible(QString("ustar checksum code does not match what was expected"));
        }

        QString filename(QString::fromUtf8(&header[0], static_cast<int>(strnlen(&header[0], 100))));
        if(header[345] != '\0')
        {
            // this one has a prefix (long filename)
            QString prefix(QString::fromUtf8(&header[345], static_cast<int>(strnlen(&header[345], 155))));
            if(prefix.endsWith("/"))
            {
                // I think this case is considered a bug in a tarball...
                filename = prefix + filename;
            }
            else
            {
                filename = prefix + "/" + filename;
            }
        }
        file.set_filename(filename);

        switch(header[156])
        {
        case '\0':
        case '0':
            file.set_type(file_t::type_t::FILE_TYPE_REGULAR);
            break;

        case '5':
            file.set_type(file_t::type_t::FILE_TYPE_DIRECTORY);
            break;


        default:
            throw compression_exception_not_supported("file in tarball not supported (we accept regular and directory files only)");

        }

        file.set_mode (read_int(&header[100],  8, 8));
        file.set_mtime(read_int(&header[136], 12, 8));

        uid_t uid(read_int(&header[108],  8, 8));
        file.set_user (QString::fromUtf8(&header[265], 32), uid);

        gid_t gid(read_int(&header[116],  8, 8));
        file.set_group(QString::fromUtf8(&header[297], 32), gid);

        f_pos += 512;

        if(file.get_type() == file_t::type_t::FILE_TYPE_REGULAR)
        {
            uint32_t const size(read_int(&header[124], 12, 8));
            int const total_size((size + 511) & -512);
            if(f_pos + total_size > f_archive.size())
            {
                throw compression_exception_not_supported("file data not available (archive too small)");
            }
            QByteArray data;
            data.append(f_archive.data() + f_pos, size);
            file.set_data(data);

            f_pos += total_size;
        }

        return true;
    }

    virtual void rewind_file()
    {
        f_pos = 0;
    }

private:
    void append_int(char * header, int value, unsigned int length, int base, char fill)
    {
        // save the number (minimum 1 digit)
        do
        {
            // base is 8 or 10
            header[length] = static_cast<char>((value % base) + '0');
            value /= base;
            --length;
        }
        while((length > 0) && (value != 0));

        // fill the left side with 'fill'
        while(length > 0)
        {
            header[length] = fill;
            --length;
        }
    }

    uint32_t read_int(char const * header, int length, int base) const
    {
        // TODO: add tests
        uint32_t result(0);
        for(; length > 0 && *header != '\0' && *header != ' '; --length, ++header)
        {
            result = result * base + (*header - '0');
        }
        return result;
    }

    uint32_t tar_check_sum(char const * s) const
    {
        uint32_t result = 8 * ' '; // the checksum field

        // name + mode + uid + gid + size + mtime = 148 bytes
        for(int n(148); n > 0; --n, ++s)
        {
            result += *s;
        }

        s += 8; // skip the checksum field

        // everything after the checksum is another 356 bytes
        for(int n(356); n > 0; --n, ++s)
        {
            result += *s;
        }

        return result;
    }

    mutable int32_t   f_pos = 0;
} g_tar; // declare statically



} // namespace snap
} // namespace compression
// vim: ts=4 sw=4 et
