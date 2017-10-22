// Snap Websites Server -- advanced handling of lists
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

// self
//
#include "list.h"

// other plugins
//
#include "../links/links.h"
#include "../path/path.h"
#include "../output/output.h"

// snapwebsites lib
//
#include <snapwebsites/chownnm.h>
#include <snapwebsites/dbutils.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/snap_backend.h>
#include <snapwebsites/snap_expr.h>
#include <snapwebsites/snap_lock.h>
#include <snapwebsites/tokenize_string.h>

// csspp lib
//
#include <csspp/csspp.h>

// Qt lib
//
#include <QtCore>
#include <QtSql>

// C++ lib
//
#include <iostream>

// C lib
//
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(list, 1, 0)


/** \brief Get a fixed list name.
 *
 * The list plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_LIST_ITEM_KEY_SCRIPT: // compiled
        return "list::item_key_script";

    case name_t::SNAP_NAME_LIST_KEY: // list of ordered pages
        return "list::key"; // + "::<list uri>" (cell includes <item sort key>)

    case name_t::SNAP_NAME_LIST_LAST_UPDATED:
        return "list::last_updated";

    case name_t::SNAP_NAME_LIST_LISTJOURNAL: // --action listjournal (and corresponding PING)
        return "listjournal";

    case name_t::SNAP_NAME_LIST_LINK: // standard link between list and list items
        return "list::link";

    case name_t::SNAP_NAME_LIST_NAME: // name for query string
        return "list::name";

    case name_t::SNAP_NAME_LIST_NAMESPACE:
        return "list";

    case name_t::SNAP_NAME_LIST_NUMBER_OF_ITEMS:
        return "list::number_of_items";

    case name_t::SNAP_NAME_LIST_ORDERED_PAGES: // list of ordered pages
        return "list::ordered_pages"; // + "::<item sort key>"

    case name_t::SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT: // text format
        return "list::original_item_key_script";

    case name_t::SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT: // text format
        return "list::original_test_script";

    case name_t::SNAP_NAME_LIST_PAGE: // query string name "...?page=..."
        return "page";

    case name_t::SNAP_NAME_LIST_PAGELIST: // --action pagelist (and corresponding PING)
        return "pagelist";

    case name_t::SNAP_NAME_LIST_PAGE_SIZE:
        return "list::page_size";

    case name_t::SNAP_NAME_LIST_PROCESSALLLISTS: // --action processalllists
        return "processalllists";

    case name_t::SNAP_NAME_LIST_PROCESSLIST: // --action processlist
        return "processlist";

    case name_t::SNAP_NAME_LIST_RESETLISTS: // --action resetlists
        return "resetlists";

    case name_t::SNAP_NAME_LIST_SELECTOR: // all, public, children, hand-picked, type=name, ...
        return "list::selector";

    case name_t::SNAP_NAME_LIST_STANDALONE: // when present in list table as a column name of a site row: signals a website managed as a standalone site
        return "*standalone*";

    case name_t::SNAP_NAME_LIST_STANDALONELIST: // --action standalonelist
        return "standalonelist";

    case name_t::SNAP_NAME_LIST_TABLE:
        return "list";

    case name_t::SNAP_NAME_LIST_TABLE_REF:
        return "listref";

    case name_t::SNAP_NAME_LIST_TAXONOMY_PATH:
        return "types/taxonomy/system/list";

    case name_t::SNAP_NAME_LIST_THEME: // filter function
        return "list::theme";

    case name_t::SNAP_NAME_LIST_TEST_SCRIPT: // compiled
        return "list::test_script";

    case name_t::SNAP_NAME_LIST_TYPE:
        return "list::type";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_LIST_...");

    }
    NOTREACHED();
}





namespace
{


namespace
{

void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}

} // no name namespace



/** \brief Timeout for our list journal data message in seconds.
 *
 * Whenever we send a message to the "pagelist" backend processing our
 * "LISTDATA" message, we expect a reply to be sent back to us very
 * quickly. This timeout defines the maximum amount of time we are
 * willing to wait for the message acknowledgement.
 *
 * At this time we wait up to 1 minute, which is probably way more
 * than necessary.
 */
list::timeout_t g_listdata_timeout = 60LL;      // seconds


/** \brief The default snapcommunicator address.
 *
 * This variable holds the snapcommunicator IP address used to send
 * our "LISTDATA" messages.
 *
 * \todo
 * It cannot currently be changed.
 */
std::string g_snapcommunicator_address = "127.0.0.1";


/** \brief The default snapcommunicator port.
 *
 * This variable holds the snapcommunicator port used to send
 * our "LISTDATA" messages.
 *
 * \todo
 * It cannot currently be changed.
 */
int g_snapcommunicator_port = 4040;


/** \brief The default snapcommunicator mode.
 *
 * This variable holds the snapcommunicator mode used to send
 * our "LISTDATA" messages.
 *
 * \todo
 * It cannot currently be changed.
 */
tcp_client_server::bio_client::mode_t g_snapcommunicator_mode = tcp_client_server::bio_client::mode_t::MODE_PLAIN;


/** \brief A unique number used to name each service.
 *
 * Each time we connect to snapcommunicator we need to have a different
 * name otherwise we would take the risk of getting shutdown from a
 * previous UNREGISTER message.
 *
 * This unique number is used for that purpose.
 */
int g_unique_service = 0;


/** \brief A unique number used to serialize each message.
 *
 * Each time we send a "LISTDATA" message, we give that message an identifier
 * which we expected to be returned in the acknowledgement. To acknowledge,
 * the "pagelist" backend will send us that identifier back.
 */
int g_unique_id = 0;


}
// no name namespace



/** \brief The implementation of a message handler for the LISTDATA messages.
 *
 * For us to handle the "LISTDATA" messages sent to "pagelist" and
 * acknowledged by it, we need a connection. We use a blocking
 * connection which is used to send one "LISTDATA", wait for the
 * acknowledgement, and then request for the next piece of data.
 *
 * The class handles the parsing of the local journal file in order
 * to be able to reuse the same connection for all the entries found
 * in the journal. Otherwise it would have to reconnect each time
 * which would slow down the entire cluster quite a bit.
 */
class listdata_connection
        : public snap_communicator::snap_tcp_blocking_client_message_connection
{
public:
                            listdata_connection(QString const & list_data_path);
                            ~listdata_connection();

    int                     did_work();

    // implementation of snap_communicator::snap_tcp_blocking_client_message_connection
    void                    process_timeout();
    void                    process_message(snap_communicator_message const & message);

private:
    void                    process_data(QString const & acknowledgement_id);
    void                    mark_done();

    std::string             f_data;
    std::string::size_type  f_start = std::string::npos;
    std::string::size_type  f_pos = std::string::npos;
    QString                 f_service_name;
    bool                    f_success = false;
    bool                    f_done = false;
    QString const           f_path;
    int64_t                 f_hour = 0;
    int64_t                 f_end_hour = 0;
    int64_t                 f_keep_hour1 = -1;
    int64_t                 f_keep_hour2 = -1;
    int                     f_fd = -1;
    std::string             f_filename;
    int                     f_did_work = 0;
};





/** \brief Initialize the LISTDATA handler.
 *
 * To handle the data we use the timeout set to "now". This calls the
 * `process_timeout()` immediately which allows us to read one more line
 * of data. If such a line exists, we send the message and setup the
 * timer for now + the "LISTDATA" message timeout.
 *
 * Once we receive the acknowledgement, we again set the timeout to "now"
 * so we can process the next message.
 *
 * \param[in] list_data_path  The path to the journal files.
 */
listdata_connection::listdata_connection(QString const & list_data_path)
    : snap_tcp_blocking_client_message_connection(g_snapcommunicator_address, g_snapcommunicator_port, g_snapcommunicator_mode)
    , f_path(list_data_path + "/" + snap::get_name(snap::name_t::SNAP_NAME_CORE_LIST_JOURNAL_PATH))
{
    f_service_name = QString("listdata_%1").arg(++g_unique_service);

    // make sure that path exists and is a directory
    //
    struct stat st;
    if(stat(f_path.toUtf8().data(), &st) != 0
    || !S_ISDIR(st.st_mode))
    {
        SNAP_LOG_ERROR("could not access list journal directory \"")(f_path)("\"");
        return;
    }

    // compute the hour from "now"
    //
    // this gives us a number between 0 and 23
    // the division removes microseconds, seconds, and minutes
    //
    // the keep hour 1 and 2 are used to prevent the deletion of
    // those files once done with them because we cannot be sure
    // whether an append is going to happen on them and if it does
    // and we delete, we would break the write
    //
    int64_t const now(snap_child::get_current_date());
    f_end_hour   = (now / (60LL * 60LL * 1000000LL)) % 24;
    f_keep_hour1 = (f_end_hour + 23) % 24;      // +23 is like -1 except it works correctly with the %24 (i.e. (f_end_hour - 1) % 24 == -1 when f_end_hour is 0, with +23, it becomes 23 as expected.)
    f_keep_hour2 = f_end_hour;
    f_hour       = (f_end_hour + 1) % 24;

    // we want to always timeout so that way we can process the next
    //
    set_timeout_date(now + g_listdata_timeout * 1000000LL);

    // need to register with snap communicator
    //
    snap::snap_communicator_message register_message;
    register_message.set_command("REGISTER");
    register_message.add_parameter("service", f_service_name);
    register_message.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_message);

    // now wait for the READY and HELP replies, send LISTDATA, and
    // either timeout or get the GOTLISTDATA message (or on failure
    // get a LISTDATAFAILED)
    //
    run();
}


/** \brief Make sure the last journal handle is closed.
 *
 * The destructor makes sure that the file description last used gets closed.
 */
listdata_connection::~listdata_connection()
{
    if(f_fd >= 0)
    {
        ::close(f_fd);
        f_fd = -1;
    }
}


/** \brief Check whether work was done.
 *
 * In general, a backend is asked to tell the main loop whether it did work
 * or not. If it did work, when it should return a non-zero value.
 *
 * The listdata_connection object is considered to have done work if the
 * object sends at least one LISTDATA message.
 *
 * \return 0 if no work was done, 1 if some work was done.
 */
int listdata_connection::did_work()
{
    return f_did_work;
}


/** \brief The "LISTDATA" was not acknowledge in time.
 *
 * This function gets called whenever the "LISTDATA" was sent and the
 * "GOTLISTDATA" was not received with the 'listdata_timeout' amount.
 *
 * Here we tell the system we are done with the that file so that way
 * the run() function returns silently (instead of throwing an error.)
 *
 * Whatever we already sent will be marked as processed in the input
 * file. The rest will still be in the file so we can process that
 * later.
 */
void listdata_connection::process_timeout()
{
    mark_done();
}


/** \brief Process messages as we receive them.
 *
 * This function is called whenever a complete message is read from
 * the snapcommunicator.
 *
 * In a perfect world, the following shows what happens message wise.
 *
 * \note
 * The REGISTER message is sent from the constructor to initiate the
 * whole process. This function starts by receiving the READY message
 * and call the process_data() function as a result.
 *
 * \msc
 *    list,snapcommunicator,pagelist;
 *
 *    list->snapcommunicator [label="REGISTER"];
 *    snapcommunicator->list [label="READY"];
 *    snapcommunicator->list [label="HELP"];
 *    list->snapcommunicator [label="COMMANDS"];
 *    list->snapcommunicator [label="LISTDATA"];
 *    snapcommunicator->pagelist [label="LISTDATA"];
 *    pagelist->snapcommunicator [label="GOTLISTDATA"];
 *    snapcommunicator->list [label="GOTLISTDATA"];
 *    ...;
 *    list->snapcommunicator [label="UNREGISTER"];
 * \endmsc
 *
 * If the LISTDATA message fails, we either timeout or receive a
 * LISTDATAFAILED message back from pagelist.
 *
 * \param[in] message  The message we just received.
 */
void listdata_connection::process_message(snap_communicator_message const & message)
{
    // This adds way too many messages! Use only to debug.
    //SNAP_LOG_TRACE("received messenger message [")(message.to_message())("]");

    QString const command(message.get_command());

    switch(command[0].unicode())
    {
    case 'G':
        if(command == "GOTLISTDATA")
        {
            // our last 'LISTDATA' worked, so we did some work!
            //
            f_did_work |= 1;

            process_data(message.get_parameter("listdata_id"));

            return;
        }
        break;

    case 'H':
        if(command == "HELP")
        {
            // snapcommunicator wants us to tell it what commands
            // we accept
            //
            snap_communicator_message commands_message;
            commands_message.set_command("COMMANDS");
            commands_message.add_parameter("list", "GOTLISTDATA,HELP,LISTDATAFAILED,QUITTING,READY,STOP,UNKNOWN");
            send_message(commands_message);

            // process one message and send it to "pagelist"
            //
            // processing one message is pretty lengthy so it has its
            // own function
            //
            process_data(QString());

            return;
        }
        break;

    case 'L':
        if(command == "LISTDATAFAILED")
        {
            // this is an error on the other end
            // here it's not really a bad error, other than the fact that
            // we'll have to try again later
            //
            SNAP_LOG_WARNING("we received the LISTDATAFAILED command while waiting for a GOTLISTDATA.");

            mark_done();
            return;
        }
        break;

    case 'Q':
        if(command == "QUITTING")
        {
            SNAP_LOG_WARNING("we received the QUITTING command while waiting for a GOTLISTDATA.");

            mark_done();
            return;
        }
        break;

    case 'R':
        if(command == "READY")
        {
            // the REGISTER worked, wait for the HELP message
            return;
        }
        break;

    case 'S':
        if(command == "STOP")
        {
            SNAP_LOG_WARNING("we received the STOP command while waiting for a GOTLISTDATA.");

            mark_done();
            return;
        }
        break;

    case 'U':
        if(command == "UNKNOWN")
        {
            // we sent a command that Snap! Communicator did not understand
            //
            SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
            return;
        }
        break;

    }

    // unknown command is reported and process goes on
    //
    {
        SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received by listdata on the connection with Snap! Communicator.");

        snap::snap_communicator_message unknown_message;
        unknown_message.set_command("UNKNOWN");
        unknown_message.add_parameter("command", command);
        send_message(unknown_message);
    }
}


/** \brief Process the next line of data.
 *
 * This function is called once after snapcommunicator acknowledge
 * our registration and then once each time a new acknowledgement
 * of our LISTDATA is received.
 *
 * The function will send the next message to the "pagelist" process.
 *
 * \param[in] acknowledgement_id  The identifier returned by GOTDATALIST.
 */
