/*
 * Text:
 *      src/snaplock.h
 *
 * Description:
 *      A daemon to synchronize processes between any number of computers
 *      by blocking all of them but one.
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

// our lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_communicator.h>


class snaplock;



class snaplock_interrupt
        : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<snaplock_interrupt>     pointer_t;

                                snaplock_interrupt(snaplock * sl);
    virtual                     ~snaplock_interrupt() override {}

    // snap::snap_communicator::snap_signal implementation
    virtual void                process_signal() override;

private:
    snaplock *                  f_snaplock = nullptr;
};


class snaplock_messenger
        : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<snaplock_messenger>     pointer_t;

                                snaplock_messenger(snaplock * sl, std::string const & addr, int port);
    virtual                     ~snaplock_messenger() override {}

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void                process_message(snap::snap_communicator_message const & message) override;
    virtual void                process_connection_failed(std::string const & error_message) override;
    virtual void                process_connected() override;

protected:
    // this is owned by a snaplock function so no need for a smart pointer
    // (and it would create a loop)
    snaplock *                  f_snaplock = nullptr;
};


class snaplock_tool
        : public snaplock_messenger
{
public:
    typedef std::shared_ptr<snaplock_tool>    pointer_t;

                                snaplock_tool(snaplock * sl, std::string const & addr, int port);

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void                process_message(snap::snap_communicator_message const & message);
    virtual void                process_connection_failed(std::string const & error_message);
    virtual void                process_connected();
};


struct ticket_info_t
{
    int64_t                     f_number = 0;
    int64_t                     f_timeout = 0;
};



class snaplock_ticket
{
public:
    typedef std::shared_ptr<snaplock_ticket>    pointer_t;
    typedef std::vector<pointer_t>              vector_t;
    typedef std::map<QString, pointer_t>        key_map_t;      // sorted by key
    typedef std::map<QString, key_map_t>        object_map_t;   // sorted by object_name

    typedef uint32_t                            ticket_id_t;

    static ticket_id_t const                    NO_TICKET = 0;

                        snaplock_ticket(
                                  snaplock * sl
                                , snaplock_messenger::pointer_t messenger
                                , QString const & object_name
                                , QString const & entering_key
                                , time_t obtention_timeout
                                , int32_t lock_duration
                                , QString const & server_name
                                , QString const & service_name
                                );

    void                entering();
    void                entered();
    ticket_id_t         get_ticket_number() const;
    time_t              get_obtention_timeout() const;
    time_t              get_lock_timeout() const;
    void                max_ticket(int64_t new_max_ticket);
    void                add_ticket();
    void                ticket_added(snaplock_ticket::key_map_t const & entering);
    void                exiting();
    void                remove_entering(QString const & key);
    void                activate_lock();

    bool                timed_out() const;
    void                lock_failed();
    QString const &     get_object_name() const;
    QString const &     get_entering_key() const;

    // this is called when we receive the UNLOCK event
    void                drop_ticket();

private:
    // this is owned by a snaplock function so no need for a smart pointer
    // (and it would create a loop)
    snaplock *                      f_snaplock = nullptr;

    // initialization
    snaplock_messenger::pointer_t   f_messenger;
    QString                         f_object_name;
    time_t                          f_obtention_timeout = 0;
    int32_t                         f_lock_duration = 0;
    QString                         f_server_name;
    QString                         f_service_name;

    // initialized, entering
    QString                         f_entering_key;
    int                             f_entered_count = 0;
    bool                            f_get_max_ticket = false;

    // entered, adding ticket
    int                             f_max_ticket_count = 0;
    ticket_id_t                     f_our_ticket = NO_TICKET;
    bool                            f_added_ticket = false;
    QString                         f_ticket_key;

    // ticket added, exiting
    int                             f_ticket_added_count = 0;
    bool                            f_added_ticket_quorum = false;
    snaplock_ticket::key_map_t      f_still_entering;

    // exited, ticket ready
    bool                            f_ticket_ready = false;

    // locked
    bool                            f_locked = false;
    time_t                          f_lock_timeout = 0;

    // the lock did not take (in most cases, this is because of a timeout)
    bool                            f_lock_failed = false;
};



class snaplock
{
public:
    typedef std::shared_ptr<snaplock>      pointer_t;

    static int64_t const        DEFAULT_TIMEOUT = 5; // in seconds
    static int64_t const        MIN_TIMEOUT     = 3; // in seconds

                                snaplock( int argc, char * argv[] );
    virtual                     ~snaplock();

    void                        run();
    void                        process_message(snap::snap_communicator_message const & message);
    void                        tool_message(snap::snap_communicator_message const & message);
    void                        process_connection(int const s);
    void                        stop(bool quitting);

    int                         get_computer_count() const;
    int                         quorum() const;
    QString const &             get_server_name() const;

    static void                 sighandler( int sig );

private:
                                snaplock( snaplock const & ) = delete;
    snaplock &                  operator = ( snaplock const & ) = delete;

    void                        usage(advgetopt::getopt::status_t status);
    void                        setup_firewall();
    void                        next_wakeup();
    void                        get_parameters(snap::snap_communicator_message const & message, QString * object_name, pid_t * client_pid, time_t * timeout, QString * key, QString * source);
    void                        lock(snap::snap_communicator_message const & message);
    void                        unlock(snap::snap_communicator_message const & message);
    void                        dropticket(snap::snap_communicator_message const & message);
    void                        entering(snap::snap_communicator_message const & message);
    void                        exiting(snap::snap_communicator_message const & message);
    void                        lockentering(snap::snap_communicator_message const & message);
    void                        lockentered(snap::snap_communicator_message const & message);
    void                        lockexiting(snap::snap_communicator_message const & message);
    void                        lockready(snap::snap_communicator_message const & message);
    void                        interpret_status(snap::snap_communicator_message const & message);
    void                        add_ticket(snap::snap_communicator_message const & message);
    void                        ticket_added(snap::snap_communicator_message const & message);
    void                        get_max_ticket(snap::snap_communicator_message const & message);
    void                        max_ticket(snap::snap_communicator_message const & message);
    void                        lockgone(snap::snap_communicator_message const & message);
    void                        drop_ticket(snap::snap_communicator_message const & message);
    void                        activate_first_lock(QString const & object_name);
    void                        ticket_list(snap::snap_communicator_message const & message);
    void                        cleanup();
    void                        send_lockready();

    static pointer_t                            f_instance;
    advgetopt::getopt                           f_opt;
    snap::snap_config                           f_config;
    QString                                     f_log_conf = "/etc/snapwebsites/logger/snaplock.properties";
    QString                                     f_server_name;
    QString                                     f_service_name = "snaplock";
    QString                                     f_communicator_addr = "localhost";
    int                                         f_communicator_port = 4040;
    snap::snap_communicator::pointer_t          f_communicator;
    QString                                     f_host_list = "localhost";
    int                                         f_port = 9042;
    int                                         f_max_pending_connections = 20;
    snaplock_messenger::pointer_t               f_messenger;
    snaplock_interrupt::pointer_t               f_interrupt;
    bool                                        f_stop_received = false;
    bool                                        f_debug = false;
    bool                                        f_debug_lock_messages = false;
    std::map<QString, bool>                     f_computers;
    snaplock_ticket::object_map_t               f_entering_tickets;
    snaplock_ticket::object_map_t               f_tickets;
};


// vim: ts=4 sw=4 et
