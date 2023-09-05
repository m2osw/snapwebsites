// Copyright (c) 2013-2022  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief The anti-hammering checks the number of hits from a given IP.
 *
 * The anti-hammering plugin is used to count the number of hits from
 * any one source. If the number of hits goes beyond a predefined set
 * of thresholds, then the client gets a 503 as an answer, instead of
 * the expected 200.
 *
 * The counting must include the main HTML page and all of its attachments
 * (which can be quite a few pages since.) The HTML pages is what we
 * primary want to count, although the first hit could be to any
 * other type of data (i.e. someone who links to one of our images.)
 */


// self
//
#include    "antihammering.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace antihammering
{


SERVERPLUGINS_START(antihammering)
    , ::serverplugins::description(
            "System used to avoid hammering of our Snap! Websites."
            " The plugin counts the number of hits and blocks users who"
            " really hammers a website. The thresholds used against these"
            " counters are defined in the settings.")
    , ::serverplugins::icon("/images/antihammering/antihammering-logo-64x64.png")
    , ::serverplugins::settings_path("/admin/settings/antihammering")
    , ::serverplugins::dependency("messages")
    , ::serverplugins::dependency("path")
    , ::serverplugins::dependency("output")
    , ::serverplugins::dependency("sessions")
    , ::serverplugins::help_uri("https://snapwebsites.org/help")
    , ::serverplugins::categorization_tag("security")
    , ::serverplugins::categorization_tag("spam")
SERVERPLUGINS_END(antihammering)


/** \brief Get a fixed antihammering name.
 *
 * The antihammering plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_ANTIHAMMERING_BLOCKED:
        return "*blocked*";

    case name_t::SNAP_NAME_ANTIHAMMERING_FIRST_PAUSE:
        return "antihammering::first_pause";

    case name_t::SNAP_NAME_ANTIHAMMERING_HIT_LIMIT:
        return "antihammering::hit_limit";

    case name_t::SNAP_NAME_ANTIHAMMERING_HIT_LIMIT_DURATION:
        return "antihammering::hit_limit_duration";

    case name_t::SNAP_NAME_ANTIHAMMERING_SECOND_PAUSE:
        return "antihammering::second_pause";

    case name_t::SNAP_NAME_ANTIHAMMERING_TABLE:
        return "antihammering";

    case name_t::SNAP_NAME_ANTIHAMMERING_THIRD_PAUSE:
        return "antihammering::thrid_pause";

    default:
        // invalid index
        throw snap_logic_error("invalid name_t::SNAP_NAME_ANTIHAMMERING_...");

    }
    snapdev::NOT_REACHED();
}


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
time_t antihammering::do_update(time_t last_updated, unsigned int phase)
{
    SERVERPLUGINS_PLUGIN_UPDATE_INIT();

    if(phase == 0)
    {
        SERVERPLUGINS_PLUGIN_UPDATE(2015, 12, 27, 5, 36, 57, content_update);
    }

    SERVERPLUGINS_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our antihammering references.
 *
 * Send our antihammering to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void antihammering::content_update(int64_t variables_timestamp)
{
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the antihammering.
 *
 * This function terminates the initialization of the antihammering plugin
 * by registering for different events.
 */
void antihammering::bootstrap()
{
    SERVERPLUGINS_LISTEN(antihammering, "server", server, output_result, boost::placeholders::_1, boost::placeholders::_2);
    SERVERPLUGINS_LISTEN(antihammering, "path", path::path, check_for_redirect, boost::placeholders::_1);
}


/** \brief Initialize the content table.
 *
 * This function creates the antihammering table if it doesn't exist yet. Otherwise
 * it simple initializes the f_antihammering_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \return The pointer to the antihammering table.
 */
libdbproxy::table::pointer_t antihammering::get_antihammering_table()
{
    if(!f_antihammering_table)
    {
        f_antihammering_table = f_snap->get_table(get_name(name_t::SNAP_NAME_ANTIHAMMERING_TABLE));
    }
    return f_antihammering_table;
}


/** \brief Count the hits from the output result.
 *
 * We count the hits whenever the hit goes out, this way we actually have
 * a very interesting side effect: we get the output status (i.e. 200, 302,
 * 404, etc.) and thus can have thresholds that match each HTTP code and
 * even calculate ratios:
 *
 * \code
 *      if(count(200) / count(404) > 0.2)
 *      {
 *          // "bad robot"
 *      }
 * \endcode
 */
void antihammering::on_output_result(QString const & uri_path, QByteArray & output)
{
    snapdev::NOT_USED(output);

    // retrieve the status
    //
    // when undefined, the status is expected to be 200
    //
    int code(200);
    if(f_snap->has_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_STATUS_HEADER)))
    {
        // status is defined and thus it may not be 200
        //
        QString const code_str(f_snap->get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_STATUS_HEADER)).mid(0, 3));
        bool ok(false);
        code = code_str.toInt(&ok, 10);
        if(!ok)
        {
            // not too sure what we could do here, but it should never happen
            return;
        }
        if(code >= 300 && code <= 399)
        {
            // completely ignore redirects; to find redirect problems
            // check Apache2 logs or the tracker; here we cannot really
            // do much of anything with those
            return;
        }
    }

    // save the entry
    //
    // we save each entry separately because that way we can use Cassandra
    // to auto-delete entries using the TTL instead of having to do that
    // with a backend
    //
    content::path_info_t ipath;
    ipath.set_path(uri_path);

    libdbproxy::value value;
    value.setStringValue(ipath.get_key());
    value.setTtl(10 * 60); // 10 min. -- change to use settings?

    QString type("html-page");
    if(f_snap->has_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)))
    {
        QString const content_type(f_snap->get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
        if(content_type == "text/html"
        || content_type.startsWith("text/html;"))
        {
            type = "html-page";
        }
        else
        {
            type = "attachment";
        }
    }

    QString const key(QString("%1 %2 %3").arg(code).arg(type).arg(f_snap->get_start_date(), 19, 10, QChar('0')));

    QString const ip(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));

    libdbproxy::table::pointer_t antihammering_table(get_antihammering_table());
    antihammering_table->getRow(ip)->getCell(key)->setValue(value);
}


