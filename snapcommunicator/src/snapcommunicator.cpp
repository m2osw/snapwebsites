// Snap Websites Server -- server to handle inter-process communication
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
#include <snapwebsites/addr.h>
#include <snapwebsites/chownnm.h>
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/loadavg.h>
#include <snapwebsites/log.h>
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/tokenize_string.h>

// Qt lib
//
#include <QFile>

// C++ lib
//
#include <atomic>
#include <cmath>
#include <fstream>
#include <thread>

// C lib
//
#include <grp.h>
#include <pwd.h>
#include <sys/resource.h>

// included last
//
#include <snapwebsites/poison.h>


/** \file
 * \brief Implementation of the snap inter-process communication.
 *
 * This file is the binary we start to allow inter-process communication
 * between front and back end processes on all computers within a Snap
 * cluster.
 *
 * The idea is to have ONE inter-process communicator server running
 * PER computer. These then communicate between each others and are
 * used to send messages between each process that registered with
 * them.
 *
 * This means if you want to send a signal (i.e. PING) to the "images"
 * backend, you connect with this inter-process communicator on your
 * computer, and send the PING command to that process. The communicator
 * then takes care of finding the "images" backend on any one of your
 * Snap servers, and send the PING there.
 *
 * The following shows a simple setup with two computers. Each have a
 * Snap Communicator server running. Both of these servers are connected
 * to each others. When the Snap! Server spawns a child process (because
 * a client connected) and that child process wants to send a PING to the
 * Image Backend it sends it using a UDP signal to the Snap Communicator
 * on Computer 2. That then gets transmitted to Computer 1 Snap Communitor
 * and finally to the Image Backend.
 *
 * \code
 * +------------------------+     +-----------------------------------------+
 * |  Computer 1            |     |  Computer 2                             |
 * |                        |     |                                         |
 * |  +----------------+  Connect |  +----------------+                     |
 * |  |     Snap       |<----------->|     Snap       |<-------+            |
 * |  |  Communicator  |  (TCP/IP)|  |  Communicator  |        | images     |
 * |  +----------------+    |     |  +----------------+        |  PING      |
 * |      ^                 |     |      ^                     |  (UDP)     |
 * |      | Connect         |     |      | Connect      +------+---------+  |
 * |      | (TCP/IP)        |     |      | (TCP/IP)     |   Snap Child   |  |
 * |      |                 |     |      |              |    Process     |  |
 * |      v                 |     |      v              +----------------+  |
 * |  +----------------+    |     |  +----------------+        ^            |
 * |  |     Images     |    |     |  |     Snap!      |        |            |
 * |  |    Backend     |    |     |  |    Server      +--------+            |
 * |  +----------------+    |     |  +----------------+  fork()             |
 * |                        |     |                                         |
 * +------------------------+     +-----------------------------------------+
 * \endcode
 *
 * The connection between Snap Communicator servers may happen in any
 * direction. In general, it will happen from the last communicator started
 * to the first running (since the first will fail to connect to the last
 * since the last is still not listening.) That connection makes use of
 * TCP/IP and has a protocol similar to the communication between various
 * parts and the communicator. That is, it sends commands written on one
 * line. The commands may be followed by parameters separated by spaces.
 *
 * Replies are also commands. For example, the HELP command is a way to
 * request a system to send us the COMMANDS and SIGNALS commands to tell
 * us about its capabilities.
 *
 * See also:
 * http://snapwebsites.org/implementation/feature-requirements/inter-process-signalling-core
 */



namespace
{


typedef QMap<QString, bool>                 sorted_list_of_strings_t;


/** \brief The sequence number of a message being broadcast.
 *
 * Each instance of snapcommunicator may broadcast a message to other
 * snapcommunicators. When that happens, we want to ignore that
 * message in case it comes again to the same snapcommunicator. This
 * can be accomplished by saving which messages we received.
 *
 * We also control a number of hops and a timeout.
 *
 * This counter is added to the name of the computer running this
 * snapcommunicator (i.e. f_server_name) so for example it would
 * look as following if the computer name is "snap":
 *
 * \code
 *          snap-123
 * \endcode
 */
int64_t             g_broadcast_sequence = 0;


sorted_list_of_strings_t canonicalize_services(QString const & services)
{
    snap::snap_string_list list(services.split(',', QString::SkipEmptyParts));

    // use a map to remove duplicates
    //
    sorted_list_of_strings_t result;

    int const max_services(list.size());
    for(int idx(0); idx < max_services; ++idx)
    {
        QString const service(list[idx].trimmed());
        if(service.isEmpty())
        {
            // this can happen because of the trimmed() call
            continue;
        }

        // TBD: add a check on the name? (i.e. "[A-Za-z_][A-Za-z0-9_]*")
        //

        result[service] = true;
    }

    return result;
}


QString canonicalize_server_types(QString const & server_types)
{
    snap::snap_string_list list(server_types.split(',', QString::SkipEmptyParts));

    // use a map to remove duplicates
    //
    QMap<QString, bool> result;

    int const max_types(list.size());
    for(int idx(0); idx < max_types; ++idx)
    {
        QString const type(list[idx].trimmed());
        if(type.isEmpty())
        {
            // this can happen, especially because of the trimmed() call
            //
            continue;
        }
        if(type != "apache"
        && type != "frontend"
        && type != "backend"
        && type != "cassandra")
        {
            // ignore unknown/unwanted types
            // (i.e. we cannot have "client" here since that is reserved
            // for processes that use REGISTER)
            //
            SNAP_LOG_WARNING("received an invalid server type \"")(type)("\", ignoring.");
        }
        else
        {
            result[type] = true;
        }
    }

    return static_cast<snap::snap_string_list const>(result.keys()).join(",");
}


QString canonicalize_neighbors(QString const & neighbors)
{
    snap::snap_string_list list(neighbors.split(','));

    int const max_addr_port(list.size());
    for(int idx(0); idx < max_addr_port; ++idx)
    {
        QString const neighbor(list[idx].trimmed());
        if(neighbor.isEmpty())
        {
            // this can happen, especially because of the trimmed() call
            //
            continue;
        }
        QString address; // no default address for neighbors
        int port(4040);
        tcp_client_server::get_addr_port(neighbor, address, port, "tcp");

        // TODO: move canonicalization to tcp_client_server so other software
        //       can make use of it
        //
        if(tcp_client_server::is_ipv4(address.toUtf8().data()))
        {
            // TODO: the inet_pton() does not support all possible IPv4
            //       notations that is_ipv4() "accepts".
            //
            struct in_addr addr;
            if(inet_pton(AF_INET, address.toUtf8().data(), &addr) != 1)
            {
                SNAP_LOG_ERROR("invalid neighbor address \"")(list[idx])("\", we could not convert it to a valid IPv4 address.");
                continue;
            }
            char buf[64];
            inet_ntop(AF_INET, &addr, buf, sizeof(buf));
            // removing leading zero, making sure we have the dotted notation
            list[idx] = QString("%1:%2").arg(buf).arg(port);
        }
        else if(tcp_client_server::is_ipv6(address.toUtf8().data()))
        {
            struct in6_addr addr;
            if(inet_pton(AF_INET6, address.toUtf8().data(), &addr) != 1)
            {
                SNAP_LOG_ERROR("invalid neighbor address \"")(list[idx])("\", we could not convert it to a valid IPv6 address.");
                continue;
            }
            char buf[64];
            inet_ntop(AF_INET6, &addr, buf, sizeof(buf));
            // removing leading zero, making sure the '::' is used at the
            // right place, etc.
            list[idx] = QString("[%1]:%2").arg(buf).arg(port);
        }
        else
        {
            SNAP_LOG_ERROR("invalid neighbor address \"")(list[idx])("\", it was not recognized as an IPv4 or an IPv6 address.");
            continue;
        }
    }

    return list.join(",");
}


} // no name namespace






class snap_communicator_server;
typedef std::shared_ptr<snap_communicator_server>           snap_communicator_server_pointer_t;

class base_connection;
typedef std::shared_ptr<base_connection>                    base_connection_pointer_t;
typedef std::vector<base_connection_pointer_t>              base_connection_vector_t;

class service_connection;
typedef std::shared_ptr<service_connection>                 service_connection_pointer_t;
typedef QMap<QString, service_connection_pointer_t>         service_connection_list_t;

class remote_snap_communicator;
typedef std::shared_ptr<remote_snap_communicator>           remote_snap_communicator_pointer_t;
typedef std::vector<remote_snap_communicator_pointer_t>     remote_snap_communicator_vector_t;
typedef QMap<QString, remote_snap_communicator_pointer_t>   remote_snap_communicator_list_t;

class gossip_to_remote_snap_communicator;
typedef std::shared_ptr<gossip_to_remote_snap_communicator> gossip_snap_communicator_pointer_t;
typedef QMap<QString, gossip_snap_communicator_pointer_t>   gossip_snap_communicator_list_t;



class remote_communicator_connections
        : public std::enable_shared_from_this<remote_communicator_connections>
{
public:
    typedef std::shared_ptr<remote_communicator_connections>    pointer_t;

                                            remote_communicator_connections(snap_communicator_server_pointer_t communicator, snap_addr::addr const & my_addr);

    QString                                 get_my_address() const;
    void                                    add_remote_communicator(QString const & addr);
    void                                    stop_gossiping();
    void                                    too_busy(QString const & addr);
    void                                    shutting_down(QString const & addr);
    void                                    server_unreachable(QString const & addr);
    void                                    gossip_received(QString const & addr);
    void                                    forget_remote_connection(QString const & addr);
    tcp_client_server::bio_client::mode_t   connection_mode() const;

private:
    snap_communicator_server_pointer_t      f_communicator_server;
    snap_addr::addr const &                 f_my_address;
    QMap<QString, int>                      f_all_ips;
    int64_t                                 f_last_start_date = 0;
    remote_snap_communicator_list_t         f_smaller_ips;      // we connect to smaller IPs
    gossip_snap_communicator_list_t         f_gossip_ips;
    service_connection_list_t               f_larger_ips;       // larger IPs connect to us
};


/** \brief Set of connections in the snapcommunicator tool.
 *
 * All the connections and sockets in general will all appear
 * in this class.
 */
class snap_communicator_server
        : public std::enable_shared_from_this<snap_communicator_server>
{
public:
    typedef std::shared_ptr<snap_communicator_server>     pointer_t;

    static size_t const         SNAP_COMMUNICATOR_MAX_CONNECTIONS = 100;

                                snap_communicator_server(snap::server::pointer_t s);
                                snap_communicator_server(snap_communicator_server const & src) = delete;
    snap_communicator_server &  operator = (snap_communicator_server const & rhs) = delete;

    void                        init();
    void                        run();

    // one place where all messages get processed
    void                        process_message(snap::snap_communicator::snap_connection::pointer_t connection, snap::snap_communicator_message const & message, bool udp);

    void                        send_status(snap::snap_communicator::snap_connection::pointer_t connection, snap::snap_communicator::snap_connection::pointer_t * reply_connection = nullptr);
    QString                     get_local_services() const;
    QString                     get_services_heard_of() const;
    void                        add_neighbors(QString const & new_neighbors);
    void                        remove_neighbor(QString const & neighbor);
    void                        read_neighbors();
    void                        save_neighbors();
    inline void                 verify_command(base_connection_pointer_t connection, snap::snap_communicator_message const & message);
    void                        process_connected(snap::snap_communicator::snap_connection::pointer_t connection);
    void                        broadcast_message(snap::snap_communicator_message const & message, base_connection_vector_t const & accepting_remote_connections = base_connection_vector_t());
    void                        process_load_balancing();
    tcp_client_server::bio_client::mode_t   connection_mode() const;
    void                        shutdown(bool quitting);

private:
    struct message_cache
    {
        typedef std::vector<message_cache>  vector_t;

        time_t                              f_timeout_timestamp;        // when that message is to be removed from the cache whether it was sent or not
        snap::snap_communicator_message     f_message;                  // the message
    };

    void                        drop_privileges();
    void                        refresh_heard_of();
    void                        listen_loadavg(snap::snap_communicator_message const & message);
    void                        save_loadavg(snap::snap_communicator_message const & message);
    void                        register_for_loadavg(QString const & ip);

    snap::server::pointer_t                             f_server;

    QString                                             f_server_name;
    int                                                 f_number_of_processors = 1;
    QString                                             f_neighbors_cache_filename;
    QString                                             f_username;
    QString                                             f_groupname;
    std::string                                         f_public_ip;        // f_listener IP address
    snap::snap_communicator::pointer_t                  f_communicator;
    snap::snap_communicator::snap_connection::pointer_t f_interrupt;        // TCP/IP
    snap::snap_communicator::snap_connection::pointer_t f_local_listener;   // TCP/IP
    snap::snap_communicator::snap_connection::pointer_t f_listener;         // TCP/IP
    snap::snap_communicator::snap_connection::pointer_t f_ping;             // UDP/IP
    snap::snap_communicator::snap_connection::pointer_t f_loadavg_timer;    // a 1 second timer to calculate load (used to load balance)
    float                                               f_last_loadavg;
    snap_addr::addr                                     f_my_address;
    QString                                             f_local_services;
    sorted_list_of_strings_t                            f_local_services_list;
    QString                                             f_services_heard_of;
    sorted_list_of_strings_t                            f_services_heard_of_list;
    QString                                             f_explicit_neighbors;
    sorted_list_of_strings_t                            f_all_neighbors;
    sorted_list_of_strings_t                            f_registered_neighbors_for_loadavg;
    remote_communicator_connections::pointer_t          f_remote_snapcommunicators;
    size_t                                              f_max_connections = SNAP_COMMUNICATOR_MAX_CONNECTIONS;
    bool                                                f_shutdown = false;
    bool                                                f_debug_lock = false;
    bool                                                f_force_restart = false;
    message_cache::vector_t                             f_local_message_cache;
    std::map<QString, time_t>                           f_received_broadcast_messages;
    tcp_client_server::bio_client::mode_t               f_connection_mode = tcp_client_server::bio_client::mode_t::MODE_PLAIN;
};



class base_connection
{
public:
    typedef std::shared_ptr<base_connection>    pointer_t;


    enum class connection_type_t
    {
        CONNECTION_TYPE_DOWN,   // not connected
        CONNECTION_TYPE_LOCAL,  // a service on this computer
        CONNECTION_TYPE_REMOTE  // another snapcommunicator on another computer
    };


    /** \brief Initialize the base_connection() object.
     *
     * The constructor saves the communicator server pointer
     * so one can access it from any derived version.
     */
    base_connection(snap_communicator_server::pointer_t cs)
        : f_communicator_server(cs)
    {
    }


    /** \brief We have a destructor to make it virtual.
     *
     * Everything is otherwise automatically released.
     */
    virtual ~base_connection()
    {
    }


    /** \brief Save when the connection started.
     *
     * This function is called whenever a CONNECT or REGISTER message
     * is received since those mark the time when a connection starts.
     *
     * You can later retrieve when the connection started with the
     * get_connection_started() function.
     *
     * This call also resets the f_ended_on in case we were able to
     * reuse the same connection multiple times (reconnecting means
     * a new socket and thus a brand new connection object...)
     */
    void connection_started()
    {
        f_started_on = snap::snap_communicator::get_current_date();
        f_ended_on = -1;
    }


    /** \brief Return information on when the connection started.
     *
     * This function gives you the date and time when the connection
     * started, meaning when the connection received a CONNECT or
     * REGISTER event.
     *
     * If the events have no yet occured, then the connection returns
     * -1 instead.
     *
     * \return The date and time when the connection started in microseconds.
     */
    int64_t get_connection_started() const
    {
        return f_started_on;
    }


    /** \brief Connection ended, save the date and time of the event.
     *
     * Whenever we receive a DISCONNECT or UNREGISTER we call this
     * function. It also gets called in the event a connection
     * is deleted without first receiving a graceful DISCONNECT
     * or UNREGISTER event.
     */
    void connection_ended()
    {
        // save the current only if the connection really started
        // before and also only once (do not update the time later)
        //
        if(f_started_on != -1
        && f_ended_on == -1)
        {
            f_ended_on = snap::snap_communicator::get_current_date();
        }
    }


    /** \brief Timestamp when the connection was ended.
     *
     * This value represents the time when the UNREGISTER, DISCONNECT,
     * or the destruction of the service_connection object occurred. It
     * represents the time when the specific service was shutdown.
     *
     * \return The approximative date when the connection ended in microseconds.
     */
    int64_t get_connection_ended() const
    {
        return f_ended_on;
    }


    /** \brief Save the name of the server.
     *
     * \param[in] server_name  The name of the server that is on the other
     *                         side of this connection.
     */
    void set_server_name(QString const & server_name)
    {
        f_server_name = server_name;
    }


    /** \brief Get the name of the server.
     *
     * \return The name of the server that is on the other
     *         side of this connection.
     */
    QString get_server_name() const
    {
        return f_server_name;
    }


    /** \brief Save the address of that connection.
     *
     * This is only used for remote connections on either the CONNECT
     * or ACCEPT message.
     *
     * \param[in] my_address  The address of the server that is on the
     *                        other side of this connection.
     */
    void set_my_address(QString const & my_address)
    {
        f_my_address = my_address;
    }


    /** \brief Get the address of that connection.
     *
     * This function returns a valid address only after the CONNECT
     * or ACCEPT message were received for this connection.
     *
     * \return The address of the server that is on the
     *         other side of this connection.
     */
    QString get_my_address() const
    {
        return f_my_address;
    }


    /** \brief Define the type of snapcommunicator server.
     *
     * This function is called whenever a CONNECT or an ACCEPT is received.
     * It saves the type=... parameter. By default the type is empty meaning
     * that the connection was not yet fully initialized.
     *
     * When a REGISTER is received instead of a CONNECT or an ACCEPT, then
     * the type is set to "client".
     *
     * \param[in] type  The type of connection.
     */
    void set_connection_type(connection_type_t type)
    {
        f_type = type;
    }


