// Snap Websites Server -- firewall handling by snap
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

// ourselves
//
#include "version.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>
#include <snapwebsites/snap_cassandra.h>
#include <snapwebsites/snapwebsites.h>

#include <fstream>
#include <sstream>

#include <sys/stat.h>

#include <QDir>

namespace
{

class snap_firewall;





class snap_firewall_interrupt
        : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<snap_firewall_interrupt>    pointer_t;

                                snap_firewall_interrupt(snap_firewall * fw);
    virtual                     ~snap_firewall_interrupt() override {}

    // snap::snap_communicator::snap_signal implementation
    virtual void                process_signal() override;

private:
    snap_firewall *             f_snap_firewall = nullptr;
};





/** \brief Handle messages from the Snap Communicator server.
 *
 * This class is an implementation of the TCP client message connection
 * so we can handle incoming messages.
 */
class messenger
        : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<messenger>    pointer_t;

                        messenger(snap_firewall * sfw, std::string const & addr, int port);

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void        process_message(snap::snap_communicator_message const & message);
    virtual void        process_connection_failed(std::string const & error_message);
    virtual void        process_connected();

private:
    // this is owned by a server function so no need for a smart pointer
    snap_firewall *     f_snap_firewall;
};


/** \brief The timer used when a connection to Cassandra fails.
 *
 * When we receive the CASSANDRAREADY event, the connection is likely to
 * work. However, while reading the data in the following loop, we may
 * end up with an exception and that stops the connection right there.
 * In other words, on return the f_cassandra pointer will be reset back
 * to a null pointer.
 *
 * To allow for a little bit of time before reconnecting, we use this
 * timer. Because in most cases this happens when Cassandra is rather
 * overloaded so trying to reconnect immediately at this stage is not
 * a good plan.
 *
 * At this time we setup the timer to 30 seconds. The firewall continues
 * to be fully functional, so a longer pause should not be much of a
 * problem.
 */
class reconnect_timer
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<reconnect_timer>        pointer_t;

                                reconnect_timer(snap_firewall * sfw);

    // snap::snap_communicator::snap_timer implementation
    virtual void                process_timeout();

private:
    snap_firewall *             f_snap_firewall;
};


/** \brief The timer to produce wake up calls once in a while.
 *
 * This timer is used to wake us once in a while as determined by when
 * an IP address has to be removed from the firewall.
 *
 * The date feature is always used on this timer (i.e. wake up
 * the process at a specific date and time in microseconds.)
 */
class wakeup_timer
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<wakeup_timer>        pointer_t;

                                wakeup_timer(snap_firewall * sfw);

    // snap::snap_communicator::snap_timer implementation
    virtual void                process_timeout();

private:
    snap_firewall *             f_snap_firewall;
};


/** \brief Firewall process class.
 *
 * This class handles firewall requests.
 *
 * There are two requests that this process handles:
 *
 * 1) request to setup a firewall in the first place. This means setting
 *    up the necessary files under /etc so the server boots with a strong
 *    firewall as one would expect on any sane server;
 *
 * 2) request to, generally temporarilly, block IP addresses on the
 *    firewall; when a spam or hacker hit is detected, then a message
 *    is expected to be sent to this firewall process to block the
 *    IP address of that spammer or hacker.
 *
 * \msc
 * hscale = 2;
 * a [label="snapfirewall"],
 * b [label="snapcommunicator"],
 * c [label="other-process"],
 * d [label="iplock"];
 *
 * #
 * # Register snapfirewall
 * #
 * a=>a [label="connect socket to snapcommunicator"];
 * a->b [label="REGISTER service=snapfirewall;version=<VERSION>"];
 * b->a [label="READY"];
 * b->a [label="HELP"];
 * a->b [label="COMMANDS list=HELP,LOG,..."];
 *
 * #
 * # Reconfigure logger
 * #
 * b->a [label="LOG"];
 * a=>a [label="logging::recongigure()"];
 *
 * #
 * # Stop snapfirewall
 * #
 * b->a [label="STOP"];
 * a=>a [label="exit(0);"];
 *
 * c->a [label="kill -INT a"];
 * a->a [label="STOP"];
 * a=>a [label="exit(0);"];
 *
 * #
 * # Block an IP address
 * #
 * c->b [label="snapfirewall/BLOCK uri=...;period=...;reason=..."];
 * b->a [label="BLOCK uri=...;period=...;reason=..."];
 * a->d [label="block IP address with iptables"];
 *
 * #
 * # Unblock an IP address
 * #
 * c->b [label="snapfirewall/UNBLOCK uri=..."];
 * b->a [label="UNBLOCK uri=..."];
 * a->d [label="unblock IP address with iptables"];
 *
 * #
 * # Wakeup timer
 * #
 * a->a [label="wakeup timer timed out"];
 * a=>a [label="unblocked an IP address"];
 * \endmsc
 */
class snap_firewall
{
public:
    typedef std::shared_ptr<snap_firewall>      pointer_t;

                                snap_firewall( int argc, char * argv[] );
                                ~snap_firewall();

    static pointer_t            instance( int argc, char * argv[] );

    void                        run();
    void                        process_timeout();
    void                        process_reconnect();
    void                        process_message(snap::snap_communicator_message const & message);
    void                        stop(bool quitting);
    void                        is_db_ready();

    static void                 sighandler( int sig );

private:
    class block_info_t
    {
    public:
        typedef std::vector<block_info_t>   block_info_vector_t;

        enum class status_t
        {
            BLOCK_INFO_UNDEFINED,       // never banned before
            BLOCK_INFO_BANNED,
            BLOCK_INFO_UNBANNED         // has been banned before
        };

                            block_info_t(QString const & uri);
                            block_info_t(snap::snap_communicator_message const & message, status_t status = status_t::BLOCK_INFO_BANNED);

        bool                is_valid() const;

        void                save(libdbproxy::table::pointer_t firewall_table, QString const & server_name);

        void                set_uri(QString const & uri);
        void                set_scheme(QString scheme);
        void                set_ip(QString const & scheme);
        void                set_block_limit(QString const & period);
        void                keep_longest(block_info_t const & block);

        void                set_ban_count(int64_t count);
        int64_t             get_ban_count() const;
        int64_t             get_total_ban_count(libdbproxy::table::pointer_t firewall_table, QString const & server_name) const;
        void                set_packet_count(int64_t count);
        int64_t             get_packet_count() const;
        void                set_byte_count(int64_t count);
        int64_t             get_byte_count() const;

        QString             canonicalized_uri() const;
        QString             get_scheme() const;
        QString             get_ip() const;
        int64_t             get_block_limit() const;

        bool                operator == (block_info_t const & rhs) const;
        bool                operator < (block_info_t const & rhs) const;

        bool                iplock_block();
        bool                iplock_unblock();

    private:
        bool                iplock(QString const & cmd);

        status_t            f_status = status_t::BLOCK_INFO_BANNED;
        QString             f_scheme = "http";
        QString             f_ip;
        QString             f_reason;
        int64_t             f_block_limit = 0LL;
        int64_t             f_ban_count = 0LL;
        int64_t             f_packet_count = 0LL;
        int64_t             f_byte_count = 0LL;
    };

                                snap_firewall( snap_firewall const & ) = delete;
    snap_firewall &             operator = ( snap_firewall const & ) = delete;

    void                        usage();
    void                        setup_firewall();
    void                        next_wakeup();
    void                        block_ip(snap::snap_communicator_message const & message);
    void                        unblock_ip(snap::snap_communicator_message const & message);

    advgetopt::getopt                           f_opt;
    snap::snap_config                           f_config;
    QString                                     f_log_conf = "/etc/snapwebsites/logger/snapfirewall.properties";
    QString                                     f_server_name;
    QString                                     f_communicator_addr = "127.0.0.1";
    int                                         f_communicator_port = 4040;
    snap_firewall_interrupt::pointer_t          f_interrupt;
    snap::snap_communicator::pointer_t          f_communicator;
    snap::snap_cassandra                        f_cassandra;
    libdbproxy::table::pointer_t                f_firewall_table;
    bool                                        f_stop_received = false;
    bool                                        f_debug = false;
    bool                                        f_firewall_up = false;
    messenger::pointer_t                        f_messenger;
    reconnect_timer::pointer_t                  f_reconnect_timer;
    wakeup_timer::pointer_t                     f_wakeup_timer;
    block_info_t::block_info_vector_t           f_blocks;       // save here until connected to Cassandra
};










