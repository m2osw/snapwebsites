/*
 * Text:
 *      snapwebsites/snaplayout/src/snaplayout.cpp
 *
 * Description:
 *      Save layout files in the Snap database.
 *
 * License:
 *      Copyright (c) 2012-2017 Made to Order Software Corp.
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

#include "version.h"

#include <snapwebsites/not_reached.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_version.h>
#include <snapwebsites/snap_image.h>
#include <snapwebsites/snapwebsites.h>

#include <advgetopt/advgetopt.h>

#include <libdbproxy/value.h>

#include <casswrapper/schema.h>
#include <casswrapper/session.h>
#include <casswrapper/query.h>

#include <zipios/zipfile.hpp>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

#include <sys/stat.h>

#include <QDomDocument>
#include <QDomElement>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QXmlInputSource>

#include <snapwebsites/poison.h>

using namespace casswrapper;
using namespace libdbproxy;

namespace
{

/** \brief A vector of string is required for f_opt
 *
 * The vector is kept empty. It is required to parse the command line
 * options but snaplayout does not make use of configuration files.
 */
const std::vector<std::string> g_configuration_files; // Empty


/** \brief The options of the snaplayout command line tool.
 *
 * This table represents all the options available on the snaplayout
 * command line.
 */
advgetopt::getopt::option const g_snaplayout_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "Usage: %p [<options>] <layout filename(s)>",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "where options are one or more of:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '?',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "show this help output",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "context",
        "snap_websites",
        "Specify the context (keyspace) to connect to.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'x',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "extract",
        nullptr,
        "extract a file from the specified layout and filename",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "host",
        "localhost",
        "host IP address or name [default=localhost]",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'p',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "port",
        "9042",
        "port on the host to which to connect [default=9042]",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        '\0',
        0,
        "remove-theme",
        nullptr,
        "remove the specified theme; this remove the entire row and can allow you to reinstall a theme that \"lost\" files",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "no-ssl",
        nullptr,
        "Supress the use of SSL even if the keys are present.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    { // at least until we have a way to edit the theme from the website
        't',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "set-theme",
        nullptr,
        "usage: --set-theme URL [theme|layout] ['\"layout name\";']'",
        advgetopt::getopt::argument_mode_t::no_argument // expect 3 params as filenames
    },
    {
        'v',
        0,
        "verbose",
        nullptr,
        "show what snaplayout is doing",
        advgetopt::getopt::argument_mode_t::no_argument // expect 3 params as filenames
    },
    {
        '\0',
        0,
        "version",
        nullptr,
        "show the version of %p and exit",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        "layout-file1.xsl layout-file2.xsl ... layout-fileN.xsl or layout.zip",
        advgetopt::getopt::argument_mode_t::default_multiple_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};


/** \brief Load the data of a stream in a QByteArray.
 *
 * This function reads all the input data available in the input
 * stream \p is and saves it in the specified QByteArray \p arr
 * parameter.
 *
 * \param[in,out] is  A pointer to an input stream.
 * \param[out] arr  The array where the data read from the input stream is saved.
 */
void stream_to_bytearray( std::istream & is, QByteArray & arr )
{
    arr.clear();
    char ch('\0');
    for(;;)
    {
        is.get( ch );
        if( is.eof() )
        {
            break;
        }
        arr.push_back( ch );
    }
}


}
//namespace


/** \brief A class for easy access to all resources.
 *
 * This class is just so we use resource in an object oriented
 * manner rather than having globals, but that's clearly very
 * similar here!
 */
class snap_layout
{
public:
    snap_layout(int argc, char *argv[]);

    void usage();
    void run();
    void add_files();
    bool load_xml_info(QDomDocument& doc, QString const & filename, QString & layout_name, time_t & layout_modified);
    void load_xsl_info(QDomDocument& doc, QString const & filename, QString & layout_name, QString & layout_area, time_t & layout_modified);
    void load_css  (QString const & filename, QByteArray const & content, QString & row_name);
    void load_js   (QString const & filename, QByteArray const & content, QString & row_name);
    void load_image(QString const & filename, QByteArray const & content, QString & row_name);
    void set_theme();
    void remove_theme();
    void extract_file();

private:
    typedef std::shared_ptr<advgetopt::getopt>    getopt_ptr_t;

    // Layout file structure
    //
    struct fileinfo_t
    {
        fileinfo_t()
            : f_filetime(0)
        {
        }

        fileinfo_t( QString const & fn, QByteArray const & content, time_t const time )
            : f_filename(fn)
            , f_content(content)
            , f_filetime(time)
        {
        }

        QString    f_filename;
        QByteArray f_content;
        time_t     f_filetime;
    };
    typedef std::vector<fileinfo_t>   fileinfo_list_t;

    // Private methods
    //
    void connect();
    bool tableExists( QString const & table_name ) const;
    bool rowExists  ( QString const & table_name, QByteArray const & row_key ) const;
    bool cellExists ( QString const & table_name, QByteArray const & row_key, QByteArray const & cell_key ) const;