    /** \brief Retrieve the current type of this connection.
     *
     * By default a connection is given the type CONNECTION_TYPE_DOWN,
     * which means that it is not currently connected. To initialize
     * a connection one has to either CONNECT (between snapcommunicator
     * servers) or REGISTER (a service such as snapbackend, snapserver,
     * snapwatchdog, and others.)
     *
     * The type is set to CONNECTION_TYPE_LOCAL for local services and
     * CONNECTION_TYPE_REMOTE when representing another snapserver.
     *
     * \return The type of server this connection represents.
     */
    connection_type_t get_connection_type() const
    {
        return f_type;
    }


    /** \brief Define the list of services supported by the snapcommunicator.
     *
     * Whenever a snapcommunicator connects to another one, either by
     * doing a CONNECT or replying to a CONNECT by an ACCEPT, it is
     * expected to list services that it supports (the list could be
     * empty as it usually is on a Cassandra node.) This function
     * saves that list.
     *
     * This defines the name of services and thus where to send various
     * messages such as a PING to request a service to start doing work.
     *
     * \param[in] services  The list of services this server handles.
     */
    void set_services(QString const & services)
    {
        snap::snap_string_list list(services.split(','));
        for(auto const & s : list)
        {
            f_services[s] = true;
        }
    }


    /** \brief Retrieve the list of services offered by other snapcommunicators.
     *
     * This function saves in the input parameter \p services the list of
     * services that this very snapcommunicator offers.
     *
     * \param[in,out] services  The map where all the services are defined.
     */
    void get_services(sorted_list_of_strings_t & services)
    {
        services.unite(f_services);
    }


    /** \brief Check whether the service is known by that connection.
     *
     * This function returns true if the service was defined as one
     * this connection supports.
     *
     * \param[in] name  The name of the service to check for.
     *
     * \return true if the service is known.
     */
    bool has_service(QString const & name)
    {
        return f_services.contains(name);
    }


    /** \brief Define the list of services we heard of.
     *
     * This function saves the list of services that were heard of by
     * another snapcommunicator server. This list may be updated later
     * with an ACCEPT event.
     *
     * This list is used to know where to forward a message if we do
     * not have a more direct link to those services (i.e. the same
     * service defined in our own list or in a snapcommunicator
     * we are directly connected to.)
     *
     * \param[in] services  The list of services heard of.
     */
    void set_services_heard_of(QString const & services)
    {
        snap::snap_string_list list(services.split(','));
        for(auto const & s : list)
        {
            f_services_heard_of[s] = true;
        }
    }


    /** \brief Retrieve the list of services heard of by another server.
     *
     * This function saves in the input parameter \p services the list of
     * services that this snapcommunicator heard of.
     *
     * \param[in,out] services  The map where all the services are defined.
     */
    void get_services_heard_of(sorted_list_of_strings_t & services)
    {
        services.unite(f_services_heard_of);
    }


    /** \brief List of defined commands.
     *
     * This function saves the list of commands known by another process.
     * The \p commands parameter is broken up at each comma and the
     * resulting list saved in the f_understood_commands map for fast
     * retrieval.
     *
     * In general a process receives the COMMANDS event whenever it
     * sent the HELP event to request for this list.
     *
     * \param[in] commands  The list of understood commands.
     */
    void set_commands(QString const & commands)
    {
        snap::snap_string_list cmds(commands.split(','));
        for(auto const & c : cmds)
        {
            QString const name(c.trimmed());
            if(!name.isEmpty())
            {
                f_understood_commands[name] = true;
            }
        }
    }


    /** \brief Check whether a certain command is understood by this connection.
     *
     * This function checks whether this connection understands \p command.
     *
     * \param[in] command  The command to check for.
     *
     * \return true if the command is supported, false otherwise.
     */
    bool understand_command(QString const & command)
    {
        return f_understood_commands.contains(command);
    }


    /** \brief Check whether this connection received the COMMANDS message.
     *
     * This function returns true if the list of understood commands is
     * defined. This means we do know whether a verification (i.e. a call
     * to the understand_command() function) will return false because the
     * list of commands is empty or because a command is not understood.
     *
     * \return true if one or more commands are understood.
     */
    bool has_commands() const
    {
        return !f_understood_commands.empty();
    }


    /** \brief Remove a command.
     *
     * This function is used to make the system think that certain command
     * are actually not understood.
     *
     * At this time, it is only used when a connection goes away and we
     * want to send a STATUS message to various services interested in
     * such a message.
     *
     * \param[in] command  The command to remove.
     */
    void remove_command(QString const & command)
    {
        f_understood_commands.remove(command);
    }


    /** \brief Mark that connection as a remote connection.
     *
     * When we receive a connection from another snapconnector, we call
     * this function so later we can very quickly determine whether the
     * connection is a remote connection.
     */
    void mark_as_remote()
    {
        f_remote_connection = true;
    }


    /** \brief Check whether this connection is a remote connection.
     *
     * The function returns false by default. If the mark_as_remote()
     * was called, this function returns true.
     *
     * \return true if the connection was marked as a remote connection.
     */
    bool is_remote() const
    {
        return f_remote_connection;
    }


    /** \brief Set whether this connection wants to receive LOADAVG messages.
     *
     * Whenever a frontend wants to know which backend to use for its
     * current client request, it can check a set of IP addresses for
     * the least loaded computer. Then it can use that IP address to
     * process the request.
     *
     * \param[in] wants_loadavg  Whether this connection wants
     *    (REGISTERFORLOADAVG) or does not want (UNREGISTERFORLOADAVG)
     *    to receive LOADAVG messages from this snapcommunicator.
     */
    void set_wants_loadavg(bool wants_loadavg)
    {
        f_wants_loadavg = wants_loadavg;
    }


    /** \brief Check whether this connection wants LOADAVG messages.
     *
     * This function returns true if the connection last sent us a
     * REGISTERFORLOADAVG message.
     *
     * \return true if the LOADAVG should be sent to this connection.
     */
    bool wants_loadavg() const
    {
        return f_wants_loadavg;
    }


protected:
    snap_communicator_server::pointer_t     f_communicator_server;

private:
    sorted_list_of_strings_t                f_understood_commands;
    int64_t                                 f_started_on = -1;
    int64_t                                 f_ended_on = -1;
    connection_type_t                       f_type = connection_type_t::CONNECTION_TYPE_DOWN;
    QString                                 f_server_name;
    QString                                 f_my_address;
    sorted_list_of_strings_t                f_services;
    sorted_list_of_strings_t                f_services_heard_of;
    bool                                    f_remote_connection = false;
    bool                                    f_wants_loadavg = false;
};





/** \brief Describe a remove snapcommunicator by IP address, etc.
 *
 * This class defines a snapcommunicator server. Mainly we include
 * the IP address of the server to connect to.
 *
 * The object also maintains the status of that server. Whether we
 * can connect to it (because if not the connection stays in limbo
 * and we should not try again and again forever. Instead we can
 * just go to sleep and try again "much" later saving many CPU
 * cycles.)
 *
 * It also gives us a way to quickly track snapcommunicator objects
 * that REFUSE our connection.
 */
class remote_snap_communicator
    : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
    , public base_connection
{
public:
    static uint64_t const           REMOTE_CONNECTION_DEFAULT_TIMEOUT   =         1LL * 60LL * 1000000LL;   // 1 minute
    static uint64_t const           REMOTE_CONNECTION_RECONNECT_TIMEOUT =         5LL * 60LL * 1000000LL;   // 5 minutes
    static uint64_t const           REMOTE_CONNECTION_TOO_BUSY_TIMEOUT  = 24LL * 60LL * 60LL * 1000000LL;   // 24 hours

                                    remote_snap_communicator(snap_communicator_server::pointer_t cs, QString const & addr, int port);
    virtual                         ~remote_snap_communicator() override;

    // snap_tcp_client_permanent_message_connection implementation
    virtual void                    process_message(snap::snap_communicator_message const & message) override;
    virtual void                    process_connection_failed(std::string const & error_message) override;
    virtual void                    process_connected() override;

    snap_addr::addr const &         get_address() const;

private:
    snap_addr::addr                 f_address;
};



/** \brief To send a GOSSIP to a remote snapcommunicator.
 *
 * This class defines a connection used to send a GOSSIP message
 * to a remote communicator. Once the GOSSIP worked at least once,
 * this connection gets deleted.
 *
 * This connection is a timer, it manages an actual TCP/IP connection
 * which it attempts to create every now and then. This is because
 * we do not want to use too many resources to attempt to connect
 * to a computer which is down. (i.e. we use a thread to attempt
 * the connection since it can take forever if it does not work; i.e.
 * inter-computer socket connections may timeout after a minute or
 * two!)
 *
 * For the feat we use our 'permanent message connection.' This is
 * very well adapted. We just need to make sure to remove the
 * connection once we received confirmation the the GOSSIP message
 * was received by the remote host.
 */
class gossip_to_remote_snap_communicator
    : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    std::shared_ptr<gossip_to_remote_snap_communicator> pointer_t;

    static int64_t const        FIRST_TIMEOUT = 5LL * 1000000L;  // 5 seconds before first attempt

                                gossip_to_remote_snap_communicator(remote_communicator_connections::pointer_t rcs, QString const & addr, int port);

    // snap_connection implementation
    virtual void                process_timeout();

    // snap_tcp_client_permanent_message_connection implementation
    virtual void                process_message(snap::snap_communicator_message const & message);
    virtual void                process_connection_failed(std::string const & error_message);
    virtual void                process_connected();

    void                        kill();

private:
    QString const               f_addr;
    int const                   f_port = 0;
    int64_t                     f_wait = FIRST_TIMEOUT;
    remote_communicator_connections::pointer_t f_remote_communicators;
};


/** \brief Initialize the gossip remote communicator connection.
 *
 * This object is actually a timer. Each time we get a tick
 * (i.e. process_timeout() callback gets called), a connection
 * is attempted against the remote snapcommunicator daemon
 * specified by the addr and port parameters.
 *
 * The addr and port are both mandatory to this constructor.
 *
 * \param[in] cs  The snap communicator server object which we contact
 *                whenever the GOSSIP message was confirmed by the
 *                remote connection.
 * \param[in] addr  The IP address of the remote snap communicator.
 * \param[in] port  The port to connect to that snap communicator.
 */
gossip_to_remote_snap_communicator::gossip_to_remote_snap_communicator(remote_communicator_connections::pointer_t rcs, QString const & addr, int port)
    : snap_tcp_client_permanent_message_connection(
                  addr.toUtf8().data()
                , port
                , rcs->connection_mode()
                , -FIRST_TIMEOUT  // must be negative so first timeout is active (otherwise we get an immediately attempt, which we do not want in this case)
                , true)
    , f_addr(addr)
    , f_port(port)
    //, f_wait(FIRST_TIMEOUT) -- auto-init
    , f_remote_communicators(rcs)
{
}


/** \brief Process one timeout.
 *
 * We do not really have anything to do when a timeout happens. The
 * connection attempts are automatically done by the permanent
 * connection in the snap_communicator library.
 *
 * However, we want to increase the delay between attempts. For that,
 * we use this function and double the delay on each timeout until
 * it reaches about 1h. Then we stop doubling that delay. If the
 * remote snapcommunicator never makes it, we won't swamp the network
 * by false attempts to connect to a dead computer.
 *
 * \todo
 * We need to let the snapwatchdog know that such remote connections
 * fail for X amount of time. This is important to track what's
 * missing in the cluster (Even if we likely will have other means
 * to know of the problem.)
 */
void gossip_to_remote_snap_communicator::process_timeout()
{
    snap_tcp_client_permanent_message_connection::process_timeout();

    // increase the delay on each timeout until we reach 1h and then
    // repeat every 1h or so (i.e. if you change the FIRST_TIMEOUT
    // you may not reach exactly 1h here, also the time it takes
    // to try to connect is added to the delay each time.)
    //
    if(f_wait < 3600LL * 1000000LL)
    {
        f_wait *= 2;
        set_timeout_delay(f_wait);
    }
}


/** \brief Process the reply from our GOSSIP message.
 *
 * This function processes any messages received from the remote
 * system.
 *
 * We currently really only expect RECEIVED as a reply.
 *
 * \param[in] message  The message received from the remote snapcommunicator.
 */
void gossip_to_remote_snap_communicator::process_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("gossip connection received a message [")(message.to_message())("]");

    QString const & command(message.get_command());
    if(command == "RECEIVED")
    {
        // we got confirmation that the GOSSIP went across
        //
        f_remote_communicators->gossip_received(f_addr);
    }
}


/** \brief The remote connection failed, we cannot gossip with it.
 *
 * This function gets called if a connection to a remote communicator fails.
 *
 * In case of a gossip, this is because that other computer is expected to
 * connect with us, but it may not know about us so we tell it hello for
 * that reason.
 *
 * We have this function because on a failure we want to mark that
 * computer as being down. This is important for the snapmanagerdaemon.
 *
 * \param[in] error_message  The error that occurred.
 */
void gossip_to_remote_snap_communicator::process_connection_failed(std::string const & error_message)
{
    // make sure the default function does its job.
    //
    snap::snap_communicator::snap_tcp_client_permanent_message_connection::process_connection_failed(error_message);

    // now let people know about the fact that this other computer is
    // unreachable
    //
    f_remote_communicators->server_unreachable(f_addr);
}


/** \brief Once connected send the GOSSIP message.
 *
 * This function gets called whenever the connection is finally up.
 * This gives us the opportunity to send the GOSSIP message to the
 * remote host.
 *
 * Note that at this time this happens in the main thread. The
 * secondary thread was used to call the connect() function, but
 * it is not used to send or receive any messages.
 */
void gossip_to_remote_snap_communicator::process_connected()
{
    // TODO:
    // The default process_connected() function disables the timer
    // of the gossip connection. This means that we will not get
    // any further process_timeout() calls until we completely
    // lose the connection. This is possibly not what we want, or
    // at least we should let the snapwatchdog know that we were
    // connected to a snapcommunicator, yes, sent the GOSSIP,
    // all good up to here, but never got a reply! Not getting
    // a reply is likely to mean that the connection we establish
    // is somehow bogus even if it does not Hang Up on us.
    //
    // You may read the Byzantine fault tolerance in regard to
    // supporting a varied set of processes to detect the health
    // of many different nodes in a cluster.
    //
    // https://en.wikipedia.org/wiki/Byzantine_fault_tolerance
    //
    snap_tcp_client_permanent_message_connection::process_connected();

    // we are connected so we can send the GOSSIP message
    // (each time we reconnect!)
    //
    snap::snap_communicator_message gossip;
    gossip.set_command("GOSSIP");
    gossip.add_parameter("my_address", f_remote_communicators->get_my_address());
    send_message(gossip); // do not cache, if we lose the connection, we lose the message and that's fine in this case
}










remote_communicator_connections::remote_communicator_connections(snap_communicator_server::pointer_t communicator_server, snap_addr::addr const & my_addr)
    : f_communicator_server(communicator_server)
    , f_my_address(my_addr)
{
}



QString remote_communicator_connections::get_my_address() const
{
    return f_my_address.get_ipv4or6_string(true).c_str();
}


void remote_communicator_connections::add_remote_communicator(QString const & addr_port)
{
    SNAP_LOG_DEBUG("adding remote communicator at ")(addr_port);

    // no default address for neighbors
    snap_addr::addr remote_addr(addr_port.toUtf8().data(), "", 4040, "tcp");

    if(remote_addr == f_my_address)
    {
        // TBD: this may be normal (i.e. neighbors should send us our IP
        //      right back to us!)
        //
        SNAP_LOG_WARNING("address of remote snapcommunicator, \"")(addr_port)("\", is the same as my address, which means it is not remote.");
        return;
    }

    QString const addr(remote_addr.get_ipv4or6_string().c_str());
    int const port(remote_addr.get_port());

    // was this address already added
    //
    // TODO: use snap_addr::addr objects in the map and the == operator
    //       will then use the one from snap_addr::addr (and not a string)
    //
    if(f_all_ips.contains(addr))
    {
        if(remote_addr < f_my_address)
        {
            // make sure it is defined!
            //
            if(f_smaller_ips.contains(addr)
            && f_smaller_ips[addr])
            {
                if(f_smaller_ips[addr]->is_enabled())
                {
                    // reset that timer to run ASAP in case the timer is enabled
                    //
                    f_smaller_ips[addr]->set_timeout_date(snap::snap_communicator::get_current_date());
                }
            }
            else
            {
                SNAP_LOG_ERROR("smaller remote address is defined in f_all_ips but not in f_smaller_ips?");
            }
        }
        // else -- we may already be GOSSIP-ing about this one (see below)
        return;
    }

    // keep a copy of all addresses
    //
    f_all_ips[addr] = port;

    // if this new IP is smaller than ours, then we start a connection
    //
    if(remote_addr < f_my_address)
    {
        // smaller connections are created as remote snap communicator
        // which are permanent message connections
        //
        remote_snap_communicator_pointer_t remote_communicator(std::make_shared<remote_snap_communicator>(f_communicator_server, addr, port));
        f_smaller_ips[addr] = remote_communicator;
        remote_communicator->set_name(QString("remote communicator connection: %1").arg(addr)); // we connect to remote host

        // make sure not to try to connect to all remote communicators
        // all at once
        //
        int64_t const now(snap::snap_communicator::get_current_date());
        if(now > f_last_start_date)
        {
            f_last_start_date = now;
        }
        remote_communicator->set_timeout_date(f_last_start_date);

        // TBD: 1 second between attempts, should that be smaller?
        //
        f_last_start_date += 1000000LL;

        if(!snap::snap_communicator::instance()->add_connection(remote_communicator))
        {
            // this should never happens here since each new creates a
            // new pointer
            //
            SNAP_LOG_ERROR("new remote connection could not be added to the snap_communicator list of connections");

            auto it(f_smaller_ips.find(addr));
            if(it != f_smaller_ips.end())
            {
                f_smaller_ips.erase(it);
            }
        }
        else
        {
            SNAP_LOG_DEBUG("new remote connection added for ")(addr);
        }
    }
    else //if(remote_addr != f_my_address) -- already tested at the beginning of the function
    {
        // in case the remote snapcommunicator has a larger address
        // it is expected to CONNECT to us; however, it may not yet
        // know about us so we want to send a GOSSIP message; this
        // means creating a special connection which attempts to
        // send the GOSSIP message up until it succeeds or the
        // application quits
        //
        f_gossip_ips[addr].reset(new gossip_to_remote_snap_communicator(shared_from_this(), addr, port));
        f_gossip_ips[addr]->set_name(QString("gossip to remote snap communicator: %1").arg(addr));

        if(!snap::snap_communicator::instance()->add_connection(f_gossip_ips[addr]))
        {
            // this should never happens here since each new creates a
            // new pointer
            //
            SNAP_LOG_ERROR("new gossip connection could not be added to the snap_communicator list of connections");

            auto it(f_gossip_ips.find(addr));
            if(it != f_gossip_ips.end())
            {
                f_gossip_ips.erase(it);
            }
        }
        else
        {
            SNAP_LOG_DEBUG("new gossip connection added for ")(addr);
        }
    }
}


