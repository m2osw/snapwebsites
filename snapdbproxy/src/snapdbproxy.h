/*
 * Text:
 *      snapdbproxy/src/snapdbproxy.h
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
 *      Copyright (c) 2016-2021  Made to Order Software Corp.  All Rights Reserved
 *
 *      https://snapwebsites.org/
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
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snap_lock.h>
#include <snapwebsites/snap_tables.h>
#include <snapwebsites/snap_thread.h>
#include <snapwebsites/snapwebsites.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>

// libdbproxy lib
//
#include <libdbproxy/order.h>
#include <libdbproxy/proxy.h>

// CassWrapper lib
//
#include <casswrapper/batch.h>
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
                                snapdbproxy_interrupt(snapdbproxy_interrupt const & rhs) = delete;
    virtual                     ~snapdbproxy_interrupt() override {}

    snapdbproxy_interrupt &     operator = (snapdbproxy_interrupt const & rhs) = delete;

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
                                snapdbproxy_nocassandra(snapdbproxy_nocassandra const & rhs) = delete;
    virtual                     ~snapdbproxy_nocassandra() override {}

    snapdbproxy_nocassandra &   operator = (snapdbproxy_nocassandra const & rhs) = delete;

    // snap::snap_communicator::snap_signal implementation
    virtual void                process_signal() override;

private:
    snapdbproxy *               f_snapdbproxy = nullptr;
};


class snapdbproxy_statuschanged
    : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<snapdbproxy_statuschanged>     pointer_t;

                                snapdbproxy_statuschanged(snapdbproxy * s);
                                snapdbproxy_statuschanged(snapdbproxy_statuschanged const & rhs) = delete;
    virtual                     ~snapdbproxy_statuschanged() override {}

    snapdbproxy_statuschanged & operator = (snapdbproxy_statuschanged const & rhs) = delete;

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

    snapdbproxy_timer(snapdbproxy_timer const & rhs) = delete;
    snapdbproxy_timer & operator = (snapdbproxy_timer const & rhs) = delete;

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
                                snapdbproxy_messenger(snapdbproxy_messenger const & rhs) = delete;
    snapdbproxy_messenger &     operator = (snapdbproxy_messenger const & rhs) = delete;

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
                                snapdbproxy_listener(snapdbproxy_listener const & rhs) = delete;
    snapdbproxy_listener &      operator = (snapdbproxy_listener const & rhs) = delete;

    // snap_communicator::snap_tcp_server_connection implementation
    virtual void                process_accept();

private:
    // this is owned by a snapdbproxy function so no need for a smart pointer
    // (and it would create a loop)
    snapdbproxy *               f_snapdbproxy = nullptr;
};



class snapdbproxy_connection
    : public snap::snap_thread::snap_runner
    , public libdbproxy::proxy_io
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
                                snapdbproxy_connection(snapdbproxy_connection const & rhs) = delete;
    virtual                     ~snapdbproxy_connection() override;

    snapdbproxy_connection &    operator = (snapdbproxy_connection const & rhs) = delete;

    // implement snap_runner
    virtual void                run() override;

    // implement proxy_io
    virtual ssize_t             read(void * buf, size_t count) override;
    virtual ssize_t             write(void const * buf, size_t count) override;

    void                        kill();  // this just shuts down the socket (READ only)

private:
    struct cursor_t
    {
        casswrapper::Query::pointer_t f_query = casswrapper::Query::pointer_t();
        int                           f_column_count = 0;
    };

    struct batch_t
    {
        casswrapper::Query::pointer_t f_query = casswrapper::Query::pointer_t();
        casswrapper::Batch::pointer_t f_batch = casswrapper::Batch::pointer_t();
    };

    void                        send_order(casswrapper::Query::pointer_t q, libdbproxy::order const & order);
    void                        declare_cursor(libdbproxy::order const & order);
    void                        declare_batch(libdbproxy::order const & order);
    void                        describe_cluster(libdbproxy::order const & order);
    void                        clear_cluster_description();
    void                        fetch_cursor(libdbproxy::order const & order);
    void                        close_cursor(libdbproxy::order const & order);
    void                        commit_batch(libdbproxy::order const & order);
    void                        read_data(libdbproxy::order const & order);
    void                        rollback_batch(libdbproxy::order const & order);
    void                        execute_command(libdbproxy::order const & order);
    void                        close();

    void                        clear_batch( int32_t const batch_index );

    // this is owned by a snapdbproxy function so no need for a smart pointer
    // (and it would create a loop or we'd need a weak pointer and locks
    // everywhere we use it...)
    //
    snapdbproxy *                               f_snapdbproxy = nullptr;

    libdbproxy::proxy                           f_proxy = libdbproxy::proxy();
    casswrapper::Session::pointer_t             f_session = casswrapper::Session::pointer_t();
    std::vector<cursor_t>                       f_cursors = std::vector<cursor_t>();
    std::vector<batch_t>                        f_batches = std::vector<batch_t>();
    tcp_client_server::bio_client::pointer_t    f_client = tcp_client_server::bio_client::pointer_t();
    std::atomic<int>                            f_socket /* = -1*/;
    QString                                     f_cassandra_host_list = QString("localhost");
    int                                         f_cassandra_port = 9042;
    bool                                        f_use_ssl = false;
};


