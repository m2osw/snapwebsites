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
#include <snapwebsites/snap_console.h>
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



/** \file
 * \brief A tool to send and receive messages to services to test them.
 *
 * This tool can be used to test various services and make sure they
 * work as expected, at least for their control feed. If they have
 * network connections that have nothing to do with snap_communicator
 * messaging feeds, then it won't work well.
 *
 * The organization of this file is as follow:
 *
 * +------------------------+
 * |                        |
 * |        Base            |
 * |      (Connection)      |
 * |                        |
 * +------------------------+
 *        ^              ^
 *        |              |
 *        |              +----------------------------+
 *        |              |                            |
 *        |   +----------+-------------+   +----------+-------------+
 *        |   |                        |   |                        |
 *        |   |      GUI Object        |   |     CUI Object         |
 *        |   |                        |   |                        |
 *        |   +------------------------+   +------------------------+
 *        |        ^                                   ^
 *        |        |                                   |
 *        |        |       +---------------------------+
 *        |        |       |
 *     +--+--------+-------+----+
 *     |                        |
 *     |  Snap Message Obj.     |
 *     |                        |
 *     +------------------------+
 */


namespace
{
    char const * history_file = "~/.snapmessage_history";

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



class connection
{
public:
    typedef std::shared_ptr<connection>     pointer_t;

    enum class connection_t
    {
        NONE,
        TCP,
        UDP
    };

    void disconnect()
    {
        snap::snap_communicator::instance()->remove_connection(f_tcp_connection);
        f_tcp_connection = nullptr;
        f_connection_type = connection_t::NONE;
    }

    void set_address(std::string const & addr)
    {
        f_address = addr;
    }

    void create_tcp_connection()
    {
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
            f_connection_type = connection_t::TCP;
        }
        else
        {
            // keep NONE (not connected)
            std::cerr << "error: could not connect--verify the IP, the port, and make sure that do or do not need to use the --ssl flag." << std::endl;
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

    // if not yet connected, attempt a connection
    bool connect()
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

        return true;
    }

