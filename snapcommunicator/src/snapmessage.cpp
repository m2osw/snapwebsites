// Snap Websites Server -- to send UDP signals to backends
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

// snapwebsites library
//
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snapwebsites.h>

// advgetopt library
//
#include <advgetopt/advgetopt.h>

// C++ library
//
#include <atomic>

// readline library
//
#include <readline/readline.h>
#include <readline/history.h>


namespace
{
    char const * history_file = ".snapmessage_history";

    const std::vector<std::string> g_configuration_files
    {
    };

    const advgetopt::getopt::option g_command_line_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "Usage: %p [-<opt>] [message]",
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
            'a',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
            "address",
            nullptr,
            "the address and port to connect to (i.e. \"127.0.0.1:4040\")",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
            "cui",
            nullptr,
            "start in interactive mode in your console",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
            "gui",
            nullptr,
            "open a graphical window with a input and an output console",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "show this help output",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "ssl",
            nullptr,
            "if specified, make a secure connection (with encryption)",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "tcp",
            nullptr,
            "send a TCP message; use --wait to also wait for a reply and display it in your console",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "udp",
            nullptr,
            "send a UDP message and quit",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'v',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "verbose",
            nullptr,
            "make the output verbose",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "show the version of %p and exit",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "wait",
            nullptr,
            "in case you used --tcp, this tells sendmessage to wait for a reply before quiting",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "message",
            nullptr,
            nullptr, // hidden argument in --help screen
            advgetopt::getopt::argument_mode_t::default_multiple_argument
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
}
//namespace


class tcp_message_connection
    : public snap::snap_communicator::snap_tcp_client_message_connection
{
public:
    typedef std::shared_ptr<tcp_message_connection> message_pointer_t;

    tcp_message_connection(std::string const & addr, int port, mode_t const mode)
        : snap_tcp_client_message_connection(addr, port, mode, false)
    {
    }

    virtual ~tcp_message_connection()
    {
    }

    virtual void process_error() override
    {
        snap_tcp_client_message_connection::process_error();

        std::cerr << "error: an error occurred while handling a message." << std::endl;
    }

    virtual void process_hup() override
    {
        snap_tcp_client_message_connection::process_hup();

        std::cerr << "error: the connection hang up on us, while handling a message." << std::endl;
    }

    virtual void process_invalid() override
    {
        snap_tcp_client_message_connection::process_invalid();

        std::cerr << "error: the connection is invalid." << std::endl;
    }

    virtual void process_message(snap::snap_communicator_message const & message) override
    {
        std::cout << std::endl
                  << "Received message:" << std::endl
                  << "  " << message.to_message() << std::endl;
    }

private:
};


class snapmessage
{
public:
    enum class connection_t
    {
        NONE,
        TCP,
        UDP
    };

    class connection_handler
    {
    public:
        typedef std::shared_ptr<connection_handler>     pointer_t;