void listdata_connection::process_data(QString const & acknowledgement_id)
{
    // if f_start is not `npos` then we have to have received an
    // acknowledgement (i.e. if f_start is `npos` it is the first
    // time we are calling this function.)
    //
    // note that internally we reset the f_start variable back to
    // std::string::npos whenever we are done with one file, but
    // we never return with such
    //
    if(f_start != std::string::npos)
    {
        // make sure the acknowledgement is correct
        //
        if(acknowledgement_id.isEmpty())
        {
            SNAP_LOG_ERROR("acknowledgement_id is empty when we call process_data() again.");
            mark_done();
            return;
        }
        bool ok(false);
        if(g_unique_id != acknowledgement_id.toInt(&ok, 10)
        || !ok)
        {
            SNAP_LOG_ERROR("acknowledgement_id does not match the expected id. (")(acknowledgement_id)(" <> ")(g_unique_id)(")");
            mark_done();
            return;
        }

        std::string empty(f_pos - f_start, '\n');
        if(lseek(f_fd, f_start, SEEK_SET) != static_cast<off_t>(f_start))
        {
            SNAP_LOG_ERROR("could not seek to overwrite message.");
            mark_done();
            return;
        }
        if(::write(f_fd, empty.c_str(), empty.length()) != static_cast<ssize_t>(empty.length()))
        {
            SNAP_LOG_ERROR("could not overwrite message properly.");
            mark_done();
            return;
        }
    }
    else
    {
        if(!acknowledgement_id.isEmpty())
        {
            SNAP_LOG_ERROR("acknowledgement_id is not empty when we call process_data() for the first time.");
            mark_done();
            return;
        }
    }

    // find the next message, if there is one, and send it to the
    // "pagelist" process
    //
    // this can be really slow but since we unlocked the file while
    // we work on this data, we do not have too much to worry about
    //
    // note that each time we receive an acknowledgement message that
    // a message was properly processed by the "pagelist" process,
    // we overwrite it with '\n' characters in the original file
    //
    for(;; f_hour = (f_hour + 1) % 24)
    {
        // go around the clock for 1 whole day, if a file is empty, ignore
        // quickly, otherwise read the next message that was not yet managed
        // and send it to pagelist
        //
        // TODO: optimize by looking into a way to remember how much of the
        //       file we already sent to the pagelist backend; right now
        //       we re-read the whole file and reparse it (the parsing is
        //       very fast, the re-reading could be slow if the computer
        //       start swapping!)
        //
        if(f_start == std::string::npos)
        {
            if(f_fd >= 0)
            {
                ::close(f_fd);
                f_fd = -1;
            }

            if(f_done)
            {
                mark_done();
                f_success = true;
                return;
            }

            if(f_hour == f_end_hour)
            {
                // this is the last file, mark ourselves done
                //
                f_done = true;
            }

            f_filename = std::string(f_path.toUtf8().data()) + "/" + std::to_string(f_hour) + ".msg";
            f_fd = open(f_filename.c_str(), O_RDWR);
            if(f_fd < 0)
            {
                // the ENOENT is an expected error here, totally ignore it
                //
                if(errno != ENOENT)
                {
                    SNAP_LOG_DEBUG("could not open file \"")(f_filename)("\" for reading");
                }
                continue;
            }
            // just in case, do a chown()/chgrp()
            //
            if(chownnm(QString::fromUtf8(f_filename.c_str()), "snapwebsites", "snapwebsites") != 0)
            {
                SNAP_LOG_WARNING("could not properly change the ownership to snapwebsites:snapwebsites");
            }

            if(flock(f_fd, LOCK_EX) != 0)
            {
                SNAP_LOG_WARNING("could not lock file \"")(f_filename)("\" before appending message");
                continue;
            }

            ssize_t const l(lseek(f_fd, 0, SEEK_END));
            if(l == -1)
            {
                SNAP_LOG_WARNING("could not seek to the end of the file \"")(f_filename)("\"");
                continue;
            }

            // we can lose the lock because the other processes just do an append
            // so we do not need any more protection here (we needed it to
            // determine the file size, that's all!)
            //
            // i.e. unlocking as quickly as possible is best because that way we
            //      can very quickly let other processes write to this file
            //      again
            //
            if(flock(f_fd, LOCK_UN) != 0)
            {
                SNAP_LOG_INFO("could not unlock file \"")(f_filename)("\" after reading message");
            }

            if(lseek(f_fd, 0, SEEK_SET) == -1)
            {
                SNAP_LOG_WARNING("could not seek back the beginning of the file \"")(f_filename)("\"");
                continue;
            }

            // read as much as the size was while the file was locked
            //
            f_data.resize(l);
            if(::read(f_fd, &f_data[0], l) != static_cast<ssize_t>(l))
            {
                SNAP_LOG_ERROR("could not read file \"")(f_filename)("\"");
                continue;
            }

            // find the first character which is not a '\n'
            // (i.e. the start of a message)
            //
            f_pos = f_data.find_first_not_of('\n');
        }

        for(;;)
        {
            f_start = f_pos;

            // f_pos points to the start of the next message
            //
            f_pos = f_data.find_first_of('\n', f_start);
            if(f_pos == std::string::npos)
            {
                // no more '\n' found, so we assume we are done with this
                // file
                //
                f_start = std::string::npos;

                // done with the file so close it
                //
                ::close(f_fd);
                f_fd = -1;

                // if not a file we are supposed to keep around, unlink
                //
                if(f_hour != f_keep_hour1
                && f_hour != f_keep_hour2)
                {
                    unlink(f_filename.c_str());
                }
                break;
            }

            std::string uri;
            std::string priority;
            std::string key_start_date;

            // retrieve a copy of the message without the '\n'
            //
            std::string const message(f_data.substr(f_start, f_pos - f_start));
            std::vector<std::string> variables;
            tokenize_string(variables, message, ";", true, " ");
            for(size_t idx(0); idx < variables.size(); ++idx)
            {
                auto const equal(variables[idx].find_first_of('='));
                auto const varname(variables[idx].substr(0, equal));
                auto const value(variables[idx].substr(equal + 1));
                if(varname == "uri")
                {
                    uri = value;
                }
                else if(varname == "priority")
                {
                    priority = value;
                }
                else if(varname == "key_start_date")
                {
                    key_start_date = value;
                }
            }

            // skip the '\n'
            //
            ++f_pos;

            if(uri.empty()
            || priority.empty()
            || key_start_date.empty())
            {
                SNAP_LOG_WARNING("required message parameter is missing");

                // at this time we ignore such messages and go on
                // which means we go and read the next message and
                // try to process it as normal
                f_start = std::string::npos;
            }
            else
            {
                // message is valid, send it
                //
                // do not let snapcommunicator cache those messages, we will
                // resend them as required (to be sure they get there, because
                // snapcommunicator caches are in memory only!)
                //
                snap::snap_communicator_message listdata_message;
                listdata_message.set_command("LISTDATA");
                listdata_message.set_service("snaplistd");
                listdata_message.add_parameter("service", f_service_name);
                listdata_message.add_parameter("version", snap::snap_communicator::VERSION);
                listdata_message.add_parameter("uri", uri);
                listdata_message.add_parameter("priority", priority);
                listdata_message.add_parameter("key_start_date", key_start_date);
                listdata_message.add_parameter("listdata_id", ++g_unique_id);
                listdata_message.add_parameter("cache", "no");
                send_message(listdata_message);

                // next message timeout date
                // (i.e. we give each message the same amount of time to timeout)
                //
                set_timeout_date(snap_child::get_current_date() + g_listdata_timeout * 1000000LL);

                // we sent a message, return and wait until we get the
                // acknowledgement
                //
                return;
            }
        }
    }
}


/** \brief Mark that we are done.
 *
 * This function marks this connection as done. This means it will
 * exit the run() loop on return from one of the callback functions.
 *
 * The function also sends the UNREGISTER message to the other side
 * so that way we cleanly disconnect from the snapcommunicator.
 */
void listdata_connection::mark_done()
{
    snap_communicator_message unregister_message;
    unregister_message.set_command("UNREGISTER");
    unregister_message.add_parameter("service", f_service_name);
    send_message(unregister_message);

    snap_tcp_blocking_client_message_connection::mark_done();
}






/** \brief Initializes an object to access a list with paging capability.
 *
 * This function initializes this paging object with defaults.
 *
 * The \p ipath parameter is the page that represent a Snap list. It
 * will be read later when you call the read_list() function.
 *
 * \param[in,out] snap  Pointer to the snap_child object.
 * \param[in,out] ipath  The path to the page representing a list.
 *
 * \sa get_query_string_info()
 */
paging_t::paging_t(snap_child * snap, content::path_info_t & ipath)
    : f_snap(snap)
    , f_ipath(ipath)
    //, f_retrieved_list_name(false) -- auto-init
    //, f_list_name("") -- auto-init
    //, f_maximum_number_of_items(-1) -- auto-init
    //, f_number_of_items(-1) -- auto-init
    //, f_start_offset(-1) -- auto-init
    //, f_page(1) -- auto-init
    //, f_page_size(-1) -- auto-init
    //, f_default_page_size(-1) -- auto-init
{
}


/** \brief Read the current page of this list.
 *
 * This function calls the list read_list() function with the parameters
 * as defined in this paging object.
 *
 * \return The list of items as read using the list plugin.
 *
 * \sa get_start_offset()
 * \sa get_page_size()
 */
list_item_vector_t paging_t::read_list()
{
    int count(get_page_size());
    if(f_maximum_number_of_items > 0 && count > f_maximum_number_of_items)
    {
        count = f_maximum_number_of_items;
    }
    return list::list::instance()->read_list(f_ipath, get_start_offset() - 1, count);
}


/** \brief Retrieve the name of the list.
 *
 * This function returns the name of this paging object. This is the
 * name used to retrieve the current information about the list position
 * from the query string.
 *
 * The name is retrieved from the database using the referenced page.
 * It is valid to not define a name. Without a name, the simple "page"
 * query string variable is used. A name is important if the page is
 * to appear in another which also represents a list.
 *
 * \note
 * The name is cached so calling this function more than once is fast.
 *
 * \return The name of the list.
 */
QString paging_t::get_list_name() const
{
    if(!f_retrieved_list_name)
    {
        f_retrieved_list_name = true;

        content::content *content_plugin(content::content::instance());
        libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
        f_list_name = branch_table->getRow(f_ipath.get_branch_key())->getCell(get_name(name_t::SNAP_NAME_LIST_NAME))->getValue().stringValue();
    }
    return f_list_name;
}


/** \brief Set a maximum number of items to gather.
 *
 * This function defines the maximum number of items one wants to show
 * in a list being paged. This value shadows the total number of items
 * defined in the list if that total number is larger.
 *
 * This is particularly useful to control the length a list so it does
 * not go out of hands. For example, if you create one page per day, you
 * may want to show a list of up to 30 entries (nearly one month) instead
 * of all the entries that have been created from the beginning of time.
 *
 * By default this value is set to -1 which means it has no effect. You
 * may call this function with -1 as well.
 *
 * \param[in] maximum_number_of_items  The new maximum number of items.
 */
void paging_t::set_maximum_number_of_items(int32_t maximum_number_of_items)
{
    if(maximum_number_of_items < 1)
    {
        // make sure that turning this feature off is done using exactly -1
        f_maximum_number_of_items = -1;
    }
    else
    {
        f_maximum_number_of_items = maximum_number_of_items;
    }
}


/** \brief Get the current maximum number of items.
 *
 * This function returns the current maximum number of items. By default
 * this value is set to -1 which means the number of items is not going
 * to get clamped.
 *
 * \return The current maximum number of items.
 */
int32_t paging_t::get_maximum_number_of_items() const
{
    return f_maximum_number_of_items;
}


/** \brief Retrieve the total number of items in a list.
 *
 * This function retrieves the total number of items found in a list.
 * This value is defined in the database under the name
 * name_t::SNAP_NAME_LIST_NUMBER_OF_ITEMS.
 *
 * \note
 * This function always returns a positive number or zero.
 *
 * \note
 * The number is cached so this function can be called any number of
 * times.
 *
 * \warning
 * This is not the number of pages. Use the get_total_pages() to
 * determine the total number of pages available in a list.
 *
 * \warning
 * The exact number of items cannot currently be retrieved. This
 * function is clamped to the maximum number of items as defined
 * by set_maximum_number_of_items()
 *
 * \return The number of items in the list.
 */
int32_t paging_t::get_number_of_items() const
{
    if(f_number_of_items < 0)
    {
        // if the number of items is not (yet) defined in the database
        // then it will be set to zero
        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
        f_number_of_items = branch_table->getRow(f_ipath.get_branch_key())->getCell(get_name(name_t::SNAP_NAME_LIST_NUMBER_OF_ITEMS))->getValue().safeInt32Value();
    }

    // the total count may have been limited by the programmer
    if(f_maximum_number_of_items == -1
    || f_number_of_items < f_maximum_number_of_items)
    {
        return f_number_of_items;
    }

    return f_maximum_number_of_items;
}


/** \brief Define the start offset to use with read_list().
 *
 * This function is used to define the start offset. By default this
 * value is set to -1 meaning that the start page parameter is used
 * instead. This is useful in case you want to show items at any
 * offset instead of an exact page multiple.
 *
 * You make set the parameter back to -1 to ignore it.
 *
 * If the offset is larger than the total number of items present in
 * the list, the read_list() will return an empty list. You may test
 * the limit using the get_number_of_items() function. This function
 * does not prevent you from using an offsets larger than the
 * number of available items.
 *
 * \warning
 * The first item offset is 1, not 0 as generally expected in C/C++.
 *
 * \param[in] start_offset  The offset at which to start showing the list.
 */
void paging_t::set_start_offset(int32_t start_offset)
{
    // any invalid number, convert to -1 (ignore)
    if(start_offset < 1)
    {
        f_start_offset = -1;
    }
    else
    {
        f_start_offset = start_offset;
    }
}


/** \brief Retrieve the start offset.
 *
 * This function returns the start offset. This represents the number
 * of the first item to return to the caller of the read_list() function.
 * The offset may point to an item after the last item in which case the
 * read_list() function will return an empty list of items.
 *
 * If the start offset is not defined (is -1) then this function calculates
 * the start offset using the start page information:
 *
 * \code
 *      return (f_page - 1) * get_page_size() + 1;
 * \endcode
 *
 * Note that since f_page can be set to a number larger than the maximum
 * number of pages, the offset returned in that situation may also be
 * larger than the total number of items present in the list.
 *
 * \note
 * The function returns one for the first item (and NOT zero as generally
 * expected in C/C++).
 *
 * \warning
 * There is no way to retrieve the f_start_offset value directly.
 *
 * \return The start offset.
 */
int32_t paging_t::get_start_offset() const
{
    int const offset(f_start_offset < 1 ? 1 : static_cast<int>(f_start_offset));
    return offset + (f_page - 1) * get_page_size();
}


/** \brief Retrieve the query string page information.
 *
 * This function reads the query string page information and saves
 * it in this paging object.
 *
 * The query string name is defined as:
 *
 * \code
 *      page
 *   or
 *      page-<list_name>
 * \endcode
 *
 * If the list name is empty or undefined, then the name of the query
 * string variable is simply "page". If the name is defined, then the
 * system adds a dash and the name of the list.
 *
 * The value of the query string is generally just the page number.
 * The number is expected to be between 1 and the total number of
 * pages available in this list. The number 1 is not required as it
 * is the default.
 *
 * Multiple numbers can be specified by separating them with commas
 * and preceeding them with a letter as follow:
 *
 * \li 'p' -- page number, the 'p' is always optional
 * \li 'o' -- start offset, an item number, ignores the page number
 * \li 's' -- page size, the number of items per page
 *
 * For example, to show page 3 of a list named blog with 300 items,
 * showing 50 items per page, you can use:
 *
 * \code
 *      page-blog=3,s50
 *   or
 *      page-blog=p3,s50
 * \endcode
 *
 * \sa get_page()
 * \sa get_page_size()
 * \sa get_start_offset()
 */
void paging_t::process_query_string_info()
{
    // define the query string variable name
    QString const list_name(get_list_name());
    QString variable_name(get_name(name_t::SNAP_NAME_LIST_PAGE));
    if(!list_name.isEmpty())
    {
        variable_name += "-";
        variable_name += list_name;
    }

    // check whether such a variable exists in the query string
    if(!f_snap->get_uri().has_query_option(variable_name))
    {
        return;
    }

    // got such, retrieve it
    QString const variable(f_snap->get_uri().query_option(variable_name));
    snap_string_list const params(variable.split(","));
    bool defined_page(false);
    bool defined_size(false);
    bool defined_offset(false);
    for(int idx(0); idx < params.size(); ++idx)
    {
        QString const p(params[idx]);
        if(p.isEmpty())
        {
            continue;
        }
        switch(p[0].unicode())
        {
        case 'p':   // explicit page number
            if(!defined_page)
            {
                defined_page = true;
                bool ok(false);
                int const page(p.mid(1).toInt(&ok));
                if(ok && page > 0)
                {
                    f_page = page;
                }
            }
            break;

        case 's':   // page size (number of items per page)
            if(!defined_size)
            {
                defined_size = true;
                bool ok(false);
                int const size = p.mid(1).toInt(&ok);
                if(ok && size > 0 && size <= list::LIST_MAXIMUM_ITEMS)
                {
                    f_page_size = size;
                }
            }
            break;

        case 'o':   // start offset (specific number of items)
            if(!defined_offset)
            {
                defined_offset = true;
                bool ok(false);
                int offset = p.mid(1).toInt(&ok);
                if(ok && offset > 0)
                {
                    f_start_offset = offset;
                }
            }
            break;

        case '0': // the page number (like "p123")
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(!defined_page)
            {
                defined_page = true;
                bool ok(false);
                int page = p.toInt(&ok);
                if(ok && page > 0)
                {
                    f_page = page;
                }
            }
            break;

        }
    }
}