/** \brief Initializes the reconnect timer with a pointer to the snap firewall.
 *
 * The constructor saves the pointer of the snap_firewall object so
 * it can later be used when the reconnect timer events occurs.
 *
 * By default the timer is "off" meaning that it will not trigger
 * a process_reconnect() call until you turn it on.
 *
 * \param[in] sfw  A pointer to the snap_firewall object.
 */
reconnect_timer::reconnect_timer(snap_firewall * sfw)
    : snap_timer(-1)
    , f_snap_firewall(sfw)
{
    set_name("snap_firewall reconnect_timer");
}


/** \brief The reconnect timer timed out.
 *
 * The reconnect timer is used to force a CASSANDRAREADY some time after
 * a failure in the setup_firewall() function happens.
 *
 * In most cases, this gets used when a timeout happens in the Cassandra
 * cluster. If the timeout happens while running the setup function, then
 * we do not want to try again immediately. Instead, we wait a little
 * while and send a CASSANDRAREADY message to snapdbproxy whenever this
 * function gets called.
 */
void reconnect_timer::process_timeout()
{
    f_snap_firewall->process_reconnect();
}





/** \brief Initializes the timer with a pointer to the snap firewall.
 *
 * The constructor saves the pointer of the snap_firewall object so
 * it can later be used when the process timeouts.
 *
 * By default the timer is "off" meaning that it will not trigger
 * a process_timeout() call until you turn it on.
 *
 * \param[in] sfw  A pointer to the snap_firewall object.
 */
wakeup_timer::wakeup_timer(snap_firewall * sfw)
    : snap_timer(-1)
    , f_snap_firewall(sfw)
{
    set_name("snap_firewall wakeup_timer");
}


/** \brief The wake up timer timed out.
 *
 * The wake up timer is used to know when we have to remove IP
 * addresses from the firewall. Adding happens at the start and
 * whenever another service tells us to add an IP. Removal,
 * however, we are on our own.
 *
 * Whenever an IP is added by a service, it is accompagned by a
 * time period it should be blocked for. This may be forever, however,
 * when the amount of time is not forever, the snapfirewall tool
 * needs to wake up at some point. Note that those times are saved in
 * the database so one can know when to remove IPs even across restart
 * (actually, on a restart we usually do the opposite, we refill the
 * firewall with existing IP addresses that have not yet timed out;
 * however, if this was not a full server restart, then we do removals
 * only.)
 *
 * Note that the messenger may receive an UNBLOCK command in which
 * case an IP gets removed immediately and the timer reset to the
 * next IP that needs to be removed as required.
 */
void wakeup_timer::process_timeout()
{
    f_snap_firewall->process_timeout();
}





/** \brief The interrupt initialization.
 *
 * The interrupt uses the signalfd() function to obtain a way to listen on
 * incoming Unix signals.
 *
 * Specifically, it listens on the SIGINT signal, which is the equivalent
 * to the Ctrl-C.
 *
 * \param[in] fw  The snap_firewall server we are listening for.
 */
snap_firewall_interrupt::snap_firewall_interrupt(snap_firewall * fw)
    : snap_signal(SIGINT)
    , f_snap_firewall(fw)
{
    unblock_signal_on_destruction();
    set_name("snapfirewall interrupt");
}


/** \brief Call the stop function of the snaplock object.
 *
 * When this function is called, the signal was received and thus we are
 * asked to quit as soon as possible.
 */
void snap_firewall_interrupt::process_signal()
{
    // we simulate the STOP, so pass 'false' (i.e. not quitting)
    //
    f_snap_firewall->stop(false);
}






/** \brief The messenger initialization.
 *
 * The messenger is a connection to the snapcommunicator server.
 *
 * In most cases we receive BLOCK, STOP, and LOG messages from it. We
 * implement a few other messages too (HELP, READY...)
 *
 * We use a permanent connection so if the snapcommunicator restarts
 * for whatever reason, we reconnect automatically.
 *
 * \note
 * The messenger connection used by the snapfirewall tool makes use
 * of a thread. You will want to change this initialization function
 * if you intend to fork() direct children of ours (i.e. not fork()
 * + execv() as we do to run iptables.)
 *
 * \param[in] sfw  The snap firewall server we are listening for.
 * \param[in] addr  The address to connect to. Most often it is 127.0.0.1.
 * \param[in] port  The port to connect to (4040).
 */
messenger::messenger(snap_firewall * sfw, std::string const & addr, int port)
    : snap_tcp_client_permanent_message_connection(addr, port)
    , f_snap_firewall(sfw)
{
    set_name("snap_firewall messenger");
}


/** \brief Pass messages to the Snap Firewall.
 *
 * This callback is called whenever a message is received from
 * Snap! Communicator. The message is immediately forwarded to the
 * snap_firewall object which is expected to process it and reply
 * if required.
 *
 * \param[in] message  The message we just received.
 */
void messenger::process_message(snap::snap_communicator_message const & message)
{
    f_snap_firewall->process_message(message);
}


/** \brief The messenger could not connect to snapcommunicator.
 *
 * This function is called whenever the messengers fails to
 * connect to the snapcommunicator server. This could be
 * because snapcommunicator is not running or because the
 * configuration information for the snapfirewall is wrong...
 *
 * With snapinit the snapcommunicator should always already
 * be running so this error should not happen once everything
 * is properly setup.
 *
 * \param[in] error_message  An error message.
 */
void messenger::process_connection_failed(std::string const & error_message)
{
    SNAP_LOG_ERROR("connection to snapcommunicator failed (")(error_message)(")");

    // also call the default function, just in case
    snap_tcp_client_permanent_message_connection::process_connection_failed(error_message);
}


/** \brief The connection was established with Snap! Communicator.
 *
 * Whenever the connection is established with the Snap! Communicator,
 * this callback function is called.
 *
 * The messenger reacts by REGISTERing the snap_firewall with the Snap!
 * Communicator. The name of the backend is taken from the action
 * it was called with.
 */
void messenger::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    snap::snap_communicator_message register_firewall;
    register_firewall.set_command("REGISTER");
    register_firewall.add_parameter("service", "snapfirewall");
    register_firewall.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_firewall);
}









snap_firewall::block_info_t::block_info_t(snap::snap_communicator_message const & message, status_t status)
{
    // retrieve scheme and IP
    //
    if(!message.has_parameter("uri"))
    {
        // TODO: create a snap_exception instead
        //
        throw std::runtime_error("a BLOCK message \"uri\" parameter is mandatory.");
    }

    set_uri(message.get_parameter("uri"));

    if(!message.has_parameter("period"))
    {
        // if period was not specified, block for a day
        //
        set_block_limit("day");
    }
    else
    {
        set_block_limit(message.get_parameter("period"));
    }

    if(message.has_parameter("reason"))
    {
        f_reason = message.get_parameter("reason");
    }

    f_status = status;
}


snap_firewall::block_info_t::block_info_t(QString const & uri)
{
    set_uri(uri);
    set_block_limit(QString());

    // TBD: call load() but then we need a pointer to server_name
    //      and the snapfirewall_table
}


/** \brief Check whether this block info is considered valid.
 *
 * A block info may be setup to an invalid IP address or some other
 * invalid parameter. In that case we may end up with an invalid
 * \p block_info_t object. For example, a local IP address is never
 * blocked by snapfirewall since the default set of rules already
 * blocks all local network IP addresses.
 *
 * This function returns true if the object is considered valid and
 * can be used for a block and saved in the database.
 *
 * \return true if valid.
 *
 * \sa set_uri()
 */
bool snap_firewall::block_info_t::is_valid() const
{
    return !f_ip.isEmpty();
}


