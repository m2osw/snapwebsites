/*
 * Text:
 *      snaplock/src/snaplock.h
 *
 * Description:
 *      A daemon to synchronize processes between any number of computers
 *      by blocking all of them but one.
 *
 * License:
 *      Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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

// our lib
//
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snap_communicator_dispatcher.h>
#include <snapwebsites/snap_exception.h>
#include <snapwebsites/snap_lock.h>
#include <snapwebsites/snapwebsites.h>


namespace snaplock
{


class snaplock_exception : public snap::snap_exception
{
public:
    explicit snaplock_exception(char const *        what_msg) : snap_exception("snaplock", what_msg) {}
    explicit snaplock_exception(std::string const & what_msg) : snap_exception("snaplock", what_msg) {}
    explicit snaplock_exception(QString const &     what_msg) : snap_exception("snaplock", what_msg) {}
};

class snaplock_exception_content_invalid_usage : public snaplock_exception
{
public:
    explicit snaplock_exception_content_invalid_usage(char const *        what_msg) : snaplock_exception(what_msg) {}
    explicit snaplock_exception_content_invalid_usage(std::string const & what_msg) : snaplock_exception(what_msg) {}
    explicit snaplock_exception_content_invalid_usage(QString const &     what_msg) : snaplock_exception(what_msg) {}
};


class snaplock;



class snaplock_interrupt
    : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<snaplock_interrupt>     pointer_t;

                                snaplock_interrupt(snaplock * sl);
                                snaplock_interrupt(snaplock_interrupt const & rhs) = delete;
    virtual                     ~snaplock_interrupt() override {}

    snaplock_interrupt &        operator = (snaplock_interrupt const & rhs) = delete;

    // snap::snap_communicator::snap_signal implementation
    virtual void                process_signal() override;

private:
    snaplock *                  f_snaplock = nullptr;
};



class snaplock_timer
    : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<snaplock_timer>     pointer_t;

                                snaplock_timer(snaplock * sl);
                                snaplock_timer(snaplock_timer const & rhs) = delete;
    virtual                     ~snaplock_timer() override {}

    snaplock_timer &            operator = (snaplock_timer const & rhs) = delete;

    // snap::snap_communicator::snap_connection implementation
    virtual void                process_timeout() override;

private:
    snaplock *                  f_snaplock = nullptr;
};



class snaplock_info
    : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<snaplock_info>     pointer_t;

                            snaplock_info(snaplock * sl);
                            snaplock_info(snaplock_info const & rhs) = delete;
    virtual                 ~snaplock_info() override {}

    snaplock_info const &   operator = (snaplock_info const & rhs) = delete;

    // snap::snap_communicator::snap_signal implementation
    virtual void            process_signal() override;

private:
    snaplock *              f_snaplock = nullptr;
};



class snaplock_debug_info
    : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<snaplock_debug_info>     pointer_t;

                            snaplock_debug_info(snaplock * sl);
                            snaplock_debug_info(snaplock_debug_info const & rhs) = delete;
    virtual                 ~snaplock_debug_info() override {}

    snaplock_debug_info const &   operator = (snaplock_debug_info const & rhs) = delete;

    // snap::snap_communicator::snap_signal implementation
    virtual void            process_signal() override;

private:
    snaplock *              f_snaplock = nullptr;
};



class snaplock_messenger
    : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<snaplock_messenger>     pointer_t;

                                snaplock_messenger(snaplock * sl, std::string const & addr, int port);
                                snaplock_messenger(snaplock_messenger const & rhs) = delete;
    virtual                     ~snaplock_messenger() override {}

    snaplock_messenger &        operator = (snaplock_messenger const & rhs) = delete;

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
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
                                snaplock_tool(snaplock_tool const & rhs) = delete;
    virtual                     ~snaplock_tool() override {}

    snaplock_tool &             operator = (snaplock_tool const & rhs) = delete;

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
    : public std::enable_shared_from_this<snaplock_ticket>
{
public:
    typedef std::shared_ptr<snaplock_ticket>    pointer_t;
    typedef std::vector<pointer_t>              vector_t;
    typedef std::map<QString, pointer_t>        key_map_t;      // sorted by key
    typedef std::map<QString, key_map_t>        object_map_t;   // sorted by object_name
    typedef int32_t                             serial_t;
    typedef uint32_t                            ticket_id_t;

    static serial_t const                       NO_SERIAL = -1;
    static ticket_id_t const                    NO_TICKET = 0;

                                snaplock_ticket(
                                          snaplock * sl
                                        , snaplock_messenger::pointer_t messenger
                                        , QString const & object_name
                                        , QString const & entering_key
                                        , time_t obtention_timeout
                                        , snap::snap_lock::timeout_t lock_duration
                                        , QString const & server_name
                                        , QString const & service_name
                                        );
                                snaplock_ticket(snaplock_ticket const & rhs) = delete;
    snaplock_ticket &           operator = (snaplock_ticket const & rhs) = delete;

    // message handling
    //
    bool                        send_message_to_leaders(snap::snap_communicator_message & message);
    void                        entering();
    void                        entered();
    void                        max_ticket(int64_t new_max_ticket);
    void                        add_ticket();
    void                        ticket_added(snaplock_ticket::key_map_t const & entering);
    void                        remove_entering(QString const & key);
    void                        activate_lock();
    void                        lock_activated();
    void                        drop_ticket(); // this is called when we receive the UNLOCK event
    void                        lock_failed();
    void                        lock_tickets();

    // object handling
    //
    void                        set_owner(QString const & owner);
    QString const &             get_owner() const;
    pid_t                       get_client_pid() const;
    void                        set_serial(serial_t owner);
    serial_t                    get_serial() const;
    void                        set_unlock_duration(snap::snap_lock::timeout_t duration);
    snap::snap_lock::timeout_t  get_unlock_duration() const;
    void                        set_ready();
    void                        set_ticket_number(ticket_id_t number);
    ticket_id_t                 get_ticket_number() const;
    bool                        is_locked() const;
    time_t                      get_obtention_timeout() const;
    void                        set_alive_timeout(time_t timeout);
    snap::snap_lock::timeout_t  get_lock_duration() const;
    time_t                      get_lock_timeout() const;
    time_t                      get_current_timeout() const;
    bool                        timed_out() const;
    QString const &             get_object_name() const;
    QString const &             get_server_name() const;
    QString const &             get_service_name() const;
    QString const &             get_entering_key() const;
    QString const &             get_ticket_key() const;
    QString                     serialize() const;
    void                        unserialize(QString const & data);

private:
    // this is owned by a snaplock function so no need for a smart pointer
    // (and it would create a loop)
    snaplock *                      f_snaplock = nullptr;

    // initialization
    snaplock_messenger::pointer_t   f_messenger = snaplock_messenger::pointer_t();
    QString                         f_object_name = QString();
    time_t                          f_obtention_timeout = 0;
    time_t                          f_alive_timeout = 0;
    snap::snap_lock::timeout_t      f_lock_duration = 0;
    snap::snap_lock::timeout_t      f_unlock_duration = 0;
    QString                         f_server_name = QString();
    QString                         f_service_name = QString();
    QString                         f_owner = QString();
    serial_t                        f_serial = NO_SERIAL;

    // initialized, entering
    QString                         f_entering_key = QString();
    bool                            f_get_max_ticket = false;

    // entered, adding ticket
    ticket_id_t                     f_our_ticket = NO_TICKET;
    bool                            f_added_ticket = false;
    QString                         f_ticket_key = QString();

    // ticket added, exiting
    bool                            f_added_ticket_quorum = false;
    snaplock_ticket::key_map_t      f_still_entering = snaplock_ticket::key_map_t();

    // exited, ticket ready
    bool                            f_ticket_ready = false;

    // locked
    bool                            f_locked = false;
    time_t                          f_lock_timeout = 0;

    // the lock did not take (in most cases, this is because of a timeout)
    bool                            f_lock_failed = false;
};



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class snaplock
    : public snap::snap_communicator::connection_with_send_message
    , public snap::dispatcher<snaplock>
    , public std::enable_shared_from_this<snaplock>
{
public:
    typedef std::shared_ptr<snaplock>      pointer_t;

    class computer_t
    {
    public:
        typedef int64_t                             priority_t;
        typedef uint32_t                            random_t;
        typedef std::shared_ptr<computer_t>         pointer_t;
        typedef std::map<QString, pointer_t>        map_t;
        typedef std::vector<pointer_t>              vector_t;

        static priority_t const PRIORITY_UNDEFINED = -1;
        static priority_t const PRIORITY_MIN = 0;
        static priority_t const PRIORITY_LEADER = 0;
        static priority_t const PRIORITY_USER_MIN = 1;
        static priority_t const PRIORITY_DEFAULT = 14;
        static priority_t const PRIORITY_OFF = 15;
        static priority_t const PRIORITY_MAX = 15;

                                computer_t();
                                computer_t(QString const & name, uint8_t priority);

        bool                    is_self() const;
        void                    set_connected(bool connected);
        bool                    get_connected() const;
        bool                    set_id(QString const & id);
        priority_t              get_priority() const;
        void                    set_start_time(time_t start_time);
        time_t                  get_start_time() const;

        QString const &         get_name() const;
        QString const &         get_id() const;
        QString const &         get_ip_address() const;

    private:
        mutable QString         f_id = QString();

        bool                    f_connected = true;
        bool                    f_self = false;

        priority_t              f_priority = PRIORITY_UNDEFINED;
        random_t                f_random_id = 0;
        QString                 f_ip_address = QString();
        pid_t                   f_pid = 0;
        QString                 f_name = QString();

        time_t                  f_start_time = -1;
    };

    static int64_t const        DEFAULT_TIMEOUT = 5; // in seconds
    static int64_t const        MIN_TIMEOUT     = 3; // in seconds

                                snaplock(int argc, char * argv[]);
                                snaplock(snaplock const & rhs) = delete;
    virtual                     ~snaplock();

    snaplock &                  operator = (snaplock const & rhs) = delete;

    void                        run();
    //void                        process_message(snap::snap_communicator_message const & message);
    void                        tool_message(snap::snap_communicator_message const & message);
    void                        process_connection(int const s);

    // connection_with_send_message overloads
    //
    virtual bool                send_message(snap::snap_communicator_message const & message, bool cache = false) override;

    int                         get_computer_count() const;
    int                         quorum() const;
    QString const &             get_server_name() const;
    bool                        is_ready() const;
    computer_t::pointer_t       is_leader(QString id = QString()) const;
    computer_t::pointer_t       get_leader_a() const;
    computer_t::pointer_t       get_leader_b() const;
    void                        info();
    void                        debug_info();
    void                        cleanup();
    snaplock_ticket::ticket_id_t get_last_ticket(QString const & object_name);
    void                        set_ticket(QString const & object_name, QString const & key, snaplock_ticket::pointer_t ticket);
    void                        lock_exiting(snap::snap_communicator_message & message);
    snaplock_ticket::key_map_t const get_entering_tickets(QString const & object_name);
    QString                     serialized_tickets();
    snaplock_ticket::pointer_t  find_first_lock(QString const & object_name);

    // implementation of snap::snapcommunicator::connection_with_send_message
    //
    // note: ready() is part of the understood messages, it has to be
    //       public for the snaplock_interrupt.cpp to call it
    //
    virtual void                ready(snap::snap_communicator_message & message) override; // no "msg_" because that's in connection_with_send_message
    virtual void                stop(bool quitting) override; // no "msg_" because that's in connection_with_send_message

    static void                 sighandler(int sig);
    static void                 sigloghandler(int sig);

private:
    struct message_cache
    {
        typedef std::vector<message_cache>  vector_t;

        time_t                              f_timeout = 0;
        snap::snap_communicator_message     f_message = snap::snap_communicator_message();
    };

    void                        get_parameters(snap::snap_communicator_message const & message, QString * object_name, pid_t * client_pid, time_t * timeout, QString * key, QString * source);
    void                        activate_first_lock(QString const & object_name);
    void                        ticket_list(snap::snap_communicator_message const & message);
    void                        send_lockstarted(snap::snap_communicator_message const * message);
    void                        election_status();
    void                        check_lock_status();
    void                        synchronize_leaders();
    void                        forward_message_to_leader(snap::snap_communicator_message & message);

    // messages handled by the dispatcher
    // (see also ready() and stop() above)
    //
    void                        msg_absolutely(snap::snap_communicator_message & message);
    void                        msg_activate_lock(snap::snap_communicator_message & message);
    void                        msg_add_ticket(snap::snap_communicator_message & message);
    void                        msg_cluster_up(snap::snap_communicator_message & message);
    void                        msg_cluster_down(snap::snap_communicator_message & message);
    void                        msg_server_gone(snap::snap_communicator_message & message);
    void                        msg_drop_ticket(snap::snap_communicator_message & message);
    void                        msg_get_max_ticket(snap::snap_communicator_message & message);
    void                        msg_lock(snap::snap_communicator_message & message);
    void                        msg_lock_activated(snap::snap_communicator_message & message);
    void                        msg_lock_entered(snap::snap_communicator_message & message);
    void                        msg_lock_entering(snap::snap_communicator_message & message);
    void                        msg_lock_exiting(snap::snap_communicator_message & message);
    void                        msg_lock_failed(snap::snap_communicator_message & message);
    void                        msg_lock_leaders(snap::snap_communicator_message & message);
    void                        msg_lock_started(snap::snap_communicator_message & message);
    void                        msg_lock_status(snap::snap_communicator_message & message);
    void                        msg_lock_tickets(snap::snap_communicator_message & message);
    void                        msg_list_tickets(snap::snap_communicator_message & message);
    void                        msg_max_ticket(snap::snap_communicator_message & message);
    void                        msg_status(snap::snap_communicator_message & message);
    void                        msg_ticket_added(snap::snap_communicator_message & message);
    void                        msg_ticket_ready(snap::snap_communicator_message & message);
    void                        msg_unlock(snap::snap_communicator_message & message);

    static snap::dispatcher<snaplock>::dispatcher_match::vector_t const    g_snaplock_service_messages;

    advgetopt::getopt                   f_opt;
    snap::snap_config                   f_config;
    time_t                              f_start_time = -1;
    QString                             f_log_conf = QString("/etc/snapwebsites/logger/snaplock.properties");
    QString                             f_server_name = QString();
    QString                             f_service_name = QString("snaplock");
    QString                             f_communicator_addr = QString("localhost");
    int                                 f_communicator_port = 4040;
    snap::snap_communicator::pointer_t  f_communicator = snap::snap_communicator::pointer_t();
    QString                             f_host_list = QString("localhost");
    snaplock_messenger::pointer_t       f_messenger = snaplock_messenger::pointer_t();
    snaplock_interrupt::pointer_t       f_interrupt = snaplock_interrupt::pointer_t();
    snaplock_timer::pointer_t           f_timer = snaplock_timer::pointer_t();
    snaplock_info::pointer_t            f_info = snaplock_info::pointer_t();
    snaplock_debug_info::pointer_t      f_debug_info = snaplock_debug_info::pointer_t();
    bool                                f_stop_received = false;
    bool                                f_debug = false;
    bool                                f_debug_lock_messages = false;
    size_t                              f_neighbors_count = 0;
    size_t                              f_neighbors_quorum = 0;
    QString                             f_my_id = QString();
    QString                             f_my_ip_address = QString();
    QString                             f_lock_status = QString("NOLOCK");
    computer_t::map_t                   f_computers = computer_t::map_t();     // key is the computer name
    computer_t::vector_t                f_leaders = computer_t::vector_t();
    int                                 f_next_leader = 0;
    message_cache::vector_t             f_message_cache = message_cache::vector_t();
    snaplock_ticket::object_map_t       f_entering_tickets = snaplock_ticket::object_map_t();
    snaplock_ticket::object_map_t       f_tickets = snaplock_ticket::object_map_t();
    int64_t                             f_election_date = 0;
    snaplock_ticket::serial_t           f_ticket_serial = 0;
    mutable time_t                      f_pace_lockstarted = 0;
};
#pragma GCC diagnostic pop


}
// snaplock namespace
// vim: ts=4 sw=4 et