/** \brief Generate the query string representing this paging information.
 *
 * This function is used to generate a link to a page as defined by this
 * paging information.
 *
 * The \p page_offset parameter is expected to be zero (0) for a link
 * to the current page. It is expected to be negative to go to a previous
 * page and positive to go to a following page.
 *
 * \param[in] page_offset  The offset to the page to generate a query string for.
 *
 * \return The query string variable and value for the specified page.
 */
QString paging_t::generate_query_string_info(int32_t page_offset) const
{
    QString result(get_name(name_t::SNAP_NAME_LIST_PAGE));
    QString const list_name(get_list_name());
    if(!list_name.isEmpty())
    {
        result += "-";
        result += list_name;
    }
    result += "=";

    int32_t const page_size(get_page_size());

    bool need_comma(false);
    if(f_start_offset > 1)
    {
        // keep using the offset if defined
        int32_t offset(f_start_offset + page_offset * page_size);
        if(offset <= 0)
        {
            offset = 1;
        }
        else if(offset > get_number_of_items())
        {
            offset = get_number_of_items();
        }
        result += QString("o%1").arg(offset);
        need_comma = true;
    }
    else
    {
        int32_t page(f_page + page_offset);
        int32_t const max_pages(get_total_pages());
        if(page > max_pages && max_pages != -1)
        {
            // maximum limit
            page = max_pages;
        }
        if(page < 1)
        {
            // minimum limit
            page = 1;
        }

        if(page != f_page)
        {
            // use the page only if no offset specified
            // also we do not need to specify page=1 since that is the default
            result += QString("%1").arg(page);
            need_comma = true;
        }
    }

    if(page_size != f_default_page_size)
    {
        if(need_comma)
        {
            result += "%2C";
        }
        result += QString("s%1").arg(page_size);
        need_comma = true;
    }

    if(!need_comma)
    {
        // page 1 with default size, add nothing to the query string
        return QString();
    }

    return result;
}


/** \brief Generate the query string to access the first page.
 *
 * This function calculates the query string to send the user to the
 * first page of this list. The first page is often represented by an
 * empty query string so this function may return such when the offset
 * was not specified and no specific page size was defined.
 *
 * \return The query string to send the user to the last page.
 */
QString paging_t::generate_query_string_info_for_first_page() const
{
    if(f_start_offset > 0)
    {
        int32_t const page_size(get_page_size());
        return generate_query_string_info((1 - f_start_offset + page_size - 1) / page_size);
    }

    return generate_query_string_info(1 - f_page);
}


/** \brief Generate the query string to access the last page.
 *
 * This function calculates the query string to send the user to the
 * last page of this list. The last page may be the first page in
 * which case the function may return an empty string.
 *
 * \return The query string to send the user to the last page.
 */
QString paging_t::generate_query_string_info_for_last_page() const
{
    int32_t const max_pages(get_total_pages());
    if(max_pages == -1)
    {
        // this also represents the very first page with the default
        // page size... but without a valid max_pages, what can we do
        // really?
        return QString();
    }

    if(f_start_offset > 0)
    {
        int32_t const page_size(get_page_size());
        return generate_query_string_info((get_number_of_items() - f_start_offset + page_size - 1) / page_size);
    }

    return generate_query_string_info(max_pages - f_page);
}


/** \brief Generate a set of anchors for navigation purposes.
 *
 * This function generates the navigation anchors used to let the
 * end user move between pages quickly.
 *
 * \todo
 * The next / previous anchors make use of characters that the end
 * user should be able to change (since we have access to the list
 * we can define them in the database.)
 *
 * \param[in,out] element  A QDomElement object where we add the navigation
 *                         elements.
 * \param[in] uri  The URI used to generate the next/previous, pages 1, 2, 3...
 * \param[in] next_previous_count  The number of anchors before and after
 *                                 the current page.
 * \param[in] next_previous  Whether to add a next and previous set of anchors.
 * \param[in] first_last  Whether to add a first and last set of anchors.
 * \param[in] next_previous_page  Whether to add a ... for next and previous
 *                                pages.
 */
void paging_t::generate_list_navigation(QDomElement element, snap_uri uri, int32_t next_previous_count, bool const next_previous, bool const first_last, bool const next_previous_page) const
{
    if(element.isNull())
    {
        return;
    }

    // no navigation necessary if the number of items is limited and
    // that limit is smaller or equal to the size of one page
    if((f_maximum_number_of_items != -1 && f_maximum_number_of_items <= f_page_size)
    || get_number_of_items() <= f_page_size)
    {
        return;
    }

    QDomDocument doc(element.ownerDocument());
    QDomElement ul(doc.createElement("ul"));

    // add a root tag to encompass all the other tags
    QString list_name(get_list_name());
    if(!list_name.isEmpty())
    {
        list_name = " " + list_name;
    }
    ul.setAttribute("class", "list-navigation" + list_name);
    element.appendChild(ul);

    // generate the URIs in before/after the current page
    int32_t first(0);
    int32_t last(0);
    int32_t current_index(0);
    snap_string_list qs;
    QString const current_page_query_string(generate_query_string_info(0));
    qs.push_back(current_page_query_string);
    for(int32_t i(-1); i >= -next_previous_count; --i)
    {
        QString const query_string(generate_query_string_info(i));
        if(qs.first() == query_string)
        {
            break;
        }
        if(i < first)
        {
            first = i;
        }
        qs.push_front(query_string);
    }
    current_index = qs.size() - 1;
    for(int32_t i(1); i <= next_previous_count; ++i)
    {
        QString const query_string(generate_query_string_info(i));
        if(qs.last() == query_string)
        {
            break;
        }
        if(i > last)
        {
            last = i;
        }
        qs.push_back(query_string);
    }

    // add the first anchor only if we are not on the first page
    if(first_last && first < 0)
    {
        // add the first button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "list-navigation-first");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info_for_first_page());
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x21E4))));
        anchor.appendChild(text);
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    // add the previous anchor only if we are not on the first page
    if(next_previous && first < 0)
    {
        // add the previous button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "list-navigation-previous");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info(-1));
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x2190))));
        anchor.appendChild(text);
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    if(next_previous_page && first < 0)
    {
        QString const query_string(generate_query_string_info(-1 - next_previous_count));
        if(qs.first() != query_string)
        {
            // add the previous page button
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "list-navigation-previous-page");
            ul.appendChild(li);

            snap_uri anchor_uri(uri);
            anchor_uri.set_query_string(generate_query_string_info(-1 - next_previous_count));
            QDomElement anchor(doc.createElement("a"));
            QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x2026))));
            anchor.appendChild(text);
            anchor.setAttribute("href", "?" + anchor_uri.query_string());
            li.appendChild(anchor);
        }
    }

    // add the navigation links now
    int32_t const max_qs(qs.size());
    for(int32_t i(0); i < max_qs; ++i)
    {
        QString query_string(qs[i]);
        if(i == current_index)
        {
            // the current page (not an anchor)
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "list-navigation-current");
            ul.appendChild(li);
            QDomText text(doc.createTextNode(QString("%1").arg(f_page)));
            li.appendChild(text);
        }
        else if(i < current_index)
        {
            // a previous anchor
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "list-navigation-preceeding-page");
            ul.appendChild(li);

            snap_uri anchor_uri(uri);
            anchor_uri.set_query_string(query_string);
            QDomElement anchor(doc.createElement("a"));
            QDomText text(doc.createTextNode(QString("%1").arg(f_page + i - current_index)));
            anchor.appendChild(text);
            anchor.setAttribute("href", "?" + anchor_uri.query_string());
            li.appendChild(anchor);
        }
        else
        {
            // a next anchor
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "list-navigation-following-page");
            ul.appendChild(li);

            snap_uri anchor_uri(uri);
            anchor_uri.set_query_string(query_string);
            QDomElement anchor(doc.createElement("a"));
            QDomText text(doc.createTextNode(QString("%1").arg(f_page + i - current_index)));
            anchor.appendChild(text);
            anchor.setAttribute("href", "?" + anchor_uri.query_string());
            li.appendChild(anchor);
        }
    }

    if(next_previous_page && last > 0)
    {
        QString const query_string(generate_query_string_info(next_previous_count + 1));
        if(qs.last() != query_string)
        {
            // add the previous page button
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "list-navigation-previous-page");
            ul.appendChild(li);

            snap_uri anchor_uri(uri);
            anchor_uri.set_query_string(generate_query_string_info(next_previous_count + 1));
            QDomElement anchor(doc.createElement("a"));
            QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x2026))));
            anchor.appendChild(text);
            anchor.setAttribute("href", "?" + anchor_uri.query_string());
            li.appendChild(anchor);
        }
    }

    // add the previous anchor only if we are not on the first page
    if(next_previous && last > 0)
    {
        // add the previous button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "list-navigation-next");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info(1));
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x2192))));
        anchor.appendChild(text);
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    // add the last anchor only if we are not on the last page
    if(first_last && last > 0)
    {
        // add the last button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "list-navigation-last");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info_for_last_page());
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x21E5))));
        anchor.appendChild(text);
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    QDomElement div_clear(doc.createElement("div"));
    div_clear.setAttribute("class", "div-clear");
    element.appendChild(div_clear);
}


/** \brief Define the page with which the list shall start.
 *
 * This function defines the start page you want to read with the read_list()
 * function. By default this is set to 1 to represent the very first page.
 *
 * This parameter must be at least 1. If larger than the total number of
 * pages available, then the read_list() will return an empty list.
 *
 * \param[in] page  The page to read with read_list().
 */
void paging_t::set_page(int32_t page)
{
    // make sure this is at least 1
    f_page = std::max(1, page);
}


/** \brief Retrieve the start page.
 *
 * This function retrieves the page number that is to be read by the
 * read_list() function. The first page is represented with 1 and not
 * 0 as normally expected by C/C++.
 *
 * \note
 * The page number returned here will always be 1 or more.
 *
 * \return The start page.
 */
int32_t paging_t::get_page() const
{
    return f_page;
}


/** \brief Calculate the next page number.
 *
 * This function calculates the page number to use to reach the next
 * page. If the current page is the last page, then this function
 * returns -1 meaning that there is no next page.
 *
 * \warning
 * The function returns -1 if the total number of pages is not
 * yet known. That number is known only after you called The
 * read_list() at least once.
 *
 * \return The next page or -1 if there is no next page.
 */
int32_t paging_t::get_next_page() const
{
    int32_t const max_pages(get_total_pages());
    if(f_page >= max_pages || max_pages == -1)
    {
        return -1;
    }
    return f_page + 1;
}


/** \brief Calculate the previous page number.
 *
 * This function calculates the page number to use to reach the
 * previous page. If the current page is the first page, then this
 * function returns -1 meaning that there is no previous page.
 *
 * \return The previous page or -1 if there is no previous page.
 */
int32_t paging_t::get_previous_page() const
{
    if(f_page <= 1)
    {
        return -1;
    }

    return f_page - 1;
}


/** \brief Calculate the total number of pages.
 *
 * This function calculates the total number of pages available in
 * a list. This requires the total number of items available and
 * thus it is known only after the read_list() function was called
 * at least once.
 *
 * Note that a list may be empty. In that case the function returns
 * zero (no pages available.)
 *
 * \return The total number of pages available.
 */
int32_t paging_t::get_total_pages() const
{
    int32_t const page_size(get_page_size());
    return (get_number_of_items() + page_size - f_start_offset) / page_size;
}


/** \brief Set the size of a page.
 *
 * Set the number of items to be presented in a page.
 *
 * The default list paging mechanism only supports a constant
 * number of items per page.
 *
 * By default the number of items in a page is defined using the
 * database name_t::SNAP_NAME_LIST_PAGE_SIZE from the branch table. This
 * function can be used to force the size of a page and ignore
 * the size defined in the database.
 *
 * \param[in] page_size  The number of items per page for that list.
 *
 * \sa get_page_size()
 */
void paging_t::set_page_size(int32_t page_size)
{
    f_page_size = std::max(1, page_size);
}


/** \brief Retrieve the number of items per page.
 *
 * This function returns the number of items defined in a page.
 *
 * By default the function reads the size of a page for a given list
 * by reading the size from the database. This way it is easy for the
 * website owner to change that size.
 *
 * If the size is not defined in the database, then the DEFAULT_PAGE_SIZE
 * value is used (20 at the time of writing.)
 *
 * If you prefer to enforce a certain size for your list, you may call
 * the set_page_size() function. This way the data will not be hit.
 *
 * \return The number of items defined in one page.
 *
 * \sa set_page_size()
 */
int32_t paging_t::get_page_size() const
{
    if(f_default_page_size < 1)
    {
        content::content *content_plugin(content::content::instance());
        libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
        f_default_page_size = branch_table->getRow(f_ipath.get_branch_key())->getCell(get_name(name_t::SNAP_NAME_LIST_PAGE_SIZE))->getValue().safeInt32Value();
        if(f_default_page_size < 1)
        {
            // not defined in the database, bump it to 20
            f_default_page_size = DEFAULT_PAGE_SIZE;
        }
    }

    if(f_page_size < 1)
    {
        f_page_size = f_default_page_size;
    }

    return f_page_size;
}





/** \class list
 * \brief The list plugin to handle list of pages.
 *
 * The list plugin makes use of many references and links and thus it
 * is documented here:
 *
 *
 * 1) Pages that represent lists are all categorized under the following
 *    system content type:
 *
 * \code
 *     /types/taxonomy/system/list
 * \endcode
 *
 * We use that list to find all the lists defined on a website so we can
 * manage them all in our loops.
 *
 *
 *
 * 2) Items are linked to their list so that way when deleting an item
 *    we can immediately remove that item from that list. Note that an
 *    item may be part of many lists so it is a "multi" on both sides
 *    ("*:*").
 *
 *
 * 3) The list page includes links to all the items that are part of
 *    the list. These links do not use the standard link capability
 *    because the items are expected to be ordered and that is done
 *    using the Cassandra sort capability, in other words, we need
 *    to have a key which includes the sort parameters (i.e. an index).
 *
 * \code
 *    list::items::<sort key>
 * \endcode
 *
 * Important Note: This special link is double linked too, that is, the
 * item page links back to the standard list too (more precisly, it knows
 * of the special ordered key used in the list.) This is important to
 * make sure we can manage lists properly. That is, if the expression
 * used to calculate the key changes, then we could not instantly find
 * the old key anymore (i.e. we'd have to check each item in the list
 * to find the one that points to a given item... in a list with 1 million
 * pages, it would be really slow.)
 *
 *
 * Recap:
 *
 * \li Standard Link: List Page \<-\> /types/taxonomy/system/list
 * \li Standard Link: List Page \<-\> Item Page
 * \li Ordered List: List Page -\> Item Page,
 *                   Item Page includes key used in List Page
 *
 * \note
 * We do not repair list links when a page is cloned. If the clone is
 * to be part of a list the links will be updated accordingly. This
 * means if you do not write specialized code to make sure the clone
 * is a list, the "list::type" link is missing and thus no checks
 * are done to update the list data of the clone which by default will
 * be empty (inexistant may be a better way to describe this one.)
 */




/** \fn list::list_modified(content::path_info_t & ipath)
 * \brief Signal that a list was modified.
 *
 * In some cases you want to immediately be alerted of a change in a list.
 * The functions that modify lists (add or remove elements from lists)
 * end by calling this signal. The parameter is the path to the list that
 * changed.
 *
 * Lists that are newly created get all their elements added and once
 * and then the list_modified() function gets called.
 *
 * Lists that get many pages added at once, but are not new, will get
 * this signal called once per element added or removed.
 *
 * \note
 * Remember that although you are running in a backend, it is timed
 * and lists should not take more than 10 seconds to all be worked on
 * before another website gets a chance to be worked on. It is more
 * polite to do the work you need to do quickly or memorize what needs
 * to be done and do it in your backend process instead of the pagelist
 * process if it is to take a quite long time to finish up.
 *
 * \param[in] ipath  The path to the list that was just updated (added/removed
 *                   an item in that list.)
 */