void snap_firewall::block_info_t::save(libdbproxy::table::pointer_t firewall_table, QString const & server_name)
{
    if(!is_valid())
    {
        return;
    }

    // this is probably wrong, we may not want to save anything
    // if still undefined
    //
    if(f_status == status_t::BLOCK_INFO_UNDEFINED)
    {
        f_status = status_t::BLOCK_INFO_BANNED;
    }

    // we want to check the info row to see whether we had an old entry
    // because we have to remove it! (otherwise the block would stop
    // at the time the old entry was entered instead of the new time!)
    //
    libdbproxy::row::pointer_t info_row(firewall_table->getRow(QString("ip::%1").arg(f_ip)));
    QString const block_limit_key(QString("%1::block_limit").arg(server_name));

    // here we create a row if the item is banned
    // and we drop the row if the item got unbanned
    //
    // TODO: we need to handle the case where the item get re-banned before
    //       it gets unbanned; we need to delete the old one
    //
    {
        libdbproxy::row::pointer_t ban_row(firewall_table->getRow(server_name));

        // for a block, if the existing limit is further in the past,
        // then it accept the new block, if the existing limit is in
        // the future, then the old limit still applies and we ignore
        // the new limit; this happens if an IP is first block for 1 year
        // and later is blocked for 1 day (although once blocked an IP
        // should not trigger any more blocks, but it can easily happen
        // in a cluster)
        //
        // Timeline:
        // =========
        //
        //              +-- current block
        //              |
        //              v
        // -----+-------+-----------+------>
        //      ^                   ^
        //      |                   |
        //      |                   +--- new block in the future, accepted
        //      |
        //      +--- new block in the past, ignored
        //
        //
        // for an unblock, it does not apply because the unblock must always
        // happens now.
        //
        if(info_row->exists(block_limit_key))
        {
            libdbproxy::value old_limit_value(info_row->getCell(block_limit_key)->getValue());
            int64_t const old_limit(old_limit_value.safeInt64Value());

            if(f_status == status_t::BLOCK_INFO_BANNED
            && old_limit >= f_block_limit)
            {
                // it is already blocked for a longer time than the new
                // time, keep the longest
                //
                // TODO: should we still look into saving the reason?
                //       and ban counters?
                //
                return;
            }
            if(old_limit != f_block_limit)
            {
                // drop the old row
                //
                ban_row->dropCell(old_limit_value.binaryValue());
            }
        }

        QByteArray limit_value;
        libdbproxy::setInt64Value(limit_value, f_block_limit);
        if(f_status == status_t::BLOCK_INFO_BANNED)
        {
            ban_row->getCell(limit_value)->setValue(canonicalized_uri());
        }
        else
        {
            // Note: this does not seem useful with the new scheme since
            //       the cell should be dropped in the previous if() block
            //       unless `old_limit == f_block_limit`...
            //
            ban_row->dropCell(limit_value);
        }
    }

    {
        info_row->getCell(block_limit_key)->setValue(f_block_limit);
        info_row->getCell(QString("%1::status").arg(server_name))->setValue(QString(f_status == status_t::BLOCK_INFO_BANNED ? "banned" : "unbanned"));

        if(!f_reason.isEmpty())
        {
            QString const reason_key(QString("%1::reason").arg(server_name));
            if(info_row->exists(reason_key))
            {
                QString const old_reasons(info_row->getCell(reason_key)->getValue().stringValue());
                if(old_reasons != f_reason) // avoid an update (i.e. a tombstone) if same
                {
                    if(old_reasons.indexOf(f_reason) == -1)
                    {
                        // separate reasons with a "\n"
                        info_row->getCell(reason_key)->setValue(old_reasons + "\n" + f_reason);
                    }
                    else
                    {
                        info_row->getCell(reason_key)->setValue(f_reason);
                    }
                }
            }
            else
            {
                info_row->getCell(reason_key)->setValue(f_reason);
            }
        }

        // TODO: what we really want here are statistics such as # of packets
        //       per hour, etc. so we know whether we should extend the ban
        //       or remove it, dynamically. Also this will be part of the
        //       history of this IP address with our services.
        //
        // No lock is required to increase that counter because the counter
        // is specific to this computer and only one instance of snapfirewall
        // runs on one computer
        //
        if(f_ban_count > 0)
        {
            QString const ban_count_key(QString("%1::ban_count").arg(server_name));
            // add the existing value first
            //
            f_ban_count += info_row->getCell(ban_count_key)->getValue().safeInt64Value();
            info_row->getCell(ban_count_key)->setValue(f_ban_count);

            // since this counter is cumulative, we have to reset it to zero
            // each time otherwise we would double it each time we save
            //
            f_ban_count = 0;
        }
        if(f_packet_count > 0)
        {
            info_row->getCell(QString("%1::packet_count").arg(server_name))->setValue(f_packet_count);
        }
        if(f_byte_count > 0)
        {
            info_row->getCell(QString("%1::byte_count").arg(server_name))->setValue(f_byte_count);
        }

        // save when it was created / modified
        //
        int64_t const now(snap::snap_communicator::get_current_date());
        QString const created_key(QString("%1::created").arg(server_name));
        if(!info_row->exists(created_key))
        {
            info_row->getCell(created_key)->setValue(now);
        }
        info_row->getCell(QString("%1::modified").arg(server_name))->setValue(now);
    }
}


void snap_firewall::block_info_t::set_uri(QString const & uri)
{
    {
        int const pos(uri.indexOf("://"));
        if(pos > 0)
        {
            // there is a scheme and an IP
            //
            set_scheme(uri.mid(0, pos));
            set_ip(uri.mid(pos + 3));
        }
        else
        {
            // no scheme specified, directly use the IP
            //
            set_ip(uri);
        }
    }
}


void snap_firewall::block_info_t::set_ip(QString const & ip)
{
    // make sure IP is not empty
    //
    if(ip.isEmpty())
    {
        SNAP_LOG_ERROR("BLOCK without a URI (or at least an IP in the \"uri\" parameter.) BLOCK will be ignored.");
        return;
    }

    try
    {
        // at some point we could support "udp"?
        //
        // it does not matter much here, I would think, since we will ignore the
        // port from the addr object, we are just verifying the IP address
        //
        snap_addr::addr a(ip.toUtf8().data(), "", 123, "tcp");

        switch(a.get_network_type())
        {
        case snap_addr::addr::network_type_t::NETWORK_TYPE_UNDEFINED:
        case snap_addr::addr::network_type_t::NETWORK_TYPE_PRIVATE:
        case snap_addr::addr::network_type_t::NETWORK_TYPE_CARRIER:
        case snap_addr::addr::network_type_t::NETWORK_TYPE_LINK_LOCAL:
        case snap_addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK:
        case snap_addr::addr::network_type_t::NETWORK_TYPE_ANY:
            SNAP_LOG_ERROR("BLOCK with an unexpected IP address type in \"")(ip)("\". BLOCK will be ignored.");
            return;

        case snap_addr::addr::network_type_t::NETWORK_TYPE_MULTICAST:
        case snap_addr::addr::network_type_t::NETWORK_TYPE_PUBLIC: // == NETWORK_TYPE_UNKNOWN
            break;

        }
    }
    catch(snap_addr::addr_invalid_argument_exception const & e)
    {
        SNAP_LOG_ERROR("BLOCK with an invalid IP address in \"")(ip)("\". BLOCK will be ignored.");
        return;
    }

    f_ip = ip;
}


