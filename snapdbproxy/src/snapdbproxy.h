/*
 * Text:
 *      snapwebsites/snapdbproxy/src/snapdbproxy.h
 *
 * Description:
 *      Proxy database access for two main reasons:
 *
 *      1. keep connections between this computer and the database
 *         computer open (i.e. opening remote TCP connections taken
 *         "much" longer than opening local connections.)
 *
 *      2. remove threads being forced on us by the C/C++ driver from
 *         cassandra (this causes problems with the snapserver that
 *         uses fork() to create the snap_child processes.)
 *
 * License:
 *      Copyright (c) 2016-2017 Made to Order Software Corp.
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
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snap_thread.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>

// QtCassandra lib
//
#include <QtCassandra/QCassandraOrder.h>
#include <QtCassandra/QCassandraProxy.h>

// CassWrapper lib
//
#include <casswrapper/query.h>
#include <casswrapper/session.h>

// C++
//
#include <atomic>


class snapdbproxy;


class snapdbproxy_interrupt
        : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<snapdbproxy_interrupt>     pointer_t;

                                snapdbproxy_interrupt(snapdbproxy * s);
    virtual                     ~snapdbproxy_interrupt() override {}

    // snap::snap_communicator::snap_signal implementation
    virtual void                process_signal() override;

private:
    snapdbproxy *               f_snapdbproxy = nullptr;
};


class snapdbproxy_nocassandra
        : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<snapdbproxy_nocassandra>     pointer_t;

                                snapdbproxy_nocassandra(snapdbproxy * s);
    virtual                     ~snapdbproxy_nocassandra() override {}

    // snap::snap_communicator::snap_signal implementation
    virtual void                process_signal() override;

private:
    snapdbproxy *               f_snapdbproxy = nullptr;
};


/** \brief Provide a tick in can we cannot immediately connect to Cassandra.
 *
 * The snapdbproxy tries to connect to Cassandra on startup. It is part
 * of its initialization procedure.
 *
 * If that fails, it needs to try again later. This timer is used for
 * that purpose.
 */
class snapdbproxy_timer
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<snapdbproxy_timer>    pointer_t;

    /** \brief The timer initialization.
     *
     * The timer ticks once per second to retrieve the current load of the
     * system and forward it to whichever computer that requested the
     * information.
     *
     * \param[in] cs  The snap communicator server we are listening for.
     *
     * \sa process_timeout()
     */
    snapdbproxy_timer(snapdbproxy * proxy)
        : snap_timer(0)  // run immediately
        , f_snapdbproxy(proxy)
    {
    }

    // snap::snap_communicator::snap_timer implementation
    virtual void process_timeout() override;

private:
    // this is owned by a server function so no need for a smart pointer
    snapdbproxy *               f_snapdbproxy = nullptr;
};



class snapdbproxy_messenger
        : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<snapdbproxy_messenger>    pointer_t;

                                snapdbproxy_messenger(snapdbproxy * proxy, std::string const & addr, int port);

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void                process_message(snap::snap_communicator_message const & message);
    virtual void                process_connection_failed(std::string const & error_message);
    virtual void                process_connected();

private:
    // this is owned by a snapdbproxy function so no need for a smart pointer
    // (and it would create a loop)
    snapdbproxy *               f_snapdbproxy = nullptr;
};


class snapdbproxy_listener
        : public snap::snap_communicator::snap_tcp_server_connection
{
public:
    typedef std::shared_ptr<snapdbproxy_listener>    pointer_t;

                                snapdbproxy_listener(snapdbproxy * proxy, std::string const & addr, int port, int max_connections, bool reuse_addr);

    // snap_communicator::snap_tcp_server_connection implementation
    virtual void                process_accept();

private:
    // this is owned by a snapdbproxy function so no need for a smart pointer
    // (and it would create a loop)
    snapdbproxy *            	f_snapdbproxy = nullptr;
};