/** \brief Stop all gossiping at once.
 *
 * This function can be called to remove all the gossip connections
 * at once.
 *
 * In most cases this function is called whenever the snapcommunicator
 * daemon receives a STOP or a SHUTDOWN.
 *
 * Also these connections do not support any other messages than the
 * GOSSIP and RECEIVED.
 */
void remote_communicator_connections::stop_gossiping()
{
    while(!f_gossip_ips.empty())
    {
        snap::snap_communicator::instance()->remove_connection(*f_gossip_ips.begin());
        f_gossip_ips.erase(f_gossip_ips.begin());
    }
}


/** \brief A remote communicator refused our connection.
 *
 * When a remote snap communicator server already manages too many
 * connections, it may end up refusing our additional connection.
 * When this happens, we have to avoid trying to connect again
 * and again.
 *
 * Here we use a very large delay of 24h before trying to connect
 * again later. I do not really think this is necessary because
 * if we have too many connections we anyway always have too many
 * connections. That being said, once in a while a computer dies
 * and thus the number of connections may drop to a level where
 * we will be accepted.
 *
 * At some point we may want to look into having seeds instead
 * of allowing connections to all the nodes.
 *
 * \param[in] addr  The address of the snapcommunicator that refused a
 *                  CONNECT because it is too busy.
 */
void remote_communicator_connections::too_busy(QString const & addr)
{
    if(f_smaller_ips.contains(addr))
    {
        // wait for 1 day and try again (is 1 day too long?)
        f_smaller_ips[addr]->set_timeout_delay(remote_snap_communicator::REMOTE_CONNECTION_TOO_BUSY_TIMEOUT);
        SNAP_LOG_INFO("remote communicator ")(addr)(" was marked as too busy. Pause for 1 day before trying to connect again.");
    }
}


/** \brief Another system is shutting down, maybe rebooting.
 *
 * This function makes sure we wait for some time, instead of waisting
 * our time trying to reconnect again and again.
 *
 * \param[in] addr  The address of the snapcommunicator that refused a
 *                  CONNECT because it is shutting down.
 */
void remote_communicator_connections::shutting_down(QString const & addr)
{
    if(f_smaller_ips.contains(addr))
    {
        // wait for 5 minutes and try again
        //
        f_smaller_ips[addr]->set_timeout_delay(remote_snap_communicator::REMOTE_CONNECTION_RECONNECT_TIMEOUT);
    }
}


void remote_communicator_connections::server_unreachable(QString const & addr)
{
    // we do not have the name of the computer in snapcommunicator so
    // we just broadcast the IP address of the non-responding computer
    //
    snap::snap_communicator_message unreachable;
    unreachable.set_service(".");
    unreachable.set_command("UNREACHABLE");
    unreachable.add_parameter("who", addr);
    f_communicator_server->broadcast_message(unreachable);
}


void remote_communicator_connections::gossip_received(QString const & addr)
{
    auto it(f_gossip_ips.find(addr));
    if(it != f_gossip_ips.end())
    {
        snap::snap_communicator::instance()->remove_connection(*it);
        f_gossip_ips.erase(it);
    }
}


void remote_communicator_connections::forget_remote_connection(QString const & addr_port)
{
    QString addr(addr_port);
    int const pos(addr.indexOf(':'));
    if(pos > 0)
    {
        // forget about the port if present
        //
        addr = addr.mid(0, pos);
    }
    auto it(f_smaller_ips.find(addr));
    if(it != f_smaller_ips.end())
    {
        snap::snap_communicator::instance()->remove_connection(*it);
        f_smaller_ips.erase(it);
    }
}


tcp_client_server::bio_client::mode_t remote_communicator_connections::connection_mode() const
{
    return f_communicator_server->connection_mode();
}





/** \brief Listen for messages.
 *
 * The snapcommunicator TCP connection simply listen for process_message()
 * callbacks and processes those messages by calling the process_message()
 * of the connections class.
 *
 * It also listens for disconnections so it can send a new STATUS command
 * whenever the connection goes down.
 */
class service_connection
        : public snap::snap_communicator::snap_tcp_server_client_message_connection
        , public base_connection
{
public:
    typedef std::shared_ptr<service_connection>    pointer_t;


    /** \brief Create a service connection and assigns \p socket to it.
     *
     * The constructor of the service connection expects a socket that
     * was just accept()'ed.
     *
     * The snapcommunicator daemon listens on to two different ports
     * and two different addresses on those ports:
     *
     * \li TCP 127.0.0.1:4040 -- this address is expected to be used by all the
     * local services
     *
     * \li TCP 0.0.0.0:4040 -- this address is expected to be used by remote
     * snapcommunicators; it is often changed to a private network IP
     * address such as 192.168.0.1 to increase safety. However, if your
     * cluster spans multiple data centers, it will not be possible to
     * use a private network IP address.
     *
     * \li UDP 127.0.0.1:4041 -- this special port is used to accept UDP
     * signals sent to the snapcommunicator; UDP signals are most often
     * used to very quickly send signals without having to have a full
     * TCP connection to a daemon
     *
     * The connections happen on 127.0.0.1 are fully trusted. Connections
     * happening on 0.0.0.0 are generally viewed as tainted.
     *
     * \param[in] cs  The communicator server (i.e. parent)
     * \param[in] socket  The socket that was just created by the accept()
     *                    command.
     * \param[in] server_name  The name of the server we are running on
     *                         (i.e. generally your hostname.)
     */
    service_connection(snap_communicator_server::pointer_t cs, tcp_client_server::bio_client::pointer_t client, QString const & server_name)
        : snap_tcp_server_client_message_connection(client)
        , base_connection(cs)
        , f_server_name(server_name)
        //, f_address(get_client_addr_port(), "tcp")   // TODO: see if we could instead have a get address which returns a sockaddr_in[6] or even have the snap_addr::addr in our tcp_client_server classes
        , f_address(get_remote_address().toUtf8().data(), "tcp")  // this is the address:port of the peer (the computer on the other side)
    {
    }


    /** \brief Connection lost.
     *
     * When a connection goes down it gets deleted. This is when we can
     * send a new STATUS event to all the other STATUS hungry connections.
     */
    virtual ~service_connection() override
    {
        // save when it is ending in case we did not get a DISCONNECT
        // or an UNREGISTER event
        //
        connection_ended();

        // clearly mark this connection as down
        //
        set_connection_type(connection_type_t::CONNECTION_TYPE_DOWN);

        // make sure that if we were a connection understanding STATUS
        // we do not send that status
        //
        remove_command("STATUS");

        // now ask the server to send a new STATUS to all connections
        // that understand that message; we pass our pointer since we
        // want to send the info about this connection in that STATUS
        // message
        //
        // TODO: we cannot use shared_from_this() in the destructor,
        //       it's too late since when we reach here the pointer
        //       was already destroyed so we get a bad_weak_ptr
        //       exception; we need to find a different way if we
        //       want this event to be noticed and a STATUS sent...
        //
        //f_communicator_server->send_status(shared_from_this());
    }


    // snap::snap_communicator::snap_tcp_server_client_message_connection implementation
    virtual void process_message(snap::snap_communicator_message const & message) override
    {
        // make sure the destination knows who sent that message so it
        // is possible to directly reply to that specific instance of
        // a service
        //
        if(f_named)
        {
            snap::snap_communicator_message forward_message(message);
            forward_message.set_sent_from_server(f_server_name);
            forward_message.set_sent_from_service(get_name());
            f_communicator_server->process_message(shared_from_this(), forward_message, false);
        }
        else
        {
            f_communicator_server->process_message(shared_from_this(), message, false);
        }
    }


    /** \brief We are losing the connection, send a STATUS message.
     *
     * This function is called in all cases where the connection is
     * lost so we can send a STATUS message with information saying
     * that the connection is gone.
     */
    void send_status()
    {
        // mark connection as down before we call the send_status()
        //
        set_connection_type(connection_type_t::CONNECTION_TYPE_DOWN);

        f_communicator_server->send_status(shared_from_this());
    }


    /** \brief Remove ourselves when we receive a timeout.
     *
     * Whenever we receive a shutdown, we have to remove everything but
     * we still want to send some message and to do so we need to use
     * the timeout which happens after we finalize all read and write
     * callbacks.
     */
    virtual void process_timeout() override
    {
        remove_from_communicator();

        send_status();
    }


    virtual void process_error() override
    {
        snap_tcp_server_client_message_connection::process_error();

        send_status();
    }


    /** \brief Process a hang up.
     *
     * It is important for some processes to know when a remote connection
     * is lost (i.e. for dynamic QUORUM calculations in snaplock, for
     * example.) So we handle the process_hup() event and send a
     * HANGUP if this connection is a remote connection.
     */
    virtual void process_hup() override
    {
        snap_tcp_server_client_message_connection::process_hup();

        if(is_remote()
        && !get_server_name().isEmpty())
        {
            snap::snap_communicator_message hangup;
            hangup.set_command("HANGUP");
            hangup.set_service(".");
            hangup.add_parameter("server_name", get_server_name());
            f_communicator_server->broadcast_message(hangup);
        }

        send_status();
    }


    virtual void process_invalid() override
    {
        snap_tcp_server_client_message_connection::process_invalid();

        send_status();
    }


    /** \brief Tell that the connection was given a real name.
     *
     * Whenever we receive an event through this connection,
     * we want to mark the message as received from the service.
     *
     * However, by default the name of the service is on purpose
     * set to an "invalid value" (i.e. a name with a space.) That
     * value is not expected to be used when forwarding the message
     * to another service.
     *
     * Once a system properly registers with the REGISTER message,
     * we receive a valid name then. That name is saved in the
     * connection and the connection is marked as having a valid
     * name.
     *
     * This very function must be called once the proper name was
     * set in this connection.
     */
    void properly_named()
    {
        f_named = true;
    }


    /** \brief Return the type of address this connection has.
     *
     * This function determines the type of address of the connection.
     *
     * \return A reference to the remote address of this connection.
     */
    snap_addr::addr const & get_address() const
    {
        return f_address;
    }


private:
    QString const               f_server_name;
    snap_addr::addr             f_address;
    bool                        f_named = false;
};







/** \brief Handle the SIGINT that is expected to stop the server.
 *
 * This class is an implementation of the snap_signal that listens
 * on the SIGINT.
 */
class interrupt_impl
        : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<interrupt_impl>     pointer_t;

                        interrupt_impl(snap_communicator_server::pointer_t cs);
    virtual             ~interrupt_impl() override {}

    // snap::snap_communicator::snap_signal implementation
    virtual void        process_signal() override;

private:
    snap_communicator_server::pointer_t     f_communicator_server;
};


/** \brief The interrupt initialization.
 *
 * The interrupt uses the signalfd() function to obtain a way to listen on
 * incoming Unix signals.
 *
 * Specifically, it listens on the SIGINT signal, which is the equivalent
 * to the Ctrl-C.
 *
 * \param[in] cs  The snapcommunicator we are listening for.
 */
interrupt_impl::interrupt_impl(snap_communicator_server::pointer_t cs)
    : snap_signal(SIGINT)
    , f_communicator_server(cs)
{
    unblock_signal_on_destruction();
    set_name("snap communicator interrupt");
}


/** \brief Call the stop function of the snaplock object.
 *
 * When this function is called, the signal was received and thus we are
 * asked to quit as soon as possible.
 */
void interrupt_impl::process_signal()
{
    // we simulate the STOP, so pass 'false' (i.e. not quitting)
    //
    f_communicator_server->shutdown(false);
}




/** \brief Handle new connections from clients.
 *
 * This class is an implementation of the snap server connection so we can
 * handle new connections from various clients.
 */
class listener
        : public snap::snap_communicator::snap_tcp_server_connection
{
public:
    /** \brief The listener initialization.
     *
     * The listener creates a new TCP server to listen for incoming
     * TCP connection.
     *
     * \warning
     * At this time the \p max_connections parameter is ignored.
     *
     * \param[in] addr  The address to listen on. Most often it is 0.0.0.0.
     * \param[in] port  The port to listen on.
     * \param[in] certificate  The filename of a PEM file with a certificate.
     * \param[in] private_key  The filename of a PEM file with a private key.
     * \param[in] max_connections  The maximum number of connections to keep
     *            waiting; if more arrive, refuse them until we are done with
     *            some existing connections.
     * \param[in] local  Whether this connection expects local services only.
     * \param[in] server_name  The name of the server running this instance.
     */
    listener(snap_communicator_server::pointer_t cs, std::string const & addr, int port, std::string const & certificate, std::string const & private_key, int max_connections, bool local, QString const & server_name)
        : snap_tcp_server_connection(
                          addr
                        , port
                        , certificate
                        , private_key
                        // convert client mode to a server mode
                        , cs->connection_mode() == tcp_client_server::bio_client::mode_t::MODE_PLAIN
                                ? tcp_client_server::bio_server::mode_t::MODE_PLAIN
                                : tcp_client_server::bio_server::mode_t::MODE_SECURE
                        , max_connections
                        , true)
        , f_communicator_server(cs)
        , f_local(local)
        , f_server_name(server_name)
    {
    }

    // snap::snap_communicator::snap_server_connection implementation
    virtual void process_accept()
    {
        // a new client just connected, create a new service_connection
        // object and add it to the snap_communicator object.
        //
        tcp_client_server::bio_client::pointer_t const new_client(accept());
        if(new_client == nullptr)
        {
            // an error occurred, report in the logs
            int const e(errno);
            SNAP_LOG_ERROR("somehow accept() failed with errno: ")(e)(" -- ")(strerror(e));
            return;
        }

        service_connection::pointer_t connection(new service_connection(f_communicator_server, new_client, f_server_name));

        // TBD: is that a really weak test?
        //
        // TODO: use the snap::addr class and use the type of IP address
        //       instead of what we have here
        //
        // XXX: add support for IPv6 (automatic with snap::addr?)
        //
        std::string const addr(connection->get_client_addr());
        if(f_local)
        {
            if(addr != "127.0.0.1")
            {
                // TODO: find out why we do not get 127.0.0.1 when using such to connect...
                SNAP_LOG_WARNING("received what should be a local connection from \"")(addr)("\".");
                //return;
            }

            // set a default name in each new connection, this changes
            // whenever we receive a REGISTER message from that connection
            //
            connection->set_name("client connection");

            connection->set_server_name(f_server_name);
        }
        else
        {
            if(addr == "127.0.0.1")
            {
                SNAP_LOG_ERROR("received what should be a remote connection from 127.0.0.1");
                return;
            }

            // set a name for remote connections
            //
            // the following name includes a space which prevents someone
            // from send to such a connection, which is certainly a good
            // thing since there can be duplicate and that name is not
            // sensible as a destination
            //
            // we will change the name once we receive the CONNECT message
            // and as we send the ACCEPT message
            //
            connection->set_name(QString("remote connection from: %1").arg(QString::fromUtf8(addr.c_str()))); // remote host connected to us
            connection->mark_as_remote();
        }

        if(!snap::snap_communicator::instance()->add_connection(connection))
        {
            // this should never happen here since each new creates a
            // new pointer
            //
            SNAP_LOG_ERROR("new client connection could not be added to the snap_communicator list of connections");
        }
    }

private:
    snap_communicator_server::pointer_t     f_communicator_server;
    bool const                              f_local = false;
    QString const                           f_server_name;
};








/** \brief Handle UDP messages from clients.
 *
 * This class is an implementation of the snap server connection so we can
 * handle new connections from various clients.
 */
class ping_impl
        : public snap::snap_communicator::snap_udp_server_message_connection
{
public:
    /** \brief The messenger initialization.
     *
     * The messenger receives UDP messages from various sources (mainly
     * backends at this point.)
     *
     * \param[in] cs  The snap communicator server we are listening for.
     * \param[in] addr  The address to listen on. Most often it is 127.0.0.1.
     *                  for the UDP because we currently only allow for
     *                  local messages.
     * \param[in] port  The port to listen on.
     */
    ping_impl(snap_communicator_server::pointer_t cs, std::string const & addr, int port)
        : snap_udp_server_message_connection(addr, port)
        , f_communicator_server(cs)
    {
    }

    // snap::snap_communicator::snap_server_connection implementation
    virtual void process_message(snap::snap_communicator_message const & message)
    {
        f_communicator_server->process_message(shared_from_this(), message, true);
    }

private:
    // this is owned by a server function so no need for a smart pointer
    snap_communicator_server::pointer_t f_communicator_server;
};





/** \brief Provide a tick to offer load balancing information.
 *
 * This class is an implementation of a timer to offer load balancing
 * information between various front and backend computers in the cluster.
 */
class timer_impl
        : public snap::snap_communicator::snap_timer
{
public:
    /** \brief The timer initialization.
     *
     * The timer ticks once per second to retrieve the current load of the
     * system and forward it to whichever computer that requested the
     * information.
     *
     * \param[in] cs  The snap communicator server we are listening for.
     */
    timer_impl(snap_communicator_server::pointer_t cs)
        : snap_timer(1000000LL)  // 1 second in microseconds
        , f_communicator_server(cs)
    {
        set_enable(false);
    }

    // snap::snap_communicator::snap_timer implementation
    virtual void process_timeout() override
    {
        f_communicator_server->process_load_balancing();
    }

private:
    // this is owned by a server function so no need for a smart pointer
    snap_communicator_server::pointer_t f_communicator_server;
};









/** \brief Construct the snap_communicator_server object.
 *
 * This function saves the server pointer in the snap_communicator_server
 * object. It is used later to gather various information and call helper
 * functions.
 */