    bool send_message(std::string const & message)
    {
        // are we or can we connect?
        //
        if(!connect())
        {
            return false;
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

    // only use at initialization time, otherwise use switch_mode()
    void set_mode(tcp_client_server::bio_client::mode_t mode)
    {
        f_mode = mode;
    }

    void switch_mode(tcp_client_server::bio_client::mode_t mode)
    {
        if(f_mode != mode)
        {
            disconnect();
            f_mode = mode;
        }
    }

    // call when you do /tcp and /udp in CUI/GUI
    void set_selected_connection_type(connection_t type)
    {
        if(f_selected_connection_type != type)
        {
            disconnect();
            f_selected_connection_type = type;
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

    std::string define_prompt() const
    {
        std::string prompt;
        if(f_selected_connection_type == connection_t::TCP)
        {
            prompt = "tcp";
        }
        else
        {
            prompt = "udp";
        }
        if(f_mode == tcp_client_server::bio_client::mode_t::MODE_SECURE)
        {
            prompt += "(ssl)";
        }
        prompt += "> ";
        return prompt;
    }

private:
    // WARNING: The following variables are accessed by another process
    //          when running in GUI or CUI mode (i.e. we do a `fork()`.)
    //
    //          The only way to modify those values once the fork() happened
    //          is by sending messages to the child process.
    //
    std::string                             f_address;
    QString                                 f_addr;
    int                                     f_port                      = 0;
    tcp_client_server::bio_client::mode_t   f_mode                      { tcp_client_server::bio_client::mode_t::MODE_PLAIN };
    connection_t                            f_selected_connection_type  { connection_t::UDP }; // never set to NONE; default to UDP unless user uses --tcp on command line
    connection_t                            f_connection_type           { connection_t::NONE };
    tcp_message_connection::pointer_t       f_tcp_connection;
};






class cui_connection
    : public snap::snap_console
{
public:
    typedef std::shared_ptr<cui_connection>     pointer_t;

    cui_connection(connection::pointer_t connection)
        : snap_console(history_file)
        , f_connection(connection)
    {
    }

    virtual ~cui_connection()
    {
    }

    void reset_prompt()
    {
        set_prompt(f_connection->define_prompt());
    }

    virtual void process_command(std::string const & command) override
    {
        if(execute_command(command))
        {
            // reset the prompt
            //
            reset_prompt();
        }
    }

    virtual void process_quit() override
    {
        f_connection->disconnect();
        snap::snap_communicator::instance()->remove_connection(shared_from_this());
    }

    bool execute_command(std::string const & command)
    {
        // /quit
        //
        // request to quit the process, equivalent to Ctrl-D
        //
// that should be a statistic in a stats window...
//output("# of con: " + std::to_string(snap::snap_communicator::instance()->get_connections().size()));
        if(command == "/quit")
        {
            // the "/quit" internal command
            //
            process_quit();
            return false;
        }

        // /help
        //
        // print out help screen
        //
        if(command == "/help"
        || command == "/?"
        || command == "?")
        {
            output("Help:");
            output("Internal commands start with a  slash (/). Supported commands:");
            output("  /connect <ip>:<port> -- connect to specified IP and port");
            output("  /disconnect -- explicitly disconnect any existing connection");
            output("  /help or /? or ? -- print this help screen");
            output("  /plain -- get a plain connection");
            output("  /quit -- leave snapmessage");
            output("  /tcp -- send messages using our TCP connectionse");
            output("  /udp -- send messages using our UDP connectionse");
            output("  /ssl -- get an SSL connection");
            return false;
        }

        // /connect <IP>:<port>
        //
        // connect to service listening at <IP> on port <port>
        //
        if(command.compare(0, 9, "/connect ") == 0)
        {
            f_connection->set_address(command.substr(9));
            f_connection->connect();
            return false;
        }

        // /disconnect
        //
        // remove the existing connection
        //
        if(command == "/disconnect")
        {
            f_connection->disconnect();
            return false;
        }

        // /tcp
        //
        // switch to TCP
        //
        if(command == "/tcp")
        {
            f_connection->set_selected_connection_type(connection::connection_t::TCP);
            return true;
        }

        // /udp
        //
        // switch to UDP
        //
        if(command == "/udp")
        {
            f_connection->set_selected_connection_type(connection::connection_t::UDP);
            return true;
        }

        // /plain
        //
        // switch to plain mode (opposed to SSL)
        //
        if(command == "/plain")
        {
            f_connection->switch_mode(tcp_client_server::bio_client::mode_t::MODE_PLAIN);
            return true;
        }

        // /ssl
        //
        // switch to SSL mode (opposed to plain, unencrypted)
        //
        if(command == "/ssl")
        {
            f_connection->switch_mode(tcp_client_server::bio_client::mode_t::MODE_SECURE);
            return true;
        }

        // "/.*" is not a valid message beginning, we suspect that the user
        // misstyped a command and thus generate an error instead
        //
        if(command[0] == '/')
        {
            output("error: unknown command: \"" + command + "\".");
            return false;
        }

        // by default, if not an internal command, we consider the command
        // to be the content a message and therefore we just send it
        //
        f_connection->send_message(command);
        return false;
    }

private:
    connection::pointer_t                   f_connection;
};





class snapmessage
{
public:
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

        f_connection = std::make_shared<connection>();

        if(f_opt.is_defined("address"))
        {
            f_connection->set_address(f_opt.get_string("address"));
        }

        f_connection->set_mode(f_opt.is_defined("ssl")
                ? tcp_client_server::bio_client::mode_t::MODE_SECURE
                : tcp_client_server::bio_client::mode_t::MODE_PLAIN);

        f_connection->set_selected_connection_type(f_opt.is_defined("tcp") ? connection::connection_t::TCP : connection::connection_t::UDP);
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

        if(f_opt.is_defined("tcp")
        || f_opt.is_defined("udp"))
        {
            return f_connection->send_message(f_opt.get_string("message")) ? 0 : 1;
        }

        std::cerr << "error: no command specified, one of --gui, --cui, --tcp, --udp is required." << std::endl;

        return 1;
    }

    int start_gui()
    {
        std::cerr << "error: the --gui is not yet implemented." << std::endl;
        return 1;
    }

    int enter_cui()
    {
        // add a CUI connection to the snap_communicator
        //
        {
            cui_connection::pointer_t cui(std::make_shared<cui_connection>(f_connection));
            cui->reset_prompt();
            if(!snap::snap_communicator::instance()->add_connection(cui))
            {
                std::cerr << "error: could not add CUI snap_console to list of snap_communicator connections." << std::endl;
                return 1;
            }
        }

        // run until we are asked to exit
        //
        if(snap::snap_communicator::instance()->run())
        {
            return 0;
        }

        // run() returned with an error
        //
        std::cerr << "error: something went wrong in the snap_communicator run() loop." << std::endl;
        return 1;
    }

private:
    advgetopt::getopt                       f_opt;
    bool                                    f_gui = false;
    bool                                    f_cui = false;
    connection::pointer_t                   f_connection;
    //connection_handler::pointer_t           f_connection_handler;
};


int main(int argc, char *argv[])
{
    try
    {
        snapmessage sm(argc, argv);

        return sm.run();
    }
    catch(std::exception const & e)
    {
        // clean error on exception
        //
        std::cerr << "snapsignal: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