void snap_firewall::block_info_t::set_scheme(QString scheme)
{
    // verify the scheme
    //
    // scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    //
    // See:
    // https://tools.ietf.org/html/rfc3986#section-3.1
    //
    bool bad_scheme(false);
    int const max(scheme.length());
    if(max > 0)
    {
        int const c(scheme.at(0).unicode());
        if(c >= 'A' && c <= 'Z')
        {
            // transform to lowercase (canonicalization)
            //
            scheme[0] = c | 0x20;
        }
        else
        {
            bad_scheme = c < 'a' || c > 'z';
        }
    }
    if(!bad_scheme)
    {
        for(int idx(1); idx < max; ++idx)
        {
            int const c(scheme.at(idx).unicode());
            if(c >= 'A' && c <= 'Z')
            {
                // transform to lowercase (canonicalization)
                //
                scheme[idx] = c | 0x20;
            }
            else if((c < 'a' || c > 'z')
                 && (c < '0' || c > '9')
                 && c != '+'
                 && c != '-'
                 && c != '.')
            {
                bad_scheme = true;
                break;
            }
        }
    }

    // further we limit the length of the protocol to 20 characters
    //
    if(bad_scheme || scheme.length() > 20)
    {
        // invalid protocol, forget about the wrong one
        //
        // (i.e. an invalid protocol is not fatal at this point)
        //
        SNAP_LOG_ERROR("unsupported scheme \"")(scheme)("\" to block an IP address. We will use the default of \"http\".");
        scheme.clear();
    }

    if(scheme.isEmpty())
    {
        scheme = "http";
    }

    // now that we have a valid scheme, make sure there is a
    // corresponding iplock configuration file
    //
    std::string filename("/etc/iplock/schemes/");
    filename += scheme.toUtf8().data();
    filename += ".conf";
    if(access(filename.c_str(), F_OK) != 0)
    {
        filename = "/etc/iplock/schemes/schemes.d/";
        filename += scheme.toUtf8().data();
        filename += ".conf";
        if(access(filename.c_str(), F_OK) != 0)
        {
            if(scheme != "http")
            {
                // no message if http.conf does not exist; the iplock.conf
                // is the default and is to block HTTP so all good anyway
                //
                SNAP_LOG_WARNING("unsupported scheme \"")(scheme)("\" to block an IP address. The iplock default will be used.");
            }
            return;
        }
    }

    f_scheme = scheme;
}


void snap_firewall::block_info_t::set_block_limit(QString const & period)
{
    int64_t const now(snap::snap_communicator::get_current_date());
    if(!period.isEmpty())
    {
        // IMPORTANT NOTE: We have a "5min" period for test purposes
        //                 but do NOT document it because we do not
        //                 want to actually offer to anyone; blocking
        //                 an IP address for just 5min is a waste of
        //                 time, block it at least for 1 hour, probably
        //                 for 1 day or more
        //
        if(period == "5min")
        {
            f_block_limit = now + 5LL * 60LL * 1000000LL;
            return;
        }
        else if(period == "hour")
        {
            f_block_limit = now + 60LL * 60LL * 1000000LL;
            return;
        }
        else if(period == "day")
        {
            f_block_limit = now + 24LL * 60LL * 60LL * 1000000LL;
            return;
        }
        else if(period == "week")
        {
            f_block_limit = now + 7LL * 24LL * 60LL * 60LL * 1000000LL;
            return;
        }
        else if(period == "month")
        {
            f_block_limit = now + 31LL * 24LL * 60LL * 60LL * 1000000LL;
            return;
        }
        else if(period == "year")
        {
            f_block_limit = now + 366LL * 24LL * 60LL * 60LL * 1000000LL;
            return;
        }
        else if(period == "forever")
        {
            // 5 years is certainly very much like forever on the Internet!
            //
            f_block_limit = now + 5LL * 366LL * 24LL * 60LL * 60LL * 1000000LL;
            return;
        }
        else
        {
            // keep default of 1 day, but log an error
            //
            SNAP_LOG_ERROR("unknown period \"")(period)("\" to block an IP address. Revert to default of 1 day.");
        }
    }

    // default is now + 1 day
    //
    f_block_limit = now + 24LL * 60LL * 60LL * 1000000LL;
}


/** \brief Received the another ban on the same IP, so extend the duration.
 *
 * This should not happen since a first ban should prevent further access
 * from that one user and thus further sight of the IP.
 *
 * Yet it can happen if the scheme does not block all the ports and the
 * new scheme is "all". Note that the 'this' object will have its scheme
 * set to "all" if the scheme of \p block is "all".
 *
 * As a side effect, this function adds all the counters from \p block
 * to 'this' counters.
 *
 * \param[in] block  The block being checked against 'this' block.
 */
void snap_firewall::block_info_t::keep_longest(block_info_t const & block)
{
    if(block.f_scheme == "all"
    && f_scheme != "all")
    {
        // for obvious security reasons, we first block with the "all"
        // scheme then unblock with the specific scheme used by that
        // entry before the change
        //
        QString const old_scheme(f_scheme);
        f_scheme = "all";
        iplock_block();
        f_scheme = old_scheme;
        iplock_unblock();
        f_scheme = "all";
    }

    if(f_block_limit < block.f_block_limit)
    {
        f_block_limit = block.f_block_limit;
    }

    f_ban_count += block.f_ban_count;
    f_packet_count += block.f_packet_count;
    f_byte_count += block.f_byte_count;
}


void snap_firewall::block_info_t::set_ban_count(int64_t count)
{
    f_ban_count = count;
}


int64_t snap_firewall::block_info_t::get_ban_count() const
{
    return f_ban_count;
}


/** \brief Get the total number of bans that this IP received.
 *
 * \note
 * This is mainly for documentation at this point as we are more likely
 * to get the counter directly from the database without the pending
 * value that may be in the running snapfirewalls. also the grand
 * total would include all the computers and not just the one running.
 */
int64_t snap_firewall::block_info_t::get_total_ban_count(libdbproxy::table::pointer_t firewall_table, QString const & server_name) const
{
    // the total number of bans is the current counter plus the saved
    // counter so we have to retrieve the saved counter first
    //
    libdbproxy::row::pointer_t row(firewall_table->getRow(QString("ip::%1").arg(f_ip)));
    QString const ban_count_key(QString("%1::ban_count").arg(server_name));
    int64_t const saved_ban_count(row->getCell(ban_count_key)->getValue().safeInt64Value());

    return f_ban_count + saved_ban_count;
}


void snap_firewall::block_info_t::set_packet_count(int64_t count)
{
    f_packet_count = count;
}


int64_t snap_firewall::block_info_t::get_packet_count() const
{
    return f_packet_count;
}


void snap_firewall::block_info_t::set_byte_count(int64_t count)
{
    f_byte_count = count;
}


int64_t snap_firewall::block_info_t::get_byte_count() const
{
    return f_byte_count;
}


QString snap_firewall::block_info_t::canonicalized_uri() const
{
    // if no IP defined, return an empty string
    //
    if(f_ip.isEmpty())
    {
        return f_ip;
    }

    // if no scheme is defined (maybe it was invalid) then just return
    // the IP
    //
    if(f_scheme.isEmpty())
    {
        return f_ip;
    }

    // both scheme and IP are valid, return both
    //
    return f_scheme + "://" + f_ip;
}


QString snap_firewall::block_info_t::get_scheme() const
{
    return f_scheme;
}


QString snap_firewall::block_info_t::get_ip() const
{
    return f_ip;
}


int64_t snap_firewall::block_info_t::get_block_limit() const
{
    return f_block_limit;
}


/** \brief Check whether two block_info_t objects are considered equal.
 *
 * Note that the test compares the scheme and the ip. If either one of
 * the block_info_t objects has as the scheme "all", then it automatically
 * matches the other scheme.
 *
 * \param[in] rhs  The right hand side object to test.
 *
 * \return true if both info objects are considered equal.
 */
bool snap_firewall::block_info_t::operator == (block_info_t const & rhs) const
{
    if(f_scheme == "all"
    || rhs.f_scheme == "all")
    {
        return f_ip == rhs.f_ip;
    }

    return f_scheme == rhs.f_scheme
        && f_ip == rhs.f_ip;
}


bool snap_firewall::block_info_t::operator < (block_info_t const & rhs) const
{
    return f_block_limit < rhs.f_block_limit;
}


bool snap_firewall::block_info_t::iplock_block()
{
    f_status = status_t::BLOCK_INFO_BANNED;
    return iplock("--block");
}


bool snap_firewall::block_info_t::iplock_unblock()
{
    f_status = status_t::BLOCK_INFO_UNBANNED;
    return iplock("--unblock");
}


