// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include    "mimetype.h"


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>


// C++
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace mimetype
{


SERVERPLUGINS_START(mimetype, 1, 0)
    , ::serverplugins::description(
            "Add support detection of many file MIME types in JavaScript.")
    , ::serverplugins::dependency("output")
    , ::serverplugins::help_uri("https://snapwebsites.org/help")
    , ::serverplugins::categorization_tag("security")
    , ::serverplugins::categorization_tag("spam")
SERVERPLUGINS_END(mimetype)


namespace
{

struct mimetype_to_path
{
    char const *    f_mimetype;
    char const *    f_filename;
    char const *    f_extension;
};

mimetype_to_path const g_mimetype_to_path[] =
{
    {
        "application/pdf",
        "pdf",
        "pdf"
    },
    {
        "image/gif",
        "gif",
        "gif"
    },
    {
        "image/jpeg",
        "jpg",
        "jpg"
    },
    {
        "image/png",
        "png",
        "png"
    }
};


int find_mimetype(QString const & mime_type)
{
    QString type;
    int const pos(mime_type.indexOf(';'));
    if(pos < 0)
    {
        type = mime_type;
    }
    else
    {
        type = mime_type.mid(0, pos);
    }
    QByteArray const mt(type.toUtf8());
    size_t const max(sizeof(g_mimetype_to_path) / sizeof(g_mimetype_to_path[0]));

#ifdef DEBUG
    for(size_t idx(0); idx < max - 1; ++idx)
    {
        if(strcmp(g_mimetype_to_path[idx].f_mimetype, g_mimetype_to_path[idx + 1].f_mimetype) >= 0)
        {
            throw mimetype_exception_invalid_data(QString("The g_mimetyoe_to_path table is not properly sorted, all f_mimetype strings must be in byte order (strmp). Error found at position: %1 (\"%2\" vs \"%3\").")
                        .arg(idx)
                        .arg(g_mimetype_to_path[idx].f_mimetype)
                        .arg(g_mimetype_to_path[idx + 1].f_mimetype));
        }
    }
#endif

    size_t i(0);
    size_t j(max);
    size_t p(static_cast<size_t>(-1));
    while(i < j)
    {
        p = i + (j - i) / 2;
        int const r(strcmp(mt.data(), g_mimetype_to_path[p].f_mimetype));
        if(r == 0)
        {
            // found a match!
            return static_cast<int>(p);
        }

        if(r > 0)
        {
            // move the range up (we already checked p so use p + 1)
            i = p + 1;
        }
        else
        {
            // move the range down (we never check an item at position j)
            j = p;
        }
    }

    // anything else is unknown
    return -1;
}


} // no name namespace


/* \brief Get a fixed MIME type name.
 *
 * The MIME type plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
//char const * get_name(name_t name)
//{
//    switch(name)
//    {
//    case name_t::SNAP_NAME_MIMETYPE_ACCEPTED:
//        return "mimetype::accepted";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid name_t::SNAP_NAME_MIMETYPE_...");
//
//    }
//    snapdev::NOT_REACHED();
//}








/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
time_t mimetype::do_update(time_t last_updated, unsigned int phase)
{
    SERVERPLUGINS_PLUGIN_UPDATE_INIT();

    if(phase == 0)
    {
        SERVERPLUGINS_PLUGIN_UPDATE(2016, 3, 14, 19, 39, 30, content_update);
    }

    SERVERPLUGINS_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void mimetype::content_update(int64_t variables_timestamp)
{
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the MIME type.
 *
 * This function terminates the initialization of the MIME type plugin
 * by registering for different events.
 */
void mimetype::bootstrap()
{
}


/** \brief Transform a MIME type to the path of an icon.
 *
 * This function is used to convert a valid MIME type of a path to
 * an icon.
 *
 * \todo
 * We want to move the list to the database to allow website administrator
 * to select the icons they want for such and such MIME type and to give
 * them the ability to add new types. These types can be common to all
 * websites by adding a MIME type specific table, and it can be specific
 * to a website so that way a website can present its own MIME type icons.
 *
 * \todo
 * At some point we can change the path to point to one common resource
 * so client's browsers can cache those images once for all of
 * our websites.
 *
 * \param[in] mime_type  The MIME type to convert.
 *
 * \return The path to the image as it can be used in this website.
 */
QString mimetype::mimetype_to_icon(QString const & mime_type)
{
    QString const site_key(f_snap->get_site_key_with_slash());
    int const p(find_mimetype(mime_type));
    if(p == -1)
    {
        // anything else is unknown
        return QString("%1images/mimetype/file-unknown.png").arg(site_key);
    }
    else
    {
        // found a match!
        return QString("%1images/mimetype/file-%2.png")
                            .arg(site_key)
                            .arg(g_mimetype_to_path[p].f_filename);
    }
}



/** \brief Transform a MIME type to a file extension.
 *
 * This function is used to convert a valid MIME type of a file
 * extension.
 *
 * \param[in] mime_type  The MIME type to convert.
 *
 * \return The extension that best represents that MIME type.
 */
QString mimetype::mimetype_to_extension(QString const & mime_type)
{
    int const p(find_mimetype(mime_type));
    if(p == -1)
    {
        // anything else is unknown
        return "ext";
    }
    else
    {
        // found a match!
        return g_mimetype_to_path[p].f_extension;
    }
}



} // namespace mimetype
} // namespace snap
// vim: ts=4 sw=4 et