/** \brief Initialize the list plugin.
 *
 * This function is used to initialize the list plugin object.
 */
list::list()
    //: f_snap(nullptr) -- auto-init
    //, f_backend(nullptr) -- auto-init
    //, f_list_table(nullptr) -- auto-init
    //, f_listref_table(nullptr) -- auto-init
    //, f_check_expressions() -- auto-init
    //, f_item_key_expressions() -- auto-init
    //, f_ping_backend(false) -- auto-init
    //, f_list_link(false) -- auto-init
    //, f_priority(LIST_PRIORITY_NEW_PAGE) -- auto-init
    //, f_start_date_offset(LIST_PROCESSING_LATENCY) -- auto-init
    //, f_date_limit(0) -- auto-init
{
}


/** \brief Clean up the list plugin.
 *
 * Ensure the list object is clean before it is gone.
 */
list::~list()
{
}


/** \brief Get a pointer to the list plugin.
 *
 * This function returns an instance pointer to the list plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the list plugin.
 */
list * list::instance()
{
    return g_plugin_list_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString list::icon() const
{
    return "/images/list/list-logo-64x64.png";
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
QString list::description() const
{
    return "Generate lists of pages using a set of parameters as defined"
          " by the system (some lists are defined internally) and the end"
          " users.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString list::dependencies() const
{
    return "|filter|layout|links|messages|output|";
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
int64_t list::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 1, 16, 21, 10, 30, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void list::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the list.
 *
 * This function terminates the initialization of the list plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void list::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(list, "server", server, attach_to_session);
    SNAP_LISTEN(list, "server", server, register_backend_cron, _1);
    SNAP_LISTEN(list, "server", server, register_backend_action, _1);
    SNAP_LISTEN(list, "content", content::content, create_content, _1, _2, _3);
    SNAP_LISTEN(list, "content", content::content, modified_content, _1);
    SNAP_LISTEN(list, "content", content::content, copy_branch_cells, _1, _2, _3);
    SNAP_LISTEN(list, "links", links::links, modified_link, _1, _2);
    SNAP_LISTEN(list, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(list, "filter", filter::filter, token_help, _1);

    //SNAP_TEST_PLUGIN_SUITE_LISTEN(list);
}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the \<page\> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void list::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Signal that a page was created.
 *
 * This function is called whenever the content plugin creates a new page.
 * At that point the page may not yet be complete so we could not handle
 * the possible list updates.
 *
 * So instead the function saves the full key to the page that was just
 * created so lists that include this page can be updated by the backend
 * as required.
 *
 * \param[in,out] ipath  The path to the page being modified.
 * \param[in] owner  The plugin owner of the page.
 * \param[in] type  The type of the page.
 */
void list::on_create_content(content::path_info_t & ipath, QString const & owner, QString const & type)
{
    NOTUSED(owner);
    NOTUSED(type);

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());

    // if a list is defined in this content, make sure to mark the
    // row as having a list with the last updated data set to zero
    //
    // Note: the exists() call is going to be very fast since the data will
    //       be in memory if true (if false, we still send a network request
    //       to Cassandra... but you never know in case the cache was reset!)
    //       this is going to be faster than such a test in the backend loop
    //       and replacing that with the test of the last update is going to
    //       make it a lot faster overall.
    QString const branch_key(ipath.get_branch_key());
    if(branch_table->getRow(branch_key)->exists(get_name(name_t::SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT)))
    {
        // zero marks the list as brand new so we use a different
        // algorithm to check the data in that case (i.e. the list of
        // rows in the list table is NOT complete!)
        QString const key(ipath.get_branch_key());
        int64_t const zero(0);
        branch_table->getRow(key)->getCell(get_name(name_t::SNAP_NAME_LIST_LAST_UPDATED))->setValue(zero);
    }

    on_modified_content(ipath); // then it is the same as on_modified_content()
}


/** \brief Signal that a page was modified by a new link.
 *
 * This function is called whenever the links plugin modifies a page by
 * adding a link or removing a link. By now the page should be quite
 * complete, outside of other links still missing.
 *
 * \warning
 * As a limitation, a list script that checks the links of another list
 * will likely not update properly. This is because this function will
 * no mark a page as modified when the link being created is a link
 * from the list to a page that the list includes.
 *
 * \param[in] link  The link that was just created or deleted.
 * \param[in] created  Whether the link was created (true) or deleted (false).
 */
void list::on_modified_link(links::link_info const & link, bool const created)
{
    NOTUSED(created);

    // no need to record the fact that we added a link in a list
    // (that is, at this point a list script cannot depend on the
    // links of another list...)
    if(!f_list_link)
    {
        content::path_info_t ipath;
        ipath.set_path(link.key());
        on_modified_content(ipath); // same as on_modified_content()
    }
}


/** \brief Signal that a page was modified.
 *
 * This function is called whenever a plugin modified a page and then called
 * the modified_content() signal of the content plugin.
 *
 * The function needs to save the information so the pagelist backend has
 * a chance to process that modified page.
 *
 * The key used to handle this information includes the following 4 parameters:
 *
 * \li The protocol + website complete domain name
 *
 * The "protocol + website complete domain name"
 * (such as "http://snapwebsites.org/") is used to aggregate the data changes
 * on a per website basis. This is important for the backend processing which
 * happens on one website at a time.
 *
 * \li The current priority
 *
 * The priority is used to handle entries with a lower priority first.
 *
 * The backend is responsible for the final sorting and removal of duplicates.
 * Here we just append data to a journal and let a backend process send the
 * data to the pagelist process. We want to lists to work so we want the
 * pagelist to acknowledge the fact that it received our requests (and save
 * them in its own journals) so we use a backend. If the pagelist is down
 * a the time we process many lists, it will cummulate on a computer or
 * another but the data won't be lost.
 *
 * \li The start date + start date offset
 *
 * The time defined by "start date + start date offset" is used to make sure
 * that this page is handled on or after that time (too soon and the page
 * may not yet be ready!)
 *
 * This date is also used by the sorting algorithm.
 *
 * \li The ipath URL
 *
 * The ipath URL represents the page to be updated.
 *
 * This parameter is also used in the sorting algorithm, but in a different
 * way: if two requests are made with the same URL, then only the one with
 * the largest date (start date + start date offset) is kept. The others
 * are dropped.
 *
 * \param[in,out] ipath  The path to the page being modified.
 */
void list::on_modified_content(content::path_info_t & ipath)
{
    // there are times when you may want to debug your code to know which
    // pages are marked as modified; this debug log will help with that
    //
    SNAP_LOG_DEBUG("list detected that page \"")(ipath.get_key())("\" got modified.");

    // if the same page is modified multiple times then we overwrite the
    // same entry multiple times
    content::content * content_plugin(content::content::instance());

    QByteArray key;

    int64_t const start_date(f_snap->get_start_date());
    int64_t key_start_date(start_date + f_start_date_offset);
    priority_t priority(f_priority);

    // content cannot access list information so we have to change the
    // priority for it...
    if(content_plugin->is_updating())
    {
        // +1h is quite arbitrary, but we assume that a website may have
        // a few lists that may require the entire setup to be complete
        key_start_date += 60LL * 60LL * 1000000LL;

        priority = LIST_PRIORITY_UPDATES;
    }

    // get a copy of the path to the list journal
    //
    std::string const path((f_snap->get_list_data_path() + "/" + snap::get_name(snap::name_t::SNAP_NAME_CORE_LIST_JOURNAL_PATH)).toUtf8().data());

    // make sure that path exists and is a directory
    //
    struct stat st;
    if(stat(path.c_str(), &st) != 0
    || !S_ISDIR(st.st_mode))
    {
        SNAP_LOG_ERROR("could not access list journal directory \"")(path)("\"");
        return;
    }

    // compute the hour from start date
    // this gives us a number between 0 and 23
    // the division removes microseconds, seconds, and minutes
    //
    int64_t const hour(start_date / (60LL * 60LL * 1000000LL) % 24);

    // build the string we'll send to the pagelist backend
    // (i.e. the backend running on each system reads that string and
    // sends it in a message using snapcommunicator)
    //
    // the order is not important, although we put the URI last in case
    // it were to include a semicolon (;).
    //
    QString canonicalized_key(ipath.get_key());
    canonicalized_key.replace(";", "%3A");
    std::string const list_item(
                  "priority=" + std::to_string(static_cast<int>(priority))
                + ";key_start_date=" + std::to_string(key_start_date)
                + ";uri=" + canonicalized_key.toUtf8().data()
                + "\n");

    std::string journal_filename(std::string(path.c_str()) + "/" + std::to_string(hour) + ".msg");
    int fd(open(journal_filename.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR));
    if(fd < 0)
    {
        SNAP_LOG_ERROR("could not open file \"")(journal_filename)("\" for writing");
        return;
    }

    // create a block so fd gets closed ASAP (since we have a lock on it,
    // it is best this way)
    {
        std::shared_ptr<int> safe_fd(&fd, file_descriptor_deleter);

        if(flock(fd, LOCK_EX) != 0)
        {
            SNAP_LOG_ERROR("could not lock file \"")(journal_filename)("\" before appending message");
            return;
        }

        if(write(fd, list_item.c_str(), list_item.length()) != static_cast<ssize_t>(list_item.length()))
        {
            SNAP_LOG_FATAL("could not write to file \"")(journal_filename)("\", list manager may be hosed now");
            return;
        }
    }

    // move that to the backend!
    //
    // just in case the row changed, we delete the pre-compiled (cached)
    // scripts (this could certainly be optimized but really the scripts
    // are compiled so quickly that it won't matter.)
    //
    libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
    QString const branch_key(ipath.get_branch_key());
    branch_table->getRow(branch_key)->dropCell(get_name(name_t::SNAP_NAME_LIST_TEST_SCRIPT)); // was using start_date instead of "now"...
    branch_table->getRow(branch_key)->dropCell(get_name(name_t::SNAP_NAME_LIST_ITEM_KEY_SCRIPT)); // was using start_date instead of "now"...

    f_ping_backend = true;
}


/** \brief Capture this event which happens last.
 *
 * \note
 * We may want to create another "real" end of session message?
 *
 * \todo
 * The on_attach_to_session() does NOT get called when we are running
 * a backend. We probably want two additional signals: "before execute"
 * and "after execute" (names are still TBD). Then this event would be
 * changed to the "after execute" event.
 *
 * \bug
 * There is a 10 seconds latency between the last hit and the time when
 * the list data is taken in account (see LIST_PROCESSING_LATENCY).
 * At this point I am not too sure how we can handle this problem
 * although I added a 10 seconds pause in the code receiving a PING which
 * seems to help quite a bit.
 */
void list::on_attach_to_session()
{
    if(f_ping_backend)
    {
        // send a PING to the journal backend
        //
        f_snap->udp_ping(get_name(name_t::SNAP_NAME_LIST_LISTJOURNAL));
    }
}


/** \brief Change the priority.
 *
 * This function saves a new priority to use on any future pages being
 * created or modified (see the on_modified_content() function.)
 *
 * The priority should NOT be changed by directly calling this function.
 * Instead, you want to use the safe_priority_t object on your stack.
 * For example, when the user is trying to reset a page in a list,
 * the processing function uses:
 *
 * \code
 *     {
 *         safe_priority_t safe_priority(LIST_PRIORITY_RESET);
 *         on_modified_content(ipath);
 *     }
 * \endcode
 *
 * The use of the { ... } makes sure that the priority gets modified only
 * for that specific call to on_modified_content().
 *
 * \param[in] priority  The new priority to use for any future addition
 *                      of pages to the list table.
 */
void list::set_priority(priority_t priority)
{
    f_priority = priority;
}


/** \brief Retrieve the current list priority.
 *
 * This function returns the priority that the list is to used to and
 * any pages to its list of pages to be processed.
 *
 * \return The current priority.
 */
list::priority_t list::get_priority() const
{
    return f_priority;
}


/** \brief Change the start date offset to increase latency.
 *
 * The offset is defined in microseconds. It defines the amount of time
 * it takes before the list plugin is allowed to process that page. By
 * default is is set to LIST_PROCESSING_LATENCY, which at time of writing
 * is 10 seconds.
 *
 * In most cases you do not need to change this value. However, if you
 * are working with a special plugin that needs to create many pages,
 * especially permissions to change who has access to those pages, then
 * the process may take more or around the default 10 seconds. In that
 * case, you want to change the start date offset with a (much) larger
 * amount.
 *
 * You should never call this function directly. Instead look into
 * using the RAII class safe_start_date_offset, which will automatically
 * restore the default offset once you are done.
 *
 * \code
 *      {
 *          // set your my_new_offset value to the amount in microseconds
 *          // you want the list plugin to wait before processing your
 *          // new content
 *          //
 *          list::safe_start_date_offset saved_offset(my_new_offset);
 *
 *          content::content::instance()->create_content(...);
 *      }
 * \endcode
 *
 * \note
 * The minimum value of offset_us is LIST_PROCESSING_LATENCY.
 * We also clamp to a maximum of 24h.
 *
 * \param[in] offset_us  The offset to add to the start date of items
 *                       added to the list.
 */
void list::set_start_date_offset(int64_t offset_us)
{
    if(offset_us < LIST_PROCESSING_LATENCY)
    {
        f_start_date_offset = LIST_PROCESSING_LATENCY;
    }
    else if(offset_us > 24LL * 60LL * 60LL * 1000000LL)
    {
        f_start_date_offset = 24LL * 60LL * 60LL * 1000000LL;
    }
    else
    {
        f_start_date_offset = offset_us;
    }
}


/** \brief Retrieve the start date offset.
 *
 * By default, the act of creating or modifying a page is registered for
 * immediate processing by the list plugin.
 *
 * There are cases, however, where an item is created and needs some time
 * before getting 100% ready. This offset defines how long the list plugin
 * should wait.
 *
 * The default wait is LIST_PROCESSING_LATENCY, which at time of writing is
 * 10 seconds.
 *
 * \return The offset to add to the start date when registering a page for
 *         reprocessing after modification, in microseconds.
 */
int64_t list::get_start_date_offset() const
{
    return f_start_date_offset;
}


/** \brief Read a set of URIs from a list.
 *
 * This function reads a set of URIs from the list specified by \p ipath.
 *
 * The first item returned is defined by \p start. It is inclusive and the
 * very first item is number 0.
 *
 * The maximum number of items returned is defined by \p count. The number
 * may be set of -1 to returned as many items as there is available starting
 * from \p start. However, the function limits all returns to 10,000 items
 * so if the returned list is exactly 10,000 items, it is not unlikely that
 * you did not get all the items after the \p start point.
 *
 * The items are sorted by key as done by Cassandra.
 *
 * The count parameter cannot be set to zero. The function throws if you do
 * that.
 *
 * \todo
 * Note that at this point this function reads ALL item item from 0 to start
 * and throw them away. Later we'll add sub-indexes that will allow us to
 * reach any item very quickly. The sub-index will be something like this:
 *
 * \code
 *     list::index::100 = <key of the 100th item>
 *     list::index::200 = <key of the 200th item>
 *     ...
 * \endcode
 *
 * That way we can go to item 230 be starting the list scan at the 200th item.
 * We read the list::index:200 and us that key to start reading the list
 * (i.e. in the column predicate would use that key as the start key.)
 *
 * When a list name is specified, the \em page query string is checked for
 * a parameter that starts with that name, followed by a dash and a number.
 * Multiple lists can exist on a web page, and each list may be at a
 * different page. In this way, each list can define a different page
 * number, you only have to make sure that all the lists that can appear
 * on a page have a different name.
 *
 * The syntax of the query string for pages is as follow:
 *
 * \code
 *      page-<name>=<number>
 * \endcode
 *
 * \exception snap_logic_exception
 * The function raises the snap_logic_exception exception if the start or
 * count values are incompatible. The start parameter must be positive or
 * zero. The count value must be position (larger than 0) or -1 to use
 * the system maximum allowed.
 *
 * \param[in,out] ipath  The path to the list to be read.
 * \param[in] start  The first item to be returned (must be 0 or larger).
 * \param[in] count  The number of items to return (-1 for the maximum allowed).
 *
 * \return The list of items
 */
list_item_vector_t list::read_list(content::path_info_t & ipath, int start, int count)
{
    list_item_vector_t result;

    if(count == -1 || count > LIST_MAXIMUM_ITEMS)
    {
        count = LIST_MAXIMUM_ITEMS;
    }
    if(start < 0 || count <= 0)
    {
        throw snap_logic_exception(QString("list::read_list(ipath, %1, %2) called with invalid start and/or count values...")
                    .arg(start).arg(count));
    }

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());

    QString const branch_key(ipath.get_branch_key());
    libdbproxy::row::pointer_t list_row(branch_table->getRow(branch_key));
    list_row->clearCache();

    char const * ordered_pages(get_name(name_t::SNAP_NAME_LIST_ORDERED_PAGES));
    int const len(static_cast<int>(strlen(ordered_pages) + 2));

    auto column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
    column_predicate->setStartCellKey(QString("%1::").arg(ordered_pages));
    column_predicate->setEndCellKey(QString("%1;").arg(ordered_pages));
    column_predicate->setCount(std::min(100, count)); // optimize the number of cells transferred
    column_predicate->setIndex(); // behave like an index
    for(;;)
    {
        // clear the cache before reading the next load
        list_row->readCells(column_predicate);
        libdbproxy::cells const cells(list_row->getCells());
        if(cells.empty())
        {
            // all columns read
            break;
        }
        for(libdbproxy::cells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
        {
            if(start > 0)
            {
                --start;
            }
            else
            {
                // we keep the sort key in the item
                list_item_t item;
                item.set_sort_key(cell_iterator.key().mid(len));
                item.set_uri(cell_iterator.value()->getValue().stringValue());
                result.push_back(item);
                if(result.size() == count)
                {
                    // we got the count we wanted, return now
                    return result;
                }
            }
        }
    }

    return result;
}


/** \brief Register the pagelist action.
 *
 * This function registers this plugin CRON action named pagelist.
 *
 * \li listjournal
 *
 * The "listjournal" is used by the backend system to continuously
 * manage the list journal on front end computers (at least computers
 * that end up running the list plugin--it could be middle end computers,
 * wherever snapserver runs and starts snap_child processes.)
 *
 * \warning
 * This backend works against all the domains accessed on this computer.
 * It won't returned until done, also.
 *
 * \li pagelist
 *
 * The "pagelist" is used by the backend to continuously and as fast as
 * possible build and update lists of pages.
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void list::on_register_backend_cron(server::backend_action_set & actions)
{
    actions.add_action(get_name(name_t::SNAP_NAME_LIST_LISTJOURNAL), this);
    actions.add_action(get_name(name_t::SNAP_NAME_LIST_PAGELIST), this);
}


/** \brief Register the various list actions.
 *
 * This function registers this plugin as supporting the following
 * one time actions:
 *
 * \li list::processalllists
 * \li list::processlist
 * \li list::resetlists
 *
 * The "processalllist" adds all the pages of a website to the 'list'
 * table. This will force the system to re-check every single page.
 * In this case, the pages are give a really low priority which means
 * pretty much all other requests will be worked on first. This is
 * similar to running "list::resetlists" except that it does not
 * recompute lists in one go.
 *
 * \code
 * snapbackend http://example.com/ --action list::processalllists
 * \endcode
 *
 * The "processlist" expects a URL parameter set to the page to be
 * checked, in other words, the URL of a page for which we want to
 * simulate a change to. This is useful to get the system to re-build
 * lists that may include that page as soon as possible. That being said,
 * it appends it to the existing list of pages to be processed and that
 * list could be (very) long so it may still take a moment before it
 * gets processed. That being said, it will get processed way sooner than
 * without doing such. The URL may just include the path.
 *
 * \code
 * snapbackend http://example.com/ --action list::processlist -p URL=journal/201508
 * \endcode
 *
 * The "list::resetlists" goes through the pages marked as lists and delete
 * the existing list scripts (but not the content of the lists.) This
 * will force the list process to recalculate the entire list instead
 * of just a few changes.
 *
 * \code
 * snapbackend http://example.com/ --action list::resetlists
 * \endcode
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void list::on_register_backend_action(server::backend_action_set & actions)
{
    actions.add_action(get_name(name_t::SNAP_NAME_LIST_PROCESSALLLISTS), this);
    actions.add_action(get_name(name_t::SNAP_NAME_LIST_PROCESSLIST),     this);
    actions.add_action(get_name(name_t::SNAP_NAME_LIST_RESETLISTS),      this);
}


/** \brief Start the page list server.
 *
 * When running the backend the user can ask to run the pagelist
 * server (--cron-action list::pagelist). This function captures those
 * events. It loops until stopped with a STOP message via the UDP
 * address/port. Note that Ctrl-C will not work because it does not
 * support killing both: the parent and child processes (we do a
 * fork() to create this child.)
 *
 * The loop updates all the lists as required, then it returns.
 * The snap_backend object will call us back in 5 minutes or when
 * a PING is received.
 *
 * The pagelist backend can be stopped with the snapsignal tool
 * using the STOP event as follow:
 *
 * \code
 * snapsignal pagelist/STOP
 * \endcode
 *
 * However, if you are running snapinit, you want to STOP snapinit
 * instead:
 *
 * \code
 * snapinit stop
 * \endcode
 *
 * \param[in] action  The action this function is being called with.
 */
void list::on_backend_action(QString const & action)
{
    if(action == get_name(name_t::SNAP_NAME_LIST_LISTJOURNAL))
    {
        f_backend = dynamic_cast<snap_backend *>(f_snap);
        if(!f_backend)
        {
            throw list_exception_no_backend("list::on_backend_action(): could not determine the snap_backend pointer for the listjournal action");
        }

        // if we did some work, we want to restart our process again
        // as soon as possible (although we give other websites a chance
        // to also get their lists up to date)
        //
        int const did_work(send_data_to_journal());
        if(did_work != 0)
        {
            // now it's the PAGELIST's turn, wake it up ASAP since we did some work
            //
            f_snap->udp_ping(get_name(name_t::SNAP_NAME_LIST_PAGELIST));
        }
    }
    else if(action == get_name(name_t::SNAP_NAME_LIST_PAGELIST))
    {
        f_backend = dynamic_cast<snap_backend *>(f_snap);
        if(!f_backend)
        {
            throw list_exception_no_backend("list::on_backend_action(): could not determine the snap_backend pointer for the pagelist action");
        }

        // by default the date limit is 'now + 5 minutes'
        //
        f_date_limit = f_snap->get_start_date() + 5LL * 60LL * 1000000LL;

        // if we did some work, we want to restart our process again
        // as soon as possible (although we give other websites a chance
        // to also get their lists up to date)
        //
        QString const site_key(f_snap->get_site_key_with_slash());
        int const did_work(generate_new_lists(site_key)
                         | generate_all_lists(site_key));

        // Calculate when we want to be awaken again and transmit that
        // information to the backend process via the database
        //
        int64_t date_limit(f_date_limit);
        if(date_limit < f_snap->get_current_date()
        || did_work != 0)
        {
            date_limit = f_snap->get_start_date();
        }
        else if(date_limit > f_snap->get_start_date() + 5LL * 60LL * 1000000LL)
        {
            // wait at most 5 min. from the start date
            //
            // note that should never happen since we start with
            // "now + 5min." in f_date_limit and only reduce that
            // value in the loops below
            //
            date_limit = f_snap->get_start_date() + 5LL * 60LL * 1000000LL;
        }

        f_backend->add_uri_for_processing(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_LIST_NAMESPACE)).arg(action), date_limit, site_key);
    }
    else if(action == get_name(name_t::SNAP_NAME_LIST_PROCESSLIST))
    {
        QString const url(f_snap->get_server_parameter("URL"));
        content::path_info_t ipath;
        ipath.set_path(url);
        on_modified_content(ipath);
        f_snap->udp_ping(get_name(name_t::SNAP_NAME_LIST_LISTJOURNAL));
    }
    else if(action == get_name(name_t::SNAP_NAME_LIST_PROCESSALLLISTS))
    {
        // re-add all the pages back to the list table; this is very similar
        // to the "resetlists", only instead of reseting the lists themselves,
        // we "process" all the pages that may go in those lists
        //
        add_all_pages_to_list_table(f_snap->get_site_key_with_slash());
        f_snap->udp_ping(get_name(name_t::SNAP_NAME_LIST_LISTJOURNAL));
    }
    else if(action == get_name(name_t::SNAP_NAME_LIST_RESETLISTS))
    {
        // go through all the lists and delete the compiled script, this
        // will force the list code to regenerate all the lists; this
        // should be useful only when the code changes in such a way
        // that the current lists may not be 100% correct as they are
        //
        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());

        content::path_info_t ipath;
        QString const site_key(f_snap->get_site_key_with_slash());
        ipath.set_path(site_key + get_name(name_t::SNAP_NAME_LIST_TAXONOMY_PATH));
        links::link_info info(get_name(name_t::SNAP_NAME_LIST_TYPE), false, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info child_info;
        while(link_ctxt->next_link(child_info))
        {
            QString const key(child_info.key());
            content::path_info_t list_ipath;
            list_ipath.set_path(key);
            branch_table->getRow(list_ipath.get_branch_key())->dropCell(get_name(name_t::SNAP_NAME_LIST_TEST_SCRIPT)); // was using start_date instead of "now"...
            branch_table->getRow(list_ipath.get_branch_key())->dropCell(get_name(name_t::SNAP_NAME_LIST_ITEM_KEY_SCRIPT)); // was using start_date instead of "now"...
        }
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        //
        throw snap_logic_exception(QString("list.cpp:on_backend_action(): list::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


/** \brief Add all website pages to the list table.
 *
 * This function is used once in a while whenever no other data is being
 * processed by the list backend processes. It adds all the pages of
 * a website back to the list table so they all get re-processed.
 *
 * Since these are added with a very slow priority, they are likely to
 * be ignored for a while if important work appears in the list.
 *
 * \param[in] site_key  The key of the site to add for review in the list of
 *                      pages to check with the list backend processes.
 */
void list::add_all_pages_to_list_table(QString const & site_key)
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    content_table->clearCache();

    safe_priority_t safe_priority(LIST_PRIORITY_REVIEW);

    // TODO: use the '*index*' row which is sorted

    auto row_predicate = std::make_shared<libdbproxy::row_predicate>();
    row_predicate->setCount(1000);
    for(;;)
    {
        uint32_t const count(content_table->readRows(row_predicate));
        if(count == 0)
        {
            // no more pages to process
            break;
        }
        libdbproxy::rows const rows(content_table->getRows());
        for(libdbproxy::rows::const_iterator o(rows.begin());
                o != rows.end(); ++o)
        {
            QString key(QString::fromUtf8(o.key().data()));
            if(key.startsWith(site_key))
            {
                content::path_info_t page_ipath;
                page_ipath.set_path(o.key());
                on_modified_content(page_ipath);
            }

            // The STOP is problematic because we would stop mid-way meaning that
            // some pages could end up being processed over and over whereas
            // others not at all... to support a stop in this loop we would
            // need to know where we stopped to start again at that position
            // next time (only rows are not sorted so we cannot really do
            // that...)
            //
            // // quickly end this process if the user requested a stop
            // if(f_backend->stop_received())
            // {
            //     // clean STOP
            //     // we have to exit otherwise we'd get called again with
            //     // the next website!?
            //     exit(0);
            // }
        }
    }
}



/** \brief Implementation of the backend process signal.
 *
 * This function captures the backend processing signal which is sent
 * by the server whenever the backend tool is run against a cluster.
 *
 * The list plugin refreshes lists of pages on websites when it receives
 * this signal assuming that the website has the parameter PROCESS_LIST
 * defined.
 *
 * This backend may end up taking a lot of processing time and may need to
 * run very quickly (i.e. within seconds when a new page is created or a
 * page is modified). For this reason we also offer an action which supports
 * the PING signal.
 *
 * This backend process will actually NOT run if the PROCESS_LISTS parameter
 * is not defined as a site parameter. With the command line:
 *
 * \code
 * snapbackend [--config snapserver.conf] --param PROCESS_LISTS=1
 * \endcode
 *
 * At this time the value used with PROCESS_LIST is not tested, however, it
 * is strongly recommended you use 1.
 *
 * It is also important to mark the list as a standalone list to avoid
 * parallelism which is NOT checked by the backend at this point (because
 * otherwise you take the risk of losing the list updating process
 * altogether.) So you want to run this command once:
 *
 * \code
 * snapbackend [--config snapserver.conf] --action standalonelist http://my-website.com/
 * \endcode
 *
 * Make sure to specify the URI of your website because otherwise all the
 * sites will be marked as standalone sites!
 *
 * Note that if you create a standalone site, then you have to either
 * allow its processing with the PROCESS_LISTS parameter, or you have
 * to start it with the pagelist and its URI:
 *
 * \code
 * snapbackend [--config snapserver.conf] --action pagelist http://my-website.com/
 * \endcode
 */
void list::on_backend_process()
{
    SNAP_LOG_TRACE("backend_process: update specialized lists.");

    // only process if the user clearly specified that we should do so;
    // we should never run in parallel with a background backend, hence
    // this flag (see the on_backend_action() function)
    QString const process_lists(f_snap->get_server_parameter("PROCESS_LISTS"));
    if(!process_lists.isEmpty())
    {
        // we ignore the result in this case, the backend will
        // run again soon and take care of the additional data
        // accordingly (with the action we process as much as
        // possible all in one go)
        generate_new_lists(f_snap->get_site_key_with_slash());
        generate_all_lists(f_snap->get_site_key_with_slash());
    }
}


/** \brief Send data to the pagelist backend for later processing.
 *
 * This function check for some data in the list journal, if any is
 * found, it sends a message to the pagelist backend via
 * snapcommunicator and wait for a reply. A valid reply means
 * that the backend received the data and it is safe to delete
 * it from this computer.
 *
 * \todo
 * If you are running "pagelist" on more than one backend, then we
 * need to find a way where the data is sent to only one of these
 * and not each one of them so we do not process every list item
 * that many times! At some point we could look into having a
 * load balancer and send data to a specific computer because its
 * load is pretty low. (or at least foreseeable load since it may
 * not yet be working but may already have 100 entries in its
 * database.) So... the current version is going to work with a
 * single pagelist backend for now.
 */
int list::send_data_to_journal()
{
    // because we want to connect to snapcommunicator only once, the
    // whole loop going through all the data files is done in the
    // listdata_connection object
    //
    listdata_connection connection(f_snap->get_list_data_path());

    return connection.did_work();
}


/** \brief This function regenerates new lists for this websites.
 *
 * When creating a list for the first time, it is empty and yet it may
 * need to include all sorts of pages which are not in the "new pages"
 * table.
 *
 * This function goes through all the pages that this list expects and
 * checks whether those pages are part of the list. The function is
 * optimized by the fact that the list defines a selector. For example
 * the "children" selector means that only direct children of the
 * list are to be checked. This is most often used to build a tree like
 * set of pages (however, not only those because otherwise all lists
 * that are not listing children would need to be terminal!)
 *
 * The available selectors are:
 *
 * \li all -- all the pages of this site
 * \li children -- direct children of the list itself
 * \li children=path -- direct children of the specified specified path
 * \li descendants -- children, children of children, etc. of the list itself
 * \li descendants=path -- descendants starting at the specified path
 * \li public -- use the list of public pages (a shortcut for
 *               type=types/taxonomy/system/content-types/page/public
 * \li type=cpath -- pages of that the specified type as a canonicalized path
 * \li hand-picked=path-list -- a hand defined list of paths that represent
 *                              the pages to put in the list, the cpaths are
 *                              separated by new-line (\n) characters
 *
 * \param[in] site_key  The site we want to process.
 *
 * \return 1 if the function changed anything, 0 otherwise
 */
int list::generate_new_lists(QString const & site_key)
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());

    int did_work(0);

    QVector<QString> lists_to_work_on;

    content::path_info_t ipath;
    ipath.set_path(site_key + get_name(name_t::SNAP_NAME_LIST_TAXONOMY_PATH));
    links::link_info info(get_name(name_t::SNAP_NAME_LIST_TYPE), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        QString const key(child_info.key());
        content::path_info_t list_ipath;
        list_ipath.set_path(key);
        libdbproxy::value last_updated(branch_table->getRow(list_ipath.get_branch_key())->getCell(get_name(name_t::SNAP_NAME_LIST_LAST_UPDATED))->getValue());
        if(last_updated.nullValue()
        || last_updated.int64Value() == 0)
        {
            lists_to_work_on.push_back(list_ipath.get_key());
        }
    }

    for(auto const & it : lists_to_work_on)
    {
        content::path_info_t list_ipath;
        list_ipath.set_path(it);

        // IMPORTANT NOTE: We may see this message many times for a brand
        //                 new list; this happens when no items are ready
        //                 to be added so the list continues to look like
        //                 it is brand new... (i.e. list::last_updated is
        //                 not getting set to anything)
        //
        SNAP_LOG_TRACE("list plugin working on new list \"")(list_ipath.get_key())("\"");

        libdbproxy::row::pointer_t list_row(branch_table->getRow(list_ipath.get_branch_key()));
        QString const selector(list_row->getCell(get_name(name_t::SNAP_NAME_LIST_SELECTOR))->getValue().stringValue());

        int did_work_on_list(0);

        if(selector == "children")
        {
            did_work_on_list |= generate_new_list_for_children(site_key, list_ipath);
        }
        else if(selector.startsWith("children="))
        {
            content::path_info_t root_ipath;
            root_ipath.set_path(selector.mid(9));
            did_work_on_list |= generate_new_list_for_all_descendants(list_ipath, root_ipath, false);
        }
        else if(selector == "descendants")
        {
            did_work_on_list |= generate_new_list_for_descendants(site_key, list_ipath);
        }
        else if(selector.startsWith("descendants="))
        {
            content::path_info_t root_ipath;
            root_ipath.set_path(selector.mid(12));
            did_work_on_list |= generate_new_list_for_all_descendants(list_ipath, root_ipath, true);
        }
        else if(selector == "public")
        {
            did_work_on_list |= generate_new_list_for_public(site_key, list_ipath);
        }
        else if(selector.startsWith("type="))
        {
            // user can specify any type!
            did_work_on_list |= generate_new_list_for_type(site_key, list_ipath, selector.mid(5));
        }
        else if(selector.startsWith("hand-picked="))
        {
            // user can specify any page directly!
            did_work_on_list |= generate_new_list_for_hand_picked_pages(site_key, list_ipath, selector.mid(12));
        }
        else // "all"
        {
            if(selector != "all")
            {
                if(selector.isEmpty())
                {
                    // the default is all because we cannot really know
                    // what pages should be checked (although the field
                    // is considered mandatory, but we ought to forget
                    // once in a while)
                    SNAP_LOG_WARNING("Mandatory field \"")(get_name(name_t::SNAP_NAME_LIST_SELECTOR))("\" not defined for \"")(list_ipath.get_key())("\". Using \"all\" as a fallback.");
                }
                else
                {
                    // this could happen if you are running different
                    // versions of snap and an old backend hits a new
                    // still unknown selector
                    SNAP_LOG_WARNING("Field \"")(get_name(name_t::SNAP_NAME_LIST_SELECTOR))("\" set to unknown value \"")(selector)("\" in \"")(list_ipath.get_key())("\". Using \"all\" as a fallback.");
                }
            }
            did_work_on_list |= generate_new_list_for_all_pages(site_key, list_ipath);
        }

        if(did_work_on_list != 0)
        {
            list_modified(list_ipath);
        }

        did_work |= did_work_on_list;
    }

    return did_work;
}


int list::generate_new_list_for_all_pages(QString const & site_key, content::path_info_t & list_ipath)
{
    // This is an extremely costly search which is similar to descendants starting from root instead of list_ipath
    content::path_info_t root_ipath;
    root_ipath.set_path(site_key);
    return generate_new_list_for_all_descendants(list_ipath, root_ipath, true);
}


int list::generate_new_list_for_descendants(QString const & site_key, content::path_info_t & list_ipath)
{
    NOTUSED(site_key);
    return generate_new_list_for_all_descendants(list_ipath, list_ipath, true);
}


int list::generate_new_list_for_children(QString const & site_key, content::path_info_t & list_ipath)
{
    NOTUSED(site_key);
    return generate_new_list_for_all_descendants(list_ipath, list_ipath, false);
}


int list::generate_new_list_for_all_descendants(content::path_info_t & list_ipath, content::path_info_t & parent, bool const descendants)
{
    int did_work(0);

    links::link_info info(content::get_name(content::name_t::SNAP_NAME_CONTENT_CHILDREN), false, parent.get_key(), parent.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        content::path_info_t child_ipath;
        child_ipath.set_path(child_info.key());
        did_work |= generate_list_for_page(child_ipath, list_ipath, INT64_MAX);

        if(descendants)
        {
            did_work |= generate_new_list_for_all_descendants(list_ipath, child_ipath, true);
        }
    }

    return did_work;
}


int list::generate_new_list_for_public(QString const & site_key, content::path_info_t & list_ipath)
{
    return generate_new_list_for_type(site_key, list_ipath, "types/taxonomy/system/content-types/page/public");
}


int list::generate_new_list_for_type(QString const & site_key, content::path_info_t & list_ipath, QString const & type)
{
#ifdef DEBUG
    if(type.startsWith("/"))
    {
        throw snap_logic_exception("list type cannot start with a slash (it won't work because we do not canonicalize the path here)");
    }
    if(type.endsWith("/"))
    {
        throw snap_logic_exception("list type cannot end with a slash (it won't work because we do not canonicalize the path here)");
    }
#endif

    int did_work(0);

    content::path_info_t ipath;
    ipath.set_path(QString("%1%2").arg(site_key).arg(type));
    links::link_info info(content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        content::path_info_t child_ipath;
        child_ipath.set_path(child_info.key());
        did_work |= generate_list_for_page(child_ipath, list_ipath, INT64_MAX);
    }

    return did_work;
}


int list::generate_new_list_for_hand_picked_pages(QString const & site_key, content::path_info_t & list_ipath, QString const & hand_picked_pages)
{
    NOTUSED(site_key);

    int did_work(0);

    snap_string_list pages(hand_picked_pages.split("\n"));
    int const max_pages(pages.size());
    for(int i(0); i < max_pages; ++i)
    {
        QString const path(pages[i]);
        if(path.isEmpty())
        {
            continue;
        }
        content::path_info_t page_ipath;
        page_ipath.set_path(path);
        did_work |= generate_list_for_page(page_ipath, list_ipath, INT64_MAX);
    }

    return did_work;
}


/** \brief This function regenerates all the lists of all the websites.
 *
 * This function reads the complete list of all the lists as defined in the
 * lists table for each website defined in there.
 *
 * The process can take a very long time, especially if you have a large
 * number of websites with a lot of activity. For this reason the system
 * allows you to run this process on a backend server with the --action
 * command line option.
 *
 * The process is to:
 *
 * 1. go through all the rows of the list table (one row per website)
 * 2. go through all the columns of each row of the list table
 *    (one column per page that changed since the last update; note that
 *    it can continue to grow as we work on the list!)
 * 3. if the last update(s) happened more than LIST_PROCESSING_LATENCY
 *    then that specific page is processed and any list that include
 *    this page get updated appropriately
 * 4. entries that were processed between now and now + latency are
 *    ignored in this run (this way we avoid some problems where a client
 *    is still working on that page and thus the resulting sort of the
 *    list is not going to be accurate)
 *    TBD -- we may want to preprocess these and reprocess them at least
 *    LIST_PROCESSING_LATENCY later to make sure that the sort is correct;
 *    that way lists are still, in most cases, updated really quickly
 * 5. once we got a page that needs to be checked, we look whether this
 *    page is part of a list, if not then there is nothing to do
 *
 * \param[in] site_key  The site we want to process.
 *
 * \return 1 if the function changed anything, 0 otherwise
 */
int list::generate_all_lists(QString const & site_key)
{
    // the algorithm makes use of multiple limits to keep the time as
    // low as possible and give other websites a chance to update their
    // own lists:
    //
    // . we read 100 items and start work on them; if there are less items
    //   to work on, then we work on as many as available
    //
    // . on large websites, checking a single list can take seconds so we
    //   also limit the algorithm with time; we work on items for up to
    //   one minute, if that deadline is reached, we return (albeit only
    //   after we are done working on a certain item)
    //
    // . on small websites, checking 100 lists can be very fast, in that
    //   case we may try the next 100, but only if the time spent on the
    //   first 100 was less than 10 seconds
    //
    // note that systems with a really large number of website (and "really
    // large" is currently totally open to interpretation) you may want
    // multiple backends running the list process; this means multiple
    // websites can then be updated in parallel; note, however, that one
    // website cannot be update by more than one process at a time.
    //
    // Note: because it is sorted by timestamp,
    //       the oldest entries are automatically worked on first
    //
    SNAP_LOG_TRACE("Attempting to connect to MySQL database");

    QSqlDatabase db(QSqlDatabase::addDatabase("QMYSQL"));
    if(!db.isValid())
    {
        std::string const error( "QMYSQL database is not valid for some reason in list.cpp" );
        SNAP_LOG_FATAL(error);
        throw list_exception_mysql(error.c_str());
    }

    // still open from a previous run?
    //
    if(QSqlDatabase::database().isOpen())
    {
        QSqlDatabase::database().close();
    }

    db.setHostName     ( "localhost" );   // TODO: make use of the .conf definition
    db.setUserName     ( "snaplist" );
    db.setPassword     ( "snaplist" );
    db.setDatabaseName ( "snaplist" );
    if(!db.open())
    {
        std::string const error( "Cannot open MySQL database snaplist in list.cpp" );
        SNAP_LOG_FATAL( error );
        throw list_exception_mysql(error.c_str());
    }

    // whether the process did some work on lists so far
    //
    int did_work(0);

    // we set the date when we start working on that specific item
    // Having a date in the
    //
    QString const qstatus_str(
                "UPDATE snaplist.journal"
                    " SET status = :status"
                    " WHERE id = :id"
            );

    QSqlQuery qstatus;
    qstatus.prepare(qstatus_str);

    // then we delete that entry once we are done with it
    //
    QString const qdelete_str(
                "DELETE FROM snaplist.journal"
                    " WHERE id = :id"
            );

    QSqlQuery qdelete;
    qdelete.prepare(qdelete_str);

    // the amount of time one process can take to process all its lists
    //
    auto get_timeout = [&](auto const & field_name, int64_t default_timeout)
        {
            QString const loop_timeout_str(f_snap->get_server_parameter(field_name));
            if(!loop_timeout_str.isEmpty())
            {
                // time in seconds in .conf
                //
                bool ok(false);
                int64_t const loop_timeout_sec(loop_timeout_str.toLongLong(&ok, 10) * 1000000LL);
                if(ok && loop_timeout_sec >= 1000000LL) // valid and at least 1 second
                {
                    return loop_timeout_sec;
                }
                SNAP_LOG_WARNING("invalid number or timeout too small (under 1s) in list::looptimeout");
            }
            return default_timeout;
        };
    int64_t const loop_timeout(get_timeout("list::looptimeout", 60LL * 1000000LL));

    // function to handle a row, whether it is a high priority or not
    //
    auto handle_rows = [&](QString const & query_string)
        {
            int64_t const start_date(f_snap->get_start_date());
            int64_t const loop_start_time(f_snap->get_current_date());

            QSqlQuery query;
            query.setForwardOnly(true);
            query.prepare(query_string);
            query.bindValue(":domain",          site_key                                );
            query.bindValue(":status_limit",    static_cast<qlonglong>(loop_start_time) );
            query.bindValue(":now",             static_cast<qlonglong>(start_date)      );
            query.bindValue(":slow_priority",   LIST_PRIORITY_SLOW                      );

            if(!query.exec())
            {
                // the query failed
                // (is this a fatal error?)
                //
                SNAP_LOG_WARNING("The MySQL SELECT query to retrieve journal entries failed. lastError=[")
                                (query.lastError().text())
                                ("], lastQuery=[")
                                (query.lastQuery())
                                ("]");
                did_work |= 1;
                return;
            }

            // in case field order changes on us, get the exact index from
            // the record instead of guessing later
            //
            int const id_field_no(query.record().indexOf("id"));
            int const priority_field_no(query.record().indexOf("priority"));
            int const key_start_date_field_no(query.record().indexOf("key_start_date"));
            int const uri_field_no(query.record().indexOf("uri"));

            while(query.next())
            {
                // handle one page
                //
                priority_t const priority(query.value(priority_field_no).toInt());
                int64_t const update_request_time(query.value(key_start_date_field_no).toLongLong());
                QString const row_key(query.value(uri_field_no).toString());
                QVariant const id(query.value(id_field_no));

                // print out the row being worked on
                // (if it crashes it is really good to know where)
                {
                    char buf[64];
                    struct tm t;
                    time_t const seconds(update_request_time / 1000000LL);
                    gmtime_r(&seconds, &t);
                    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
                    QString const name(QString("[%1] %2.%3 (%4) %5")
                                .arg(static_cast<int>(priority))
                                .arg(buf)
                                .arg(update_request_time % 1000000LL, 6, 10, QChar('0'))
                                .arg(update_request_time)
                                .arg(row_key));
                    SNAP_LOG_TRACE("list plugin working on column \"")(name)("\"");
                }

                // make sure only one of us is working on this row
                // (TODO: make this actually atomic!)
                //
                qstatus.bindValue(":status", static_cast<qlonglong>(f_snap->get_current_date() + 86400LL * 1000000LL));
                qstatus.bindValue(":id", id);
                if(!qstatus.exec())
                {
                    // the query failed
                    // (is this a fatal error?)
                    //
                    SNAP_LOG_WARNING("Updating of the status to 'now + 1d' failed. lastError=[")
                                    (qstatus.lastError().text())
                                    ("], lastQuery=[")
                                    (qstatus.lastQuery())
                                    ("]");
                }

                did_work |= generate_all_lists_for_page(site_key, row_key, update_request_time);

                // we handled that page for all the lists that we have on
                // this website, so delete it now
                //
                qdelete.bindValue(":id", id);
                if(!qdelete.exec())
                {
                    // the query failed
                    // (is this a fatal error?)
                    //
                    SNAP_LOG_WARNING("Delete of entry ")
                                    (id.toString())
                                    (" failed. lastError=[")
                                    (qdelete.lastError().text())
                                    ("], lastQuery=[")
                                    (qdelete.lastQuery())
                                    ("]");
                }

                did_work |= 1; // since we delete an entry, we did something and we have to return did_work != 0

                SNAP_LOG_TRACE("list is done working on this column.");

                // were we asked to stop?
                // (i.e. snap_backend received a Ctrl-C)
                //
                if(f_backend->stop_received())
                {
                    return;
                }

                // limit the time we work
                //
                int64_t const loop_time_spent(f_snap->get_current_date() - loop_start_time);
                if(loop_time_spent > loop_timeout)
                {
                    return;
                }
            }
        };

    // although we could limit the query so it only returns entries that
    // are expected to be valid time wise, we need to know when the next
    // entry is expected to be worked on and return that to the caller
    // (through f_date_limit) so we instead read all
    //
    handle_rows(
            "SELECT id, priority, key_start_date, uri"
                " FROM snaplist.journal"
                " WHERE domain = :domain"
                    " AND (status IS NULL OR status < :status_limit)"
                    " AND key_start_date <= :now"
                    " AND priority < :slow_priority"
                " ORDER BY priority, key_start_date"
        );

    // any work done so far?
    // if not, then also handle entries with a slow priority
    //
    if(did_work == 0)
    {
        handle_rows(
                "SELECT id, priority, key_start_date, uri"
                    " FROM snaplist.journal"
                    " WHERE domain = :domain"
                        " AND (status IS NULL OR status < :status_limit)"
                        " AND key_start_date <= :now"
                        " AND priority >= :slow_priority"
                    " ORDER BY priority, key_start_date"
            );
    }

    // now determine when is a good time to wake up again
    {
        QString const qnext_str(
                    "SELECT next_processing"
                        " FROM snaplist.journal"
                        " WHERE domain = :domain"
                        " ORDER BY next_processing"
                        " LIMIT 1"
                );

        QSqlQuery qnext;
        qnext.setForwardOnly(true);
        qnext.prepare(qnext_str);
        qnext.bindValue(":domain", site_key);

        if(!qnext.exec())
        {
            // the query failed
            // (is this a fatal error?)
            //
            SNAP_LOG_WARNING("The MySQL SELECT query to retrieve journal entries failed. lastError=[")
                            (qnext.lastError().text())
                            ("], lastQuery=[")
                            (qnext.lastQuery())
                            ("]");
        }
        else
        {
            if(qnext.next())
            {
                // in case field order changes on us, get the exact index from
                // the record instead of guessing later
                //
                int const next_processing_field_no(qnext.record().indexOf("next_processing"));
                int64_t const next_processing(qnext.value(next_processing_field_no).toLongLong());
                if(next_processing < f_date_limit)
                {
                    // next wake up is early (less than 5 min.) so use
                    // that date
                    //
                    f_date_limit = next_processing;
                }
            }
            //else -- nothing more, will sleep for 5 min. and try again
            //        or wake up on a PING
        }
    }

    // clear our cache
    f_check_expressions.clear();
    f_item_key_expressions.clear();

    return did_work;
}


int list::generate_all_lists_for_page(QString const & site_key, QString const & page_key, int64_t update_request_time)
{
    content::path_info_t page_ipath;
    page_ipath.set_path(page_key);

    int did_work(0);

    content::path_info_t ipath;
    ipath.set_path(site_key + get_name(name_t::SNAP_NAME_LIST_TAXONOMY_PATH));
    links::link_info info(get_name(name_t::SNAP_NAME_LIST_TYPE), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        // Entries are defined with the following:
        //
        // name_t::SNAP_NAME_LIST_ITEM_KEY_SCRIPT
        //    The script used to generate the item key used to sort items
        //    of the list.
        //
        // name_t::SNAP_NAME_LIST_KEY
        //    list::key::<list key>
        //
        //    The <list key> part is the the ipath.get_key() from the
        //    list page. This way we can find the lists this item is a
        //    part of.
        //
        // name_t::SNAP_NAME_LIST_ORDERED_PAGES
        //    list::ordered_pages::<item key>
        //
        //    The <item key> part is defined using the
        //    name_t::SNAP_NAME_LIST_ITEM_KEY_SCRIPT script. If not yet defined, use
        //    name_t::SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT to create the compiled
        //    script. Note that this script may change under our feet so that
        //    means we'd lose access to the reference. For this reason, the
        //    reference is saved in the item under "list::key::<list key>".
        //
        // name_t::SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT
        //    This cell includes the original script used to compute the
        //    item key. This script is compiled from the script in the
        //    name_t::SNAP_NAME_LIST_ITEM_KEY_SCRIPT.
        //
        // name_t::SNAP_NAME_LIST_TYPE
        //    The list type, used for the standard link of a list page to
        //    the list content type.
        //

        QString const key(child_info.key());
        content::path_info_t list_ipath;
        list_ipath.set_path(key);
//SNAP_LOG_WARNING("generate list \"")(list_ipath.get_key())("\" for page \"")(page_ipath.get_key());
        int const did_work_on_list(generate_list_for_page(page_ipath, list_ipath, update_request_time));
        if(did_work_on_list != 0)
        {
            did_work |= did_work_on_list;

            list_modified(list_ipath);
        }
    }

    return did_work;
}


/** \brief Add or remove a page from a list.
 *
 * This function checks the page \p page_ipath agains the script
 * defined in list \p list_ipath. If it is a match, the page is added to
 * the list (if it was not there). If it is not a match, the page is
 * removed from the list (if it was there.)
 *
 * This function can be called after you created a page to immediately
 * add the page to a list. This is useful for pages that are known to
 * at least very likely match the script of a certain list. (i.e. a
 * page that is to appear in a menu, a new page in a blog will appear
 * on the front page of that blog, etc.)
 *
 * The page will be checked again whenever the list system runs against
 * it as it will be defined in the list table. That should not have any
 * bad side effect outside of doing the work twice (although it will
 * already have been added so it will not be re-added, just updated
 * if need be.)
 *
 * \warning
 * This function verifies that the \p page_ipath is valid before
 * proceeding, however, it does not verify the \p list_ipath. It
 * is your responsibility to do so.
 *
 * \note
 * The \p update_request_time is not currently used. We ran in many
 * problems attempting to obtimize using a "last time this was
 * updated" that we abandonned the idea for now.
 *
 * \param[in,out] page_ipath  The path to the page being tested.
 * \param[in,out] list_ipath  The path to the list being worked on.
 * \param[in] update_request_time  The time when the last change was registered.
 *
 * \return Zero (0) if nothing happens, 1 if the list was modified.
 */
int list::generate_list_for_page(content::path_info_t & page_ipath, content::path_info_t & list_ipath, int64_t update_request_time)
{
    // whether the function did change something: 0 no, 1 yes
    int did_work(0);

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
    libdbproxy::row::pointer_t list_row(branch_table->getRow(list_ipath.get_branch_key()));

    // check whether we already updated that page
    // (because the same page may be listed many times in the list table)
    // we allow a 1 minute gap because right now we do not yet have the
    // correct way to create pages (i.e. create a finilized page before
    // we move forward; today we create a basic entry, send the signal
    // to the list then add content and links... which could take some
    // time which is not otherwise taken in account)
    //
    // this does not seem to do what I was hoping it would do...
    // maybe we can debug this later
    //
    //int64_t const last_updated(list_row->getCell(get_name(name_t::SNAP_NAME_LIST_LAST_UPDATED))->getValue().safeInt64Value());
    NOTUSED(update_request_time);
    //if(last_updated - 60 * 1000000 > update_request_time)
    //{
    //    return did_work;
    //}

    try
    {
        libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
        if(!content_table->exists(page_ipath.get_key())
        || !content_table->getRow(page_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
        {
            // the page is not ready yet, let it be for a little longer, it will
            // be taken in account by the standard process
            // (at this point we may not even have the branch/revision data)
            //
            return 0;
        }

        // TODO: testing just the row is not enough to know whether it was deleted
        //       (I think we will also always have content::created in the
        //       branch assuming it was properly created)
        //
        //       Note: since we are now using CQL, it is likely working right.
        //
        if(!branch_table->exists(page_ipath.get_branch_key()))
        {
            // branch disappeared... ignore
            // (it could have been deleted or moved--i.e. renamed)
            //
            return 0;
        }
        libdbproxy::row::pointer_t page_branch_row(branch_table->getRow(page_ipath.get_branch_key()));

        QString const link_name(get_name(name_t::SNAP_NAME_LIST_LINK));

        QString const list_key_in_page(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_LIST_KEY)).arg(list_ipath.get_key()));
        bool const included(run_list_check(list_ipath, page_ipath));
        QString const new_item_key(run_list_item_key(list_ipath, page_ipath));
        if(included)
        {
//SNAP_LOG_WARNING("------- include this item");
            QString const new_item_key_full(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_LIST_ORDERED_PAGES)).arg(new_item_key));

            // the check script says to include this item in this list;
            // first we need to check to find under which key it was
            // included if it is already there because it may have
            // changed
            if(page_branch_row->exists(list_key_in_page))
            {
                // check to see whether the current key changed
                // note that if the destination does not exist, we still attempt
                // the drop + create (that happens when there is a change that
                // affects the key and you get a duplicate which is corrected
                // later--but we probably need to fix duplicates at some point)
                libdbproxy::value current_item_key(page_branch_row->getCell(list_key_in_page)->getValue());
                QString const current_item_key_full(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_LIST_ORDERED_PAGES)).arg(current_item_key.stringValue()));
                if(current_item_key_full != new_item_key_full
                || !page_branch_row->exists(new_item_key_full))
                {
                    // it changed, we have to delete the old one and
                    // create a new one
                    list_row->dropCell(current_item_key_full);
                    list_row->getCell(new_item_key_full)->setValue(page_ipath.get_key());
                    page_branch_row->getCell(list_key_in_page)->setValue(new_item_key);

                    did_work = 1;
                }
                // else -- nothing changed, we are done
            }
            else
            {
                // it does not exist yet, add it

                // create a standard link between the list and the page item
                bool const source_unique(false);
                bool const destination_unique(false);
                links::link_info source(link_name, source_unique, list_ipath.get_key(), list_ipath.get_branch());
                links::link_info destination(link_name, destination_unique, page_ipath.get_key(), page_ipath.get_branch());
                {
                    csspp::safe_bool_t save_list_link(f_list_link);
                    links::links::instance()->create_link(source, destination);
                }

                // create the ordered list
                list_row->getCell(new_item_key_full)->setValue(page_ipath.get_key());

                // save a back reference to the ordered list so we can
                // quickly find it
                page_branch_row->getCell(list_key_in_page)->setValue(new_item_key);

                did_work = 1;
            }
        }
        else
        {
            // the check script says that this path is not included in this
            // list; the item may have been included earlier so we have to
            // make sure it gets removed if still there
            if(page_branch_row->exists(list_key_in_page))
            {
                libdbproxy::value current_item_key(page_branch_row->getCell(list_key_in_page)->getValue());
                QString const current_item_key_full(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_LIST_ORDERED_PAGES)).arg(current_item_key.stringValue()));

                list_row->dropCell(current_item_key_full);
                page_branch_row->dropCell(list_key_in_page);

                bool const source_unique(false);
                bool const destination_unique(false);
                links::link_info source(link_name, source_unique, list_ipath.get_key(), list_ipath.get_branch());
                links::link_info destination(link_name, destination_unique, page_ipath.get_key(), page_ipath.get_branch());
                csspp::safe_bool_t save_list_link(f_list_link);
                links::links::instance()->delete_this_link(source, destination);

                did_work = 1;
            }
        }
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_ERROR("exception \"")(e.what())("\" occurred while attempting to create the list for page \"")(page_ipath.get_key())("\".");
        did_work = 1;
    }
    catch(...)
    {
        SNAP_LOG_ERROR("an unknown exception occurred while attempting to create the list for page \"")(page_ipath.get_key())("\".");
        did_work = 1;
    }

    // if a new list failed in some way, we still get this value because
    // trying again will probably not help; also empty lists would otherwise
    // not get this date
    //
    // WARNING: it is VERY important that we use a date defined BEFORE
    //          we started the generation of the list; this is very
    //          important otherwise we would miss many updates required
    //          by the front end.
    //
    // TODO: make sure we do not set this flag if we are quitting early
    //       (i.e. child receives a STOP signal)
    //
    int64_t const start_date(f_snap->get_start_date());
    list_row->getCell(get_name(name_t::SNAP_NAME_LIST_LAST_UPDATED))->setValue(start_date);

    // TODO
    // if we did work, the list size changed so we have to recalculate the
    // length (list::number_of_items) -- since we cannot be totally sure that
    // something was added or removed, we recalculate the size each time for
    // now but this is very slow so we will want to optimize that
    // at a later time to make sure we do not take forever to build lists
    //
    // on the other hand, once a list is complete and we just add an
    // entry every now and then, this is not much of an overhead at all
    //
    if(did_work != 0)
    {
        list_row->clearCache();

        char const * ordered_pages(get_name(name_t::SNAP_NAME_LIST_ORDERED_PAGES));

        // count the new total number of ordered pages
        int32_t count(0);
        auto column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
        column_predicate->setStartCellKey(QString("%1::").arg(ordered_pages));
        column_predicate->setEndCellKey(QString("%1;").arg(ordered_pages));
        column_predicate->setCount(100);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // clear the cache before reading the next load
            list_row->readCells(column_predicate);
            libdbproxy::cells const cells(list_row->getCells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            count += cells.size();
        }

        list_row->getCell(get_name(name_t::SNAP_NAME_LIST_NUMBER_OF_ITEMS))->setValue(count);
    }

    return did_work;
}