    // Common attributes
    //
    Session::pointer_t f_session;
    fileinfo_list_t                 f_fileinfo_list;
    getopt_ptr_t                    f_opt;
    bool                            f_verbose = false;
};


snap_layout::snap_layout(int argc, char * argv[])
    : f_session( Session::create() )
    //, f_fileinfo_list -- auto-init
    , f_opt( new advgetopt::getopt( argc, argv, g_snaplayout_options, g_configuration_files, "SNAPSERVER_OPTIONS" ) )
    , f_verbose(f_opt->is_defined("verbose"))
{
    if( f_opt->is_defined( "help" ) )
    {
        usage();
    }
    if( f_opt->is_defined( "version" ) )
    {
        std::cout << SNAPLAYOUT_VERSION_STRING << std::endl;
        exit(0);
        snap::NOTREACHED();
    }
    //
    if( !f_opt->is_defined( "--" ) )
    {
        if( f_opt->is_defined( "set-theme" ) )
        {
            std::cerr << "usage: snaplayout --set-theme URL [theme|layout] ['\"layout_name\";']'" << std::endl;
            std::cerr << "note: if layout_name is not specified, the theme/layout is deleted from the database." << std::endl;
            exit(1);
            snap::NOTREACHED();
        }
        if( f_opt->is_defined( "extract" ) )
        {
            std::cerr << "usage: snaplayout --extract <layout name> <filename>" << std::endl;
            exit(1);
            snap::NOTREACHED();
        }
        if( f_opt->is_defined( "remove-theme" ) )
        {
            std::cerr << "usage: snaplayout --remove-theme <layout name>" << std::endl;
            exit(1);
            snap::NOTREACHED();
        }
        std::cerr << "one or more layout files are required!" << std::endl;
        usage();
        snap::NOTREACHED();
    }
    if(!f_opt->is_defined("set-theme")
    && !f_opt->is_defined("remove-theme")
    && !f_opt->is_defined("extract"))
    {
        for( int idx(0); idx < f_opt->size( "--" ); ++idx )
        {
            QString const filename ( f_opt->get_string( "--", idx ).c_str() );
            const int e(filename.lastIndexOf("."));
            QString const extension( filename.mid(e) );
            if( extension == ".zip" )
            {
                std::cout << "Unpacking zipfile '" << filename << "':" << std::endl;

                // zipios2 throws if it cannot open the input file
                zipios::ZipFile zf( filename.toUtf8().data() );
                //
                for( auto const & ent : zf.entries() )
                {
                    if( ent && ent->isValid() && !ent->isDirectory() )
                    {
                        if(f_verbose)
                        {
                            std::cout << "\t" << *ent << std::endl;
                        }

                        std::string const fn( ent->getName() );
                        try
                        {
                            zipios::FileCollection::stream_pointer_t is( zf.getInputStream( fn ) ) ;

                            QByteArray byte_arr;
                            stream_to_bytearray( *is.get(), byte_arr );

                            f_fileinfo_list.push_back( fileinfo_t( fn.c_str(), byte_arr, ent->getUnixTime() ) );
                        }
                        catch( std::ios_base::failure const & except )
                        {
                            std::cerr << "Caught an ios_base::failure while extracting file '"
                                << fn << "'." << std::endl
                                << "Explanatory string: " << except.what() << std::endl
                                //<< "Error code: " << except.code() << std::endl
                                ;
                            exit(1);
                            snap::NOTREACHED();
                        }
                        catch( std::exception const & except )
                        {
                            std::cerr << "Error extracting '" << fn << "': Exception caught: " << except.what() << std::endl;
                            exit(1);
                            snap::NOTREACHED();
                        }
                        catch( ... )
                        {
                            std::cerr << "Caught unknown exception attempting to extract '" << fn << "'!" << std::endl;
                            exit(1);
                            snap::NOTREACHED();
                        }
                    }
                }
            }
            else
            {
                std::ifstream ifs( filename.toUtf8().data() );
                if( !ifs.is_open() )
                {
                    std::cerr << "error: could not open layout file named \"" << filename << "\"" << std::endl;
                    exit(1);
                    snap::NOTREACHED();
                }

                time_t filetime(0);
                struct stat s;
                if( stat(filename.toUtf8().data(), &s) == 0 )
                {
                    filetime = s.st_mtime;
                }
                else
                {
                    std::cerr << "error: could not get mtime from file \"" << filename << "\"." << std::endl;
                    exit(1);
                    snap::NOTREACHED();
                }

                QByteArray byte_arr;
                stream_to_bytearray( ifs, byte_arr );
                f_fileinfo_list.push_back( fileinfo_t( filename, byte_arr, filetime ) );
            }
        }
    }
}


void snap_layout::usage()
{
    f_opt->usage( advgetopt::getopt::status_t::no_error, "snaplayout" );
    exit(1);
    snap::NOTREACHED();
}