snap_communicator_server::snap_communicator_server(snap::server::pointer_t s)
    : f_server(s)
{
}


/** \brief Initialize the snap_communicator_server.
 *
 * This function is used to initialize the connetions object. This means
 * setting up a few parameters such as the nice level of the application
 * and priority scheme for listening to events.
 *
 * Then it creates two sockets: one listening on TCP/IP and the other
 * listening on UDP/IP. The TCP/IP is for other servers to connect to
 * and listen communicate various status between various servers. The
 * UDP/IP is used to very quickly send messages between servers. The
 * UDP/IP messages are viewed as signals to wake up a server so it
 * starts working on new data (in most cases, at least.)
 */
void snap_communicator_server::init()
{
    // keep a copy of the server name handy
    f_server_name = QString::fromUtf8(snap::server::get_server_name().c_str());

    f_number_of_processors = std::max(1U, std::thread::hardware_concurrency());

    f_debug_lock = !f_server->get_parameter("debug_lock_messages").isEmpty();

    {
        // check a user defined maximum number of connections
        // by default this is set to SNAP_COMMUNICATOR_MAX_CONNECTIONS,
        // which at this time is 100
        //
        QString const max_connections(f_server->get_parameter("max_connections"));
        if(!max_connections.isEmpty())
        {
            bool ok(false);
            f_max_connections = max_connections.toLongLong(&ok, 10);
            if(!ok
            || f_max_connections < 10)
            {
                SNAP_LOG_FATAL("the max_connections parameter is not a valid decimal number or is smaller than 10 (")(max_connections)(").");
                f_server->exit(1);
            }
        }
    }

    // read the list of available services
    //
    {
        QString path_to_services(f_server->get_parameter("services"));
        if(path_to_services.isEmpty())
        {
            path_to_services = "/usr/share/snapwebsites/services";
        }
        path_to_services += "/*.service";
#if 0
        QByteArray pattern(path_to_services.toUtf8());
        glob_t dir = glob_t();
        int const r(glob(
                      pattern.data()
                    , GLOB_NOESCAPE
                    , glob_error_callback
                    , &dir));
        std::shared_ptr<glob_t> ai(&dir, glob_deleter);

        if(r != 0)
        {
            // do nothing when errors occur
            //
            switch(r)
            {
            case GLOB_NOSPACE:
                SNAP_LOG_FATAL("glob() did not have enough memory to alllocate its buffers.");
                throw snap::snap_exception("glob() did not have enough memory to alllocate its buffers.");

            case GLOB_ABORTED:
                SNAP_LOG_FATAL("glob() was aborted after a read error.");
                throw snap::snap_exception("glob() was aborted after a read error.");

            case GLOB_NOMATCH:
                // this is a legal case, absolutely no local services...
                //
                SNAP_LOG_DEBUG("glob() could not find any status information.");
                break;

            default:
                SNAP_LOG_FATAL(QString("unknown glob() error code: %1.").arg(r));
                throw snap::snap_exception(QString("unknown glob() error code: %1.").arg(r));

            }
        }
        else
        {
            // we have some local services (note that snapcommunicator is
            // not added as a local service)
            //
            for(size_t idx(0); idx < dir.gl_pathc; ++idx)
            {
                char const * basename(strrchr(dir.gl_pathv[idx], '/'));
                if(basename == nullptr)
                {
                    basename = dir.gl_pathv[idx];
                }
                else
                {
                    ++basename;
                }
                char const * end(strstr(basename, ".service"));
                if(end == nullptr)
                {
                    end = basename + strlen(basename);
                }
                QString const key(QString::fromUtf8(basename, end - basename));
                f_local_services_list[key] = true;
            }

            // the list of local services cannot (currently) change while
            // snapcommunicator is running so generate the corresponding
            // string once
            //
            f_local_services = f_local_services_list.keys().join(",");
        }
#else
        try
        {
            snap::glob_dir dir( path_to_services, GLOB_NOESCAPE );

            // we have some local services (note that snapcommunicator is
            // not added as a local service)
            //
            dir.enumerate_glob( [&]( QString path )
            {
                // TODO: change away from C-lib stuff and use QString methods.
                //
                char const * p_path( path.toUtf8().data() );
                char const * basename(strrchr(p_path, '/'));
                if( basename == nullptr )
                {
                    basename = p_path;
                }
                else
                {
                    ++basename;
                }
                char const * end(strstr(basename, ".service"));
                if(end == nullptr)
                {
                    end = basename + strlen(basename);
                }
                QString const key(QString::fromUtf8(basename, end - basename));
                f_local_services_list[key] = true;
            });

            // the list of local services cannot (currently) change while
            // snapcommunicator is running so generate the corresponding
            // string once
            //
            f_local_services = f_local_services_list.keys().join(",");
        }
        catch( snap::glob_dir_exception const & x )
        {
            switch( int const r = x.get_error_num() )
            {
            case GLOB_NOSPACE:
                SNAP_LOG_FATAL("glob_dir did not have enough memory to alllocate its buffers.");
                throw snap::snap_exception("glob_dir did not have enough memory to alllocate its buffers.");

            case GLOB_ABORTED:
                SNAP_LOG_FATAL("glob_dir was aborted after a read error.");
                throw snap::snap_exception("glob_dir was aborted after a read error.");

            case GLOB_NOMATCH:
                // this is a legal case, absolutely no local services...
                //
                SNAP_LOG_DEBUG("glob_dir could not find any status information.");
                break;

            default:
                SNAP_LOG_FATAL(QString("unknown glob_dir error code: %1.").arg(r));
                throw snap::snap_exception(QString("unknown glob_dir error code: %1.").arg(r));
            }
        }
        catch( std::exception const & x )
        {
            SNAP_LOG_FATAL( QString("Exception caught! what=[%1].").arg(x.what()) );
            throw;
        }
        catch( ... )
        {
            SNAP_LOG_FATAL("Unknown exception caught!");
            throw snap::snap_exception("Unknown exception caught!");
        }
#endif
    }

    f_communicator = snap::snap_communicator::instance();

    // capture Ctrl-C (SIGINT)
    //
    f_interrupt.reset(new interrupt_impl(shared_from_this()));
    f_communicator->add_connection(f_interrupt);

    int max_pending_connections(-1);
    {
        QString const max_pending_connections_str(f_server->get_parameter("max_pending_connections"));
        if(!max_pending_connections_str.isEmpty())
        {
            bool ok(false);
            max_pending_connections = max_pending_connections_str.toInt(&ok, 10);
            if(!ok
            || max_pending_connections < 5
            || max_pending_connections > 1000)
            {
                SNAP_LOG_FATAL("the max_pending_connections parameter from the configuration file must be a valid number between 5 and 1000. %1 is not valid.")(max_pending_connections_str);
                f_server->exit(1);
            }
        }
    }

    // create two listeners, for new arriving TCP/IP connections
    //
    // one listener is used to listen for local services which have to
    // connect using the 127.0.0.1 IP address
    //
    // the other listener listens to your local network and accepts
    // connections from other snapcommunicator servers
    //
    // local
    {
        // TODO: convert to use the 'addr' class instead
        //       and properly accept all local addresses (i.e. 127.0.0.0/8)
        QString addr("127.0.0.1");
        int port(4040);
        tcp_client_server::get_addr_port(f_server->get_parameter("local_listen"), addr, port, "tcp");
        if(addr != "127.0.0.1")
        {
            SNAP_LOG_FATAL("The local_listen parameter must have 127.0.0.1 as the IP address. %1 is not acceptable.")(addr);
            f_server->exit(1);
        }

        // make this listener the local listener
        //
        f_local_listener.reset(new listener(shared_from_this(), addr.toUtf8().data(), port, std::string(), std::string(), max_pending_connections, true, f_server_name));
        f_local_listener->set_name("snap communicator local listener");
        f_communicator->add_connection(f_local_listener);
    }
    // remote
    QString const listen_str(f_server->get_parameter("listen"));
    snap_addr::addr listen_addr(listen_str.toUtf8().data(), "0.0.0.0", 4040, "tcp");
    {
        // make this listener the remote listener, however, if the IP
        // address is 127.0.0.1 we skip on this one, we do not need
        // two listener on the local IP address
        //
        if(listen_addr.get_network_type() != snap_addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK)
        {
            // remote connections may make use of SSL, check whether there
            // are certificate and private key files defined (by default
            // there are)
            //
            std::string const certificate(f_server->get_parameter("ssl_certificate").toUtf8().data());
            std::string const private_key(f_server->get_parameter("ssl_private_key").toUtf8().data());

            f_connection_mode = certificate.empty() && private_key.empty()
                                ? tcp_client_server::bio_client::mode_t::MODE_PLAIN
                                : tcp_client_server::bio_client::mode_t::MODE_SECURE;

            f_public_ip = listen_addr.get_ipv4or6_string();
            f_listener.reset(new listener(shared_from_this(), f_public_ip, listen_addr.get_port(), certificate, private_key, max_pending_connections, false, f_server_name));
            f_listener->set_name("snap communicator listener");
            f_communicator->add_connection(f_listener);
        }
        else
        {
            SNAP_LOG_WARNING("remote \"listen\" parameter is \"")(listen_str)("\" so it is ignored and no remote connections will be possible.");
        }
    }

    {
        QString addr("127.0.0.1"); // this default should work just fine
        int port(4041);
        tcp_client_server::get_addr_port(f_server->get_parameter("signal"), addr, port, "tcp");

        f_ping.reset(new ping_impl(shared_from_this(), addr.toUtf8().data(), port));
        f_ping->set_name("snap communicator messenger (UDP)");
        f_communicator->add_connection(f_ping);
    }

    {
        f_loadavg_timer.reset(new timer_impl(shared_from_this()));
        f_loadavg_timer->set_name("snap communicator load balancer timer");
        f_communicator->add_connection(f_loadavg_timer);
    }

    // transform the my_address to a snap_addr::addr object
    //
    f_my_address = snap_addr::addr(f_server->get_parameter("my_address").toUtf8().data(), "", listen_addr.get_port(), "tcp");
    snap_addr::addr::computer_interface_address_t cia(f_my_address.is_computer_interface_address());
    if(cia == snap_addr::addr::computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_ERROR)
    {
        int const e(errno);
        SNAP_LOG_ERROR("somehow getifaddrs() failed with errno: ")(e)(" -- ")(strerror(e));
        // we go on anyway...
    }
    else if(cia != snap_addr::addr::computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_TRUE)
    {
        std::string const addr(f_my_address.get_ipv6_string());
        SNAP_LOG_FATAL("my_address \"")(addr)("\" not found on this computer. Did a copy of the configuration file and forgot to change that entry?");
        throw snap::snap_exception(QString("my_address \"%1\" not found on this computer. Did a copy of the configuration file and forgot to change that entry?.").arg(QString::fromUtf8(addr.c_str())));
    }

    f_remote_snapcommunicators.reset(new remote_communicator_connections(shared_from_this(), f_my_address));

    // the add_neighbors() function parses the list of neighbors and
    // creates a permanent connection
    //
    // note that the first time add_neighbors is called it reads the
    // list of cached neighbor IP:port info and connects those too
    //
    f_explicit_neighbors = canonicalize_neighbors(f_server->get_parameter("neighbors"));
    add_neighbors(f_explicit_neighbors);
}


/** \brief Get the mode in which connections are expected to be established.
 *
 * This function returns the mode (MODE_PLAIN or MODE_SECURE) used
 * by the listener. All remote connections initiateed by this
 * snapcommunicator instance are expected to use the same mode.
 *
 * This is applied to the GOSSIP and remote connection to snapcommunicator
 * objects with a smaller IP address.
 *
 * \return The listener connection mode.
 */
tcp_client_server::bio_client::mode_t snap_communicator_server::connection_mode() const
{
    return f_connection_mode;
}


void snap_communicator_server::drop_privileges()
{
    // drop to non-priv user/group if we are root
    // (i.e. this code is skip on programmer's machines)
    //
    if( getuid() == 0 )
    {
        // Group first, then user. Otherwise you lose privs to change your group!
        //
        {
            struct group const * grp(getgrnam(f_groupname.toUtf8().data()));
            if( nullptr == grp )
            {
                SNAP_LOG_FATAL("Cannot locate group \"")(f_groupname)("\"! Create it first, then run the server.");
                throw snap::snap_exception(QString("Cannot locate group \"%1\"! Create it first, then run the server.")
                                    .arg(f_groupname));
            }
            int const sw_grp_id(grp->gr_gid);
            //
            if( setegid( sw_grp_id ) != 0 )
            {
                int const e(errno);
                SNAP_LOG_FATAL("Cannot drop privileges to group \"")(f_groupname)("\"! errno: ")(e)(", ")(strerror(e));
                throw snap::snap_exception(QString("Cannot drop privileges group \"%1\"!")
                                    .arg(f_groupname));
            }
        }
        //
        {
            struct passwd const * pswd(getpwnam(f_username.toUtf8().data()));
            if( nullptr == pswd )
            {
                SNAP_LOG_FATAL("Cannot locate user \"")(f_username)("\"! Create it first, then run the server.");
                throw snap::snap_exception(QString("Cannot locate user \"%1\"! Create it first, then run the server.")
                                    .arg(f_username));
            }
            int const sw_usr_id(pswd->pw_uid);
            //
            if( seteuid( sw_usr_id ) != 0 )
            {
                int const e(errno);
                SNAP_LOG_FATAL("Cannot drop privileges to user \"")(f_username)("\"! Create it first, then run the server. errno: ")(e)(", ")(strerror(e));
                throw snap::snap_exception(QString("Cannot drop privileges to user \"%1\"! Create it first, then run the server.")
                                    .arg(f_username));
            }
        }
    }
}


/** \brief The execution loop.
 *
 * This function runs the execution loop until the snapcommunicator
 * system receives a QUIT message.
 */
void snap_communicator_server::run()
{
    // run "forever" (until we receive a QUIT message)
    //
    f_communicator->run();

    // we are done, cleanly get rid of the communicator
    //
    f_communicator.reset();

    // we received a RELOADCONFIG, exit with 1 so systemd restarts us
    //
    if(f_force_restart)
    {
        exit(1);
    }
}


/** \brief Make sure that the connection understands a command.
 *
 * This function checks whether the specified connection (\p connection)
 * understands the command about to be sent to it (\p reply).
 *
 * \note
 * The test is done only when snapcommunicator is run in debug
 * mode to not waste time.
 *
 * \param[in,out] connection  The concerned connection that has to understand the command.
 * \param[in] message  The message about to be sent to \p connection.
 */
void snap_communicator_server::verify_command(base_connection::pointer_t connection, snap::snap_communicator_message const & message)
{
    // debug turned on?
    if(!f_server->is_debug())
    {
        // nope, do not waste any more time
        return;
    }

    if(!connection->has_commands())
    {
        // if we did not yet receive the COMMANDS message then we cannot
        // pretend that the understand_command() will return a sensible
        // result, so ignore that test...
        //
        return;
    }

    if(connection->understand_command(message.get_command()))
    {
        // all good, the command is implemented
        //
        return;
    }

    // if you get this message, it could be that you do implement
    // the command, but do not advertise it in your COMMANDS
    // reply to the HELP message sent by snapcommunicator
    //
    snap::snap_communicator::snap_connection::pointer_t c(std::dynamic_pointer_cast<snap::snap_communicator::snap_connection>(connection));
    if(c)
    {
        SNAP_LOG_FATAL("connection \"")(c->get_name())("\" does not understand ")(message.get_command())(".");
        throw snap::snap_exception(QString("Connection \"%1\" does not implement command \"%2\".").arg(c->get_name()).arg(message.get_command()));
    }

    SNAP_LOG_FATAL("connection does not understand ")(message.get_command())(".");
    throw snap::snap_exception(QString("Connection does not implement command \"%1\".").arg(message.get_command()));
}


/** \brief Process a message we just received.
 *
 * This function is called whenever a TCP or UDP message is received.
 * The function accepts all TCP messages, however, UDP messages are
 * limited to a very few such as STOP and SHUTDOWN. You will want to
 * check the documentation of each message to know whether it can
 * be sent over UDP or not.
 *
 * Note that the main reason why the UDP port is not allowed for most
 * messages is to send a reply you have to have TCP. This means responses
 * to those messages also need to be sent over TCP (because we could
 * not have sent an ACCEPT as a response to a CONNECT over a UDP
 * connection.)
 *
 * \param[in] connection  The connection that just sent us that message.
 * \param[in] message  The message were were just sent.
 * \param[in] udp  If true, this was a UDP message.
 */
