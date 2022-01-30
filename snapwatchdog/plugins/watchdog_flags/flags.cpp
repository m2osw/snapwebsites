// Snap Websites Server -- Flags watchdog: check for raised flags
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "flags.h"


// snapwebsites lib
//
#include <snapwebsites/flags.h>
#include <snapwebsites/log.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/meminfo.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// last include
//
#include <snapdev/poison.h>




SNAP_PLUGIN_START(flags, 1, 0)


/** \brief Get a fixed flags plugin name.
 *
 * The flags plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_WATCHDOG_FLAGS_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_FLAGS_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize the flags plugin.
 *
 * This function is used to initialize the flags plugin object.
 */
flags::flags()
{
}


/** \brief Clean up the flags plugin.
 *
 * Ensure the flags object is clean before it is gone.
 */
flags::~flags()
{
}


/** \brief Get a pointer to the flags plugin.
 *
 * This function returns an instance pointer to the flags plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the flags plugin.
 */
flags * flags::instance()
{
    return g_plugin_flags_factory.instance();
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString flags::description() const
{
    return "Check raised flags and generate errors accordingly.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString flags::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in the watchdog.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t flags::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);
    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize flags.
 *
 * This function terminates the initialization of the flags plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void flags::bootstrap(snap_child * snap)
{
    f_snap = static_cast<watchdog_child *>(snap);

    SNAP_LISTEN(flags, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void flags::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG("flags::on_process_watch(): processing");

    // check whether we have any flags that are currently raised
    // if not, we just return ASAP
    //
    snap_flag::vector_t list(snap_flag::load_flags());

    if(list.empty())
    {
        return;
    }

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "flags"));

    // add each flag to the DOM
    //
    int priority(5);
    QString names;
    for(auto f : list)
    {
        QDomElement flag(doc.createElement("flag"));
        e.appendChild(flag);

        // basics
        //
        QString const name(QString::fromUtf8(f->get_name().c_str()));
        flag.setAttribute("unit",     QString::fromUtf8(f->get_unit().c_str()));
        flag.setAttribute("section",  QString::fromUtf8(f->get_section().c_str()));
        flag.setAttribute("name",     name);
        flag.setAttribute("priority", QString("%1").arg(f->get_priority()));

        if(!names.isEmpty())
        {
            names += ", ";
        }
        names += name;

        // manual
        //
        QDomElement manual_down_element(doc.createElement("manual-down"));
        flag.appendChild(manual_down_element);

        QDomText manual_down(doc.createTextNode(QString("%1").arg(f->get_manual_down() ? "yes" : "no")));
        manual_down_element.appendChild(manual_down);

        // date
        //
        QDomElement date_element(doc.createElement("date"));
        flag.appendChild(date_element);

        QDomText date(doc.createTextNode(QString("%1").arg(f->get_date())));
        date_element.appendChild(date);

        // modified
        //
        QDomElement modified_element(doc.createElement("modified"));
        flag.appendChild(modified_element);

        QDomText modified(doc.createTextNode(QString("%1").arg(f->get_modified())));
        modified_element.appendChild(modified);

        // message
        //
        QDomElement message(doc.createElement("message"));
        flag.appendChild(message);

        QDomText msg(doc.createTextNode(QString::fromUtf8(f->get_message().c_str())));
        message.appendChild(msg);

        // source
        //
        QDomElement source(doc.createElement("source"));
        flag.appendChild(source);

        source.setAttribute("source-file", QString::fromUtf8(f->get_source_file().c_str()));
        source.setAttribute("function",    QString::fromUtf8(f->get_function().c_str()));
        source.setAttribute("line",        f->get_line());

        // tags
        //
        QDomElement tags(doc.createElement("tags"));
        flag.appendChild(tags);

        snap_flag::tag_list_t const & tag_list(f->get_tags());

        for(auto t : tag_list)
        {
            QDomElement tag_element(doc.createElement("tag"));
            tags.appendChild(tag_element);

            QDomText tag(doc.createTextNode(QString::fromUtf8(t.c_str())));
            tag_element.appendChild(tag);
        }

        if(f->get_priority() > priority)
        {
            priority = f->get_priority();
        }
    }

    f_snap->append_error(
              doc
            , "flags"
            , QString("%1 flag%2 %3 raised -- %4")
                    .arg(list.size())
                    .arg(list.size() == 1 ? "" : "s")
                    .arg(list.size() == 1 ? "is" : "are")
                    .arg(names)
            , priority);
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