class snapdbproxy_thread
{
public:
    typedef std::shared_ptr<snapdbproxy_thread> pointer_t;

                            snapdbproxy_thread
                                ( snapdbproxy * proxy
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





//class snapdbproxy_table
//{
//public:
//    typedef std::shared_ptr<snapdbproxy_table>              pointer_t;
//    typedef std::map<QString, snapdbproxy_table::pointer_t> map_t;
//
//                            snapdbproxy_table(QString const & name);
//
//    QString const &         name() const;
//
//    void                    found();
//    bool                    exists() const;
//
//private:
//    QString                 f_name = QString();
//    bool                    f_exists = false;
//};


class snapdbproxy_initializer
    : public snap::snap_thread::snap_runner
{
public:
    static int const INITIALIZER_SESSION_TIMEOUT = 5 * 60 * 1000; // timeout = 5 min.

                                snapdbproxy_initializer
                                    ( snapdbproxy * proxy
                                    , QString const & cassandra_host_list
                                    , int cassandra_port
                                    , bool use_ssl
                                    );
                                snapdbproxy_initializer(snapdbproxy_initializer const & rhs) = delete;
    virtual                     ~snapdbproxy_initializer() override;

    snapdbproxy_initializer &    operator = (snapdbproxy_initializer const & rhs) = delete;

    // implement snap_runner
    virtual bool                continue_running() const;
    virtual void                run() override;

private:
    bool                        load_tables();
    bool                        connect();
    bool                        load_cassandra_tables();
    bool                        load_cassandra_indexes();
    bool                        has_missing_tables();
    bool                        has_missing_indexes();
    bool                        obtain_lock();
    bool                        create_context();
    bool                        create_tables();
    void                        create_table(snap::snap_tables::table_schema_t const & schema);
    void                        drop_table(snap::snap_tables::table_schema_t const & schema);
    bool                        create_indexes();
    void                        create_index(
                                      snap::snap_tables::table_schema_t const & schema
                                    , snap::snap_tables::secondary_index_t const & index);

    // this is owned by a snapdbproxy function so no need for a smart pointer
    // (and it would create a loop or we'd need a weak pointer and locks
    // everywhere we use it...)
    //
    snapdbproxy *                               f_snapdbproxy = nullptr;

    casswrapper::Session::pointer_t             f_session = casswrapper::Session::pointer_t();
    QString const                               f_cassandra_host_list = QString("localhost");
    int const                                   f_cassandra_port = 9042;
    bool const                                  f_use_ssl = false;
    bool                                        f_locked = false;
    QString                                     f_context_name = QString("snap_websites");
    snap::snap_tables                           f_tables = snap::snap_tables();
    snap::snap_string_list                      f_existing_tables = snap::snap_string_list();
    snap::snap_string_list                      f_existing_indexes = snap::snap_string_list();
};


class snapdbproxy_initializer_thread
{
public:
    typedef std::shared_ptr<snapdbproxy_initializer_thread> pointer_t;

                            snapdbproxy_initializer_thread
                                ( snapdbproxy * proxy
                                , QString const & cassandra_host_list
                                , int cassandra_port
                                , bool use_ssl
                                );
                            snapdbproxy_initializer_thread(snapdbproxy_initializer_thread const & rhs) = delete;
                            ~snapdbproxy_initializer_thread();

    snapdbproxy_initializer_thread &
                            operator = (snapdbproxy_initializer_thread const & rhs) = delete;

    bool                    is_running() const;

private:
    snapdbproxy *           f_snapdbproxy = nullptr;

    snapdbproxy_initializer f_initializer;
    snap::snap_thread       f_thread;
};







class snapdbproxy
{
public:
    typedef std::shared_ptr<snapdbproxy>      pointer_t;

    enum class status_t
    {
        STATUS_START,       // need to connect
        STATUS_LOCK,        // obtaining lock before creating context & tables
        STATUS_PAUSE,       // pause for a little while, unlock while paused
        STATUS_NO_LOCK,     // could not obtain lock
        STATUS_STOPPED,     // could not finish before receiving a stop signal
        STATUS_CONTEXT,     // creating context ("snap_websites")
        STATUS_TABLES,      // creating tables
        STATUS_READY,       // ready to accept connections
        STATUS_INVALID      // an unknown exception occurred
    };

                                snapdbproxy(int argc, char * argv[]);
                                ~snapdbproxy();

    std::string                 server_name() const;

    void                        run();
    void                        process_message(snap::snap_communicator_message const & message);
    void                        process_connection(tcp_client_server::bio_client::pointer_t & client);
    void                        process_timeout();
    void                        stop(bool quitting);

    void                        set_status(status_t status);
    status_t                    get_status() const;
    void                        status_changed();
    void                        no_cassandra();
    void                        cassandra_ready();

    static void                 sighandler( int sig );

private:
                                snapdbproxy( snapdbproxy const & ) = delete;
    snapdbproxy &               operator = ( snapdbproxy const & ) = delete;

    bool                        use_ssl() const;
    void                        setup_dbproxy();
    void                        next_wakeup();

    static pointer_t                            g_instance;

    mutable snap::snap_thread::snap_mutex       f_mutex = snap::snap_thread::snap_mutex();
    advgetopt::getopt                           f_opt;
    snap::snap_config                           f_config = snap::snap_config();
    QString                                     f_log_conf = QString("/etc/snapwebsites/logger/snapdbproxy.properties");
    std::string                                 f_server_name = std::string();
    QString                                     f_communicator_addr = QString("127.0.0.1");
    int                                         f_communicator_port = 4040;
    status_t                                    f_status = status_t::STATUS_START;
    snapdbproxy_initializer_thread::pointer_t   f_initializer_thread = snapdbproxy_initializer_thread::pointer_t();
    snap::snap_lock::pointer_t                  f_initializer_lock = nullptr;
    QString                                     f_snapdbproxy_addr = QString("127.0.0.1");
    int                                         f_snapdbproxy_port = 4048;
    snap::snap_communicator::pointer_t          f_communicator = snap::snap_communicator::pointer_t();
    QString                                     f_cassandra_host_list = QString("localhost");
    int                                         f_cassandra_port = 9042;
    snapdbproxy_interrupt::pointer_t            f_interrupt = snapdbproxy_interrupt::pointer_t();
    snapdbproxy_nocassandra::pointer_t          f_nocassandra = snapdbproxy_nocassandra::pointer_t();
    snapdbproxy_statuschanged::pointer_t        f_statuschanged = snapdbproxy_statuschanged::pointer_t();
    snapdbproxy_messenger::pointer_t            f_messenger = snapdbproxy_messenger::pointer_t();
    snapdbproxy_listener::pointer_t             f_listener = snapdbproxy_listener::pointer_t();
    snapdbproxy_timer::pointer_t                f_timer = snapdbproxy_timer::pointer_t();
    int                                         f_max_pending_connections = -1;
    bool                                        f_ready = false;
    bool                                        f_lock_ready = false;
    bool                                        f_force_restart = false;
    bool                                        f_stop_received = false;
    bool                                        f_debug = false;
    bool                                        f_no_cassandra_sent = false;
    float                                       f_cassandra_connect_timer_index = 1.25f;
    casswrapper::Session::pointer_t             f_session = casswrapper::Session::pointer_t();
    std::vector<snapdbproxy_thread::pointer_t>  f_connections = std::vector<snapdbproxy_thread::pointer_t>();
};



// vim: ts=4 sw=4 et