/** \brief Execute the test script of a list.
 *
 * This function is used to run the test script of a list object against a
 * page. It returns whether it is a match.
 *
 * The function compiles the script and saves it in the "list::test_script"
 * field of the list if it is not there yet. That way we can avoid the
 * compile step on future access.
 *
 * If the script cannot be compiled for any reason, then the function returns
 * false as if the page was not part of the list.
 *
 * The script has to return a result which can be converted to a boolean.
 *
 * \param[in,out] list_ipath  The ipath used to the list.
 * \param[in,out] page_ipath  The ipath used to the page.
 *
 * \return true if the page is to be included.
 */
bool list::run_list_check(content::path_info_t & list_ipath, content::path_info_t & page_ipath)
{
    QString const branch_key(list_ipath.get_branch_key());
    snap_expr::expr::expr_pointer_t e(nullptr);
    if(!f_check_expressions.contains(branch_key))
    {
        e = snap_expr::expr::expr_pointer_t(new snap_expr::expr);
        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
        libdbproxy::value compiled_script(branch_table->getRow(branch_key)->getCell(get_name(name_t::SNAP_NAME_LIST_TEST_SCRIPT))->getValue());
        if(compiled_script.nullValue())
        {
            libdbproxy::value script(branch_table->getRow(branch_key)->getCell(get_name(name_t::SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT))->getValue());
            if(script.nullValue())
            {
                // no list here?!
                // TODO: generate an error
                return false;
            }
            if(!e->compile(script.stringValue()))
            {
                // script could not be compiled (invalid script!)
                // TODO: generate an error

                // create a default script so we do not try to compile the
                // broken script over and over again
                if(!e->compile("0"))
                {
                    // TODO: generate a double error!
                    //       this should really not happen
                    //       because "0" is definitively a valid script
                    return false;
                }
            }
            // save the result for next time
            branch_table->getRow(branch_key)->getCell(get_name(name_t::SNAP_NAME_LIST_TEST_SCRIPT))->setValue(e->serialize());
        }
        else
        {
            e->unserialize(compiled_script.binaryValue());
        }
        f_check_expressions[branch_key] = e;
    }
    else
    {
        e = f_check_expressions[branch_key];
    }

    // run the script with this path
    snap_expr::variable_t result;
    snap_expr::variable_t::variable_map_t variables;
    snap_expr::variable_t var_path("path");
    var_path.set_value(page_ipath.get_cpath());
    variables["path"] = var_path;
    snap_expr::variable_t var_page("page");
    var_page.set_value(page_ipath.get_key());
    variables["page"] = var_page;
    snap_expr::variable_t var_list("list");
    var_list.set_value(list_ipath.get_key());
    variables["list"] = var_list;
    snap_expr::functions_t functions;
    e->execute(result, variables, functions);

#if 0
#ifdef DEBUG
    {
        content::content *content_plugin(content::content::instance());
        libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
        libdbproxy::value script(branch_table->getRow(branch_key)->getCell(get_name(name_t::SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT))->getValue());
        SNAP_LOG_TRACE() << "script [" << script.stringValue() << "] result [" << (result.is_true() ? "true" : "false") << "]";
    }
#endif
#endif

    return result.is_true();
}