void snap_communicator_server::process_message(snap::snap_communicator::snap_connection::pointer_t connection, snap::snap_communicator_message const & message, bool udp)
{
    // messages being broadcast to us have a unique ID, if that ID is
    // one we already received we must ignore the message altogether;
    // also, a broadcast message has a timeout, we must ignore the
    // message if it already timed out
    //
    if(message.has_parameter("broadcast_msgid"))
    {
        // check whether the message already timed out
        //
        // this is a safety feature of our broadcasting capability
        // which should rarely be activated unless you have multiple
        // data center locations
        //
        time_t const timeout(message.get_integer_parameter("broadcast_timeout"));
        time_t const now(time(nullptr));
        if(timeout < now)
        {
            return;
        }

        // check whether we already received that message, if so ignore
        // the second instance (it should not happen with the list of
        // neighbors included in the message, but just in case...)
        //
        QString const broadcast_msgid(message.get_parameter("broadcast_msgid"));
        auto const received_it(f_received_broadcast_messages.find(broadcast_msgid));
        if(received_it != f_received_broadcast_messages.cend())     // message arrived again?
        {
            // note that although we include neighbors it is normal that
            // this happens in a cluster where some computers are not
            // aware of certain nodes; for example, if A sends a
            // message to B and C, both B and C know of a node D
            // which is unknown to A, then both B and C will end
            // up forward that same message to D, so D will discard
            // the second instance it receives.
            //
            return;
        }
    }

    // if the destination server was specified, we have to forward
    // the message to that specific server
    //
    QString const server_name(message.get_server() == "." ? f_server_name : message.get_server());
    QString const service(message.get_service());
    QString const command(message.get_command());
    QString const sent_from_service(message.get_sent_from_service());

    if(f_debug_lock
    || (command != "UNLOCKED"
     && sent_from_service != "snaplock"
     && !sent_from_service.startsWith("lock_")
     && (command != "REGISTER"
         || !message.has_parameter("service")
         || !message.get_parameter("service").startsWith("lock_"))
     && command != "SNAPLOG"))
    {
        SNAP_LOG_TRACE("received command=[")(command)
                ("], server_name=[")(server_name)("], service=[")(service)
                ("], message=[")(message.to_message())("]");
    }

    base_connection::pointer_t base(std::dynamic_pointer_cast<base_connection>(connection));
    remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(connection));
    service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(connection));

    // TODO: move all the command bodies to sub-funtions.

    // check whether this message is for us
    //
    if((server_name.isEmpty() || server_name == f_server_name || server_name == "*")    // match server
    && (service.isEmpty() || service == "snapcommunicator"))                            // and service?
    {
        if(f_shutdown)
        {
            // if the user sent us an UNREGISTER we should not generate a
            // QUITTING because the UNREGISTER is in reply to our STOP
            // TBD: we may want to implement the UNREGISTER in this
            //      situation?
            //
            if(!udp)
            {
                if(command != "UNREGISTER")
                {
                    // we are shutting down so just send a quick QUTTING reply
                    // letting the other process know about it
                    //
                    snap::snap_communicator_message reply;
                    reply.set_command("QUITTING");

                    verify_command(base, reply);
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception(QString("message \"%1\" sent on a \"weird\" connection.").arg(command));
                    }
                }

                // get rid of that connection now, we don't need any more
                // messages coming from it
                //
                f_communicator->remove_connection(connection);
            }
            //else -- UDP message arriving after f_shutdown are ignored
            return;
        }

        // this one is for us!
        switch(command[0].unicode())
        {
        case 'A':
            if(command == "ACCEPT")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("ACCEPT is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    // the type is mandatory in an ACCEPT message
                    //
                    if(!message.has_parameter("server_name")
                    || !message.has_parameter("my_address"))
                    {
                        SNAP_LOG_ERROR("ACCEPT was received without a \"server_name\" parameter, which is mandatory.");
                        return;
                    }
                    base->set_connection_type(base_connection::connection_type_t::CONNECTION_TYPE_REMOTE);
                    QString const & remote_server_name(message.get_parameter("server_name"));
                    base->set_server_name(remote_server_name);

                    // reply to a CONNECT, this was to connect to another
                    // snapcommunicator on another computer, retrieve the
                    // data from that remote computer
                    //
                    base->connection_started();
                    QString const his_address(message.get_parameter("my_address"));
                    base->set_my_address(his_address);

                    if(message.has_parameter("services"))
                    {
                        base->set_services(message.get_parameter("services"));
                    }
                    if(message.has_parameter("heard_of"))
                    {
                        base->set_services_heard_of(message.get_parameter("heard_of"));
                    }
                    if(message.has_parameter("neighbors"))
                    {
                        add_neighbors(message.get_parameter("neighbors"));
                    }

                    // we just got some new services information,
                    // refresh our cache
                    //
                    refresh_heard_of();

                    // also request the COMMANDS of this connection
                    //
                    snap::snap_communicator_message help;
                    help.set_command("HELP");
                    //verify_command(base, help); -- precisely
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(help);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(help);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception(QString("message \"%1\" sent on a \"weird\" connection.").arg(command));
                    }

                    // if a local service was interested in this specific
                    // computer, then we have to start receiving LOADAVG
                    // messages from it
                    //
                    register_for_loadavg(his_address);

                    // now let local services know that we have a new
                    // remote connections (which may be of interest
                    // for that service--see snapmanagerdaemon)
                    //
                    // TODO: to be symmetrical, we should also have
                    //       a message telling us when a remote
                    //       connection goes down...
                    //
                    snap::snap_communicator_message new_remote_connection;
                    new_remote_connection.set_command("NEWREMOTECONNECTION");
                    new_remote_connection.set_service(".");
                    new_remote_connection.add_parameter("server_name", remote_server_name);
                    broadcast_message(new_remote_connection);
                    return;
                }
            }
            break;

        case 'C':
            if(command == "COMMANDS")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("COMMANDS is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    if(message.has_parameter("list"))
                    {
                        base->set_commands(message.get_parameter("list"));

                        // here we verify that a few commands are properly
                        // defined, for some becausesince we already sent
                        // them to that connection and thus it should
                        // understand them; and a few more that are very
                        // possibly going to be sent
                        //
                        if(f_server->is_debug())
                        {
                            bool ok(true);
                            if(!base->understand_command("HELP"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand HELP.");
                                ok = false;
                            }
                            if(!base->understand_command("QUITTING"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand QUITTING.");
                                ok = false;
                            }
                            // on a remote we get ACCEPT instead of READY
                            if(remote_communicator
                            || base->is_remote())
                            {
                                if(!base->understand_command("ACCEPT"))
                                {
                                    SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand ACCEPT.");
                                    ok = false;
                                }
                            }
                            else
                            {
                                if(!base->understand_command("READY"))
                                {
                                    SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand READY.");
                                    ok = false;
                                }
                            }
                            if(!base->understand_command("STOP"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand STOP.");
                                ok = false;
                            }
                            if(!base->understand_command("UNKNOWN"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand UNKNOWN.");
                                ok = false;
                            }
                            if(!ok)
                            {
                                // end the process so developers can fix their
                                // problems (this is only if --debug was
                                // specified)
                                //
                                throw snap::snap_exception(QString("Connection %1 does not implement some required commands.").arg(connection->get_name()));
                            }
                        }
                    }
                    else
                    {
                        SNAP_LOG_ERROR("COMMANDS was sent without a \"list\" parameter.");
                    }
                    return;
                }
            }
            else if(command == "CONNECT")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("CONNECT is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    // first we verify that we have a valid version to
                    // communicate between two snapcommunicators
                    //
                    if(!message.has_parameter("version")
                    || !message.has_parameter("my_address")
                    || !message.has_parameter("server_name"))
                    {
                        SNAP_LOG_ERROR("CONNECT was sent without a \"version\", or \"my_address\" parameter, both are mandatory.");
                        return;
                    }
                    if(message.get_integer_parameter("version") != snap::snap_communicator::VERSION)
                    {
                        SNAP_LOG_ERROR("CONNECT was sent with an incompatible version. Expected ")(snap::snap_communicator::VERSION)(", received ")(message.get_integer_parameter("version"));
                        return;
                    }

                    snap::snap_communicator_message reply;
                    snap::snap_communicator_message new_remote_connection;

                    QString const remote_server_name(message.get_parameter("server_name"));
                    snap::snap_communicator::snap_connection::vector_t const all_connections(f_communicator->get_connections());
                    snap::snap_communicator::snap_connection::pointer_t snap_conn(std::dynamic_pointer_cast<snap::snap_communicator::snap_connection>(base));
                    auto const & name_match(std::find_if(
                            all_connections.begin(),
                            all_connections.end(),
                            [remote_server_name, snap_conn](auto const & it)
                            {
                                // ignore ourselves
                                //
                                if(it == snap_conn)
                                {
                                    return false;
                                }
                                base_connection::pointer_t b(std::dynamic_pointer_cast<base_connection>(it));
                                if(!b)
                                {
                                    return false;
                                }
                                return remote_server_name == b->get_server_name();
                            }));

                    bool refuse(name_match != all_connections.end());
                    if(refuse)
                    {
                        SNAP_LOG_ERROR("CONNECT from \"")(remote_server_name)("\" but we already have another computer using that same name.");

                        reply.set_command("REFUSE");
                        reply.add_parameter("conflict", "name");

                        // we may also be shutting down
                        //
                        // Note: we cannot get here if f_shutdown is true...
                        //
                        if(f_shutdown)
                        {
                            reply.add_parameter("shutdown", "true");
                        }
                    }
                    else
                    {
                        base->set_server_name(remote_server_name);

                        // add neighbors with which the guys asking to
                        // connect can attempt to connect with...
                        //
                        if(!f_explicit_neighbors.isEmpty())
                        {
                            reply.add_parameter("neighbors", f_explicit_neighbors);
                        }

                        // Note: we cannot get here if f_shutdown is true...
                        //
                        refuse = f_shutdown;
                        if(refuse)
                        {
                            // okay, this guy wants to connect we us but we
                            // are shutting down, so refuse and put the shutdown
                            // flag to true
                            //
                            reply.set_command("REFUSE");
                            reply.add_parameter("shutdown", "true");
                        }
                        else
                        {
                            // cool, a remote snapcommunicator wants to connect
                            // with us, make sure we did not reach the maximum
                            // number of connections though...
                            //
                            refuse = f_communicator->get_connections().size() >= f_max_connections;
                            if(refuse)
                            {
                                // too many connections already, refuse this new
                                // one from a remove system
                                //
                                reply.set_command("REFUSE");
                            }
                            else
                            {
                                // set the connection type if we are not refusing it
                                //
                                base->set_connection_type(base_connection::connection_type_t::CONNECTION_TYPE_REMOTE);

                                // same as ACCEPT (see above) -- maybe we could have
                                // a sub-function...
                                //
                                base->connection_started();

                                if(message.has_parameter("services"))
                                {
                                    base->set_services(message.get_parameter("services"));
                                }
                                if(message.has_parameter("heard_of"))
                                {
                                    base->set_services_heard_of(message.get_parameter("heard_of"));
                                }
                                if(message.has_parameter("neighbors"))
                                {
                                    add_neighbors(message.get_parameter("neighbors"));
                                }

                                // we just got some new services information,
                                // refresh our cache
                                //
                                refresh_heard_of();

                                // the message expects the ACCEPT reply
                                //
                                reply.set_command("ACCEPT");
                                reply.add_parameter("server_name", f_server_name);
                                reply.add_parameter("my_address", QString::fromUtf8(f_my_address.get_ipv4or6_string(true).c_str()));

                                // services
                                if(!f_local_services.isEmpty())
                                {
                                    reply.add_parameter("services", f_local_services);
                                }

                                // heard of
                                if(!f_services_heard_of.isEmpty())
                                {
                                    reply.add_parameter("heard_of", f_services_heard_of);
                                }

                                QString const his_address(message.get_parameter("my_address"));
                                base->set_my_address(his_address);

                                // if a local service was interested in this specific
                                // computer, then we have to start receiving LOADAVG
                                // messages from it
                                //
                                register_for_loadavg(his_address);

                                // he is a neighbor too, make sure to add it
                                // in our list of neighbors (useful on a restart
                                // to connect quickly)
                                //
                                add_neighbors(his_address);

                                // since we are accepting a CONNECT we have to make
                                // sure we cancel the GOSSIP events to that remote
                                // connection; it won't hurt, but it is a waste if
                                // we do not need it
                                //
                                // Note: the name of the function is "GOSSIP"
                                //       received because if the "RECEIVED"
                                //       message was sent back from that remote
                                //       snapcommunicator then it means that
                                //       remote daemon received our GOSSIP message
                                //       and receiving the "CONNECT" message is
                                //       very similar to receiving the "RECEIVED"
                                //       message after a "GOSSIP"
                                //
                                f_remote_snapcommunicators->gossip_received(his_address);

                                // now let local services know that we have a new
                                // remote connections (which may be of interest
                                // for that service--see snapmanagerdaemon)
                                //
                                // TODO: to be symmetrical, we should also have
                                //       a message telling us when a remote
                                //       connection goes down...
                                //
                                new_remote_connection.set_command("NEWREMOTECONNECTION");
                                new_remote_connection.set_service(".");
                                new_remote_connection.add_parameter("server_name", remote_server_name);
                            }
                        }
                    }

                    //verify_command(base, reply); -- we do not yet have a list of commands understood by the other snapcommunicator daemon

                    // also request the COMMANDS of this connection with a HELP
                    // if the connection was not refused
                    //
                    snap::snap_communicator_message help;
                    help.set_command("HELP");
                    //verify_command(base, help); -- precisely
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                        if(!refuse)
                        {
                            remote_communicator->send_message(help);
                            broadcast_message(new_remote_connection);
                        }
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                        if(!refuse)
                        {
                            service_conn->send_message(help);
                            broadcast_message(new_remote_connection);
                        }
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("CONNECT sent on a \"weird\" connection.");
                    }

                    // status changed for this connection
                    //
                    send_status(connection);
                    return;
                }
            }
            break;

        case 'D':
            if(command == "DISCONNECT")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("DISCONNECT is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    base->connection_ended();

                    // this has to be another snapcommunicator
                    // (i.e. an object that sent ACCEPT or CONNECT)
                    //
                    base_connection::connection_type_t const type(base->get_connection_type());
                    if(type == base_connection::connection_type_t::CONNECTION_TYPE_REMOTE)
                    {
                        // we must ignore and we do ignore connections with a
                        // type of "" since they represent an uninitialized
                        // connection item (unconnected)
                        //
                        base->set_connection_type(base_connection::connection_type_t::CONNECTION_TYPE_DOWN);

                        if(!remote_communicator)
                        {
                            // disconnecting means it is gone so we can remove
                            // it from the communicator since the other end
                            // will be reconnected (we are not responsible
                            // for that in this case)
                            //
                            // Note: this one happens when the computer that
                            //       sent us a CONNECT later sends us the
                            //       DISCONNECT
                            //
                            f_communicator->remove_connection(connection);
                        }
                        else
                        {
                            // in this case we are in charge of attempting
                            // to reconnect until it works... however, it
                            // is likely that the other side just shutdown
                            // so we want to "induce a long enough pause"
                            // to avoid attempting to reconnect like crazy
                            //
                            remote_communicator->disconnect();
                            QString const addr(QString::fromUtf8(remote_communicator->get_client_addr().c_str()));
                            f_remote_snapcommunicators->shutting_down(addr);
                        }

                        // we just got some new services information,
                        // refresh our cache
                        //
                        refresh_heard_of();

                        if(!base->get_server_name().isEmpty())
                        {
                            snap::snap_communicator_message disconnected;
                            disconnected.set_command("DISCONNECTED");
                            disconnected.set_service(".");
                            disconnected.add_parameter("server_name", base->get_server_name());
                            broadcast_message(disconnected);
                        }
                    }
                    else
                    {
                        SNAP_LOG_ERROR("DISCONNECT was sent from a connection which is not of the right type (")
                                      (type == base_connection::connection_type_t::CONNECTION_TYPE_DOWN ? "down" : "client")
                                      (").");
                    }

                    // status changed for this connection
                    //
                    send_status(connection);
                    return;
                }
            }
            break;

        case 'F':
            if(command == "FORGET")
            {
                // whenever computers connect between each others, their
                // IP address gets added to our list of neighbors; this
                // means that the IP address is now stuck in the
                // computer's brain "forever"
                //
                QString const forget_ip(message.get_parameter("ip"));

                // self is not a connection that get broadcast messages
                // for snapcommunicator, so we also call the remove_neighbor()
                // function now
                //
                remove_neighbor(forget_ip);

                // once you notice many connection errors to other computers
                // that have been removed from your cluster, you want the
                // remaining computers to forget about that IP address and
                // it is done by broadcasting a FORGET message to everyone
                //
                if(!message.has_parameter("broadcast_hops"))
                {
                    // this was sent directly to this instance only,
                    // make sure to broadcast the message instead
                    //
                    snap::snap_communicator_message forget;
                    forget.set_command("FORGET");
                    forget.set_server("*");
                    forget.set_service("snapcommunicator");
                    forget.add_parameter("ip", forget_ip);
                    broadcast_message(forget);
                }
                return;
            }
            break;

        case 'G':
            if(command == "GOSSIP")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("GOSSIP is only accepted over a TCP connection.");
                    break;
                }

                if(base)
                {
                    // we got a GOSSIP message, this one will have addresses
                    // with various neighbors; we have two modes:
                    //
                    // 1) my_address=... is defined -- in this case the
                    //    remote host sent us his address because he was
                    //    not sure whether we knew about him; add that
                    //    address as a neighbor and go on as normal
                    //
                    // 2) heard_of=... is defined -- in this case, the
                    //    remote host received a GOSSIP from any one
                    //    snapcommunicator and it is propagating the
                    //    message; check all the IPs in that list and
                    //    if all are present in our list of neighbors,
                    //    do nothing; if all are not present, proceed
                    //    as normal in regard to attempt connections
                    //    and also forward our own GOSSIP to others
                    //    since we just heard of some new neighbors!
                    //
                    //    Note that at this point we use the Flooding
                    //    scheme and we implemented the Eventual
                    //    Consistency (because at some point in time
                    //    we eventually have an exact result.)
                    //
                    // When using (2) we are using what is called
                    // Gossiping in Computer Science. At thist time
                    // we use what is called the Flooding Algorithm.
                    //
                    // https://en.wikipedia.org/wiki/Flooding_(computer_networking)
                    //
                    // See also doc/focs2003-gossip.pdf
                    //
                    // We add two important features: (a) the list of
                    // nodes we already sent the message to, in
                    // order to avoid sending it to the same node
                    // over and over again; and (b) a serial number
                    // to be able to identify the message.
                    //
                    // Two other features that could be added are:
                    // (c) counting hops, after X hops were reached,
                    // stop forwarding the message because we should
                    // already have reached all nodes; (d) a specific
                    // date when the message times out.
                    //
                    // The serial number is used to know whether we
                    // already received a certain message. These can
                    // expire after a while (we may actually want to
                    // implement (d) from the get go so we know
                    // exactly when such expires).
                    //
                    // Our GOSSIP has one advantage, it is used to
                    // connect all the snapcommunicators together
                    // once. After that, the GOSSIP messages stop,
                    // no matter what (i.e. if a new snapcommunicator
                    // daemon is started, then the GOSSIP restart
                    // for that instance, but that's it.)
                    //
                    // However, we also offer a way to broadcast
                    // messages and these happen all the time
                    // (i.e. think of the snaplock broadcast messages).
                    // In those cases, we do not need to use the same
                    // algorithm because at that point we are expected
                    // to have a complete list of all the
                    // snapcommunicators available.
                    //
                    // (TODO: only we may not be connected to all of them,
                    // so we need to keep track of the snapcommunicators
                    // we are not connected to and ask others to do some
                    // forwarding!)
                    //
                    if(message.has_parameter("my_address"))
                    {
                        // this is a "simple" GOSSIP of a snapcommunicator
                        // telling us it exists and expects a connection
                        // from us
                        //
                        // in this case we just reply with RECEIVED to
                        // confirm that we get the GOSSIP message
                        //
                        QString const reply_to(message.get_parameter("my_address"));
                        add_neighbors(reply_to);
                        f_remote_snapcommunicators->add_remote_communicator(reply_to);

                        snap::snap_communicator_message reply;
                        reply.set_command("RECEIVED");
                        //verify_command(base, reply); -- in this case the remote snapcommunicator is not connected, so no HELP+COMMANDS and thus no verification possible
                        if(remote_communicator)
                        {
                            remote_communicator->send_message(reply);
                        }
                        else if(service_conn)
                        {
                            // Should this be an error instead since we only
                            // expect this message from remote snapcommunicators?
                            service_conn->send_message(reply);
                        }
                        else
                        {
                            // we have to have a remote or service connection here
                            //
                            throw snap::snap_exception("GOSSIP sent on a \"weird\" connection.");
                        }
                        return;
                    }
SNAP_LOG_ERROR("GOSSIP is not yet fully implemented.");
                    return;
                }
            }
            break;

        case 'H':
            if(command == "HELP")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("HELP is only accepted over a TCP connection.");
                    break;
                }

                if(base)
                {
                    // reply with COMMANDS
                    //
                    snap::snap_communicator_message reply;
                    reply.set_command("COMMANDS");

                    // list of commands understood by snapcommunicator
                    reply.add_parameter("list", "ACCEPT,COMMANDS,CONNECT,DISCONNECT,FORGET,GOSSIP,HELP,LISTENLOADAVG,LOADAVG,LOG,PUBLIC_IP,QUITTING,REFUSE,REGISTER,REGISTERFORLOADAVG,RELOADCONFIG,SERVICES,SHUTDOWN,STOP,UNKNOWN,UNREGISTER,UNREGISTERFORLOADAVG");

                    //verify_command(base, reply); -- this verification does not work with remote snap communicator connections
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("HELP sent on a \"weird\" connection.");
                    }
                    return;
                }
            }
            break;

        case 'L':
            if(command == "LOADAVG")
            {
                save_loadavg(message);
                return;
            }
            else if(command == "LISTENLOADAVG")
            {
                listen_loadavg(message);
                return;
            }
            else if(command == "LOG")
            {
                SNAP_LOG_INFO("Logging reconfiguration.");
                snap::logging::reconfigure();
                return;
            }
            else if(command == "LISTSERVICES")
            {
                snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
                std::string list;
                std::for_each(
                        all_connections.begin(),
                        all_connections.end(),
                        [&list](auto const & c)
                        {
                            if(!list.empty())
                            {
                                list += ", ";
                            }
                            list += c->get_name();
                        });
                SNAP_LOG_INFO("current list of connections: ")(list);
                return;
            }
            break;

        case 'P':
            if(command == "PUBLIC_IP")
            {
                if(service_conn)
                {
                    snap::snap_communicator_message reply;
                    reply.set_command("SERVER_PUBLIC_IP");
                    reply.add_parameter("public_ip", QString::fromUtf8(f_public_ip.c_str()));
                    verify_command(base, reply);
                    service_conn->send_message(reply);
                    return;
                }
                else
                {
                    // we have to have a remote or service connection here
                    //
                    throw snap::snap_exception("PUBLIC_IP sent on a \"weird\" connection.");
                }
            }
            break;

        case 'Q':
            if(command == "QUITTING")
            {
                // if this becomes problematic, we may need to serialize
                // our messages to know which was ignored...
                //
                SNAP_LOG_INFO("Received a QUITTING as a reply to a message.");
                return;
            }
            break;

        case 'R':
            if(command == "REFUSE")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("REFUSE is only accepted over a TCP connection.");
                    break;
                }

                // we were not connected so we do not have to
                // disconnect; mark that corresponding server
                // as too busy and try connecting again much
                // later...
                //
                QString addr;
                if(remote_communicator)
                {
                    addr = QString::fromUtf8(remote_communicator->get_client_addr().c_str());
                }
                //else if(service_conn) -- this should not happen
                //{
                //    addr = QString::fromUtf8(service_conn->get_client_addr().c_str());
                //}
                else
                {
                    // we have to have a remote or service connection here
                    //
                    throw snap::snap_exception("REFUSE sent on a \"weird\" connection.");
                }
                if(message.has_parameter("shutdown"))
                {
                    f_remote_snapcommunicators->shutting_down(addr);
                }
                else
                {
                    f_remote_snapcommunicators->too_busy(addr);
                }

                // we are responsible to try again later, so we do not
                // lose the connection, but we need to disconnect
                //
                //f_communicator->remove_connection(connection);
                remote_communicator->disconnect();
                return;
            }
            else if(command == "REGISTER")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("REGISTER is only accepted over a TCP connection.");
                    break;
                }

                if(base)
                {
                    if(!message.has_parameter("service")
                    || !message.has_parameter("version"))
                    {
                        SNAP_LOG_ERROR("REGISTER was called without a \"service\" and/or a \"version\" parameter, both are mandatory.");
                        return;
                    }
                    if(message.get_integer_parameter("version") != snap::snap_communicator::VERSION)
                    {
                        SNAP_LOG_ERROR("REGISTER was called with an incompatible version. Expected ")(snap::snap_communicator::VERSION)(", received ")(message.get_integer_parameter("version"));
                        return;
                    }
                    // the "service" parameter is the name of the service,
                    // now we can process messages for this service
                    //
                    QString const service_name(message.get_parameter("service"));
                    connection->set_name(service_name);
                    if(service_conn)
                    {
                        service_conn->properly_named();
                    }

                    base->set_connection_type(base_connection::connection_type_t::CONNECTION_TYPE_LOCAL);

                    // connection is up now
                    //
                    base->connection_started();

                    // tell the connect we are ready
                    // (the connection uses that as a trigger to start work)
                    //
                    snap::snap_communicator_message reply;
                    reply.set_command("READY");
                    //verify_command(base, reply); -- we cannot do that here since we did not yet get the COMMANDS reply
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("REGISTER sent on a \"weird\" connection (1).");
                    }

                    // request the COMMANDS of this connection
                    //
                    snap::snap_communicator_message help;
                    help.set_command("HELP");
                    //verify_command(base, help); -- we cannot do that here since we did not yet get the COMMANDS reply
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(help);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(help);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("REGISTER sent on a \"weird\" connection (2).");
                    }

                    // status changed for this connection
                    //
                    send_status(connection);

                    // remove cached messages that timed out
                    //
                    time_t const now(time(nullptr));
                    f_local_message_cache.erase(
                                  std::remove_if(
                                      f_local_message_cache.begin()
                                    , f_local_message_cache.end()
                                    , [now](auto const & cached_message)
                                    {
                                        return now > cached_message.f_timeout_timestamp;
                                    })
                                , f_local_message_cache.end()
                                );

                    // if we have local messages that were cached, then
                    // forward them now
                    //
                    // we use an index to make sure we can cleanly remove
                    // messages from the cache as we forward them to the
                    // new service
                    //
                    size_t max_messages(f_local_message_cache.size());
                    for(size_t idx(0); idx < max_messages; )
                    {
                        snap::snap_communicator_message const & m(f_local_message_cache[idx].f_message);
                        if(m.get_service() == service_name)
                        {
                            // TBD: should we remove the service name before forwarding?
                            //      (we have two instances)
                            //
                            //verify_command(base, m); -- we cannot do that here since we did not yet get the COMMANDS reply
                            if(remote_communicator)
                            {
                                remote_communicator->send_message(m);
                            }
                            else if(service_conn)
                            {
                                service_conn->send_message(m);
                            }
                            else
                            {
                                // we have to have a remote or service connection here
                                //
                                throw snap::snap_exception("REGISTER sent on a \"weird\" connection (3).");
                            }

                            // whether it works, remove the message from
                            // the cache
                            //
                            f_local_message_cache.erase(f_local_message_cache.begin() + idx);
                            --max_messages;
                            // no ++idx since we removed the item at 'idx'
                        }
                        else
                        {
                            ++idx;
                        }
                    }
                    return;
                }
            }
            else if(command == "REGISTERFORLOADAVG")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("REGISTERFORLOADAVG is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    base->set_wants_loadavg(true);
                    f_loadavg_timer->set_enable(true);
                    return;
                }
            }
            else if(command == "RELOADCONFIG")
            {
                // we need a full restart in this case (because when we
                // restart snapcommunicator it also automatically restart
                // all of its dependencies!)
                //
                // also if you are a programmer we cannot do a systemctl
                // restart so we just skip the feature...
                //
                f_force_restart = true;
                shutdown(false);
                return;
            }
            break;

        case 'S':
            if(command == "SHUTDOWN")
            {
                shutdown(true);
                return;
            }
            else if(command == "STOP")
            {
                shutdown(false);
                return;
            }
            else if(command == "SERVICESTATUS")
            {
                QString const & service_name(message.get_parameter("service"));
                if(service_name.isEmpty())
                {
                    SNAP_LOG_ERROR("The SERVICESTATUS service parameter cannot be an empty string.");
                    return;
                }
                snap::snap_communicator::snap_connection::vector_t const & named_connections(f_communicator->get_connections());
                auto const named_service(std::find_if(
                              named_connections.begin()
                            , named_connections.end()
                            , [service_name](auto const & named_connection)
                            {
                                return named_connection->get_name() == service_name;
                            }));
                if(named_service == named_connections.end())
                {
                    // service is totally unknown
                    //
                    // create a fake connection so we can call the
                    // send_status() function
                    //
                    snap::snap_communicator::snap_connection::pointer_t fake_connection(std::make_shared<snap::snap_communicator::snap_timer>(0));
                    fake_connection->set_name(service_name);
                    send_status(fake_connection, &connection);
                }
                else
                {
                    send_status(*named_service, &connection);
                }
                return;
            }
            break;

        case 'U':
            if(command == "UNKNOWN")
            {
                SNAP_LOG_ERROR("we sent command \"")
                              (message.get_parameter("command"))
                              ("\" to \"")
                              (connection->get_name())
                              ("\" which told us it does not know that command so we probably did not get the expected result.");
                return;
            }
            else if(command == "UNREGISTER")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("UNREGISTER is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    if(!message.has_parameter("service"))
                    {
                        SNAP_LOG_ERROR("UNREGISTER was called without a \"service\" parameter, which is mandatory.");
                        return;
                    }
                    // also remove all the connection types
                    // an empty string represents an unconnected item
                    //
                    base->set_connection_type(base_connection::connection_type_t::CONNECTION_TYPE_DOWN);

                    // connection is down now
                    //
                    base->connection_ended();

                    // status changed for this connection
                    //
                    send_status(connection);

                    // now remove the service name
                    // (send_status() needs the name to still be in place!)
                    //
                    QString const save_name(connection->get_name());
                    connection->set_name("");

                    // get rid of that connection now (it is faster than
                    // waiting for the HUP because it will not be in the
                    // list of connections on the next loop.)
                    //
                    f_communicator->remove_connection(connection);

                    return;
                }
            }
            else if(command == "UNREGISTERFORLOADAVG")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("UNREGISTERFORLOADAVG is only accepted over a TCP connection.");
                    break;
                }

                if(base)
                {
                    base->set_wants_loadavg(false);
                    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
                    if(all_connections.end() == std::find_if(
                            all_connections.begin(),
                            all_connections.end(),
                            [](auto const & c)
                            {
                                base_connection::pointer_t b(std::dynamic_pointer_cast<base_connection>(c));
                                return b->wants_loadavg();
                            }))
                    {
                        // no more connection requiring LOADAVG messages
                        // so stop the timer
                        //
                        f_loadavg_timer->set_enable(false);
                    }
                    return;
                }
            }
            break;

        }

        // if they used a TCP connection to send this message, let the caller
        // know that we do not understand his message
        //
        if(!udp)
        {
            snap::snap_communicator_message reply;
            reply.set_command("UNKNOWN");
            reply.add_parameter("command", command);
            verify_command(base, reply);
            if(remote_communicator)
            {
                remote_communicator->send_message(reply);
            }
            else if(service_conn)
            {
                service_conn->send_message(reply);
            }
            else
            {
                // we have to have a remote or service connection here
                //
                throw snap::snap_exception("HELP sent on a \"weird\" connection.");
            }
        }

        // done
        SNAP_LOG_ERROR("unknown command \"")(command)("\" or not sent from what is considered the correct connection for that message.");
        return;
    }

    //
    // the message includes a service name, so we want to forward that
    // message to that service
    //
    // for that purpose we consider the following three lists:
    //
    // 1. we have the service in our local services, we must forward it
    //    to that connection; if the connection is not up and running yet,
    //    cache the information
    //
    // 2. the service is not one of ours, but we found a remote
    //    snapcommunicator server that says it is his, forward the
    //    message to that snapcommunicator instead
    //
    // 3. the service is in the "heard of" list of services, send that
    //    message to that snapcommunicator, it will then forward it
    //    to the correct server (or another proxy...)
    //
    // 4. the service cannot be found anywhere, we save it in our remote
    //    cache (i.e. because it will only be possible to send that message
    //    to a remote snapcommunicator and not to a service on this system)
    //