        class connection_runner
            : public snap::snap_thread::snap_runner
            , public snap::snap_communicator::snap_signal
            //, public std::enable_shared_from_this<connection_runner> -- already defined in snap_connection
        {
        public:
            typedef std::shared_ptr<connection_runner>  pointer_t;

            enum class event
            {
                EVENT_NONE,         // no specific event

                EVENT_DISCONNECT,   // disconnect current TCP connection
                EVENT_MESSAGE,      // send specified message
                EVENT_PLAIN,        // switch to non-SSL
                EVENT_QUIT,         // request thread to quit
                EVENT_SECURE,       // switch to SSL
                EVENT_TCP,          // switch to TCP
                EVENT_UDP           // switch to UDP
            };

            class thread_message
            {
            public:
                static int const        MESSAGE_SIGNAL = SIGUSR1;

                thread_message(event e, std::string const & p = std::string())
                    : f_event(e)
                    , f_param(p)
                {
                }

                event get_event() const
                {
                    return f_event;
                }

                std::string const & get_param() const
                {
                    return f_param;
                }

            private:
                event               f_event;
                std::string         f_param;
            };

            connection_runner(snapmessage * sm)
                : snap_runner("connection_runner")
                , snap_signal(thread_message::MESSAGE_SIGNAL)
                , f_snapmessage(sm)
            {
            }

            //virtual bool is_ready() const override
            //{
            //    return true;
            //}

            //virtual bool continue_running() const override
            //{
            //    return true;
            //}

            virtual void run() override
            {
                // before we block we want to make sure our message FIFO
                // is empty
                //
std::cerr << "thread started...\n";
                process_messages();

std::cerr << "thread block on connections...\n";
                snap::snap_communicator::instance()->run();
            }

            void process_messages()
            {
                thread_message msg(event::EVENT_NONE);
                while(f_thread_messages.pop_front(msg, 0))
                {
                    switch(msg.get_event())
                    {
                    case event::EVENT_NONE:
                        break;

                    case event::EVENT_DISCONNECT:
                        f_snapmessage->switch_connection_type(connection_t::NONE);
                        //f_snapmessage->disconnect(); -- don't call directly, just in case
                        break;

                    case event::EVENT_MESSAGE:
                        snap::NOTUSED(f_snapmessage->send_message(msg.get_param()));
                        break;

                    case event::EVENT_PLAIN:
                        f_snapmessage->switch_mode(tcp_client_server::bio_client::mode_t::MODE_PLAIN);
                        break;

                    case event::EVENT_QUIT:
                        // remove the runner (signal)
                        //
                        snap::snap_communicator::instance()->remove_connection(shared_from_this());

                        // remove the TCP connection if still there
                        //
                        f_snapmessage->disconnect();
                        break;

                    case event::EVENT_SECURE:
                        f_snapmessage->switch_mode(tcp_client_server::bio_client::mode_t::MODE_SECURE);
                        break;

                    case event::EVENT_TCP:
                        f_snapmessage->switch_connection_type(connection_t::TCP);
                        break;

                    case event::EVENT_UDP:
                        f_snapmessage->switch_connection_type(connection_t::UDP);
                        break;

                    }
                }
            }

            // the parent thread added a message to our FIFO and then
            // called kill() on our thread
            //
            virtual void process_signal() override
            {
std::cerr << "received signal...\n";
                process_messages();
            }

            void new_message(thread_message const & msg)
            {
                // note: the push_back() is thread safe already so we do
                // not have to use a mutex here
                //
                f_thread_messages.push_back(msg);
                snap::snap_thread * parent(get_thread());
                if(parent != nullptr)
                {
                    // if the thread returned before we could send
                    // the signal, it does not matter much to us at
                    // this point
                    //
                    snap::NOTUSED(parent->kill(thread_message::MESSAGE_SIGNAL));
std::cerr << "signaling new data...\n";
                }
            }

        private:
            snapmessage *                                   f_snapmessage = nullptr;
            snap::snap_thread::snap_fifo<thread_message>    f_thread_messages;
        };

        class connection_thread
            : public snap::snap_thread
        {
        public:
            connection_thread(connection_runner * runner)
                : snap_thread("connection_thread", runner)
            {
            }

        };

        connection_handler(snapmessage * sm)
            : f_runner(new connection_runner(sm))
            , f_thread(new connection_thread(f_runner.get()))
        {
            // f_runner is also a snap_signal connection which we
            // use to wake the thread up and process additional
            // messages
            //
            snap::snap_communicator::instance()->add_connection(f_runner);

            if(!f_thread->start())
            {
                std::cerr << "error: could not start the connection thread.\n";
                exit(1);
            }
        }

        ~connection_handler()
        {
            connection_runner::thread_message msg(connection_runner::event::EVENT_QUIT);
            new_message(msg);

            // make sure the thread is stopped first
            //
            f_thread->stop();
        }

        void new_message(connection_runner::thread_message const & msg)
        {
            f_runner->new_message(msg);
        }

    private:
        connection_runner::pointer_t    f_runner;
        connection_thread::pointer_t    f_thread;
    };