/** \brief Generate the test script of a list.
 *
 * This function is used to extract the test script of a list object.
 * The test script is saved in the list::test_script field of a page,
 * on a per branch basis. This function makes use of the branch
 * defined in the ipath.
 *
 * \param[in,out] list_ipath  The ipath used to find the list.
 * \param[in,out] page_ipath  The ipath used to find the page.
 *
 * \return The item key as a string.
 */
QString list::run_list_item_key(content::path_info_t & list_ipath, content::path_info_t & page_ipath)
{
    QString const branch_key(list_ipath.get_branch_key());
    snap_expr::expr::expr_pointer_t e(nullptr);
    if(!f_item_key_expressions.contains(branch_key))
    {
        e = snap_expr::expr::expr_pointer_t(new snap_expr::expr);
        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
        libdbproxy::value compiled_script(branch_table->getRow(branch_key)->getCell(get_name(name_t::SNAP_NAME_LIST_ITEM_KEY_SCRIPT))->getValue());
        if(compiled_script.nullValue())
        {
            libdbproxy::value script(branch_table->getRow(branch_key)->getCell(get_name(name_t::SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT))->getValue());
            if(script.nullValue())
            {
                // no list here?!
                // TODO: generate an error
                return "";
            }
            if(!e->compile(script.stringValue()))
            {
                // script could not be compiled (invalid script!)
                // TODO: generate an error

                // create a default script so we do not try to compile the
                // broken script over and over again
                if(!e->compile("\"---\""))
                {
                    // TODO: generate a double error!
                    //       this should really not happen
                    //       because "\"---\"" is definitively a valid script
                    return "";
                }
            }
            // save the result for next time
            branch_table->getRow(branch_key)->getCell(get_name(name_t::SNAP_NAME_LIST_ITEM_KEY_SCRIPT))->setValue(e->serialize());
        }
        else
        {
            e->unserialize(compiled_script.binaryValue());
        }
        f_item_key_expressions[branch_key] = e;
    }
    else
    {
        e = f_item_key_expressions[branch_key];
    }

    // run the script with this path
    snap_expr::variable_t result;
    snap_expr::variable_t::variable_map_t variables;
    snap_expr::variable_t var_path("path");
    var_path.set_value(page_ipath.get_cpath());
    variables["path"] = var_path;
    snap_expr::variable_t var_page("page");
    var_page.set_value(page_ipath.get_key());
    variables["page"] = var_page;
    snap_expr::variable_t var_list("list");
    var_list.set_value(list_ipath.get_key());
    variables["list"] = var_list;
    snap_expr::functions_t functions;
    e->execute(result, variables, functions);

    return result.get_string("*result*");
}