//SNAP_LOG_TRACE("---------------- got message for [")(server_name)("] / [")(service)("]");

    // broadcasting?
    //
    if(service == "*"
    || service == "?"
    || service == ".")
    {
        if(!server_name.isEmpty()
        && server_name != "*"
        && (service == "*" || service == "?"))
        {
            // do not send the message in this case!
            //
            // we cannot at the same time send it to this local server
            // and broadcast it to other servers... it is contradictory;
            // either set the server to "*" or empty, or do not broadcast
            //
            SNAP_LOG_ERROR("you cannot at the same time specify a server name (")(server_name)(") and \"*\" or \"?\" as the service.");
            return;
        }
        broadcast_message(message);
        return;
    }

    base_connection_vector_t accepting_remote_connections;
    bool const all_servers(server_name.isEmpty() || server_name == "*");
    {
        // service is local, check whether the service is registered,
        // if registered, forward the message immediately
        //
        snap::snap_communicator::snap_connection::vector_t const & connections(f_communicator->get_connections());
        for(auto const & nc : connections)
        {
            base_connection::pointer_t base_conn(std::dynamic_pointer_cast<base_connection>(nc));
            if(base_conn)
            {
                // verify that there is a server name in all connections
                // (if not we have a bug somewhere else)
                //
                if(base_conn->get_server_name().isEmpty())
                {
                    if(!f_server->is_debug())
                    {
                        // ignore in non-debug versions because a throw
                        // completely breaks snapcommunicator... and it
                        // is not that important at this point without
                        // a programmer debugging this software
                        //
                        continue;
                    }
                    service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
                    if(conn)
                    {
                        throw std::runtime_error("server name missing in connection " + conn->get_name().toStdString() + "...");
                    }
                    switch(base_conn->get_connection_type())
                    {
                    case base_connection::connection_type_t::CONNECTION_TYPE_DOWN:
                        // not connected yet, forget about it
                        continue;

                    case base_connection::connection_type_t::CONNECTION_TYPE_LOCAL:
                        throw std::runtime_error("server name missing in connection \"local service\"...");

                    case base_connection::connection_type_t::CONNECTION_TYPE_REMOTE:
                        throw std::runtime_error("server name missing in connection \"remote snapcommunicator\"...");

                    }
                }

                if(all_servers
                || server_name == base_conn->get_server_name())
                {
                    service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
                    if(conn)
                    {
                        if(conn->get_name() == service)
                        {
                            // we have such a service, just forward to it now
                            //
                            // TBD: should we remove the service name before forwarding?
                            //
                            try
                            {
                                verify_command(conn, message);
                                conn->send_message(message);
                            }
                            catch(std::runtime_error const & e)
                            {
                                // ignore the error because this can come from an
                                // external source (i.e. snapsignal) where an end
                                // user may try to break the whole system!
                                //
                                SNAP_LOG_DEBUG("snapcommunicator failed to send a message to connection \"")
                                              (conn->get_name())
                                              ("\" (error: ")
                                              (e.what())
                                              (")");
                            }
                            // we found a specific service to which we could
                            // forward the message so we can stop here
                            //
                            return;
                        }
                        else
                        {
                            // if not a local connection with the proper name,
                            // still send it to that connection but only if it
                            // is a remote connection
                            //
                            base_connection::connection_type_t const type(base_conn->get_connection_type());
                            if(type == base_connection::connection_type_t::CONNECTION_TYPE_REMOTE)
                            {
                                accepting_remote_connections.push_back(conn);
                            }
                        }
                    }
                    else
                    {
                        remote_snap_communicator_pointer_t remote_connection(std::dynamic_pointer_cast<remote_snap_communicator>(nc));
                        // TODO: limit sending to remote only if they have that service?
                        //       (if we have the 'all_servers' set, otherwise it is not
                        //       required, for sure... also, if we have multiple remote
                        //       connections that support the same service we should
                        //       randomize which one is to receive that message--or
                        //       even better, check the current server load--but
                        //       seriously, if none of our direct connections know
                        //       of that service, we need to check for those that heard
                        //       of that service, and if that is also empty, send to
                        //       all... for now we send to all anyway)
                        //
                        if(remote_connection /*&& remote_connection->has_service(service)*/)
                        {
                            accepting_remote_connections.push_back(remote_connection);
                        }
                    }
                }
            }
        }

        auto transmission_report = [&message, &base, &remote_communicator, &service_conn]()
        {
            if(message.has_parameter("transmission_report"))
            {
                QString const report(message.get_parameter("transmission_report"));
                if(report == "failure")
                {
                    snap::snap_communicator_message reply;
                    reply.set_command("TRANSMISSIONREPORT");
                    reply.add_parameter("status", "failed");
                    //verify_command(base, reply);
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("No valid connection to send a reply.");
                    }
                }
            }
        };

        if((all_servers || server_name == f_server_name)
        && f_local_services_list.contains(service))
        {
            // its a service that is expected on this computer, but it is not
            // running right now... so cache the message
            //
            // TODO: we want to look into several things:
            //
            //   (1) limiting the cache size
            //   (2) not cache more than one signal message (i.e. PING, STOP, LOG...)
            //   (3) save the date when the message arrived and keep it in
            //       the cache only for a limited time (i.e. 5h)
            //
            QString cache;
            if(message.has_parameter("cache"))
            {
                cache = message.get_parameter("cache");
            }
            if(cache != "no")
            {
                // convert the cache into a map of parameters
                //
                snap::snap_string_list cache_parameters(cache.split(";"));
                QMap<QString, QString> params;
                for(auto p : cache_parameters)
                {
                    snap::snap_string_list param(p.split("="));
                    if(param.size() == 2)
                    {
                        params[param[0]] = param[1];
                    }
                }

                // get TTL if defined (1 min. per default)
                //
                int ttl(60);
                if(params.contains("ttl"))
                {
                    bool ok(false);
                    ttl = params["ttl"].toInt(&ok, 10);
                    if(!ok || ttl < 10 || ttl > 86400)
                    {
                        SNAP_LOG_ERROR("invalid TTL in message [")(message.to_message())("]");

                        // revert to default
                        ttl = 60;
                    }
                }

                // save the message
                //
                message_cache cache_message;
                cache_message.f_timeout_timestamp = time(nullptr) + ttl;
                cache_message.f_message = message;
                f_local_message_cache.push_back(cache_message);
            }
            transmission_report();
            return;
        }

        // if attempting to send to self, we cannot go on from here
        //
        if(server_name == f_server_name)
        {
            if(!service.startsWith("lock_"))
            {
                SNAP_LOG_DEBUG("received event \"")(command)("\" for local service \"")(service)("\", which is not currently registered. Dropping message.");
            }
            transmission_report();
            return;
        }
    }

    if(!accepting_remote_connections.empty())
    {
        broadcast_message(message, accepting_remote_connections);
    }
}


