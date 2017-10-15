/*
 * Text:
 *      snapwebsites/snaplistd/src/snaplistd.h
 *
 * Description:
 *      A daemon to collect all the changing website pages so the pagelist
 *      can manage them.
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


class snaplistd;



class snaplistd_interrupt
        : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<snaplistd_interrupt>     pointer_t;

                                snaplistd_interrupt(snaplistd * sl);
    virtual                     ~snaplistd_interrupt() override {}

    // snap::snap_communicator::snap_signal implementation
    virtual void                process_signal() override;

private:
    snaplistd *                  f_snaplistd = nullptr;
};



class snaplistd_messenger
        : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<snaplistd_messenger>     pointer_t;

                                snaplistd_messenger(snaplistd * sl, std::string const & addr, int port);
    virtual                     ~snaplistd_messenger() override {}

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void                process_message(snap::snap_communicator_message const & message) override;
    virtual void                process_connection_failed(std::string const & error_message) override;
    virtual void                process_connected() override;

protected:
    // this is owned by a snaplistd function so no need for a smart pointer
    // (and it would create a loop)
    snaplistd *                  f_snaplistd = nullptr;
};



class snaplistd_mysql_timer
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<snaplistd_mysql_timer>    pointer_t;

    snaplistd_mysql_timer(snaplistd * proxy);

    // snap::snap_communicator::snap_timer implementation
    virtual void process_timeout() override;

private:
    // this is owned by a server function so no need for a smart pointer
    snaplistd *             f_snaplistd = nullptr;
};



class snaplistd
{
public:
    typedef std::shared_ptr<snaplistd>      pointer_t;

    static int64_t const        DEFAULT_TIMEOUT = 5; // in seconds
    static int64_t const        MIN_TIMEOUT     = 3; // in seconds

                                snaplistd( int argc, char * argv[] );
    virtual                     ~snaplistd();

    void                        run();
    void                        process_message(snap::snap_communicator_message const & message);
    void                        tool_message(snap::snap_communicator_message const & message);
    void                        process_connection(int const s);
    void                        stop(bool quitting);
    void                        process_timeout();

    int                         get_computer_count() const;
    int                         quorum() const;
    QString const &             get_server_name() const;

    static void                 sighandler( int sig );

private:
                                snaplistd( snaplistd const & ) = delete;
    snaplistd &                  operator = ( snaplistd const & ) = delete;

    void                        usage(advgetopt::getopt::status_t status);
    void                        list_data(snap::snap_communicator_message const & message);
    void                        no_mysql();

    static pointer_t                    f_instance;
    advgetopt::getopt                   f_opt;
    snap::snap_config                   f_config;
    QString                             f_log_conf = "/etc/snapwebsites/logger/snaplistd.properties";
    QString                             f_server_name;
    QString                             f_communicator_addr = "localhost";
    int                                 f_communicator_port = 4040;
    snap::snap_communicator::pointer_t  f_communicator;
    snaplistd_messenger::pointer_t      f_messenger;
    snaplistd_interrupt::pointer_t      f_interrupt;
    snaplistd_mysql_timer::pointer_t    f_mysql_timer;
    bool                                f_debug = false;
    bool                                f_debug_listd_messages = false;
    float                               f_mysql_connect_timer_index = 1.625f;
};


// vim: ts=4 sw=4 et