/** \brief Replace a [list::...] token with the contents of a list.
 *
 * This function replaces the list tokens with themed lists.
 *
 * The supported tokens are:
 *
 * \code
 * [list::theme(path="<list path>", theme="<theme name>", start="<start>", count="<count>")]
 * \endcode
 *
 * Theme the list defined at \<list path\> with the theme \<theme name\>.
 * You may skip some items and start with item \<start\> instead of item 0.
 * You may specified the number of items to display with \<count\>. Be
 * careful because by default all the items are shown (Although there is a
 * system limit which at this time is 10,000 which is still a very LARGE
 * list!) The theme name, start, and count parameters are optional.
 * The path is mandatory. It can be empty if the root page was transformed
 * into a list.
 *
 * \param[in,out] ipath  The path to the page being worked on.
 * \param[in,out] xml  The XML document used with the layout.
 * \param[in,out] token  The token object, with the token name and optional parameters.
 */
void list::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(xml);

    // a list::... token?
    if(!token.is_namespace("list::"))
    {
        return;
    }

    if(token.is_token(get_name(name_t::SNAP_NAME_LIST_THEME)))
    {
        // list::theme expects one to four parameters
        if(!token.verify_args(1, 4))
        {
            return;
        }

        // Path
        filter::filter::parameter_t path_param(token.get_arg("path", 0, filter::filter::token_t::TOK_STRING));
        if(token.f_error)
        {
            return;
        }
        if(path_param.f_value.isEmpty())
        {
            token.f_error = true;
            token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list 'path' (first parameter) of the list::theme() function cannot be an empty string.</span>";
            return;
        }

        // Theme
        QString theme("qrc:/xsl/list/default"); // default theming, simple <ul>{<li>...</li>}</ul> list
        if(token.has_arg("theme", 1))
        {
            filter::filter::parameter_t theme_param(token.get_arg("theme", 1, filter::filter::token_t::TOK_STRING));
            if(token.f_error)
            {
                return;
            }
            // if user included the ".xsl" extension, ignore it
            if(theme_param.f_value.endsWith(".xsl"))
            {
                theme_param.f_value = theme_param.f_value.left(theme_param.f_value.length() - 4);
            }
            if(!theme_param.f_value.isEmpty())
            {
                theme = theme_param.f_value;
            }
        }

        // Start
        int start(0); // start with very first item
        if(token.has_arg("start", 2))
        {
            filter::filter::parameter_t start_param(token.get_arg("start", 2, filter::filter::token_t::TOK_INTEGER));
            if(token.f_error)
            {
                return;
            }
            bool ok(false);
            start = start_param.f_value.toInt(&ok, 10);
            if(!ok)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list start (third parameter) of the list::theme() function must be a valid integer.</span>";
                return;
            }
            if(start < 0)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list start (third parameter) of the list::theme() function must be a positive integer or zero.</span>";
                return;
            }
        }

        // Count
        int count(-1); // all items
        if(token.has_arg("count", 3))
        {
            filter::filter::parameter_t count_param(token.get_arg("count", 3, filter::filter::token_t::TOK_INTEGER));
            if(token.f_error)
            {
                return;
            }
            bool ok(false);
            count = count_param.f_value.toInt(&ok, 10);
            if(!ok)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list 'count' (forth parameter) of the list::theme() function must be a valid integer.</span>";
                return;
            }
            if(count != -1 && count <= 0)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list 'count' (forth parameter) of the list::theme() function must be a valid integer large than zero or -1.</span>";
                return;
            }
        }

        content::path_info_t list_ipath;
        list_ipath.set_path(path_param.f_value);

        token.f_replacement = generate_list(ipath, list_ipath, start, count, theme);
    }
}