void snap_communicator_server::broadcast_message(snap::snap_communicator_message const & message, base_connection_vector_t const & accepting_remote_connections)
{
    QString broadcast_msgid;
    QString informed_neighbors;
    int hops(0);
    time_t timeout(0);

    // note: the "broacast_msgid" is required when we end up sending that
    //       message forward to some other computers; so we have to go
    //       through that if() block; however, the timeout was already
    //       already checked, so we probably would not need it to do
    //       it again?
    //
    if(message.has_parameter("broadcast_msgid"))
    {
        // check whether the message already timed out
        //
        // this is a safety feature of our broadcasting capability
        // which should rarely be activated unless you have multiple
        // data center locations
        //
        timeout = message.get_integer_parameter("broadcast_timeout");
        time_t const now(time(nullptr));
        if(timeout < now)
        {
            return;
        }

        // check whether we already received that message, if so ignore
        // the second instance (it should not happen with the list of
        // neighbors included in the message, but just in case...)
        //
        broadcast_msgid = message.get_parameter("broadcast_msgid");
        auto const received_it(f_received_broadcast_messages.find(broadcast_msgid));
        if(received_it != f_received_broadcast_messages.cend())     // message arrived again?
        {
            // note that although we include neighbors it is normal that
            // this happens in a cluster where some computers are not
            // aware of certain nodes; for example, if A sends a
            // message to B and C, both B and C know of a node D
            // which is unknown to A, then both B and C will end
            // up forward that same message to D, so D will discard
            // the second instance it receives.
            //
            return;
        }

        // delete "received messages" that have now timed out (because
        // such are not going to be forwarded since we check the timeout
        // of a message early and prevent the broadcasting in that case)
        //
        // XXX: I am thinking that this loop should probably be run before
        //      the "broadcast_timeout" test above...
        //
        for(auto it(f_received_broadcast_messages.cbegin()); it != f_received_broadcast_messages.cend(); )
        {
            if(it->second < now)
            {
                // this one timed out, we do not need to keep it in this
                // list, so erase (notice the it++ opposed to the else
                // that uses ++it)
                //
                f_received_broadcast_messages.erase(it++);
            }
            else
            {
                ++it;
            }
        }

        // add the new message after we check for timed out entries
        // so that way we avoid going through this new entry within
        // the previous loop
        //
        f_received_broadcast_messages[broadcast_msgid] = timeout;

        // Note: we skip the canonicalization on this list of neighbors
        //       because we assume only us (snapcommunicator) handles
        //       that message and we know that it is already
        //       canonicalized here
        //
        informed_neighbors = message.get_parameter("broadcast_informed_neighbors");

        // get the number of hops this message already performed
        //
        hops = message.get_integer_parameter("broadcast_hops");
    }

    snap::snap_string_list informed_neighbors_list;
    if(!informed_neighbors.isEmpty())
    {
        informed_neighbors_list = informed_neighbors.split(',', QString::SkipEmptyParts);
    }

    // we always broadcast to all local services
    snap::snap_communicator::snap_connection::vector_t broadcast_connection;

    if(accepting_remote_connections.empty())
    {
        QString destination("?");
        QString const service(message.get_service());
        if(service != "."
        && service != "?"
        && service != "*")
        {
            destination = message.get_server();
            if(destination.isEmpty())
            {
                destination = "?";
            }
        }
        else
        {
            destination = service;
        }
        bool const all(hops < 5 && destination == "*");
        bool const remote(hops < 5 && (all || destination == "?"));

        snap::snap_communicator::snap_connection::vector_t const & connections(f_communicator->get_connections());
        for(auto const & nc : connections)
        {
            // try for a service or snapcommunicator that connected to us
            //
            service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
            remote_snap_communicator_pointer_t remote_communicator;
            if(!conn)
            {
                remote_communicator = std::dynamic_pointer_cast<remote_snap_communicator>(nc);
            }
            bool broadcast(false);
            if(conn)
            {
                switch(conn->get_address().get_network_type())
                {
                case snap_addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK:
                    // these are localhost services, avoid sending the
                    // message is the destination does not know the
                    // command
                    //
                    if(conn->understand_command(message.get_command())) // destination: "*" or "?" or "."
                    {
                        //verify_command(conn, message); -- we reach this line only if the command is understood, it is therefore good
                        conn->send_message(message);
                    }
                    break;

                case snap_addr::addr::network_type_t::NETWORK_TYPE_PRIVATE:
                    // these are computers within the same local network (LAN)
                    // we forward messages if at least 'remote' is true
                    //
                    broadcast = remote; // destination: "*" or "?"
                    break;

                case snap_addr::addr::network_type_t::NETWORK_TYPE_PUBLIC:
                    // these are computers in another data center
                    // we forward messages only when 'all' is true
                    //
                    broadcast = all; // destination: "*"
                    break;

                default:
                    // unknown/unexpected type of IP address, totally ignore
                    break;

                }
            }
            else if(remote_communicator)
            {
                // another snapcommunicator that connected to us
                //
                switch(remote_communicator->get_address().get_network_type())
                {
                case snap_addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK:
                    {
                        static bool warned(false);
                        if(!warned)
                        {
                            warned = true;
                            SNAP_LOG_WARNING("remote snap communicator was connected on a LOOPBACK IP address...");
                        }
                    }
                    break;

                case snap_addr::addr::network_type_t::NETWORK_TYPE_PRIVATE:
                    // these are computers within the same local network (LAN)
                    // we forward messages if at least 'remote' is true
                    //
                    broadcast = remote; // destination: "*" or "?"
                    break;

                case snap_addr::addr::network_type_t::NETWORK_TYPE_PUBLIC:
                    // these are computers in another data center
                    // we forward messages only when 'all' is true
                    //
                    broadcast = all; // destination: "*"
                    break;

                default:
                    // unknown/unexpected type of IP address, totally ignore
                    break;

                }
            }
            if(broadcast)
            {
                // get the IP address of the remote snapcommunicator
                //
                QString const address(QString::fromUtf8((conn ? conn->get_address() : remote_communicator->get_address()).get_ipv4or6_string(false, false).c_str()));
                if(!informed_neighbors_list.contains(address))
                {
                    // not in the list of informed neighbors, add it and
                    // keep nc in a list that we can use to actually send
                    // the broadcast message
                    //
                    informed_neighbors_list << address;
                    broadcast_connection.push_back(nc);
                }
            }
        }
    }
    else
    {
        // we already have a list, copy that list only as it is already
        // well defined
        //
        std::for_each(
            accepting_remote_connections.begin(),
            accepting_remote_connections.end(),
            [&broadcast_connection, &informed_neighbors_list](auto const & nc)
            {
                // the dynamic_cast<>() should always work in this direction
                //
                service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
                if(conn)
                {
                    QString const address(QString::fromUtf8(conn->get_address().get_ipv4or6_string(false, false).c_str()));
                    if(!informed_neighbors_list.contains(address))
                    {
                        // not in the list of informed neighbors, add it and
                        // keep conn in a list that we can use to actually
                        // send the broadcast message
                        //
                        informed_neighbors_list << address;
                        broadcast_connection.push_back(conn);
                    }
                }
                else
                {
                    remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(nc));
                    if(remote_communicator)
                    {
                        QString const address(QString::fromUtf8(remote_communicator->get_address().get_ipv4or6_string(false, false).c_str()));
                        if(!informed_neighbors_list.contains(address))
                        {
                            // not in the list of informed neighbors, add it and
                            // keep conn in a list that we can use to actually
                            // send the broadcast message
                            //
                            informed_neighbors_list << address;
                            broadcast_connection.push_back(remote_communicator);
                        }
                    }
                }
            });
    }

    if(!broadcast_connection.empty())
    {
        // we are broadcasting now (Gossiping a regular message);
        // for the gossiping to work, we include additional
        // information in the message
        //
        QString const originator(QString::fromUtf8(f_my_address.get_ipv4or6_string().c_str()));
        if(!informed_neighbors_list.contains(originator))
        {
            // include self since we already know of the message too!
            // (no need for others to send it back to us)
            //
            informed_neighbors_list << originator;
        }

        // message is 'const', so we need to create a copy
        snap::snap_communicator_message broadcast_msg(message);

        // generate a unique broadcast message identifier if we did not
        // yet have one, it is very important to NOT generate a new message
        // in a many to many broadcasting system because you must block
        // duplicates here
        //
        ++g_broadcast_sequence;
        if(broadcast_msgid.isEmpty())
        {
            broadcast_msgid = QString("%1-%2").arg(f_server_name).arg(g_broadcast_sequence);
        }
        broadcast_msg.add_parameter("broadcast_msgid", broadcast_msgid);

        // increase the number of hops; if we reach the limit, we still
        // want to forward the message, the destination will not forward
        // (broadcast) more, but it will possibly send that to its own
        // services
        //
        broadcast_msg.add_parameter("broadcast_hops", hops + 1);

        // mainly noise at this point, but I include the originator so
        // we can track that back if needed for debug purposes
        //
        broadcast_msg.add_parameter("broadcast_originator", originator);

        // define a timeout if this is the originator
        //
        if(timeout == 0)
        {
            // give message 10 seconds to arrive to any and all destinations
            timeout = time(nullptr) + 10;
        }
        broadcast_msg.add_parameter("broadcast_timeout", timeout);

        // note that we currently define the list of neighbors BEFORE
        // sending the message (anyway the send_message() just adds the
        // message to a memory cache at this point, so whether it will
        // be sent is not known until later.)
        //
        broadcast_msg.add_parameter("broadcast_informed_neighbors", informed_neighbors_list.join(","));

        for(auto const & bc : broadcast_connection)
        {
            service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(bc));
            if(conn)
            {
                conn->send_message(broadcast_msg);
            }
            else
            {
                remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(bc));
                if(remote_communicator) // this should always be true, but to be double sure...
                {
                    remote_communicator->send_message(broadcast_msg);
                }
            }
        }
    }
}


/** \brief Send the current status of a client to connections.
 *
 * Some connections (at this time only the snapwatchdog) may be interested
 * by the STATUS event. Any connection that understands the STATUS
 * event will be sent that event whenever the status of a connection
 * changes (specifically, on a REGISTER and on an UNREGISTER or equivalent.)
 *
 * \param[in] client  The client that just had its status changed.
 * \param[in] reply_connection  If not nullptr, the connection where the
 *                              STATUS message gets sent.
 */
void snap_communicator_server::send_status(
              snap::snap_communicator::snap_connection::pointer_t connection
            , snap::snap_communicator::snap_connection::pointer_t * reply_connection)
{
    snap::snap_communicator_message reply;
    reply.set_command("STATUS");
    reply.add_parameter("cache", "no");

    // the name of the service is the name of the connection
    reply.add_parameter("service", connection->get_name());

    base_connection::pointer_t base_connection(std::dynamic_pointer_cast<base_connection>(connection));
    if(base_connection)
    {
        // check whether the connection is now up or down
        base_connection::connection_type_t const type(base_connection->get_connection_type());
        reply.add_parameter("status", type == base_connection::connection_type_t::CONNECTION_TYPE_DOWN ? "down" : "up");

        // get the time when it was considered up
        int64_t const up_since(base_connection->get_connection_started());
        if(up_since != -1)
        {
            reply.add_parameter("up_since", up_since / 1000000); // send up time in seconds
        }

        // get the time when it was considered down (if not up yet, this will be skipped)
        int64_t const down_since(base_connection->get_connection_ended());
        if(down_since != -1)
        {
            reply.add_parameter("down_since", down_since / 1000000); // send up time in seconds
        }
    }

    if(reply_connection != nullptr)
    {
        service_connection::pointer_t sc(std::dynamic_pointer_cast<service_connection>(*reply_connection));
        if(sc)
        {
            // if the verify_command() fails then it means the caller has to
            // create a handler for the STATUS message
            //
            verify_command(sc, reply);
            sc->send_message(reply);
        }
    }
    else
    {
        // we have the message, now we need to find the list of connections
        // interested by the STATUS event
        //
        // TODO: use the broadcast_message() function instead? (with service set to ".")
        //
        snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
        for(auto const & conn : all_connections)
        {
            service_connection::pointer_t sc(std::dynamic_pointer_cast<service_connection>(conn));
            if(!sc)
            {
                // not a service_connection, ignore (i.e. servers)
                continue;
            }

            if(sc->understand_command("STATUS"))
            {
                // send that STATUS message
                //verify_command(sc, reply); -- we reach this line only if the command is understood
                sc->send_message(reply);
            }
        }
    }
}


/** \brief Request LOADAVG messages from a snapcommunicator.
 *
 * This function gets called whenever a local service sends us a
 * request to listen to the LOADAVG messages of a specific
 * snapcommunicator.
 *
 * \param[in] message  The LISTENLOADAVG message.
 */
void snap_communicator_server::listen_loadavg(snap::snap_communicator_message const & message)
{
    QString const ips(message.get_parameter("ips"));

    snap::snap_string_list ip_list(ips.split(","));

    // we have to save those as IP addresses since the remote
    // snapcommunicators come and go and we have to make sure
    // that all get our REGISERFORLOADAVG message when they
    // come back after a broken link
    //
    for(auto const & ip : ip_list)
    {
        if(!f_registered_neighbors_for_loadavg.contains(ip))
        {
            // add this one, it was not there yet
            //
            f_registered_neighbors_for_loadavg[ip] = true;

            register_for_loadavg(ip);
        }
    }
}


void snap_communicator_server::register_for_loadavg(QString const & ip)
{
    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
    auto const & it(std::find_if(
            all_connections.begin(),
            all_connections.end(),
            [ip](auto const & connection)
            {
                remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(connection));
                if(remote_communicator)
                {
                    return remote_communicator->get_my_address() == ip;
                }
                else
                {
                    service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(connection));
                    if(service_conn)
                    {
                        return service_conn->get_my_address() == ip;
                    }
                }

                return false;
            }));

    if(it != all_connections.end())
    {
        // there is such a connection, send it a request for
        // LOADAVG message
        //
        snap::snap_communicator_message register_message;
        register_message.set_command("REGISTERFORLOADAVG");

        remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(*it));
        if(remote_communicator)
        {
            remote_communicator->send_message(register_message);
        }
        else
        {
            service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(*it));
            if(service_conn)
            {
                service_conn->send_message(register_message);
            }
        }
    }
}


void snap_communicator_server::save_loadavg(snap::snap_communicator_message const & message)
{
    QString const avg_str(message.get_parameter("avg"));
    QString const my_address(message.get_parameter("my_address"));
    QString const timestamp_str(message.get_parameter("timestamp"));

    snap::loadavg_item item;

    // Note: we do not use the port so whatever number here is fine
    snap_addr::addr a(snap_addr::addr(my_address.toUtf8().data(), "127.0.0.1", 4040, "tcp"));
    a.set_port(4040); // actually force the port so in effect it is ignored
    a.get_ipv6(item.f_address);

    bool ok(false);
    item.f_avg = avg_str.toFloat(&ok);
    if(!ok
    || item.f_avg < 0.0)
    {
        return;
    }

    item.f_timestamp = timestamp_str.toLongLong(&ok, 10);
    if(!ok
    || item.f_timestamp < SNAP_UNIX_TIMESTAMP(2016, 1, 1, 0, 0, 0))
    {
        return;
    }

    snap::loadavg_file file;
    file.load();
    file.add(item);
    file.save();
}


void snap_communicator_server::process_load_balancing()
{
    std::ifstream in;
    in.open("/proc/loadavg", std::ios::in | std::ios::binary);
    if(in.is_open())
    {
        std::string avg_str;
        for(;;)
        {
            char c;
            in.read(&c, 1);
            if(in.fail())
            {
                SNAP_LOG_ERROR("error reading the /proc/loadavg data.");
                return;
            }
            if(std::isspace(c))
            {
                // we only read the first number (1 min. load avg.)
                break;
            }
            avg_str += c;
        }

        // we really only need the first number, we would not know what
        // to do with the following ones at this time...
        // (although that could help know whether the load average is
        // going up or down, but it's not that easy, really.)
        //
        // we divide by the number of processors because each computer
        // could have a different number of processors and a load
        // average of 1 on a computer with 16 processors really
        // represents 1/16th of the machine capacity.
        //
        float const avg(std::stof(avg_str) / f_number_of_processors);

        // TODO: see whether the current epsilon is good enough
        if(std::fabs(f_last_loadavg - avg) < 0.1f)
        {
            // do not send if it did not change lately
            return;
        }
        f_last_loadavg = avg;

        snap::snap_communicator_message load_avg;
        load_avg.set_command("LOADAVG");
        load_avg.add_parameter("avg", QString("%1").arg(avg));
        load_avg.add_parameter("my_address", QString::fromUtf8(f_my_address.get_ipv4or6_string(true).c_str()));
        load_avg.add_parameter("timestamp", QString("%1").arg(time(nullptr)));

        snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
        std::for_each(
                all_connections.begin(),
                all_connections.end(),
                [load_avg](auto const & connection)
                {
                    base_connection::pointer_t base(std::dynamic_pointer_cast<base_connection>(connection));
                    if(base
                    && base->wants_loadavg())
                    {
                        remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(connection));
                        if(remote_communicator)
                        {
                            remote_communicator->send_message(load_avg);
                        }
                        else
                        {
                            service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(connection));
                            if(service_conn)
                            {
                                service_conn->send_message(load_avg);
                            }
                        }
                    }
                });
    }
    else
    {
        SNAP_LOG_ERROR("error opening file \"/proc/loadavg\".");
    }
}


