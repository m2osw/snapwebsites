// Snap Websites Server -- command line to manage snapmanager.cgi users
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

// snapmanager lib
//
#include <snapmanager/version.h>

// snapwebsites lib
//
#include <snapwebsites/hexadecimal_string.h>
#include <snapwebsites/password.h>

// advgetopt
//
#include <advgetopt/advgetopt.h>

// C++ lib
//
#include <iostream>

// C lib
//
#include <fnmatch.h>
#include <unistd.h>

// last entry
//
#include <snapwebsites/poison.h>



namespace
{
    const std::vector<std::string> g_configuration_files;

    const advgetopt::getopt::option g_snappassword_options[] =
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
            'a',
            0,
            "add",
            nullptr,
            "add a user.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'c',
            0,
            "check",
            nullptr,
            "check a user's password.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'd',
            0,
            "delete",
            nullptr,
            "delete a user.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'f',
            0,
            "filename",
            "/etc/snapwebsites/passwords/snapmanagercgi.pwd",
            "password filename and path.",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "Show this help screen.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'l',
            0,
            "list",
            nullptr,
            "list existing users.",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            'u',
            0,
            "username",
            nullptr,
            "specify the name of user.",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            'p',
            0,
            "password",
            nullptr,
            "specify the password of user.",
            advgetopt::getopt::argument_mode_t::optional_argument
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
}
// no name namespace



class snappassword
{
public:
                        snappassword(int argc, char * argv[]);

    int                 run();

private:
    int                 list_all();
    int                 add_user();
    int                 delete_user();
    int                 check_password();

    advgetopt::getopt   f_opt;
};




snappassword::snappassword(int argc, char * argv[])
    : f_opt(argc, argv, g_snappassword_options, g_configuration_files, nullptr)
{
    if(f_opt.is_defined("version"))
    {
        std::cout << SNAPMANAGERCGI_VERSION_STRING << std::endl;
        exit(0);
    }
    if(f_opt.is_defined("help"))
    {
        f_opt.usage(advgetopt::getopt::status_t::no_error, "Usage: %s -<arg> ...\n", argv[0]);
        exit(1);
    }
}


int snappassword::run()
{
    if(f_opt.is_defined("list"))
    {
        return list_all();
    }

    if(f_opt.is_defined("add"))
    {
        return add_user();
    }

    if(f_opt.is_defined("delete"))
    {
        return delete_user();
    }

    if(f_opt.is_defined("check"))
    {
        return check_password();
    }

    std::cerr << "snappassword:error: no command specified, one of: --help, --version, --list, --add, --delete, or --check is required." << std::endl;

    return 1;
}


int snappassword::list_all()
{
    std::string const pattern(f_opt.get_string("list"));

    snap::password_file in(f_opt.get_string("filename"));
    for(;;)
    {
        // retrieve the next user and password details
        //
        snap::password p;
        std::string const username(in.next(p));
        if(username.empty())
        {
            return 0;
        }

        // check whether the username matches the pattern
        //
        bool match(pattern.empty());
        if(!match)
        {
            int const r(fnmatch(pattern.c_str(), username.c_str(), FNM_CASEFOLD | FNM_EXTMATCH));
            if(r == 0)
            {
                match = true;
            }
            else if(r != FNM_NOMATCH)
            {
                std::cerr << "snappassword:error: pattern matching against \"" << pattern << "\" failed." << std::endl;
                return 1;
            }
        }

        if(match)
        {
            std::cout << username
                      << ":"
                      << p.get_digest()
                      << ":"
                      << snap::bin_to_hex(p.get_salt())
                      << ":"
                      << snap::bin_to_hex(p.get_encrypted_password())
                      << std::endl;
        }
    }
}


int snappassword::add_user()
{
    snap::password_file f(f_opt.get_string("filename"));

    snap::password p;

    std::string const username(f_opt.get_string("username"));

    if(f_opt.is_defined("password"))
    {
        std::string const password(f_opt.get_string("password"));
        if(password.empty())
        {
            // user has to type a password in his TTY
            //
            if(!p.get_password_from_console())
            {
                return 1;
            }
        }
        else
        {
            // user specified password on command line
            //
            p.set_plain_password(password);
        }
    }
    else
    {
        // no password defined on the command line, auto-generate
        //
        p.generate_password();
    }

    f.save(username, p);

    return 0;
}


int snappassword::delete_user()
{
    snap::password_file f(f_opt.get_string("filename"));

    std::string const username(f_opt.get_string("username"));

    if(f.remove(username))
    {
        std::cout << "snappassword:info: user was removed successfully." << std::endl;
    }
    else
    {
        std::cout << "snappassword:error: user not found or invalid input." << std::endl;
    }

    return 0;
}


int snappassword::check_password()
{
    if(!f_opt.is_defined("password"))
    {
        std::cerr << "snappassword:error: --password must be specified with --check." << std::endl;
        return 1;
    }

    // at this point only the check command is allowed to switch to root:root
    //
    if(setuid(0) != 0)
    {
        perror("snappassword:setuid(0)");
        throw std::runtime_error("fatal error: could not setup executable to run as user root.");
    }
    if(setgid(0) != 0)
    {
        perror("snappassword:setgid(0)");
        throw std::runtime_error("fatal error: could not setup executable to run as group root.");
    }

    // initialize the password file
    //
    snap::password_file f(f_opt.get_string("filename"));

    // get the name of the user
    //
    std::string const username(f_opt.get_string("username"));

    // find the existing password information
    //
    snap::password ep;
    if(!f.find(username, ep))
    {
        std::cerr << "snappassword:error: --username \""
                  << username
                  << "\" not found in password file \""
                  << f_opt.get_string("filename")
                  << "\""
                  << std::endl;
        return 2;
    }

    // get the password to check against existing password
    //
    snap::password cp;

    std::string const password(f_opt.get_string("password"));
    if(password.empty())
    {
        // user has to type a password in his TTY
        //
        if(!cp.get_password_from_console(ep.get_salt()))
        {
            return 1;
        }
    }
    else
    {
        // user specified password on command line
        //
        cp.set_plain_password(password, ep.get_salt());
    }

    // that worked, now check whether passwords are equal
    //
    if(!(ep == cp))
    {
        // passwords differ
        //
        return 2;
    }

    return 0;
}




int main(int argc, char * argv[])
{
    try
    {
        snappassword sp(argc, argv);

        return sp.run();
    }
    catch(std::runtime_error const & e)
    {
        // this should rarely happen!
        std::cerr << "snappassword: caught a runtime exception: " << e.what() << std::endl;
    }
    catch(std::logic_error const & e)
    {
        // this should never happen!
        std::cerr << "snappassword: caught a logic exception: " << e.what() << std::endl;
    }
    catch(std::exception const & e)
    {
        // we are in trouble, we cannot even answer!
        std::cerr << "snappassword: standard exception: " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "snappassword: caught an unknown exception.";
    }

    return 1;
}


// vim: ts=4 sw=4 et