void list::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("list::theme",
        "Display a themed list. The token accepts 1 to 4 parameters:"
        " the path to the list (mandatory) [path], the name of a theme"
        " (\"default\" otherwise) [theme], the first item to display"
        " [start] (the very first item is number 0), the number of"
        " items to display [count].");
}



/** \brief Generate a list.
 *
 * This function generates the list defined by list_ipath from \p start
 * up to <code>start + count - 1</code> using the specified \p theme.
 *
 * The \p ipath represents the object for which the list is being created.
 *
 * \param[in,out] ipath  Object for which the list is being generated.
 * \param[in,out] list_ipath  The list object from which the list is created.
 * \param[in] start  The start element.
 * \param[in] count  The number of items to display.
 * \param[in] theme  The theme used to generate the output.
 *
 * \return The resulting HTML of the list or an empty string.
 */
QString list::generate_list(content::path_info_t & ipath, content::path_info_t & list_ipath, int start, int count, QString const & theme)
{
    QString const list_cpath(list_ipath.get_cpath());
    if(list_cpath == "admin"
    || list_cpath.startsWith("admin/"))
    {
        // although we are just viewing lists, only "administer" is
        // used when visiting pages under /admin...
        //
        list_ipath.set_parameter("action", "administer");
    }
    else
    {
        // we are just viewing this list
        list_ipath.set_parameter("action", "view");
    }

    quiet_error_callback list_error_callback(f_snap, true);
    plugin * list_plugin(path::path::instance()->get_plugin(list_ipath, list_error_callback));
    if(!list_error_callback.has_error() && list_plugin)
    {
        layout::layout_content * list_content(dynamic_cast<layout::layout_content *>(list_plugin));
        if(list_content == nullptr)
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Plugin Missing",
                    QString("Plugin \"%1\" does not know how to handle a list assigned to it.").arg(list_plugin->get_plugin_name()),
                    "list::on_replace_token() the plugin does not derive from layout::layout_content.");
            NOTREACHED();
        }

        // IMPORTANT NOTE: We do not check the maximum with the count
        //                 because our lists may expend with time

        // read the list of items
        //
        // TODO: use a paging_t object to read the list so we can
        //       append a navigation and handle the page parameter
        //
        paging_t paging(f_snap, list_ipath);
        paging.set_start_offset(start + 1);
        paging.set_maximum_number_of_items(count);
        paging.process_query_string_info();
        list_item_vector_t items(paging.read_list());
        snap_child::post_file_t f;

        // Load the list body
        f.set_filename(theme + "-list-body.xsl");
        if(!f_snap->load_file(f) || f.get_size() == 0)
        {
            list_ipath.set_parameter("error", "1");
            return QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list theme (%1-list-body.xsl) could not be loaded.</span>")
                                        .arg(theme);
        }
        QString const list_body_xsl(QString::fromUtf8(f.get_data()));

        // Load the list theme
        f.set_filename(theme + "-list-theme.xsl");
        if(!f_snap->load_file(f) || f.get_size() == 0)
        {
            list_ipath.set_parameter("error", "1");
            return QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list theme (%1-list-theme.xsl) could not be loaded.</span>")
                                        .arg(theme);
        }
        QString const list_theme_xsl(QString::fromUtf8(f.get_data()));

        // Load the item body
        f.set_filename(theme + "-item-body.xsl");
        if(!f_snap->load_file(f) || f.get_size() == 0)
        {
            list_ipath.set_parameter("error", "1");
            return QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list theme (%1-item-theme.xsl) could not be loaded.</span>")
                                        .arg(theme);
        }
        QString const item_body_xsl(QString::fromUtf8(f.get_data()));

        // Load the item theme
        f.set_filename(theme + "-item-theme.xsl");
        if(!f_snap->load_file(f) || f.get_size() == 0)
        {
            list_ipath.set_parameter("error", "1");
            return QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list theme (%1-item-theme.xsl) could not be loaded.</span>")
                                        .arg(theme);
        }
        QString const item_theme_xsl(QString::fromUtf8(f.get_data()));

        layout::layout * layout_plugin(layout::layout::instance());
        QDomDocument list_doc(layout_plugin->create_document(list_ipath, list_plugin));
        layout_plugin->create_body(list_doc, list_ipath, list_body_xsl, list_content);
        // TODO: fix this problem (i.e. /products, /feed...)
        // The following is a "working" fix so we can generate a list
        // for the page that defines the list, but of course, in
        // that case we have the "wrong" path... calling with the
        // list_ipath generates a filter loop problem
        //content::path_info_t random_ipath;
        //random_ipath.set_path("");
        //layout_plugin->create_body(list_doc, random_ipath, list_body_xsl, list_content);

        QDomElement body(snap_dom::get_element(list_doc, "body"));
        QDomElement list_element(list_doc.createElement("list"));
        body.appendChild(list_element);

        QString const main_path(f_snap->get_uri().path());
        content::path_info_t main_ipath;
        main_ipath.set_path(main_path);

        // now theme the list
        int const max_items(items.size());
        for(int i(0), index(1); i < max_items; ++i)
        {
            list_error_callback.clear_error();
            content::path_info_t item_ipath;
            item_ipath.set_path(items[i].get_uri());
            if(item_ipath.get_parameter("action").isEmpty())
            {
                // the default action on a link is "view" unless it
                // references an administrative task under /admin
                if(item_ipath.get_cpath() == "admin" || item_ipath.get_cpath().startsWith("admin/"))
                {
                    item_ipath.set_parameter("action", "administer");
                }
                else
                {
                    item_ipath.set_parameter("action", "view");
                }
            }
            // whether we are attempting to display this item
            // (opposed to the test when going to the page or generating
            // the list in the first place)
            item_ipath.set_parameter("mode", "display");
            plugin * item_plugin(path::path::instance()->get_plugin(item_ipath, list_error_callback));
            layout_content * l(dynamic_cast<layout_content *>(item_plugin));
            if(!list_error_callback.has_error() && item_plugin)
            {
                if(l)
                {
                    // put each box in a filter tag so that way we have
                    // a different owner and path for each
                    //
                    QDomDocument item_doc(layout_plugin->create_document(item_ipath, item_plugin));
                    QDomElement item_root(item_doc.documentElement());
                    item_root.setAttribute("index", index);

                    FIELD_SEARCH
                        (content::field_search::command_t::COMMAND_ELEMENT, snap_dom::get_element(item_doc, "metadata"))
                        (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)

                        // snap/head/metadata/desc[@type="list_uri"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, list_ipath.get_key())
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=list_uri]/data")

                        // snap/head/metadata/desc[@type="list_path"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, list_cpath)
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=list_path]/data")

                        // snap/head/metadata/desc[@type="box_uri"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, ipath.get_key())
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=box_uri]/data")

                        // snap/head/metadata/desc[@type="box_path"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, ipath.get_cpath())
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=box_path]/data")

                        // snap/head/metadata/desc[@type="main_page_uri"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, main_ipath.get_key())
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=main_page_uri]/data")

                        // snap/head/metadata/desc[@type="main_page_path"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, main_ipath.get_cpath())
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=main_page_path]/data")

                        // retrieve names of all the boxes
                        ;

//SNAP_LOG_WARNING("create body for item ")(i)(" with index ")(index);
                    layout_plugin->create_body(item_doc, item_ipath, item_body_xsl, l);
//std::cerr << "source to be parsed [" << item_doc.toString(-1) << "]\n";
                    QDomElement item_body(snap_dom::get_element(item_doc, "body"));
                    item_body.setAttribute("index", index);
//SNAP_LOG_WARNING("apply theme to item ")(i)(" with index ")(index);
                    QString const themed_item(layout_plugin->apply_theme(item_doc, item_theme_xsl, theme));
//std::cerr << "themed item [" << themed_item << "]\n";

                    // add that result to the list document
                    QDomElement item(list_doc.createElement("item"));
                    list_element.appendChild(item);
                    snap_dom::insert_html_string_to_xml_doc(item, themed_item);

                    ++index; // index only counts items added to the output
                }
                else
                {
                    SNAP_LOG_ERROR("the item_plugin pointer for \"")
                                  (item_plugin->get_plugin_name())
                                  ("\" is not a layout_content");
                }
            }
        }

        // we cannot use "navigation" as the name of this tag since it is
        // used for the navigation links defined in the header.
        //
        QDomElement navigation_tag(list_doc.createElement("list-navigation-tags"));
        body.appendChild(navigation_tag);
        paging.generate_list_navigation(navigation_tag, f_snap->get_uri(), 5, true, true, true);

//std::cerr << "resulting XML [" << list_doc.toString(-1) << "]\n";

        // now theme the list as a whole
        // we add a wrapper so we can use /node()/* in the final theme
//SNAP_LOG_WARNING("apply list theme");
        return layout_plugin->apply_theme(list_doc, list_theme_xsl, theme);
    }
    // else list is not accessible (permission "problem")
//else SNAP_LOG_FATAL("list::on_replace_token() list \"")(list_ipath.get_key())("\" is not accessible.");

    return QString();
}


void list::on_generate_boxes_content(content::path_info_t & page_cpath, content::path_info_t & ipath, QDomElement & page, QDomElement & box)
{
    NOTUSED(page_cpath);

    output::output::instance()->on_generate_main_content(ipath, page, box);
}


void list::on_copy_branch_cells(libdbproxy::cells & source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    NOTUSED(destination_branch);

    libdbproxy::cells left_cells;

    // handle one batch
    bool has_list(false);
    for(libdbproxy::cells::const_iterator nc(source_cells.begin());
            nc != source_cells.end();
            ++nc)
    {
        libdbproxy::cell::pointer_t source_cell(*nc);
        QByteArray cell_key(source_cell->columnKey());

        if(cell_key == get_name(name_t::SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT)
        || cell_key == get_name(name_t::SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT)
        || cell_key == get_name(name_t::SNAP_NAME_LIST_SELECTOR))
        {
            has_list = true;
            // copy our fields as is
            destination_row->getCell(cell_key)->setValue(source_cell->getValue());
        }
        else
        {
            // keep the other branch fields as is, other plugins can handle
            // them as required by implementing this signal
            //
            // note that the map is a map a shared pointers so it is fast
            // to make a copy like this
            left_cells[cell_key] = source_cell;
        }
    }

    if(has_list)
    {
        // make sure the (new) list is checked so we actually get a list
        content::path_info_t ipath;
        ipath.set_path(destination_row->rowName());
        on_modified_content(ipath);
    }

    // overwrite the source with the cells we allow to copy "further"
    source_cells = left_cells;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
