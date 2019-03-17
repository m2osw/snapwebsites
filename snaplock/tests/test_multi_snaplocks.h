/*
 * Text:
 *      snaplock/tests/test_multi_snaplocks.h
 *
 * Description:
 *      Test used to verify that snaplock works as expected in large numbers
 *      and only using a single computer.
 *
 * License:
 *      Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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
#pragma once


// snapwebsites lib
//
#include <snapwebsites/snap_communicator_dispatcher.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>



namespace snap_test
{


class test_exception : public snap::snap_exception
{
public:
    explicit test_exception(char const *        what_msg) : snap_exception("test", what_msg) {}
    explicit test_exception(std::string const & what_msg) : snap_exception("test", what_msg) {}
    explicit test_exception(QString const &     what_msg) : snap_exception("test", what_msg) {}
};

class test_exception_exit : public test_exception
{
public:
    explicit test_exception_exit(char const *        what_msg) : test_exception(what_msg) {}
    explicit test_exception_exit(std::string const & what_msg) : test_exception(what_msg) {}
    explicit test_exception_exit(QString const &     what_msg) : test_exception(what_msg) {}
};




// a few forward definitions
//
class snapcommunicator_listener;
class snapcommunicator_emulator;
class test_multi_snaplocks;


typedef std::shared_ptr<snapcommunicator_listener>      snapcommunicator_listener_pointer_t;
typedef std::shared_ptr<snapcommunicator_emulator>      snapcommunicator_emulator_pointer_t;
typedef std::shared_ptr<test_multi_snaplocks>           test_multi_snaplocks_pointer_t;




class signal_ctrl_c
    : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<signal_ctrl_c>     pointer_t;

                            signal_ctrl_c(test_multi_snaplocks * s);
                            signal_ctrl_c(signal_ctrl_c const & rhs) = delete;
    virtual                 ~signal_ctrl_c() override {}

    signal_ctrl_c &         operator = (signal_ctrl_c const & rhs) = delete;

    // snap_communicator::snap_signal implementation
    virtual void            process_signal() override;

private:
    test_multi_snaplocks *  f_server = nullptr;
};





class signal_terminate
    : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<signal_terminate>     pointer_t;

                            signal_terminate(test_multi_snaplocks * s);
                            signal_terminate(signal_terminate const & rhs) = delete;
    virtual                 ~signal_terminate() override {}

    signal_terminate &      operator = (signal_terminate const & rhs) = delete;

    // snap_communicator::snap_signal implementation
    virtual void            process_signal() override;

private:
    test_multi_snaplocks *  f_server = nullptr;
};





class signal_child_death
    : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<signal_child_death>     pointer_t;

                            signal_child_death(test_multi_snaplocks * s);
                            signal_child_death(signal_child_death const & rhs) = delete;
    virtual                 ~signal_child_death() override {}

    signal_child_death &    operator = (signal_child_death const & rhs) = delete;

    // snap_communicator::snap_signal implementation
    virtual void            process_signal() override;

private:
    test_multi_snaplocks *  f_server = nullptr;
};





class snapcommunicator_messenger
    : public snap::snap_communicator::snap_tcp_server_client_message_connection
{
public:
    typedef std::shared_ptr<snapcommunicator_messenger> pointer_t;

                                    snapcommunicator_messenger(snapcommunicator_listener_pointer_t listener, tcp_client_server::bio_client::pointer_t client);
                                    snapcommunicator_messenger(snapcommunicator_messenger const & rhs) = delete;
    virtual                         ~snapcommunicator_messenger() override;

    snapcommunicator_messenger &    operator = (snapcommunicator_messenger const & rhs) = delete;

    // implementation of snap_tcp_server_client_buffer_connection
    virtual void                    process_hup() override;

private:
    snapcommunicator_listener_pointer_t f_listener;
};




#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class snapcommunicator_listener
    : public snap::snap_communicator::snap_tcp_server_connection
{
public:
    typedef std::shared_ptr<snapcommunicator_listener>  pointer_t;

                        snapcommunicator_listener(test_multi_snaplocks_pointer_t test, snapcommunicator_emulator_pointer_t ce, int port);
    virtual             ~snapcommunicator_listener() override;

    void                stop();
    pointer_t           shared_from_this();
    bool                send_message(snap::snap_communicator_message const & message, bool cache = false);
    bool                is_connected() const;
    void                messenger_hup();

    // implementation of snap_tcp_server_connection
    //
    virtual void        process_accept() override;

private:
    test_multi_snaplocks_pointer_t          f_test = test_multi_snaplocks_pointer_t();
    snapcommunicator_emulator_pointer_t     f_communicator_emulator = snapcommunicator_emulator_pointer_t();
    snapcommunicator_messenger::pointer_t   f_messenger = snapcommunicator_messenger::pointer_t();
    int                                     f_port = 0;
};
#pragma GCC diagnostic pop






#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class snapcommunicator_emulator
    : public snap::snap_communicator::snap_timer
    , public snap::snap_communicator::connection_with_send_message
    , public snap::dispatcher<snapcommunicator_emulator>
    //, public std::enable_shared_from_this<snapcommunicator_emulator>
{
public:
    typedef std::shared_ptr<snapcommunicator_emulator>    pointer_t;

                        snapcommunicator_emulator(test_multi_snaplocks_pointer_t test, int port);
    virtual             ~snapcommunicator_emulator() override;

    void                cleanup();
    pointer_t           shared_from_this();
    void                start();
    int                 get_port() const;
    bool                is_connected() const;
    bool                is_locked(QString const & object_name) const;
    void                mark_unlocked();

    // implementation of connection_with_send_message
    // send message through f_messenger
    //
    virtual bool        send_message(snap::snap_communicator_message const & message, bool cache = false) override;

    // implementation of snap::snap_communicator::snap_connection
    //
    virtual void        process_timeout() override;

private:
    void                set_timer();
    bool                need_to_forward_message(snap::snap_communicator_message & message, void (snapcommunicator_emulator::*func)(snap::snap_communicator_message & message));

    void                msg_callback(snap::snap_communicator_message & message);
    void                msg_forward(snap::snap_communicator_message & message);

    void                msg_cluster_status(snap::snap_communicator_message & message);
    void                msg_commands(snap::snap_communicator_message & message);
    void                msg_locked(snap::snap_communicator_message & message);
    void                msg_lockfailed(snap::snap_communicator_message & message);
    void                msg_lockready(snap::snap_communicator_message & message);
    void                msg_nolock(snap::snap_communicator_message & message);
    void                msg_register(snap::snap_communicator_message & message);
    void                msg_ticketlist(snap::snap_communicator_message & message);
    void                msg_unlocked(snap::snap_communicator_message & message);
    void                msg_unregister(snap::snap_communicator_message & message);

    static snap::dispatcher<snapcommunicator_emulator>::dispatcher_match::vector_t const
                        g_snapcommunicator_emulator_service_messages;

    test_multi_snaplocks_pointer_t
                        f_test;
    int const           f_port;
    snapcommunicator_messenger::pointer_t
                        f_messenger = snapcommunicator_messenger::pointer_t();
    snapcommunicator_listener::pointer_t
                        f_listener = snapcommunicator_listener::pointer_t();
    QString             f_object_name = QString();
    bool                f_locked = false;
};
#pragma GCC diagnostic pop



class snaplock_executable
    : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<snaplock_executable>    pointer_t;

                        snaplock_executable(test_multi_snaplocks_pointer_t test, int port, std::string const & snaplock_path, std::string const & config_path);
                        ~snaplock_executable();

    void                start();
    void                stop(int sig = SIGTERM);
    void                wait_child();
    int                 get_port() const;
    pid_t               get_child_pid() const;

    // snap::snap_communicator::snap_connection implementation
    virtual void        process_timeout() override;

private:
    int const           f_port;
    std::string const   f_snaplock_executable;
    std::string const   f_config_path;
    pid_t               f_child = static_cast<pid_t>(-1);
    test_multi_snaplocks_pointer_t
                        f_test;
};



class communicator_and_lock
{
public:
    typedef std::shared_ptr<communicator_and_lock>  pointer_t;
    typedef std::vector<pointer_t>                  vector_t;
    typedef std::vector<decltype(std::bind(std::declval<int64_t (communicator_and_lock::*)()>(), std::declval<communicator_and_lock *>()))>
                                                    functions_t;


                                            communicator_and_lock(test_multi_snaplocks_pointer_t test, int port, std::string const & snaplock_path, std::string const & config_path);
                                            ~communicator_and_lock();

    int                                     get_port() const;
    int64_t                                 start_communicator();
    int64_t                                 start_snaplock();
    int64_t                                 pause();
    void                                    stop();
    bool                                    is_communicator_connected() const;
    bool                                    send_message(snap::snap_communicator_message const & message);
    void                                    remove_communicator();
    void                                    remove_snaplock();
    void                                    kill_snaplock();

    snapcommunicator_emulator::pointer_t    get_communicator() const;
    snaplock_executable::pointer_t          get_snaplock() const;

private:
    std::string const                       f_snaplock_executable;
    std::string const                       f_config_path;
    snapcommunicator_emulator::pointer_t    f_communicator;
    snaplock_executable::pointer_t          f_snaplock;
};





class start_timer
    : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<start_timer>    pointer_t;

                                        start_timer(test_multi_snaplocks * test);
                                        start_timer(start_timer const & rhs) = delete;
    start_timer                         operator = (start_timer const & rhs) = delete;

    // snap::snap_communicator::snap_timer implementation
    virtual void                        process_timeout() override;

private:
    // this is owned by a server function so no need for a smart pointer
    test_multi_snaplocks *              f_test = nullptr;
};









class new_connection_timer
    : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<new_connection_timer>    pointer_t;

                                        new_connection_timer(test_multi_snaplocks * test);
                                        new_connection_timer(new_connection_timer const & rhs) = delete;
    new_connection_timer                operator = (new_connection_timer const & rhs) = delete;

    // snap::snap_communicator::snap_timer implementation
    virtual void                        process_timeout() override;

private:
    // this is owned by a server function so no need for a smart pointer
    test_multi_snaplocks *              f_test = nullptr;
};









class death_timer
    : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<death_timer>    pointer_t;

                                        death_timer(test_multi_snaplocks * test);
                                        death_timer(death_timer const & rhs) = delete;
    death_timer                         operator = (death_timer const & rhs) = delete;

    // snap::snap_communicator::snap_timer implementation
    virtual void                        process_timeout() override;

private:
    // this is owned by a server function so no need for a smart pointer
    test_multi_snaplocks *              f_test = nullptr;
};









class test_multi_snaplocks
    : public std::enable_shared_from_this<test_multi_snaplocks>
{
public:
    typedef std::shared_ptr<test_multi_snaplocks>    pointer_t;

                        test_multi_snaplocks(int argc, char ** argv);
                        ~test_multi_snaplocks();

    void                run();
    void                start_next();
    void                stop();
    void                capture_zombie(pid_t child);
    void                set_death_timer_status(bool status);
    int                 get_count() const;
    int                 get_number_of_connections() const;
    bool                send_message(snap::snap_communicator_message const & message, int port);
    bool                broadcast(snap::snap_communicator_message const & message, int except_port);
    void                forward_message(snap::snap_communicator_message & message, int port, void (snapcommunicator_emulator::*func)(snap::snap_communicator_message & message));
    void                remove_communicators_and_locks();
    void                close_connections(bool force_close);
    void                received_new_connection();
    void                check_cluster_status();
    void                verify_lock(QString const & object_name, int port);
    void                kill_a_snaplock();

private:
    static void         sighandler(int sig);

    advgetopt::getopt                   f_opt;
    int                                 f_count = 0;        // number of instances
    int                                 f_port = 9000;      // starting port
    std::string                         f_snaplock_executable = std::string("snaplock");
    std::string                         f_config_path = std::string("/tmp/test-multi-snaplock");
    QString                             f_cluster_status = QString("CLUSTERDOWN");
    communicator_and_lock::vector_t     f_emulators = communicator_and_lock::vector_t();
    communicator_and_lock::functions_t  f_start = communicator_and_lock::functions_t();
    std::vector<size_t>                 f_start_indexes = std::vector<size_t>();
    start_timer::pointer_t              f_start_timer = start_timer::pointer_t();
    new_connection_timer::pointer_t     f_new_connection_timer = new_connection_timer::pointer_t();
    death_timer::pointer_t              f_death_timer = death_timer::pointer_t();
    signal_ctrl_c::pointer_t            f_signal_ctrl_c = signal_ctrl_c::pointer_t();
    signal_terminate::pointer_t         f_signal_terminate = signal_terminate::pointer_t();
    signal_child_death::pointer_t       f_signal_child_death = signal_child_death::pointer_t();
};


// special seeds at some point
//     1536887852    generate CLUSTERUP before the 1 minute wait


}
// snap_test namespace
// vim: ts=4 sw=4 et