bool snap_firewall::block_info_t::iplock(QString const & cmd)
{
    if(!is_valid())
    {
        // the IP or period are missing
        return false;
    }

    QString command("iplock ");

    snap::process iplock_process("block/unblock an IP address");
    iplock_process.set_command("iplock");

    // whether we block or unblock the specified IP address
    iplock_process.add_argument(cmd);
    iplock_process.add_argument(f_ip);

    command += cmd + " " + f_ip;

    // once we have support for configuration files and varying schemes
    if(!f_scheme.isEmpty())
    {
        iplock_process.add_argument("--scheme");
        iplock_process.add_argument(f_scheme);

        command += " --scheme ";
        command += f_scheme;
    }

    // keep the stderr output
    iplock_process.add_argument("2>&1");

    int const r(iplock_process.run());
    if(r != 0)
    {
        // Note: if the IP was not already defined, this command
        //       generates an error
        //
        int const e(errno);
        QString const output(iplock_process.get_output(true));
        SNAP_LOG_ERROR("an error occurred (")
                      (r)
                      (") trying to run \"")
                      (command)
                      ("\", errno: ")
                      (e)
                      (" -- ")
                      (strerror(e))
                      ("\nConsole output:\n")
                      (output);
        return false;
    }

    return true;
}







/** \brief List of configuration files.
 *
 * This variable is used as a list of configuration files. It is
 * empty here because the configuration file may include parameters
 * that are not otherwise defined as command line options.
 */
std::vector<std::string> const g_configuration_files; // Empty


/** \brief Command line options.
 *
 * This table includes all the options supported by the server.
 */
advgetopt::getopt::option const g_snapfirewall_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "Usage: %p [-<opt>]",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "where -<opt> is one or more of:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        'c',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        nullptr,
        "Configuration file to initialize snapfirewall.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "debug",
        nullptr,
        "Start the snapfirewall in debug mode.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "Show usage and exit.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'l',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "logfile",
        nullptr,
        "Full path to the snapfirewall logfile.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'n',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "nolog",
        nullptr,
        "Only output to the console, not a log file.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "show the version of %p and exit.",
        advgetopt::getopt::argument_mode_t::no_argument
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



/** \brief This function initialize a snap_firewall object.
 *
 * The constructor puts in place the command line options by
 * parsing them. Also if the user specified --help or
 * --version, then the corresponding data is printed and
 * the process ends immediately.
 *
 * As we are at it, we also load the configuration file and
 * setup the logger.
 *
 * \param[in] argc  The command line argc parameter.
 * \param[in] argv  The command line argv parameter.
 */
snap_firewall::snap_firewall( int argc, char * argv[] )
    : f_opt(argc, argv, g_snapfirewall_options, g_configuration_files, "SNAPFIREWALL_OPTIONS")
    , f_config("snapfirewall")
{
    if(f_opt.is_defined("help"))
    {
        usage();
        snap::NOTREACHED();
    }

    if(f_opt.is_defined("version"))
    {
        std::cout << SNAPFIREWALL_VERSION_STRING << std::endl;
        exit(0);
        snap::NOTREACHED();
    }

    f_debug = f_opt.is_defined("debug");

    // read the configuration file
    //
    if(f_opt.is_defined("config"))
    {
        f_config.set_configuration_path( f_opt.get_string("config") );
    }

    // setup the logger
    //
    if( f_opt.is_defined( "nolog" ) )
    {
        snap::logging::configure_console();
    }
    else if( f_opt.is_defined("logfile") )
    {
        snap::logging::configure_logfile( QString::fromUtf8(f_opt.get_string( "logfile" ).c_str()) );
    }
    else
    {
        if( f_config.has_parameter("log_config") )
        {
            // use .conf definition when available
            f_log_conf = f_config["log_config"];
        }
        snap::logging::configure_conffile( f_log_conf );
    }

    if( f_debug )
    {
        // Force the logger level to DEBUG
        // (unless already lower)
        //
        snap::logging::reduce_log_output_level( snap::logging::log_level_t::LOG_LEVEL_DEBUG );
    }

    // do not do too much in the constructor or we may get in
    // trouble (i.e. calling shared_from_this() from the
    // constructor fails)
}


/** \brief Clean up the snap firewall.
 *
 * This function is used to do some clean up of the snap firewall
 * environment.
 */
snap_firewall::~snap_firewall()
{
    f_communicator.reset();
}


/** \brief Print out the usage information for snapfirewall.
 *
 * This function returns the snapfirewall usage information to the
 * user whenever an invalid command line option is used or
 * --help is used explicitly.
 *
 * The function does not return.
 */
void snap_firewall::usage()
{
    f_opt.usage( advgetopt::getopt::status_t::no_error, "snapfirewall" );
    snap::NOTREACHED();
}




/** \brief Execute the firewall run() loop.
 *
 * This function initializes the various connections used by the
 * snapfirewall process and then runs the event loop.
 *
 * In effect, this function finishes the initialization of the
 * snap_firewall object.
 */
void snap_firewall::run()
{
    // Stop on these signals, log them, then terminate.
    //
    signal( SIGSEGV, snap_firewall::sighandler );
    signal( SIGBUS,  snap_firewall::sighandler );
    signal( SIGFPE,  snap_firewall::sighandler );
    signal( SIGILL,  snap_firewall::sighandler );
    signal( SIGTERM, snap_firewall::sighandler );
    signal( SIGINT,  snap_firewall::sighandler );
    signal( SIGQUIT, snap_firewall::sighandler );

    // ignore console signals
    //
    signal( SIGTSTP,  SIG_IGN );
    signal( SIGTTIN,  SIG_IGN );
    signal( SIGTTOU,  SIG_IGN );

    // get the server name
    //
    f_server_name = QString::fromUtf8(snap::server::get_server_name().c_str());

    SNAP_LOG_INFO("--------------------------------- snapfirewall started on ")(f_server_name);

    // retrieve the snap communicator information
    //
    tcp_client_server::get_addr_port(QString::fromUtf8(f_config("snapcommunicator", "local_listen").c_str()), f_communicator_addr, f_communicator_port, "tcp");

    // initialize the communicator and its connections
    //
    f_communicator = snap::snap_communicator::instance();

    f_interrupt.reset(new snap_firewall_interrupt(this));
    f_communicator->add_connection(f_interrupt);

    f_reconnect_timer.reset(new reconnect_timer(this));
    f_communicator->add_connection(f_reconnect_timer);

    f_wakeup_timer.reset(new wakeup_timer(this));
    f_communicator->add_connection(f_wakeup_timer);

    f_messenger.reset(new messenger(this, f_communicator_addr.toUtf8().data(), f_communicator_port));
    f_communicator->add_connection(f_messenger);

    f_communicator->run();
}


/** \brief Setup the firewall on startup.
 *
 * On startup we have to assume that the firewall is not yet properly setup
 * so we run the follow process once.
 *
 * The process gets all the IPs defined in the database and:
 *
 * \li unblock the addresses which timed out
 * \li unblock and (re-)block addresses that are not out of date
 *
 * The unblock and re-block process is necessary in case you are restarting
 * the process. The problem is that the IP address may already be in your
 * firewall. If that's the case, just blocking would duplicate it, which
 * would slow down the firewall for nothing and also would not properly
 * unblock the IP when we receive the timeout because that process would
 * only unblock one instance.
 */