bool snap_layout::load_xml_info(QDomDocument & doc, QString const & filename, QString & content_name, time_t & content_modified)
{
    content_name.clear();
    content_modified = 0;

    QDomElement snap_tree(doc.documentElement());
    if(snap_tree.isNull())
    {
        std::cerr << "error: the XML document does not have a root element, failed handling \"" << filename << "\"" << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    QString const content_modified_date(snap_tree.attribute("content-modified", "0"));

    bool const result(snap_tree.tagName() == "snap-tree");
    if(result)
    {
        QDomNodeList const content(snap_tree.elementsByTagName("content"));
        int const max_nodes(content.size());
        for(int idx(0); idx < max_nodes; ++idx)
        {
            // all should be elements... but still verify
            QDomNode p(content.at(idx));
            if(!p.isElement())
            {
                continue;
            }
            QDomElement e(p.toElement());
            if(e.isNull())
            {
                continue;
            }
            QString const path(e.attribute("path", ""));
            if(path.isEmpty())
            {
                // this is probably an error
                continue;
            }
            if(path.startsWith("/layouts/"))
            {
                int pos(path.indexOf('/', 9));
                if(pos < 0)
                {
                    pos = path.length();
                }
                QString name(path.mid(9, pos - 9));
                if(name.isEmpty())
                {
                    std::cerr << "error: the XML document seems to have an invalid path in \"" << filename << "\"" << std::endl;
                    exit(1);
                    snap::NOTREACHED();
                }
                if(content_name.isEmpty())
                {
                    content_name = name;
                }
                else if(content_name != name)
                {
                    std::cerr << "error: the XML document includes two different entries with layout paths that differ: \""
                              << content_name << "\" and \"" << name << "\" in \"" << filename << "\"" << std::endl;
                    exit(1);
                    snap::NOTREACHED();
                }
            }
        }
    }
    else
    {
        // in case the layout and plugin have different names, the layout
        // will be in the layout parameter
        //
        content_name = snap_tree.attribute("layout");
        if(content_name.isEmpty())
        {
            content_name = snap_tree.attribute("owner");
        }
    }

    if(content_name.isEmpty())
    {
        std::cerr << "error: the XML document is missing a path to a layout in \"" << filename << "\"" << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    if(content_modified_date.isEmpty())
    {
        std::cerr << "error: the XML document is missing its content-modified attribute in your XML document \"" << filename << "\"" << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    // now convert the date, we expect a very specific format
    QDateTime t(QDateTime::fromString(content_modified_date, "yyyy-MM-dd HH:mm:ss"));
    if(!t.isValid())
    {
        std::cerr << "error: the date \"" << content_modified_date << "\" doesn't seem valid in \"" << filename << "\", the expected format is \"yyyy-MM-dd HH:mm:ss\"" << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    content_modified = t.toTime_t();

    return result;
}


/** \brief Retrieve information from an XSL document.
 *
 * Layouts are defined in an XSL file. The information there includes
 * retrieved the \p layout_name, \p layout_area and the \p layout_modified.
 *
 * \param[in] doc  The XSL document to be checked out.
 * \param[in] filename  The name of the input file representing doc.
 * \param[out] layout_name  The name of the layout (i.e. the row in the db).
 * \param[out] layout_area  The name of the area (i.e. the cell in the db,
 *                          which represents a filename).
 * \param[out] layout_modified  The time when this info was last updated.
 */
void snap_layout::load_xsl_info(QDomDocument & doc, QString const & filename, QString & layout_name, QString & layout_area, time_t & layout_modified)
{
    layout_name.clear();
    layout_area.clear();
    layout_modified = 0;

    QString layout_modified_date;

    QDomNodeList params(doc.elementsByTagNameNS("http://www.w3.org/1999/XSL/Transform", "variable"));
    int const max_nodes(params.size());
    for(int idx(0); idx < max_nodes; ++idx)
    {
        // all should be elements... but still verify
        //
        QDomNode p(params.at(idx));
        if(!p.isElement())
        {
            continue;
        }
        QDomElement e(p.toElement());
        if(e.isNull())
        {
            continue;
        }

        // save in buffer, this is not too effective since
        // we convert all the parameters even though that
        // we'll throw away! but that way we don't have to
        // duplicate the code plus this process is not run
        // by the server
        //
        QDomNodeList children(e.childNodes());
        if(children.size() != 1)
        {
            // that's most certainly the wrong name?
            //
            continue;
        }
        QDomNode n(children.at(0));

        QString buffer;
        QTextStream data(&buffer);
        n.save(data, 0);

        // verify the name
        //
        QString const name(e.attribute("name"));
        if(name == "layout-name")
        {
            // that is the row key
            //
            layout_name = buffer;
        }
        else if(name == "layout-area")
        {
            // that is the name of the column
            //
            layout_area = buffer;
            if(!layout_area.endsWith(".xsl"))
            {
                layout_area += ".xsl";
            }
        }
        else if(name == "layout-modified")
        {
            // that is to make sure we do not overwrite a newer version
            //
            layout_modified_date = buffer;
        }
    }

    if(layout_name.isEmpty() || layout_area.isEmpty() || layout_modified_date.isEmpty())
    {
        std::cerr << "error: the layout-name, layout-area, and layout-modified parameters must all three be defined in your XSL document \"" << filename << "\"" << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    // now convert the date, we expect a very specific format
    QDateTime t(QDateTime::fromString(layout_modified_date, "yyyy-MM-dd HH:mm:ss"));
    if(!t.isValid())
    {
        std::cerr << "error: the date \"" << layout_modified_date << "\" doesn't seem valid in \"" << filename << "\", the expected format is \"yyyy-MM-dd HH:mm:ss\"" << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    layout_modified = t.toTime_t();
}


void snap_layout::load_css(QString const & filename, QByteArray const & content, QString & row_name)
{
    snap::snap_version::quick_find_version_in_source fv;
    if(!fv.find_version(content.data(), content.size()))
    {
        std::cerr << "error: the CSS file \"" << filename << "\" does not include a valid introducer comment." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    // valid comment, but we need to have a name which is not mandatory
    // in the find_version() function.
    if(fv.get_name().isEmpty())
    {
        std::cerr << "error: the CSS file \"" << filename << "\" does not define the Name: field. We cannot know where to save it." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    // now we force a Layout: field for CSS files defined in a layout
    row_name = fv.get_layout();
    if(row_name.isEmpty())
    {
        std::cerr << "error: the CSS file \"" << filename << "\" does not define the Layout: field. We cannot know where to save it." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snap_layout::load_js(QString const & filename, QByteArray const & content, QString & row_name)
{
    snap::snap_version::quick_find_version_in_source fv;
    if(!fv.find_version(content.data(), content.size()))
    {
        std::cerr << "error: the JS file \"" << filename << "\" does not include a valid introducer comment." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    // valid comment, but we need to have a name which is not mandatory
    // in the find_version() function.
    if(fv.get_name().isEmpty())
    {
        std::cerr << "error: the JS file \"" << filename << "\" does not define the Name: field. We cannot know where to save it." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    // now we force a Layout: field for JavaScript files defined in a layout
    row_name = fv.get_layout();
    if(row_name.isEmpty())
    {
        std::cerr << "error: the JS file \"" << filename << "\" does not define the Layout: field. We cannot know where to save it." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snap_layout::load_image( QString const & filename, QByteArray const & content, QString & row_name)
{
    row_name = filename;
    int pos(row_name.lastIndexOf('/'));
    if(pos < 0)
    {
        std::cerr << "error: the image file does not include the name of the theme." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    row_name = row_name.mid(0, pos);
    pos = row_name.lastIndexOf('/');
    if(pos >= 0)
    {
        row_name = row_name.mid(pos + 1);
    }

    snap::snap_image img;
    if(!img.get_info(content))
    {
        std::cerr << "error: \"image\" file named \"" << filename << "\" does not use a recognized image file format." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snap_layout::connect()
{
    const QString host( f_opt->get_string("host").c_str() );
    const long port( f_opt->get_long("port") );

    try
    {
        f_session->connect( host, port, !f_opt->is_defined("no-ssl") );
        if( !f_session->isConnected() )
        {
            std::cerr << "error: connecting to Cassandra failed on host='"
                << host
                << "', port="
                << port
                << "!"
                << std::endl;
            exit(1);
            snap::NOTREACHED();
        }

    }
    catch( const std::exception& ex )
    {
        std::cerr << "error: exception ["
                  << ex.what()
                  << "] caught"
                  << std::endl
                  << "  when trying to connect to host='"
                  << host
                  << "' on port="
                  << port
                  << "!"
                  << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


bool snap_layout::tableExists( const QString& table_name ) const
{
    try
    {
        const QString context_name( f_opt->get_string("context").c_str() );
        auto meta( schema::SessionMeta::create(f_session) );
        meta->loadSchema();
        const auto& tables( meta->getKeyspaces().at(context_name)->getTables() );
        return tables.find(table_name) != tables.end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "snap_layout::tableExists(): Exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    return false;
}


bool snap_layout::rowExists( QString const & table_name, QByteArray const & row_key ) const
{
    try
    {
        const QString context_name( f_opt->get_string("context").c_str() );
        auto q( Query::create(f_session) );
        q->query( QString("SELECT COUNT(*) FROM %1.%2 WHERE key = ?;")
                .arg(context_name)
                .arg(table_name)
                );
        q->bindByteArray( 0, row_key );
        q->start();
        return q->rowCount() > 0;
    }
    catch( std::exception const & ex )
    {
        std::cerr << "snap_layout::rowExists(): Exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    snap::NOTREACHED();
    return false;
}


bool snap_layout::cellExists( const QString& table_name, const QByteArray& row_key, const QByteArray& cell_key ) const
{
    try
    {
        const QString context_name( f_opt->get_string("context").c_str() );
        auto q( Query::create(f_session) );
        q->query( QString("SELECT COUNT(*) FROM %1.%2 WHERE key = ? AND column1 = ?;")
                .arg(context_name)
                .arg(table_name)
                );
        int bind = 0;
        q->bindByteArray( bind++, row_key  );
        q->bindByteArray( bind++, cell_key );
        q->start();
        return q->rowCount() > 0;
    }
    catch( const std::exception& ex )
    {
        std::cerr << "snap_layout::cellExists(): Exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    snap::NOTREACHED();
    return false;
}


void snap_layout::add_files()
{
    connect();

    const QString context_name( f_opt->get_string("context").c_str() );
    if( !tableExists("layout") )
    {
        //try
        //{
        //    const QString query_template(
        //            "CREATE TABLE IF NOT EXISTS %1.layout (\n"
        //            "  key BLOB,\n"
        //            "  column1 BLOB,\n"
        //            "  value BLOB,\n"
        //            ") WITH COMPACT STORAGE\n"
        //            "  AND bloom_filter_fp_chance = 0.01\n"
        //            "  AND caching = {'keys': 'ALL', 'rows_per_partition': 'NONE'}\n"
        //            "  AND comment = 'Table of layouts'\n"
        //            "  AND compaction = {'class': 'org.apache.cassandra.db.compaction.SizeTieredCompactionStrategy', 'max_threshold': '22', 'min_threshold': '4'}\n"
        //            "  AND compression = {'chunk_length_in_kb': '64', 'class': 'org.apache.cassandra.io.compress.LZ4Compressor'}\n"
        //            "  AND crc_check_chance = 1\n"
        //            "  AND dclocal_read_repair_chance = 0\n"
        //            "  AND default_time_to_live = 0\n"
        //            "  AND gc_grace_seconds = 864000\n"
        //            "  AND max_index_interval = 2048\n"
        //            "  AND memtable_flush_period_in_ms = 3600000\n"
        //            "  AND min_index_interval = 128\n"
        //            "  AND read_repair_chance = 0\n"
        //            "  AND speculative_retry = 'NONE'\n"
        //            "  ;"
        //            );

        //    std::cout << "Creating table layout";
        //    auto q( Query::create(f_session) );
        //    q->query( query_template.arg(context_name) );
        //    q->start(false);
        //    while( !q->isReady() )
        //    {
        //        std::cout << ".";
        //    }
        //    q->getQueryResult();
        //    q->end();
        //    std::cout << "done!" << std::endl;
        //}
        //catch( const std::exception& ex )
        //{
        //    std::cout << "error!" << std::endl;
        //    std::cerr << "Layout table creation Query exception caught! What=" << ex.what() << std::endl;
        //    exit(1);
        //    snap::NOTREACHED();
        //}
        std::cerr << "Layout table does not exist yet. Run snapcreatetables at least once on a computer running snapdbproxy." << std::endl;
        exit(1);
    }

    typedef QMap<QString, time_t> mtimes_t;
    mtimes_t mtimes;
    for( auto const & info : f_fileinfo_list )
    {
        QString const filename(info.f_filename);
        if(f_verbose)
        {
            std::cout << "info: working on \"" << filename << "\"." << std::endl;
        }
        QByteArray content(info.f_content);
        int const e(filename.lastIndexOf("."));
        if(e == -1)
        {
            std::cerr << "error: file \"" << filename << "\" must include an extension (end with .xml, .xsl, .css, .js, .png, .jpg, etc.)" << std::endl;
            exit(1);
            snap::NOTREACHED();
        }
        QString row_name; // == <layout name>
        QString cell_name; // == <layout_area>  or 'content'
        QString const extension(filename.mid(e));
        if(extension == ".xml") // expects the content.xml file
        {
            QDomDocument doc("content");
            QString error_msg;
            int error_line, error_column;
            content.push_back(' ');
            if(!doc.setContent(content, false, &error_msg, &error_line, &error_column))
            {
                std::cerr << "error: file \"" << filename << "\" parsing failed." << std::endl;
                std::cerr << "detail " << error_line << "[" << error_column << "]: " << error_msg << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
            time_t layout_modified;
            if(load_xml_info(doc, filename, row_name, layout_modified))
            {
                cell_name = "content.xml";
            }
            else
            {
                cell_name = filename;

                // remove the path, only keep the basename
                //
                int const last_slash(cell_name.lastIndexOf('/'));
                if(last_slash >= 0)
                {
                    cell_name = cell_name.mid(last_slash + 1);
                }
            }
        }
        else if(extension == ".css") // a CSS file
        {
            // the cell name is the basename
            cell_name = filename;
            int const pos(cell_name.lastIndexOf('/'));
            if(pos >= 0)
            {
                cell_name = cell_name.mid(pos + 1);
            }
            load_css(filename, content, row_name);
        }
        else if(extension == ".js") // a JavaScript file
        {
            // the cell name is the basename with the extension
            cell_name = filename;
            int const pos(cell_name.lastIndexOf('/'));
            if(pos >= 0)
            {
                cell_name = cell_name.mid(pos + 1);
            }
            load_js(filename, content, row_name);
        }
        else if(extension == ".png" || extension == ".gif"
             || extension == ".jpg" || extension == ".jpeg") // expects images
        {
            cell_name = filename;
            int const pos(cell_name.lastIndexOf('/'));
            if(pos >= 0)
            {
                cell_name = cell_name.mid(pos + 1);
            }
            load_image(filename, content, row_name);
        }
        else if(extension == ".xsl") // expects the body or theme XSLT files
        {
            QDomDocument doc("stylesheet");
            QString error_msg;
            int error_line, error_column;
            content.push_back(' ');
            if(!doc.setContent(content, true, &error_msg, &error_line, &error_column))
            {
                std::cerr << "error: file \"" << filename << "\" parsing failed." << std::endl;
                std::cerr << "detail " << error_line << "[" << error_column << "]: " << error_msg << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
            time_t layout_modified;
            load_xsl_info(doc, filename, row_name, cell_name, layout_modified);

            //if(table->exists(row_name))
            if( rowExists("layout", row_name.toUtf8()) )
            {
                // the row already exists, try getting the area
                //value existing(table->getRow(row_name)->getCell(cell_name)->getValue());
                libdbproxy::value existing;
                try
                {
                    auto q( Query::create(f_session) );
                    q->query( QString("SELECT value FROM %1.layout WHERE key = ? and column1 = ?;").arg(context_name) );
                    int bind = 0;
                    q->bindVariant( bind++, row_name  );
                    q->bindVariant( bind++, cell_name );
                    q->start();
                    if( q->nextRow() )
                    {
                        existing.setBinaryValue( q->getByteArrayColumn("value") );
                    }
                }
                catch( const std::exception& ex )
                {
                    std::cerr << "Get existing layout Query exception caught! what=" << ex.what() << std::endl;
                    continue;
                }
                if(!existing.nullValue())
                {
                    QDomDocument existing_doc("stylesheet");
                    if(!existing_doc.setContent(existing.stringValue(), true, &error_msg, &error_line, &error_column))
                    {
                        std::cerr << "warning: existing XSLT data parsing failed, it will get replaced." << std::endl;
                        std::cerr << "details: " << error_line << "[" << error_column << "]: " << error_msg << std::endl;
                        // it failed so we want to replace it with a valid XSLT document instead!
                    }
                    else
                    {
                        QString existing_layout_name;
                        QString existing_layout_area;
                        time_t existing_layout_modified;
                        load_xsl_info(existing_doc, QString("<existing XSLT data for %1>").arg(filename), existing_layout_name, existing_layout_area, existing_layout_modified);
                        // row_name == existing_layout_name && cell_name == existing_layout_area
                        // (since we found that data at that location in the database!)
                        if(layout_modified < existing_layout_modified)
                        {
                            // we refuse older versions
                            // (if necessary we could add a command line option to force such though)
                            std::cerr << "error: existing XSLT data was created more recently than the one specified on the command line: \"" << filename << "\"." << std::endl;
                            exit(1);
                            snap::NOTREACHED();
                        }
                        else if(layout_modified == existing_layout_modified)
                        {
                            // we accept the exact same date but emit a warning
                            std::cerr << "warning: existing XSLT data has the same date, replacing with content of file \"" << filename << "\"." << std::endl;
                        }
                    }
                }
            }
        }
        else
        {
            std::cerr << "error: file \"" << filename << "\" must be an XML file (end with the .xml or .xsl extension,) a CSS file (end with .css,) a JavaScript file (end with .js,) or be an image (end with .gif, .png, .jpg, .jpeg.)" << std::endl;
            exit(1);
            snap::NOTREACHED();
        }

        if(f_verbose && cell_name != filename)
        {
            std::cout << "info: saving file \"" << filename << "\" in field \"" << row_name << "." << cell_name << "\"." << std::endl;
        }

        try
        {
            auto q( Query::create(f_session) );
            q->query( QString("UPDATE %1.layout SET value = ? WHERE key = ? AND column1 = ?;").arg(context_name) );
            int bind = 0;
            q->bindByteArray( bind++, content            );
            q->bindByteArray( bind++, row_name.toUtf8()  );
            q->bindByteArray( bind++, cell_name.toUtf8() );
            q->start();
            q->end();
        }
        catch( const std::exception& ex )
        {
            std::cerr << "UPDATE layout Query exception caught! what=" << ex.what() << std::endl;
            exit(1);
            snap::NOTREACHED();
        }
        //table->getRow(row_name)->getCell(cell_name)->setValue(content);

        // set last modification time
        if( !mtimes.contains(row_name) || mtimes[row_name] < info.f_filetime )
        {
            mtimes[row_name] = info.f_filetime;
        }
    }

    const auto last_updated_name( snap::get_name(snap::name_t::SNAP_NAME_CORE_LAST_UPDATED) );
    for( mtimes_t::const_iterator i(mtimes.begin()); i != mtimes.end(); ++i )
    {
        // mtimes holds times in seconds, convert to microseconds
        const int64_t last_updated(i.value() * 1000000);
        libdbproxy::value existing_last_updated;
        try
        {
            auto q( Query::create(f_session) );
            q->query( QString("SELECT value FROM %1.layout WHERE key = ? and column1 = ?;").arg(context_name) );
            int bind = 0;
            q->bindByteArray( bind++, i.key().toUtf8()  );
            q->bindByteArray( bind++, last_updated_name );
            q->start();
            if( q->nextRow() )
            {
                existing_last_updated.setBinaryValue( q->getByteArrayColumn("value") );
            }
            q->end();
        }
        catch( const std::exception& ex )
        {
            std::cerr << "SELECT existing layout Query exception caught! what=" << ex.what() << std::endl;
            exit(1);
            snap::NOTREACHED();
        }

        try
        {
            if( existing_last_updated.nullValue() || existing_last_updated.int64Value() < last_updated )
            {
                auto q( Query::create(f_session) );
                q->query( QString("UPDATE %1.layout SET value = ? WHERE key = ? and column1 = ?;").arg(context_name) );
                int bind = 0;
                q->bindVariant  ( bind++, static_cast<qulonglong>(last_updated) );
                q->bindVariant  ( bind++, i.key()           );
                q->bindByteArray( bind++, last_updated_name );
                q->start();
                q->end();
            }
        }
        catch( const std::exception& ex )
        {
            std::cerr << "UPDATE layout Query exception caught! what=" << ex.what() << std::endl;
            exit(1);
            snap::NOTREACHED();
        }
#if 0
        value existing_last_updated(table->getRow(i.key())->getCell(snap::get_name(snap::name_t::SNAP_NAME_CORE_LAST_UPDATED))->getValue());
        if(existing_last_updated.nullValue()
        || existing_last_updated.int64Value() < last_updated)
        {
            table->getRow(i.key())->getCell(snap::get_name(snap::name_t::SNAP_NAME_CORE_LAST_UPDATED))->setValue(last_updated);
        }
#endif
    }
}


void snap_layout::set_theme()
{
    const auto arg_count( f_opt->size("--") );
    if( (arg_count != 2) && (arg_count != 3) )
    {
        std::cerr << "error: the --set-theme command expects 2 or 3 arguments." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    connect();

    //Table::pointer_t table(context->findTable("content"));
    //if(!table)
    if( !tableExists("content") )
    {
        std::cerr << "Content table not found. You must run the server once before we can setup the theme." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    QString uri         ( f_opt->get_string( "--", 0 ).c_str() );
    QString field       ( f_opt->get_string( "--", 1 ).c_str() );
    QString const theme ( (arg_count == 3)? f_opt->get_string( "--", 2 ).c_str(): QString() );

    if(!uri.endsWith("/"))
    {
        uri += "/";
    }

    if(field == "layout")
    {
        field = "layout::layout";
    }
    else if(field == "theme")
    {
        field = "layout::theme";
    }
    else
    {
        std::cerr << "the name of the field must be \"layout\" or \"theme\"." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }


    QString const key(QString("%1types/taxonomy/system/content-types").arg(uri));
    //if(!table->exists(key))
    if( !rowExists("content", key.toUtf8()) )
    {
        std::cerr << "content-types not found for domain \"" << uri << "\"." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    try
    {
        QString const context_name( f_opt->get_string("context").c_str() );

        if( theme.isEmpty() )
        {
            // remove the theme definition
            //table->getRow(key)->dropCell(field);
            auto q( Query::create(f_session) );
            q->query( QString("DELETE FROM %1.content WHERE key = ? AND column1 = ?;").arg(context_name) );
            int bind = 0;
            q->bindVariant( bind++, key   );
            q->bindVariant( bind++, field );
            q->start();
            q->end();
        }
        else
        {
            // remember that the layout specification is a JavaScript script
            // and not just plain text
            //
            // TODO: add a test so we can transform a simple string to a valid
            //       JavaScript string
            //table->getRow(key)->getCell(field)->setValue(theme);
            auto q( Query::create(f_session) );
            q->query( QString("UPDATE %1.content SET value = ? WHERE key = ? AND column1 = ?;").arg(context_name) );
            int bind = 0;
            q->bindVariant( bind++, theme );
            q->bindVariant( bind++, key   );
            q->bindVariant( bind++, field );
            q->start();
            q->end();
        }
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Theme set Query exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snap_layout::remove_theme()
{
    const auto arg_count( f_opt->size("--") );
    if( arg_count != 1 )
    {
        std::cerr << "error: the --remove-theme command expects 1 argument." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    connect();

    //Table::pointer_t table(context->findTable("layout"));
    //if(!table)
    if( !tableExists("layout") )
    {
        std::cerr << "warning: \"layout\" table not found. If you do not yet have a layout table then no theme can be deleted." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    QString const row_name( f_opt->get_string( "--", 0 ).c_str() );
    //if(!table->exists(row_name))
    if( !rowExists("layout", row_name.toUtf8()) )
    {
        std::cerr << "warning: \"" << row_name << "\" layout not found." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    //if(!table->getRow(row_name)->exists("theme"))
    if( !cellExists("layout", row_name.toUtf8(), QByteArray("theme")) )
    {
        std::cerr << "warning: it looks like the \"" << row_name << "\" layout does not exist (no \"theme\" found)." << std::endl;
    }

    // drop the entire row; however, remember that does not really delete
    // the row itself for a while (it's still visible in the database)
    //table->dropRow(row_name);
    try
    {
        const QString context_name( f_opt->get_string("context").c_str() );
        auto q( Query::create(f_session) );
        q->query( QString("DELETE FROM %1.layout WHERE key = ?;").arg(context_name) );
        int bind = 0;
        q->bindVariant( bind++, row_name );
        q->start();
        q->end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Remove theme Query exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    if(f_verbose)
    {
        std::cout << "info: theme \"" << row_name << "\" dropped." << std::endl;
    }
}


void snap_layout::extract_file()
{
    auto const arg_count( f_opt->size("--") );
    if( arg_count != 2 )
    {
        std::cerr << "error: the --extract command expects 2 arguments: layout name and filename. Got " << arg_count << " at this point." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    connect();

    //Table::pointer_t table(context->findTable("layout"));
    //if(!table)
    if( !tableExists("layout") )
    {
        std::cerr << "warning: \"layout\" table not found. If you do not yet have a layout table then no theme files can be extracted." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    QString const row_name( f_opt->get_string( "--", 0 ).c_str() );
    //if(!table->exists(row_name))
    if( !rowExists("layout", row_name.toUtf8()) )
    {
        std::cerr << "warning: \"" << row_name << "\" layout not found." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    //if(!table->getRow(row_name)->exists("theme"))
    if( !cellExists("layout", row_name.toUtf8(), QByteArray("theme")) )
    {
        std::cerr << "warning: it looks like the \"" << row_name << "\" layout does not fully exist (no \"theme\" found)." << std::endl;
        // try to continue anyway
    }

    //Row::pointer_t row(table->getRow(row_name));

    QString const filename( f_opt->get_string( "--", 1 ).c_str() );
    int const slash_pos(filename.lastIndexOf('/'));
    QString basename;
    if(slash_pos != -1)
    {
        basename = filename.mid(slash_pos + 1);
    }
    else
    {
        basename = filename;
    }
    //if(!row->exists(basename))
    if( !cellExists("layout",row_name.toUtf8(),basename.toUtf8()) )
    {
        int const extension_pos(basename.lastIndexOf('.'));
        if(extension_pos > 0)
        {
            basename = basename.mid(0, extension_pos);
        }
        //if(!row->exists(basename))
        if( !cellExists("layout",row_name.toUtf8(),basename.toUtf8()) )
        {
            std::cerr << "error: file \"" << filename << "\" does not exist in this layout." << std::endl;
            exit(1);
            snap::NOTREACHED();
        }
    }

    // TODO: if we reach here, the cell may have been dropped earlier...
    //value value(row->getCell(basename)->getValue());

    try
    {
        const QString context_name( f_opt->get_string("context").c_str() );
        auto q( Query::create(f_session) );
        q->query( QString("SELECT value FROM %1.layout WHERE key = ? and column1 = ?;").arg(context_name) );
        int bind = 0;
        q->bindByteArray( bind++, row_name.toUtf8() );
        q->bindByteArray( bind++, basename.toUtf8() );
        q->start();
        if( q->nextRow() )
        {
            QFile output(filename);
            if(!output.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                std::cerr << "error: could not create file \"" << filename << "\" to write the data." << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
            //output.write(value.binaryValue());
            output.write( q->getByteArrayColumn("value") );
        }
        q->end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Extract file Query exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    if(f_verbose)
    {
        std::cout << "info: extracted \"" << basename << "\" from theme \"" << row_name << "\" and saved the result in \"" << filename << "\"." << std::endl;
    }
}


void snap_layout::run()
{
    if( f_opt->is_defined( "set-theme" ) )
    {
        set_theme();
    }
    else if( f_opt->is_defined( "remove-theme" ) )
    {
        remove_theme();
    }
    else if( f_opt->is_defined( "extract" ) )
    {
        extract_file();
    }
    else
    {
        add_files();
    }
}




int main(int argc, char * argv[])
{
    try
    {
        snap_layout s(argc, argv);
        s.run();
        return 0;
    }
    catch(std::exception const & e)
    {
        std::cerr << "snaplayout: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