class snapdbproxy_connection
        : public snap::snap_thread::snap_runner
        , public QtCassandra::QCassandraProxyIO
{
public:
                                snapdbproxy_connection
                                    ( snapdbproxy * proxy
                                    , casswrapper::Session::pointer_t session
                                    , tcp_client_server::bio_client::pointer_t & client
                                    , QString const & cassandra_host_list
                                    , int cassandra_port
                                    , bool use_ssl
                                    );
    virtual                     ~snapdbproxy_connection() override;

    // implement snap_runner
    virtual void                run() override;

    // implement QCassandraProxyIO
    virtual ssize_t             read(void * buf, size_t count) override;
    virtual ssize_t             write(void const * buf, size_t count) override;

    void                        kill();

private:
    struct cursor_t
    {
        casswrapper::Query::pointer_t f_query;
        int                           f_column_count = 0;
    };

    struct batch_t
    {
        casswrapper::Query::pointer_t f_query;
    };

    void                        send_order(casswrapper::Query::pointer_t q, QtCassandra::QCassandraOrder const & order);
    void                        declare_cursor(QtCassandra::QCassandraOrder const & order);
    void                        declare_batch(QtCassandra::QCassandraOrder const & order);
    void                        describe_cluster(QtCassandra::QCassandraOrder const & order);
    void                        clear_cluster_description();
    void                        fetch_cursor(QtCassandra::QCassandraOrder const & order);
    void                        close_cursor(QtCassandra::QCassandraOrder const & order);
    void                        commit_batch(QtCassandra::QCassandraOrder const & order);
    void                        read_data(QtCassandra::QCassandraOrder const & order);
    void                        rollback_batch(QtCassandra::QCassandraOrder const & order);
    void                        execute_command(QtCassandra::QCassandraOrder const & order);
    void                        close();

    void                        clear_batch( int32_t const batch_index );

    // this is owned by a snapdbproxy function so no need for a smart pointer
    // (and it would create a loop)
    snapdbproxy *                               f_snapdbproxy = nullptr;

    QtCassandra::QCassandraProxy                f_proxy;
    casswrapper::Session::pointer_t             f_session;
    std::vector<cursor_t>                       f_cursors;
    std::vector<batch_t>                        f_batches;
    tcp_client_server::bio_client::pointer_t    f_client;
    std::atomic<int>                            f_socket /* = -1*/;
    QString                                     f_cassandra_host_list = "localhost";
    int                                         f_cassandra_port = 9042;
    bool                                        f_use_ssl = false;
};


class snapdbproxy_thread
{
public:
    typedef std::shared_ptr<snapdbproxy_thread> pointer_t;

                            snapdbproxy_thread
                                ( snapdbproxy* proxy
                                  , casswrapper::Session::pointer_t session
                                  , tcp_client_server::bio_client::pointer_t & client
                                  , QString const & cassandra_host_list
                                  , int cassandra_port
                                  , bool use_ssl
                                  );
                            ~snapdbproxy_thread();

    bool                    is_running() const;

private:
    snapdbproxy_connection  f_connection;
    snap::snap_thread       f_thread;
};




class snapdbproxy
{
public:
    typedef std::shared_ptr<snapdbproxy>      pointer_t;

                                snapdbproxy( int argc, char * argv[] );
                                ~snapdbproxy();

    std::string                 server_name() const;

    void                        run();
    void                        process_message(snap::snap_communicator_message const & message);
    void                        process_connection(tcp_client_server::bio_client::pointer_t & client);
    void                        process_timeout();
    void                        stop(bool quitting);

    void                        no_cassandra();
    void                        cassandra_ready();

    static void                 sighandler( int sig );

private:
                                snapdbproxy( snapdbproxy const & ) = delete;
    snapdbproxy &               operator = ( snapdbproxy const & ) = delete;

    bool                        use_ssl() const;
    void                        usage(advgetopt::getopt::status_t status);
    void                        setup_dbproxy();
    void                        next_wakeup();

    static pointer_t                            g_instance;

    advgetopt::getopt                           f_opt;
    snap::snap_config                           f_config;
    QString                                     f_log_conf = "/etc/snapwebsites/logger/snapdbproxy.properties";
    std::string                                 f_server_name;
    QString                                     f_communicator_addr = "127.0.0.1";
    int                                         f_communicator_port = 4040;
    QString                                     f_snapdbproxy_addr = "127.0.0.1";
    int                                         f_snapdbproxy_port = 4042;
    snap::snap_communicator::pointer_t          f_communicator;
    QString                                     f_cassandra_host_list = "localhost";
    int                                         f_cassandra_port = 9042;
    snapdbproxy_interrupt::pointer_t            f_interrupt;
    snapdbproxy_nocassandra::pointer_t          f_nocassandra;
    snapdbproxy_messenger::pointer_t            f_messenger;
    snapdbproxy_listener::pointer_t             f_listener;
    snapdbproxy_timer::pointer_t                f_timer;
    int                                         f_max_pending_connections = -1;
    bool                                        f_ready = false;
    bool                                        f_force_restart = false;
    bool                                        f_stop_received = false;
    bool                                        f_debug = false;
    bool                                        f_no_cassandra_sent = false;
    float                                       f_cassandra_connect_timer_index = 1.25f;
    casswrapper::Session::pointer_t   f_session;

    std::vector<snapdbproxy_thread::pointer_t>  f_connections;
};



// vim: ts=4 sw=4 et