/** \brief Return the list of services offered on this computer.
 */
QString snap_communicator_server::get_local_services() const
{
    return f_local_services;
}


/** \brief Return the list of services we heard of.
 */
QString snap_communicator_server::get_services_heard_of() const
{
    return f_services_heard_of;
}


/** \brief Add neighbors to this communicator server.
 *
 * Whenever a snap communicator connects to another snap communicator
 * server, it is given a list of neighbors. These are added using
 * this function. In the end, all servers are expected to have a
 * complete list of all the neighbors.
 *
 * \todo
 * Make this list survive restarts of the snap communicator server.
 *
 * \param[in] new_neighbors  The list of new neighbors
 */
void snap_communicator_server::add_neighbors(QString const & new_neighbors)
{
    SNAP_LOG_DEBUG("Add neighbors: ")(new_neighbors);

    // first time initialize and read the cache file
    //
    read_neighbors();

    if(!new_neighbors.isEmpty())
    {
        bool changed(false);
        snap::snap_string_list list(new_neighbors.split(',', QString::SkipEmptyParts));
        for(auto const & s : list)
        {
            if(!f_all_neighbors.contains(s))
            {
                changed = true;
                f_all_neighbors[s] = true;

                // in case we are already running we want to also add
                // the corresponding connection
                //
                f_remote_snapcommunicators->add_remote_communicator(s);
            }
        }

        // if the map changed, then save the change in the cache
        //
        // TODO: we may be able to optimize this by not saving on each and
        //       every call; although since it should remain relatively
        //       small, we should be fine (yes, 8,000 computers is still
        //       a small file in this cache.)
        //
        if(changed)
        {
            save_neighbors();
        }
    }
}


/** \brief Remove a neighbor from our list of neighbors.
 *
 * This function removes a neighbor from the cache of this machine. If
 * the neighbor is also defined in the configuration file, such as
 * /etc/snapwebsites/snapcommunicator.conf, then the IP will not be
 * forgotten any time soon.
 *
 * \param[in] neighbor  The neighbor to be removed.
 */
void snap_communicator_server::remove_neighbor(QString const & neighbor)
{
    SNAP_LOG_DEBUG("Forgetting neighbor: ")(neighbor)(f_all_neighbors.contains(neighbor) ? " (exists)" : "");

    // remove the IP from the neighbors.txt file if still present there
    //
    if(f_all_neighbors.contains(neighbor))
    {
        f_all_neighbors.remove(neighbor);
        save_neighbors();
    }

    // make sure we stop all gossiping toward that address
    //
    f_remote_snapcommunicators->gossip_received(neighbor);

    // also remove the remote connection otherwise it will send that
    // info in broadcast messages and the neighbor resaved in those
    // other platforms neighbors.txt files
    //
    f_remote_snapcommunicators->forget_remote_connection(neighbor);
}


/** \brief Read the list of neighbors from disk.
 *
 * The first time we deal with our list of neighbors we need to call this
 * file to make sure we get that list ready as expected, which is with
 * all the IP:port previously saved in the neighbors.txt file.
 */
void snap_communicator_server::read_neighbors()
{
    if(f_neighbors_cache_filename.isEmpty())
    {
        // get the path to the cache, create if necessary
        //
        f_neighbors_cache_filename = f_server->get_parameter("cache_path");
        if(f_neighbors_cache_filename.isEmpty())
        {
            f_neighbors_cache_filename = "/var/cache/snapwebsites";
        }
        f_neighbors_cache_filename += "/neighbors.txt";

        QFile cache(f_neighbors_cache_filename);
        if(cache.open(QIODevice::ReadOnly))
        {
            char buf[1024];
            for(;;)
            {
                qint64 const r(cache.readLine(buf, sizeof(buf)));
                if(r < 0)
                {
                    break;
                }
                if(r > 0
                && buf[0] != '#')
                {
                    QString const line(QString::fromUtf8(buf, r).trimmed());
                    f_all_neighbors[line] = true;

                    // in case we are already running we want to also add
                    // the corresponding connection
                    //
                    f_remote_snapcommunicators->add_remote_communicator(line);
                }
            }
        }
        else
        {
            SNAP_LOG_DEBUG("neighbor file \"")(f_neighbors_cache_filename)("\" could not be read.");
        }
    }
}


/** \brief Save the current list of neighbors to disk.
 *
 * Whenever the list of neighbors changes, this function gets called
 * so the changes can get save on disk and reused on a restart.
 */
void snap_communicator_server::save_neighbors()
{
    if(f_neighbors_cache_filename.isEmpty())
    {
        throw std::logic_error("Somehow save_neighbors() was called when f_neighbors_cache_filename was not set yet.");
    }

    QFile cache(f_neighbors_cache_filename);
    if(cache.open(QIODevice::WriteOnly))
    {
        for(sorted_list_of_strings_t::const_iterator n(f_all_neighbors.begin());
                                                     n != f_all_neighbors.end();
                                                     ++n)
        {
            cache.write(n.key().toUtf8());
            cache.putChar('\n');
        }
    }
    else
    {
        SNAP_LOG_ERROR("could not open cache file \"")(f_neighbors_cache_filename)("\" for writing.");
    }
}


/** \brief The list of services we know about from other snapcommunicators.
 *
 * This function gathers the list of services that this snapcommunicator
 * heard of. This means, the list of all the services offered by other
 * snapcommunicators, heard of or not, minus our own services (because
 * these other servers will return our own services as heard of!)
 */
void snap_communicator_server::refresh_heard_of()
{
    // reset the list
    f_services_heard_of_list.clear();

    // first gather all the services we have access to
    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
    for(auto const & connection : all_connections)
    {
        service_connection::pointer_t c(std::dynamic_pointer_cast<service_connection>(connection));
        if(!c)
        {
            // not a service_connection, ignore (i.e. servers)
            continue;
        }

        // get list of services and heard of services
        c->get_services(f_services_heard_of_list);
        c->get_services_heard_of(f_services_heard_of_list);
    }

    // now remove services we are in control of
    for(sorted_list_of_strings_t::const_iterator it(f_local_services_list.begin());
                                                 it != f_local_services_list.end();
                                                 ++it)
    {
        f_services_heard_of_list.remove(it.key());
    }

    // generate a string we can send in a CONNECT or an ACCEPT
    f_services_heard_of.clear();
    for(sorted_list_of_strings_t::const_iterator it(f_services_heard_of_list.begin());
                                                 it != f_services_heard_of_list.end();
                                                 ++it)
    {
        f_services_heard_of += it.key() + ",";
    }
    if(!f_services_heard_of.isEmpty())
    {
        // remove the ending ","
        f_services_heard_of.remove(f_services_heard_of.length() - 1, 1);
    }

    // done
}


/** \brief This snapcommunicator received the SHUTDOWN or a STOP command.
 *
 * This function processes the SHUTDOWN or STOP commands. It is a bit of
 * work since we have to send a message to all connections and the message
 * vary depending on the type of connection.
 *
 * \param[in] quitting  Do a full shutdown (true) or just a stop (false).
 */
void snap_communicator_server::shutdown(bool quitting)
{
    // from now on, we are shutting down; use this flag to make sure we
    // do not accept any more REGISTER, CONNECT and other similar
    // messages
    //
    f_shutdown = true;

    SNAP_LOG_DEBUG("shutting down snapcommunicator (")(quitting ? "QUIT" : "STOP")(")");

    // all gossiping can stop at once, since we cannot recognize those
    // connections in the list returned by f_communicator, we better
    // do that cleanly ahead of time
    //
    if(f_remote_snapcommunicators)
    {
        f_remote_snapcommunicators->stop_gossiping();
    }

    // DO NOT USE THE REFERENCE -- we need a copy of the vector
    // because the loop below uses remove_connection() on the
    // original vector!
    //
    snap::snap_communicator::snap_connection::vector_t const all_connections(f_communicator->get_connections());
    for(auto const & connection : all_connections)
    {
        // a remote communicator for which we initiated a new connection?
        //
        remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(connection));
        if(remote_communicator)
        {

// TODO: if the remote communicator IP address is the same as the
//       STOP, DISCONNECT, or SHUTDOWN message we just received,
//       then we have to just disconnect (HUP) instead of sending
//       a "reply"

            // remote communicators are just timers and can be removed
            // as is, no message are sent there (no interface to do so anyway)
            //
            snap::snap_communicator_message reply;

            // a remote snapcommunicator server needs to also
            // shutdown so duplicate that message there
            if(quitting)
            {
                // SHUTDOWN means we shutdown the entire cluster!!!
                //
                reply.set_command("SHUTDOWN");
            }
            else
            {
                // STOP means we do not shutdown the entire cluster
                // so here we use DISCONNECT instead
                //
                reply.set_command("DISCONNECT");
            }

            // we know this a remote snapcommunicator, no need to verify, and
            // we may not yet have received the ACCEPT message
            //verify_command(remote_communicator, reply);
            remote_communicator->send_message(reply);

            // we are quitting so we want the permanent connection
            // to exit ASAP, by marking as done, it will stop as
            // soon as the message is written to the socket
            //
            remote_communicator->mark_done(true);
        }
        else
        {
            // a standard service connection or a remote snapcommunicator server?
            //
            service_connection::pointer_t c(std::dynamic_pointer_cast<service_connection>(connection));
            if(c)
            {
                base_connection::connection_type_t const type(c->get_connection_type());
                if(type == service_connection::connection_type_t::CONNECTION_TYPE_DOWN)
                {
                    // not initialized, just get rid of that one
                    f_communicator->remove_connection(c);
                }
                else
                {
                    snap::snap_communicator_message reply;
                    if(type == service_connection::connection_type_t::CONNECTION_TYPE_REMOTE)
                    {

// TODO: if the remote communicator IP address is the same as the
//       STOP, DISCONNECT, or SHUTDOWN message we just received,
//       then we have to just disconnect (HUP) instead of sending
//       a reply

                        // a remote snapcommunicator server needs to also
                        // shutdown so duplicate that message there
                        if(quitting)
                        {
                            // SHUTDOWN means we shutdown the entire cluster!!!
                            reply.set_command("SHUTDOWN");
                        }
                        else
                        {
                            // DISCONNECT means only we are going down
                            reply.set_command("DISCONNECT");
                        }

                        verify_command(c, reply);
                        c->send_message(reply);

                        // we cannot yet remove the connection from the
                        // communicator or the message would never be sent...
                        //
                        // the remote connections are expected to disconnect
                        // us when they receive a DISCONNECT, but really we
                        // disconnect ourselves as soon as we sent the
                        // message, no need to wait any longer
                        //
                        connection->mark_done();
                    }
                    else
                    {
                        // a standard client (i.e. pagelist, images, etc.)
                        // may want to know when it gets disconnected
                        // from the snapcommunicator...
                        //
                        if(c->understand_command("DISCONNECTING"))
                        {
                            // close connection as soon as the message was
                            // sent (i.e. we are "sending the last message")
                            //
                            connection->mark_done();

                            reply.set_command("DISCONNECTING");
                            c->send_message(reply);
                        }
                        else if(c->has_output())
                        {
                            // we just sent some data to that connection
                            // so we do not want to kill it immediately
                            //
                            // instead we mark it done so once the write
                            // buffer gets empty, the connection gets
                            // removed (See process_empty_buffer())
                            //
                            connection->mark_done();
                        }
                        else
                        {
                            // that local connection does not understand
                            // DISCONNECTING and has nothing more in its
                            // buffer, so just remove it immediately
                            //
                            // we will not accept new local connections since
                            // we also remove the f_local_listener connection
                            //
                            f_communicator->remove_connection(connection);
                        }
                    }
                }
            }
            // else -- ignore the main TCP and UDP servers which we
            //         handle below
        }
    }

    // remove the two main servers; we will not respond to any more
    // requests anyway
    //
    f_communicator->remove_connection(f_interrupt);         // TCP/IP
    f_communicator->remove_connection(f_local_listener);    // TCP/IP
    f_communicator->remove_connection(f_listener);          // TCP/IP
    f_communicator->remove_connection(f_ping);              // UDP/IP
    f_communicator->remove_connection(f_loadavg_timer);     // load balancer timer

//#ifdef _DEBUG
    {
        snap::snap_communicator::snap_connection::vector_t const all_connections_remaining(f_communicator->get_connections());
        for(auto const & connection : all_connections_remaining)
        {
            SNAP_LOG_DEBUG("Connection still left after the shutdown() call: \"")(connection->get_name())("\"");
        }
    }
//#endif
}


void snap_communicator_server::process_connected(snap::snap_communicator::snap_connection::pointer_t connection)
{
    snap::snap_communicator_message connect;
    connect.set_command("CONNECT");
    connect.add_parameter("version", snap::snap_communicator::VERSION);
    connect.add_parameter("my_address", QString::fromUtf8(f_my_address.get_ipv4or6_string(true).c_str()));
    connect.add_parameter("server_name", f_server_name);
    if(!f_explicit_neighbors.isEmpty())
    {
        connect.add_parameter("neighbors", f_explicit_neighbors);
    }
    if(!f_local_services.isEmpty())
    {
        connect.add_parameter("services", f_local_services);
    }
    if(!f_services_heard_of.isEmpty())
    {
        connect.add_parameter("heard_of", f_services_heard_of);
    }
    service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(connection));
    if(service_conn)
    {
        service_conn->send_message(connect);
    }
    else
    {
        remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snap_communicator>(connection));
        if(remote_communicator)
        {
            remote_communicator->send_message(connect);
        }
    }

    // status changed for this connection
    //
    send_status(connection);
}




/** \brief Setup a remote_snap_communicator object.
 *
 * This initialization function sets up the attached snap_timer
 * to 1 second delay before we try to connect to this remote
 * snapcommunicator. The timer is reused later when the connection
 * is lost, a snapcommunicator returns a REFUSE message to our
 * CONNECT message, and other similar errors.
 *
 * \param[in] cs  The snap communicator server shared pointer.
 * \param[in] addr  The address to connect to.
 * \param[in] port  The port used to connect to.
 */
remote_snap_communicator::remote_snap_communicator(snap_communicator_server::pointer_t cs, QString const & addr, int port)
    : snap_tcp_client_permanent_message_connection(
                              addr.toUtf8().data()
                            , port
                            , cs->connection_mode()
                            , REMOTE_CONNECTION_DEFAULT_TIMEOUT)
    , base_connection(cs)
    , f_address(addr.toUtf8().data(), "", 4040, "tcp")
{
}


remote_snap_communicator::~remote_snap_communicator()
{
    SNAP_LOG_DEBUG("deleting remote_snap_communicator connection: ")(f_address.get_ipv4or6_string(true, true));
}


void remote_snap_communicator::process_message(snap::snap_communicator_message const & message)
{
    f_communicator_server->process_message(shared_from_this(), message, false);
}


void remote_snap_communicator::process_connection_failed(std::string const & error_message)
{
    snap_tcp_client_permanent_message_connection::process_connection_failed(error_message);

    SNAP_LOG_ERROR("the connection to a remote communicator failed: \"")(error_message)("\".");
}


void remote_snap_communicator::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    f_communicator_server->process_connected(shared_from_this());

    // reset the wait to the default 5 minutes
    //
    // (in case we had a shutdown event from that remote communicator
    // and changed the timer to 15 min.)
    //
    // later we probably want to change the mechanism if we want to
    // slowdown over time
    //
    set_timeout_delay(REMOTE_CONNECTION_DEFAULT_TIMEOUT);
}


snap_addr::addr const & remote_snap_communicator::get_address() const
{
    return f_address;
}



class snapcommunicator
    : public snap::server
{
public:

    virtual void show_version() override
    {
        std::cout << SNAPCOMMUNICATOR_VERSION_STRING << std::endl;
    }
};




int main(int argc, char * argv[])
{
    int exitval( 1 );
    try
    {
        // create a server object
        snap::server::pointer_t s( std::make_shared<snapcommunicator>() );
        //s->setup_as_backend();

        // parse the command line arguments (this also brings in the .conf params)
        //
        s->set_config_filename( "snapcommunicator" );
        s->config( argc, argv );

        // if possible, detach the server
        s->detach();
        // Only the child (backend) process returns here

        // Now create the qt application instance
        //
        s->prepare_qtapp( argc, argv );

        // show when we started in the log
        SNAP_LOG_INFO("--------------------------------- snapcommunicator started on ")(s->get_parameter("server_name"));

        // Run the snap communicator server; note that the snapcommunicator
        // server is snap_communicator and not snap::server
        //
        {
            snap_communicator_server::pointer_t communicator( new snap_communicator_server( s ) );
            communicator->init();
            communicator->run();
        }

        exitval = 0;
    }
    catch( snap::snap_exception const & except )
    {
        SNAP_LOG_FATAL("snapcommunicator: snap exception caught: ")(except.what());
    }
    catch( std::exception const & std_except )
    {
        SNAP_LOG_FATAL("snapcommunicator: standard exception caught: ")(std_except.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snapcommunicator: unknown exception caught!");
    }

    // exit via the server so the server can clean itself up properly
    snap::server::exit( exitval );

    snap::NOTREACHED();
    return 0;
}

// vim: ts=4 sw=4 et