void snap_firewall::setup_firewall()
{
    // make sure we are also connected with the Cassandra database
    //
    if(!f_firewall_table)
    {
        return;
    }

    int64_t const now(snap::snap_communicator::get_current_date());
    int64_t const limit(now + 60LL * 1000000LL); // "lose" 1 min. precision

    libdbproxy::row::pointer_t row(f_firewall_table->getRow(f_server_name));
    row->clearCache();

    // the first row we keep has a date we use to know when to wake up
    // next and drop that IP from our firewall
    //
    bool first(true);

    block_info_t::block_info_vector_t to_block_list;

    // run through the entire table
    //
    auto column_predicate(std::make_shared<libdbproxy::cell_range_predicate>());
    column_predicate->setCount(100);
    column_predicate->setIndex(); // behave like an index
    for(;;)
    {
        row->readCells(column_predicate);
        libdbproxy::cells const cells(row->getCells());
        if(cells.isEmpty())
        {
            // it looks like we are done
            break;
        }

        for(libdbproxy::cells::const_iterator it(cells.begin());
                                                         it != cells.end();
                                                         ++it)
        {
            libdbproxy::cell::pointer_t cell(*it);

            // first we want to unblock that IP address
            //
            QString const uri(cell->getValue().stringValue());

            try
            {
                // this one should always work since we saved it in the
                // database, only between versions the format could change
                //
                block_info_t info(uri);

                QByteArray const key(it.key());
                int64_t const drop_date(libdbproxy::safeInt64Value(key, 0, -1));
                if(drop_date < limit)
                {
                    // unblock the IP, just in case
                    //
                    //info.iplock_unblock();
                    SNAP_LOG_TRACE( "No longer blocking ip address '")(info.get_ip())("'");

                    // save with the new status of UNBANNED
                    //
                    info.save(f_firewall_table, f_server_name);

                    // now drop that row
                    //
                    // Note: the save() does that for new keys, old keys may
                    //       not get deleted properly so I kept this code
                    //       for now... generally speaking, it is safer
                    //       to have it here anyway
                    //
                    row->dropCell(key);
                }
                else
                {
                    // this IP is still expected to be blocked, so
                    // re-block it
                    //
                    if(first)
                    {
                        // on the first one, we want to mark that as the
                        // time when the block has to be dropped
                        //
                        // Note: only the first one is necessary since these
                        //       are sorted by date in the database
                        //
                        first = false;
                        f_wakeup_timer->set_timeout_date(drop_date);
                    }

                    // block the IP
                    //
                    //info.iplock_block();
                    to_block_list.push_back( info );

                    // show all the IPs (not useful at all time to reprint
                    // all the IPs, it should go a lot faster by not doing
                    // so.)
                    //
                    //SNAP_LOG_TRACE( "Add to block list address='")(info.get_ip())("', scheme='")(info.get_scheme())("'.");

                    // no save necessary, it is already as it needs to be
                }
            }
            catch(std::exception const & e)
            {
                SNAP_LOG_ERROR("an exception occurred while initializing the firewall: ")(e.what());
            }
        }
    }

    std::for_each(
              f_blocks.begin()
            , f_blocks.end()
            , [&, limit](block_info_t & info)
            {
                if(limit >= info.get_block_limit())
                {
                    // passed the limit already so we can unblock now
                    //
                    //info.iplock_unblock();
                }
                else
                {
                    to_block_list.push_back( info );

                    // show all the pending IPs
                    //
                    //SNAP_LOG_TRACE( "Add pending IP address to block list: '")(info.get_ip())("'");
                }

                // always save the IP so we know that such and such was
                // banned before (i.e. recidivists can be counted now)
                //
                info.save(f_firewall_table, f_server_name);
            }
        );

    SNAP_LOG_INFO("Block ")(to_block_list.size())(" IPs (including ")(f_blocks.size())(" from the pending IP address list).");

    f_blocks.clear();

    std::string const private_folder("/var/cache/snapwebsites/private");
    QDir pf( private_folder.c_str() );
    if( !pf.exists() )
    {
        pf.mkdir( private_folder.c_str() );
        if( ::chmod( private_folder.c_str(), 0700 ) != 0 )
        {
            // this should not happen, but at least let admins know
            //
            int const e(errno);
            SNAP_LOG_WARNING("chmod(\"")(private_folder)(", 0700\") failed. (errno: ")(e)(", ")(strerror(e));
        }
    }

    std::stringstream ss;
    ss << private_folder << "/iplock." << getpid();
    std::string const outfile(ss.str());
    {
        std::ofstream ip_list( outfile );

        for( auto const & info : to_block_list )
        {
            ip_list << info.get_ip().toUtf8().data()
                    << " "
                    << info.get_scheme().toUtf8().data()
                    << std::endl;
        }
    }

    // Run the iplock process, but in batch mode.
    //
    {
        snap::process iplock_process("block bulk IP address");
        iplock_process.set_command("iplock");

        // whether we block or unblock the specified IP address
        iplock_process.add_argument("--batch");
        iplock_process.add_argument(outfile.c_str());

        // keep the stderr output
        iplock_process.add_argument("2>&1");

        int const r(iplock_process.run());
        if(r != 0)
        {
            // Note: if the IP was not already defined, this command
            //       generates an error
            //
            int const e(errno);
            QString const output(iplock_process.get_output(true));
            SNAP_LOG_ERROR("an error occurred (")
                    (r)
                    (") trying to run \"")
                    (iplock_process.get_name())
                    ("\", errno: ")
                    (e)
                    (" -- ")
                    (strerror(e))
                    ("\nConsole output:\n")
                    (output);
        }
    }

    f_firewall_up = true;

#ifndef MO_DEBUG
    // Only remove if we are not in debug mode
    //
    unlink( outfile.c_str() );
#endif

    // send a "FIREWALLUP" message to let others know that the firewall
    // is up
    //
    // TODO
    // some daemons, like snapserver does, should wait on that
    // signal before starting... (but snapfirewall is optional,
    // so be careful on how you handle that one! in snapserver
    // we first check whehter snapfirewall is active on the
    // computer and if so request the message.)
    //
    snap::snap_communicator_message firewallup_message;
    firewallup_message.set_command("FIREWALLUP");
    firewallup_message.set_service(".");
    f_messenger->send_message(firewallup_message);
}


/** \brief Timeout is called whenever an IP address needs to be unblocked.
 *
 * This function is called when the wakeup timer times out. We set the
 * date when the wakeup timer has to time out to the next IP that
 * times out. That information comes from the Cassandra database.
 *
 * Certain IP addresses are permanently added to the firewall,
 * completely preventing the offender from accessing us for the
 * rest of time.
 */
void snap_firewall::process_timeout()
{
    // STOP received?
    // the timer may still tick once after we received a STOP event
    // so we want to check here to make sure we are good.
    //
    if(f_stop_received)
    {
        // TBD: note that this means we are not going to unblock any
        //      old IP block if we already received a STOP...
        return;
    }

    int64_t const now(snap::snap_communicator::get_current_date());

    f_blocks.erase(
            std::remove_if(
                  f_blocks.begin()
                , f_blocks.end()
                , [&, now](block_info_t & info)
                {
                    if(now > info.get_block_limit())
                    {
                        // this one timed out, remove from the
                        // firewall and the f_blocks vector
                        // (so in effect we "lose" that IP information
                        // but we do not want to use too much RAM either;
                        // in a properly setup system it should be really
                        // rare)
                        //
                        info.iplock_unblock();

                        return true;
                    }

                    return false;
                }
            )
            , f_blocks.end()
        );

    // make sure we are connected to cassandra
    //
    if(f_firewall_table)
    {
        // we are interested only by the columns that concern us, which
        // means columns that have a name starting with the server name
        // as defined in the snapserver.conf file
        //
        //      <server-name> '/' <date with leading zeroes in minutes (10 digits)>
        //

        libdbproxy::row::pointer_t row(f_firewall_table->getRow(f_server_name));
        row->clearCache();

        // unblock IP addresses which have a timeout in the past
        //
        auto column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
        QByteArray limit;
        libdbproxy::setInt64Value(limit, 0);  // whatever the first column is
        column_predicate->setStartCellKey(limit);
        libdbproxy::setInt64Value(limit, now + 60LL * 1000000LL);  // until now within 1 minute
        column_predicate->setEndCellKey(limit);
        column_predicate->setCount(100);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            row->readCells(column_predicate);
            libdbproxy::cells const cells(row->getCells());
            if(cells.isEmpty())
            {
                // it looks like we are done
                break;
            }

            // any entries we grab here, we drop right now
            //
            for(libdbproxy::cells::const_iterator it(cells.begin());
                                                             it != cells.end();
                                                             ++it)
            {
                libdbproxy::cell::pointer_t cell(*it);

                // first we want to unblock that IP address
                //
                QString const uri(cell->getValue().stringValue());

                try
                {
                    // remove the block, it timed out
                    //
                    block_info_t info(uri);
                    info.iplock_unblock();

                    // save the entry with the new status
                    //
                    info.save(f_firewall_table, f_server_name);

                    // now drop that row
                    //
                    // Note: the save() does that for new keys, old keys may
                    //       not get deleted properly so I kept this code
                    //       for now...
                    //
                    QByteArray const key(cell->columnKey());
                    row->dropCell(key);
                }
                catch(std::exception const & e)
                {
                    SNAP_LOG_ERROR("an exception occurred while checking IPs in the process_timeout() function: ")(e.what());
                }
            }
        }
    }

    next_wakeup();
}