    snapmessage(int argc, char * argv[])
        : f_opt(argc, argv, g_command_line_options, g_configuration_files, "")
    {
        // --version?
        if(f_opt.is_defined("version"))
        {
            std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
            exit(0);
            snap::NOTREACHED();
        }

        // --help?
        if(f_opt.is_defined("help"))
        {
            f_opt.usage(advgetopt::getopt::status_t::no_error, "snapmessage");
            exit(1);
            snap::NOTREACHED();
        }

        f_gui = f_opt.is_defined("gui");

        f_cui = f_opt.is_defined("cui")
            || !f_opt.is_defined("message");

        if(f_gui
        && f_cui)
        {
            std::cerr << "error: --gui and --cui are mutually exclusive." << std::endl;
            exit(1);
            snap::NOTREACHED();
        }

        if(f_cui)
        {
            if(f_opt.is_defined("message"))
            {
                std::cerr << "error: --message is not compatible with --cui." << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
        }
        else
        {
            if(!f_opt.is_defined("address"))
            {
                std::cerr << "error: --address is mandatory when not entering the CUI or GUI interface." << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
        }

        if(f_opt.is_defined("tcp")
        && f_opt.is_defined("udp"))
        {
            std::cerr << "error: --tcp and --udp are mutually exclusive" << std::endl;
            exit(1);
            snap::NOTREACHED();
        }

        if(f_opt.is_defined("address"))
        {
            f_address = f_opt.get_string("address");
        }

        f_mode = f_opt.is_defined("ssl")
                ? tcp_client_server::bio_client::mode_t::MODE_SECURE
                : tcp_client_server::bio_client::mode_t::MODE_PLAIN;
    }

    int run()
    {
        if(f_gui)
        {
            return start_gui();
        }

        if(f_cui)
        {
            return enter_cui();
        }

        if(f_opt.is_defined("tcp"))
        {
            return send_message_with_tcp();
        }

        if(f_opt.is_defined("udp"))
        {
            return send_message_with_udp();
        }

        std::cerr << "error: no command specified, one of --gui, --cui, --tcp, --udp is required." << std::endl;

        return 1;
    }

    int start_gui()
    {
        std::cerr << "error: the --gui is not yet implemented." << std::endl;
        return 1;
    }

    void start_connection_thread()
    {
        f_connection_handler.reset(new connection_handler(this));
    }

    void end_connection_thread()
    {
        if(f_connection_handler)
        {
            f_connection_handler.reset();
        }
    }

    int enter_cui()
    {
        f_selected_connection_type = f_opt.is_defined("tcp") ? connection_t::TCP : connection_t::UDP;

        start_connection_thread();

        using_history();

        std::string history_filename;
        char * home = getenv("HOME");
        if(home != nullptr)
        {
            history_filename = home;
            history_filename += "/";
            history_filename += history_file;
        }
        if(!history_filename.empty())
        {
            read_history(history_filename.c_str());
        }

        for(;;)
        {
            // read a line
            //
            std::string prompt(f_selected_connection_type == connection_t::TCP ? "tcp" : "udp");
            if(f_mode == tcp_client_server::bio_client::mode_t::MODE_SECURE)
            {
                prompt += "(ssl)";
            }
            // if the current connection type is not NONE then we consider
            // that we are connected; checking that variable is safe since
            // it is atomic
            //
            if(f_connection_type != connection_t::NONE)
            {
                prompt += "*";
            }
            prompt += "> ";
            char * l(readline(prompt.c_str()));
            if(l == nullptr
            || *l == '\0')
            {
                // skip empty lines
                //
                continue;
            }
            add_history(l);
            if(!history_filename.empty())
            {
                write_history(history_filename.c_str()); // TODO: make it every X commands and on /quit only?
            }
            std::string line(l);
            free(l);

            if(line == "/quit")
            {
                // the "/quit" internal command
                //
                end_connection_thread();
                return 0;
            }

            if(line == "/help"
            || line == "/?"
            || line == "?")
            {
                std::cout << "Help:" << std::endl;
                std::cout << "Internal commands start with a  slash (/). Supported commands:" << std::endl;
                std::cout << "  /connect <ip>:<port> -- connect to specified IP and port" << std::endl;
                std::cout << "  /disconnect -- explicitly disconnect any existing connection" << std::endl;
                std::cout << "  /help or /? or ? -- print this help screen" << std::endl;
                std::cout << "  /plain -- get a plain connection" << std::endl;
                std::cout << "  /quit -- leave snapmessage" << std::endl;
                std::cout << "  /tcp -- send messages using our TCP connectionse" << std::endl;
                std::cout << "  /udp -- send messages using our UDP connectionse" << std::endl;
                std::cout << "  /ssl -- get an SSL connection" << std::endl;
                //std::cout << "  /wait -- wait for a reply" << std::endl;
            }
            else if(line.compare(0, 9, "/connect ") == 0)
            {
                f_address = line.substr(9);

                if(f_selected_connection_type == connection_t::TCP)
                {
                    connection_handler::connection_runner::thread_message msg(connection_handler::connection_runner::event::EVENT_TCP);
                    f_connection_handler->new_message(msg);
                }
                else
                {
                    connection_handler::connection_runner::thread_message msg(connection_handler::connection_runner::event::EVENT_UDP);
                    f_connection_handler->new_message(msg);
                }
            }
            else if(line == "/disconnect")
            {
                connection_handler::connection_runner::thread_message msg(connection_handler::connection_runner::event::EVENT_DISCONNECT);
                f_connection_handler->new_message(msg);
            }
            else if(line == "/tcp")
            {
                f_selected_connection_type = connection_t::TCP;
                connection_handler::connection_runner::thread_message msg(connection_handler::connection_runner::event::EVENT_TCP);
                f_connection_handler->new_message(msg);
            }
            else if(line == "/udp")
            {
                f_selected_connection_type = connection_t::UDP;
                connection_handler::connection_runner::thread_message msg(connection_handler::connection_runner::event::EVENT_UDP);
                f_connection_handler->new_message(msg);
            }
            else if(line == "/plain")
            {
                connection_handler::connection_runner::thread_message msg(connection_handler::connection_runner::event::EVENT_PLAIN);
                f_connection_handler->new_message(msg);
            }
            else if(line == "/ssl")
            {
                connection_handler::connection_runner::thread_message msg(connection_handler::connection_runner::event::EVENT_SECURE);
                f_connection_handler->new_message(msg);
            }
            //else if(line == "/wait")
            //{
            //    if(f_connection_type == connection_t::NONE)
            //    {
            //        std::cerr << "error: can't wait as you do not seem to currently have a connection." << std::endl;
            //    }
            //}
            else if(line[0] == '/')
            {
                std::cerr << "error: unknown command: \"" << line << "\"." << std::endl;
            }
            else
            {
                // by default, if not an internal command, we consider the
                // content a message and therefore we just send it
                //
                connection_handler::connection_runner::thread_message msg(connection_handler::connection_runner::event::EVENT_MESSAGE, line);
                f_connection_handler->new_message(msg);
            }
        }

        return 0;
    }

    int send_message_with_tcp()
    {
        int result(send_message(f_opt.get_string("message")) ? 0 : 1);
        if(f_opt.is_defined("wait"))
        {
            // wait for a reply
            //
        }
        return result;
    }

    int send_message_with_udp()
    {
        return send_message(f_opt.get_string("message")) ? 0 : 1;
    }

    void disconnect()
    {
        snap::snap_communicator::instance()->remove_connection(f_tcp_connection);
        f_tcp_connection = nullptr;
        f_connection_type = connection_t::NONE;
    }

    void create_tcp_connection()
    {
std::cerr << "trying to connect with TCP...\n";
        // clear old connection first, just in case
        //
        disconnect();

        // determine the IP address and port
        //
        f_addr = "127.0.0.1";
        f_port = 4041;
        tcp_client_server::get_addr_port(QString::fromUtf8(f_address.c_str()), f_addr, f_port, "tcp");

        // create new connection
        //
        f_tcp_connection = std::make_shared<tcp_message_connection>(f_addr.toUtf8().data(), f_port, f_mode);

        if(snap::snap_communicator::instance()->add_connection(f_tcp_connection))
        {
std::cerr << "connected with TCP!\n";
            f_connection_type = connection_t::TCP;
        }
        else
        {
            // keep NONE (not connected)
            std::cerr << "error: could not connect--make sure that do or do not need to use the --ssl flag." << std::endl;
        }
    }

    void create_udp_connection()
    {
        // clear old connection first, just in case
        //
        disconnect();

        // determine the IP address and port
        //
        f_addr = "127.0.0.1";
        f_port = 4041;
        tcp_client_server::get_addr_port(QString::fromUtf8(f_address.c_str()), f_addr, f_port, "udp");

        // create new connection
        // -- at this point we only deal with client connections here
        //    and this is a UDP server; to send data we just use the
        //    send_message() which is static
        //
        //f_tcp_connection = std::make_shared<snap::snap_communicator::snap_udp_server_message_connection>(addr, port, f_mode);

        f_connection_type = connection_t::UDP;
    }

    bool send_message(std::string const & message)
    {
        // currently disconnected?
        //
        if(f_connection_type == connection_t::NONE)
        {
            // connect as selected by user
            //
            if(f_selected_connection_type == connection_t::TCP)
            {
                create_tcp_connection();
            }
            else
            {
                create_udp_connection();
            }

            if(f_connection_type == connection_t::NONE)
            {
                std::cerr << "error: could not connect, can't send message." << std::endl;
                return false;
            }
        }

        QString qmessage(QString::fromUtf8(message.c_str()));
        snap::snap_communicator_message msg;
        if(!msg.from_message(qmessage))
        {
            std::cerr << "error: message \"" << message << "\" is invalid. It won't be sent." << std::endl;
            return false;
        }

        switch(f_connection_type)
        {
        case connection_t::NONE:
std::cout << "not connected, not sending message [" << message << "]\n";
            return false;

        case connection_t::TCP:
std::cout << "sending TCP message [" << message << "]\n";
            f_tcp_connection->send_message(msg);
            break;

        case connection_t::UDP:
std::cout << "sending UDP message [" << message << "]\n";
            snap::snap_communicator::snap_udp_server_message_connection::send_message(f_addr.toUtf8().data(), f_port, msg);
            break;

        }

        return true;
    }

    void switch_mode(tcp_client_server::bio_client::mode_t mode)
    {
        if(f_mode != mode)
        {
            disconnect();
            f_mode = mode;
        }
    }

    void switch_connection_type(connection_t type)
    {
        if(f_connection_type != type)
        {
            switch(type)
            {
            case connection_t::NONE:
                disconnect();
                break;

            case connection_t::TCP:
                create_tcp_connection();
                break;

            case connection_t::UDP:
                create_udp_connection();
                break;

            }
        }
    }

private:
    advgetopt::getopt                       f_opt;
    bool                                    f_gui                       = false;
    bool                                    f_cui                       = false;
    connection_handler::pointer_t           f_connection_handler;

    // WARNING: ALL of the following variables are accessed by another thread
    //          when running in GUI or CUI mode
    //
    //          note that the f_connection_handler is not really used by
    //          the thread itself even though it itself carries that thread
    //
    //          a few that we want to use in the main thread are marked
    //          as atomic that way we avoid having to handle our own mutex
    //          all over the place
    //
    std::string                             f_address                   = "127.0.0.1:4041";
    QString                                 f_addr                      = "127.0.0.1";
    int                                     f_port                      = 4041;
    std::atomic<tcp_client_server::bio_client::mode_t>  f_mode          { tcp_client_server::bio_client::mode_t::MODE_PLAIN };
    std::atomic<connection_t>               f_selected_connection_type  { connection_t::UDP }; // never set to NONE; default to UDP unless user uses --tcp on command line
    std::atomic<connection_t>               f_connection_type           { connection_t::NONE };
    tcp_message_connection::pointer_t       f_tcp_connection;
};


int main(int argc, char *argv[])
{
    snapmessage sm(argc, argv);

    return sm.run();

    try
    {
        // create a server object
        snap::server::pointer_t s( snap::server::instance() );
        s->setup_as_backend();

        // parse the command line arguments (this also brings in the .conf params)
        s->config(argc, argv);

        // Now create the qt application instance
        //
        s->prepare_qtapp( argc, argv );

        // get the message (Excuse the naming convension...)
        //
        QString const msg(s->get_parameter("__BACKEND_URI"));

        // the message is expected to be a complete message as defined in
        // our snap_communicator system, something like:
        //
        //    <service>/<COMMAND> param=value;...
        //
        snap::snap_communicator_message message;
        message.from_message(msg);

        // get the snap communicator signal IP and port (UDP)
        //
        QString addr("127.0.0.1");
        int port(4041);
        snap::snap_config config("snapcommunicator");
        tcp_client_server::get_addr_port(QString::fromUtf8(config.get_parameter("signal").c_str()), addr, port, "udp");

        // now send the message
        //
        snap::snap_communicator::snap_udp_server_message_connection::send_message(addr.toUtf8().data(), port, message);

        // exit via the server so the server can clean itself up properly
        s->exit(0);
        snap::NOTREACHED();

        return 0;
    }
    catch(std::exception const & e)
    {
        // clean error on exception
        std::cerr << "snapsignal: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
