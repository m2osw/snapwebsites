/*
 * Text:
 *      snapimg.cpp
 *
 * Description:
 *      Get image information and display that on the screen. This is mainly
 *      a test of our image library although it can, of course, be used for
 *      any other purpose.
 *
 * License:
 *      Copyright (c) 2013-2017 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// snapwebsites lib
//
#include <snapwebsites/snap_image.h>
#include <snapwebsites/snapwebsites.h>

// advgetopt lib
//
#include <advgetopt.h>

// Qt lib
//
#include <QFile>
#include <QTextCodec>

// C++ lib
//
#include <iostream>


advgetopt::getopt::option const g_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "Usage: %p [-<opt>] <filename> ...",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "where -<opt> is one or more of:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        NULL,
        "Show usage and exit.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        '\0',
        0,
        "version",
        NULL,
        "print out the version",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        "filename",
        NULL,
        NULL, // hidden argument in --help screen
        advgetopt::getopt::argument_mode_t::default_multiple_argument
    },
    {
        '\0',
        0,
        NULL,
        NULL,
        NULL,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};
advgetopt::getopt *g_opt = NULL;

int g_errcnt = 0;



void image_info()
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    size_t const max_length(g_opt->size("filename"));
    for(size_t idx(0); idx < max_length; ++idx)
    {
        QString filename(QString::fromUtf8(g_opt->get_string("filename", static_cast<int>(idx)).c_str()));
        QFile file(filename);
        if(!file.open(QIODevice::ReadOnly))
        {
            ++g_errcnt;
            std::cerr << "error: could not load \"" << filename.toUtf8().data() << "\"" << std::endl;
            continue;
        }
        QByteArray image_file(file.readAll());
        snap::snap_image img;
        if(!img.get_info(image_file))
        {
            ++g_errcnt;
            std::cerr << "error: file format either not supported at all or not complete; could not get the info of \"" << filename.toUtf8().data() << "\"" << std::endl;
            continue;
        }
        size_t const max_buf(img.get_size());
        for(size_t j(0); j < max_buf; ++j)
        {
            snap::smart_snap_image_buffer_t buf(img.get_buffer(static_cast<int>(j)));
            std::cout << "*** " << filename.toUtf8().data();
            if(max_buf > 1)
            {
                std::cout << " (" << j << ")";
            }
            std::cout << " ***" << std::endl;
            std::cout << "MIME type:             " << buf->get_mime_type().toUtf8().data() << std::endl;
            std::cout << "File Format Version:   " << buf->get_format_version().toUtf8().data() << std::endl;
            std::cout << "Resolution Unit:       " << buf->get_resolution_unit().toUtf8().data() << std::endl;
            std::cout << "Horizontal Resolution: " << buf->get_xres() << std::endl;
            std::cout << "Vertical Resolution:   " << buf->get_yres() << std::endl;
            std::cout << "Width:                 " << buf->get_width() << std::endl;
            std::cout << "Height:                " << buf->get_height() << std::endl;
            std::cout << "Depth:                 " << buf->get_depth() << std::endl;
            std::cout << "Bit:                   " << buf->get_bits() << std::endl;
        }
    }
}



int main(int argc, char *argv[])
{
    try
    {
        const std::vector<std::string> no_config;
        g_opt = new advgetopt::getopt(argc, argv, g_options, no_config, NULL);
        if(g_opt->is_defined("version"))
        {
            std::cerr << SNAPWEBSITES_VERSION_STRING << std::endl;
            exit(1);
        }
        if(g_opt->is_defined("help"))
        {
            g_opt->usage(advgetopt::getopt::status_t::no_error, "Usage: %s [--<opts>] <imagefile> ...\n", argv[0]);
            exit(1);
        }

        image_info();

        return g_errcnt == 0 ? 0 : 1;
    }
    catch(std::exception const& e)
    {
        // clean way to exit if an exception occurs
        std::cerr << "snapimg: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