/** \brief Restart process to reconnect.
 *
 * The setup_firewall() function failed and set the reconnect_timer to
 * get this function called a little later.
 *
 * Here we simply send a CASSANDRASTATUS message to snapdbproxy to
 * get things restarted.
 */
void snap_firewall::process_reconnect()
{
    is_db_ready();
}


/** \brief Send the CASSANDRASTATUS to snapdbproxy.
 *
 * This function builds a message and sends it to snapdbproxy. It is
 * used whenever we need to know whether the database is accessible.
 *
 * Note that the function itself does not return true or false. If
 * you need to know whether we are currently connected to the
 * snapdbproxy daemon, check the f_cassandra pointer; if not nullptr
 * then we are connected and you can send a CQL order.
 */
void snap_firewall::is_db_ready()
{
    snap::snap_communicator_message isdbready_message;
    isdbready_message.set_command("CASSANDRASTATUS");
    isdbready_message.set_service("snapdbproxy");
    f_messenger->send_message(isdbready_message);
}


/** \brief Called whenever the firewall table changes.
 *
 * Whenever the firewall table changes, the next wake up date may change.
 * This function makes sure to determine what the smallest date is and
 * saves that in the wakeup timer if such a smaller date exists.
 *
 * \note
 * At this time, the setup() function does this on its own since it has
 * the information without the need for yet another access to the
 * database.
 */
void snap_firewall::next_wakeup()
{
    // by default there is nothing to wake up for
    //
    int64_t limit(0LL);
    if(f_firewall_table)
    {
        libdbproxy::row::pointer_t row(f_firewall_table->getRow(f_server_name));

        // determine whether there is another IP in the table and if so at
        // what time we need to wake up to remove it from the firewall
        //
        auto column_predicate(std::make_shared<libdbproxy::cell_range_predicate>());
        column_predicate->setCount(1);
        column_predicate->setIndex(); // behave like an index
        row->clearCache();
        row->readCells(column_predicate);
        libdbproxy::cells const cells(row->getCells());
        if(!cells.isEmpty())
        {
            QByteArray const key(cells.begin().key());
            limit = libdbproxy::safeInt64Value(key, 0, -1);
        }
        else
        {
            // no entries means no need to wakeup
            //
            limit = 0;
        }
    }
    else if(!f_blocks.empty())
    {
        // each time we add an entry to f_blocks, we re-sort the vector
        // so the first entry is always the smallest
        //
        limit = f_blocks.front().get_block_limit();
    }

    if(limit > 0)
    {
        // we have a valid date to wait on,
        // save it in our wakeup timer
        //
        f_wakeup_timer->set_timeout_date(limit);
    }
    //else -- there is nothing to wake up for...
}


/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever the Snap! Communicator sends
 * us a message. This includes the READY and HELP commands, although
 * the most important one is certainly the BLOCK and STOP commands
 * used to block an IP address for a given period of time and the
 * request for this process to STOP as soon as possible.
 *
 * \param[in] message  The message we just received.
 */
void snap_firewall::process_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("received messenger message [")(message.to_message())("] for ")(f_server_name);

    QString const command(message.get_command());

// TODO: make use of a switch() or even better: a map a la snapinit -- see SNAP-464

    if(command == "BLOCK")
    {
        // BLOCK an ip address
        //
        block_ip(message);
        return;
    }

    if(command == "UNBLOCK")
    {
        // UNBLOCK an ip address
        //
        unblock_ip(message);
        return;
    }

    if(command == "LOG")
    {
        // logrotate just rotated the logs, we have to reconfigure
        //
        SNAP_LOG_INFO("Logging reconfiguration.");
        snap::logging::reconfigure();
        return;
    }

    if(command == "STOP")
    {
        // Someone is asking us to leave (probably snapinit)
        //
        stop(false);
        return;
    }
    if(command == "QUITTING")
    {
        // If we received the QUITTING command, then somehow we sent
        // a message to Snap! Communicator, which is already in the
        // process of quitting... we should get a STOP too, but we
        // can just quit ASAP too
        //
        stop(true);
        return;
    }

    if(command == "READY")
    {
        // Snap! Communicator received our REGISTER command
        //

        // request snapdbproxy to send us a status signal about
        // Cassandra, after that one call, we will receive the
        // statuses just because we understand them.
        //
        is_db_ready();

        return;
    }

    if(command == "NOCASSANDRA")
    {
        // we lost Cassandra, disconnect from snapdbproxy until we
        // get CASSANDRAREADY again
        //
        f_cassandra.disconnect();
        f_firewall_table.reset();

        return;
    }

    if(command == "CASSANDRAREADY")
    {
        try
        {
            // connect to Cassandra and get a pointer to our firewall table
            //
            f_cassandra.connect();
            f_firewall_table = f_cassandra.get_table("firewall");

            // now that we are fully registered, setup the firewall
            //
            setup_firewall();
        }
        catch(std::runtime_error const & e)
        {
            SNAP_LOG_WARNING("failed to connect to snapdbproxy: ")(e.what());

            // make sure the table is not defined
            //
            f_cassandra.disconnect();
            f_firewall_table.reset();

            // in this particular case, we do not automatically get
            // another CASSANDRAREADY message so we have to send another
            // CASSANDRASTATUS at some point, but we want to give Cassandra
            // a break for a little while and thus ask to be awaken in
            // 30 seconds before we try again
            //
            int64_t const now(snap::snap_communicator::get_current_date());
            int64_t const reconnect_date(now + 30LL * 1000000LL);
            f_reconnect_timer->set_timeout_date(reconnect_date);
        }

        return;
    }

    if(command == "FIREWALLSTATUS")
    {
        // someone is asking us whether we are ready, reply with
        // the corresponding answer and make sure not to cache
        // the answer because it could change later (i.e. snapfirewall
        // restarts, for example.)
        //
        snap::snap_communicator_message firewallup_message;
        firewallup_message.reply_to(message);
        firewallup_message.set_command(f_firewall_up ? "FIREWALLUP" : "FIREWALLDOWN");
        firewallup_message.add_parameter("cache", "no");
        f_messenger->send_message(firewallup_message);

        return;
    }

    if(command == "HELP")
    {
        // Snap! Communicator is asking us about the commands that we support
        //
        snap::snap_communicator_message reply;
        reply.set_command("COMMANDS");

        // list of commands understood by service
        //
        reply.add_parameter("list", "BLOCK,CASSANDRAREADY,FIREWALLSTATUS,HELP,LOG,NOCASSANDRA,QUITTING,READY,STOP,UNBLOCK,UNKNOWN");

        f_messenger->send_message(reply);

        return;
    }

    if(command == "UNKNOWN")
    {
        // we sent a command that Snap! Communicator did not understand
        //
        SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
        return;
    }

    // unknown command is reported and process goes on
    //
    SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received on the connection with Snap! Communicator.");
    {
        snap::snap_communicator_message reply;
        reply.set_command("UNKNOWN");
        reply.add_parameter("command", command);
        f_messenger->send_message(reply);
    }

    return;
}