/** \brief Count the hits from the client's IP address.
 *
 * This signal callback is used for us to be able to cound the number
 * of access from a specific client's IP address. If too many hits
 * are received from the same IP to either the same page or too many
 * 404, then we block all accesses using the firewall.
 *
 * \param[in] ipath  The path the user is going to now.
 */
void antihammering::on_check_for_redirect(content::path_info_t & ipath)
{
    snapdev::NOT_USED(ipath);

    int64_t const start_date(f_snap->get_start_date());

    content::path_info_t antihammering_settings;
    antihammering_settings.set_path("admin/settings/antihammering");
    content::content * content(content::content::instance());
    libdbproxy::table::pointer_t revision_table(content->get_revision_table());
    libdbproxy::row::pointer_t row(revision_table->getRow(antihammering_settings.get_revision_key()));

    QString const ip(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));

    // get the total number of hits in the last little bit from this one
    // user IP address
    libdbproxy::table::pointer_t antihammering_table(get_antihammering_table());

    // already and still blocked?
    libdbproxy::value const blocked_value(antihammering_table->getRow(ip)->getCell(get_name(name_t::SNAP_NAME_ANTIHAMMERING_BLOCKED))->getValue());
    int64_t const blocked_time_limit(blocked_value.safeInt64Value(0, 0));
    if(blocked_time_limit > start_date)
    {
        int64_t next_pause(120LL);
        int stage(blocked_value.safeSignedCharValue(4, 1));
        switch(stage)
        {
        case 1:
            next_pause = row->getCell(get_name(name_t::SNAP_NAME_ANTIHAMMERING_SECOND_PAUSE))->getValue().safeInt64Value(0, 2LL * 60LL);
            ++stage;
            break;

        case 2:
            next_pause = row->getCell(get_name(name_t::SNAP_NAME_ANTIHAMMERING_THIRD_PAUSE))->getValue().safeInt64Value(0, 60LL * 60LL);
            ++stage;
            break;

        case 3:
            // this time, he asked for it, block the IP for a whole day
            snap::server::block_ip(ip, "day", "the anti-hammering plugin prevented a 3rd offense");
            next_pause = 24LL * 60LL * 60LL; // 1 day
            break;

        }

        libdbproxy::value blocked;
        QByteArray value;
        // the block is extended from what it was about plus the next limit
        int64_t const new_time_limit(blocked_time_limit + next_pause * 1000000LL);
        libdbproxy::setInt64Value(value, new_time_limit);
        libdbproxy::appendSignedCharValue(value, stage);
        blocked.setBinaryValue(value);
        int64_t const pause_ttl((new_time_limit - start_date) / 1000000LL);
        blocked.setTtl(static_cast<int>(pause_ttl) + 20); // to make sure we do not lose the information too soon, add 20 seconds to the TTL
        antihammering_table->getRow(ip)->getCell(get_name(name_t::SNAP_NAME_ANTIHAMMERING_BLOCKED))->setValue(blocked);
        f_snap->set_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_RETRY_AFTER_HEADER), QString("%1").arg(pause_ttl), f_snap->HEADER_MODE_EVERYWHERE);
        f_snap->die(snap_child::http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE,
                "Server Unavailable",
                "We received too many hits. Please pause for a while.",
                QString("Blocking user because too many hits happened, stage = %1.").arg(stage));
        snapdev::NOT_REACHED();
    }

    // count the number of 200 which are HTML pages (result starts with "200 html-page")
    //libdbproxy::QCassandraColumnRangePredicate html_page_predicate;
    //html_page_predicate.setStartCellKey("200 html-page 0");
    //html_page_predicate.setEndCellKey("200 html-page 9");
    //int const page_count(antihammering_table->getRow(ip)->cellCount(html_page_predicate));

    int64_t const hit_limit_duration(row->getCell(get_name(name_t::SNAP_NAME_ANTIHAMMERING_HIT_LIMIT_DURATION))->getValue().safeInt64Value(0, 1LL));
    auto html_page_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
    html_page_predicate->setStartCellKey(QString("200 html-page %1").arg(start_date - hit_limit_duration * 1000000LL, 19, 10, QChar('0')));
    html_page_predicate->setEndCellKey("200 html-page 9"); // up to the end
    int const page_count(antihammering_table->getRow(ip)->cellCount(html_page_predicate));
    int64_t const hit_limit(row->getCell(get_name(name_t::SNAP_NAME_ANTIHAMMERING_HIT_LIMIT))->getValue().safeInt64Value(0, 100LL));
    if(page_count >= hit_limit)
    {
        int64_t const first_pause(row->getCell(get_name(name_t::SNAP_NAME_ANTIHAMMERING_FIRST_PAUSE))->getValue().safeInt64Value(0, 60LL));
        libdbproxy::value blocked;
        QByteArray value;
        libdbproxy::setInt64Value(value, start_date + first_pause * 1000000LL);
        libdbproxy::appendSignedCharValue(value, 1);
        blocked.setBinaryValue(value);
        blocked.setTtl(static_cast<int>(first_pause) + 20); // to make sure we do not lose the information too soon, add 20 seconds to the TTL
        antihammering_table->getRow(ip)->getCell(get_name(name_t::SNAP_NAME_ANTIHAMMERING_BLOCKED))->setValue(blocked);
        f_snap->set_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_RETRY_AFTER_HEADER), QString("%1").arg(first_pause), f_snap->HEADER_MODE_EVERYWHERE);
        f_snap->die(snap_child::http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE,
                "Server Unavailable",
                "We received too many hits. Please pause for a while.",
                "Blocking user because too many hits happened.");
        snapdev::NOT_REACHED();
    }

    // then count the number of errors (result over 399)
    //auto error_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
    //error_predicate.setStartCellKey("400");
    //error_predicate.setEndCellKey(":");
    //int const error_count(antihammering_table->getRow(ip)->cellCount(error_predicate));

    // now check where we stand
    //int const total_count(antihammering_table->getRow(ip)->cellCount());
    //SNAP_LOG_INFO("total count: ")(total_count)(", page count: ")(page_count)(", error count: ")(error_count);

}


} // namespace antihammering
} // namespace snap
// vim: ts=4 sw=4 et