/** \brief Called whenever we receive the STOP command or equivalent.
 *
 * This function makes sure the snapfirewall exits as quickly as
 * possible.
 *
 * \li Marks the messenger as done.
 * \li Disabled wakeup timer.
 * \li UNREGISTER from snapcommunicator.
 * \li Remove wakeup timer from snapcommunicator.
 *
 * \note
 * If the f_messenger is still in place, then just sending the
 * UNREGISTER is enough to quit normally. The socket of the
 * f_messenger will be closed by the snapcommunicator server
 * and we will get a HUP signal. However, we get the HUP only
 * because we first mark the messenger as done.
 *
 * \param[in] quitting  Set to true if we received a QUITTING message.
 */
void snap_firewall::stop(bool quitting)
{
    f_stop_received = true;

    // stop the timers immediately, although that will not prevent
    // one more call to their callbacks which thus still have to
    // check the f_stop_received flag
    //
    if(f_reconnect_timer != nullptr)
    {
        f_reconnect_timer->set_enable(false);
        f_reconnect_timer->set_timeout_date(-1);
    }
    if(f_wakeup_timer != nullptr)
    {
        f_wakeup_timer->set_enable(false);
        f_wakeup_timer->set_timeout_date(-1);
    }

    if(f_messenger != nullptr)
    {
        if(quitting || !f_messenger->is_connected())
        {
            // turn off that connection now, we cannot UNREGISTER since
            // we are not connected to snapcommunicator
            //
            f_communicator->remove_connection(f_messenger);
            f_messenger.reset();
        }
        else
        {
            f_messenger->mark_done();

            // unregister if we are still connected to the messenger
            // and Snap! Communicator is not already quitting
            //
            snap::snap_communicator_message cmd;
            cmd.set_command("UNREGISTER");
            cmd.add_parameter("service", "snapfirewall");
            f_messenger->send_message(cmd);
        }
    }

    if(f_communicator != nullptr)
    {
        //f_communicator->remove_connection(f_messenger); -- this one will get an expected HUP shortly
        f_communicator->remove_connection(f_reconnect_timer);
        f_communicator->remove_connection(f_wakeup_timer);
        f_communicator->remove_connection(f_interrupt);
    }
}



void snap_firewall::block_ip(snap::snap_communicator_message const & message)
{
    // message data could be tainted, we need to protect ourselves against
    // unwanted exceptions
    //
    try
    {
        // check the "uri" and "period" parameters
        //
        // the URI may include a protocol and an IP separated by "://"
        // if no "://" appears, then only an IP is expected
        //
        block_info_t info(message);
        info.set_ban_count( /*info.get_ban_count() +*/ 1); // newly created ban count is always 0, so just set to 1

        // save in our list of blocked IP addresses
        //
        if(f_firewall_table != nullptr)
        {
            // actually add to the firewall
            //
            info.iplock_block();

            info.save(f_firewall_table, f_server_name);
        }
        else
        {
            // cache in memory for later, once we connect to Cassandra,
            // we will save those in cassandra
            //
            // TODO: I do not, right now, think that we could have such an
            //       attack that memory would be a problem because some of
            //       the largest DDoS only make use of 10 to 20,000 IPs
            //       Even a 50,000 IPs attack is just not quite likely
            //       before you connect to the databse unless somehow
            //       snapfirewall never gets a connection...
            //
            auto const & it(std::find(
                      f_blocks.begin()
                    , f_blocks.end()
                    , info));
            if(it == f_blocks.end())
            {
                // block the IP now
                //
                info.iplock_block();

                // this is a new block, keep it as is
                //
                f_blocks.push_back(info);
            }
            else
            {
                // there is a matching old block, keep the new info in
                // the old block but update as required
                //
                it->keep_longest(info);

                // no need to block the IP, it already is
                //
                // (Note: it may have changed from some scheme to "all"
                //        inside the keep_longest() function...)
            }

            // keep them sorted, as in the Cassandra database
            //
            // even if we do not push a new entry, the keep_longest()
            // may end up changing the order of the existing items...
            //
            std::sort(f_blocks.begin(), f_blocks.end());
        }

        next_wakeup();
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_ERROR("an exception occurred while checking the BLOCK message in the block_ip() function: ")(e.what());

        // we probably should not catch all exceptions here (i.e. on a
        // bad_alloc, we probably want to quit...)
        //
        // in any event, this probably means we just lost our connections
        // and need to try to reconnect
        //
        f_cassandra.disconnect();
        f_firewall_table.reset();

        // check with snapdbproxy whether it is still connected or not
        //
        is_db_ready();
    }
}


void snap_firewall::unblock_ip(snap::snap_communicator_message const & message)
{
    // message data could be tainted, we need to protect ourselves against
    // unwanted exceptions
    //
    try
    {
        // check the "uri" and "period" parameters
        //
        // the URI may include a protocol and an IP separated by "://"
        // if no "://" appears, then only an IP is expected
        //
        block_info_t info(message);

        // remove from the firewall
        //
        info.iplock_unblock();

        // save in our list of blocked IP addresses
        //
        if(f_firewall_table != nullptr)
        {
            info.save(f_firewall_table, f_server_name);
        }
        else
        {
            // find the block in the cache, it should be there unless we
            // lost the connection with the Cassandra cluster
            //
            auto const & it(std::find(
                      f_blocks.begin()
                    , f_blocks.end()
                    , info));
            if(it != f_blocks.end())
            {
                // by erasing the info we lose that data, but that only
                // happens when we are not connected to the database;
                // the connection to the database should happen very
                // quickly so most blocks will not be removed before
                // they get saved
                //
                f_blocks.erase(it);
            }
        }

        next_wakeup();
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_ERROR("an exception occurred while checking the UNBLOCK message in the unblock_ip() function: ")(e.what());

        // we probably should not catch all exceptions here (i.e. on a
        // bad_alloc, we probably want to quit...)
        //
        // in any event, this probably means we just lost our connections
        // and need to try to reconnect
        //
        f_cassandra.disconnect();
        f_firewall_table.reset();

        // check with snapdbproxy whether it is still connected or not
        //
        is_db_ready();
    }
}


/** \brief A static function to capture various signals.
 *
 * This function captures unwanted signals like SIGSEGV and SIGILL.
 *
 * The handler logs the information and then the service exists.
 * This is done mainly so we have a chance to debug problems even
 * when it crashes on a remote server.
 *
 * \warning
 * The signals are setup after the construction of the snap_firewall
 * object because that's where we initialize the logger.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void snap_firewall::sighandler( int sig )
{
    QString signame;
    bool show_stack(true);
    switch( sig )
    {
    case SIGSEGV:
        signame = "SIGSEGV";
        break;

    case SIGBUS:
        signame = "SIGBUS";
        break;

    case SIGFPE:
        signame = "SIGFPE";
        break;

    case SIGILL:
        signame = "SIGILL";
        break;

    case SIGTERM:
        signame = "SIGTERM";
        show_stack = false;
        break;

    case SIGINT:
        signame = "SIGINT";
        show_stack = false;
        break;

    case SIGQUIT:
        signame = "SIGQUIT";
        show_stack = false;
        break;

    default:
        signame = "UNKNOWN";
        break;

    }

    if(show_stack)
    {
        snap::snap_exception_base::output_stack_trace();
    }
    SNAP_LOG_FATAL("Fatal signal caught: ")(signame);

    // Exit with error status
    //
    ::exit( 1 );
    snap::NOTREACHED();
}






} // no name namespace


int main(int argc, char * argv[])
{
    try
    {
        // create an instance of the snap_firewall object
        //
        snap_firewall firewall( argc, argv );

        // Now run!
        //
        firewall.run();

        // exit normally (i.e. we received a STOP message on our
        // connection with the Snap! Communicator service.)
        //
        return 0;
    }
    catch( snap::snap_exception const & e )
    {
        SNAP_LOG_FATAL("snapfirewall: snap_exception caught! ")(e.what());
    }
    catch( std::invalid_argument const & e )
    {
        SNAP_LOG_FATAL("snapfirewall: invalid argument: ")(e.what());
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_FATAL("snapfirewall: std::exception caught! ")(e.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snapfirewall: unknown exception caught!");
    }

    return 1;
}


// vim: ts=4 sw=4 et
