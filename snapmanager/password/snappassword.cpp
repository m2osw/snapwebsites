// Snap Websites Server -- command line to manage snapmanager.cgi users
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
#include <snapwebsites/password.h>


// snapdev lib
//
#include <snapdev/hexadecimal_string.h>


// advgetopt lib
//
#include <advgetopt/advgetopt.h>
#include <advgetopt/exception.h>


// boost lib
//
#include <boost/preprocessor/stringize.hpp>


// C++ lib
//
#include <iostream>


// C lib
//
#include <fnmatch.h>
#include <unistd.h>


// last include
//
#include <snapdev/poison.h>



namespace
{


const advgetopt::option g_snappassword_options[] =
{
    {
        'a',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
        "add",
        nullptr,
        "add a user.",
        nullptr
    },
    {
        'c',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
        "check",
        nullptr,
        "check a user's password.",
        nullptr
    },
    {
        'd',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
        "delete",
        nullptr,
        "delete a user.",
        nullptr
    },
    {
        'f',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
        "filename",
        "/etc/snapwebsites/passwords/snapmanagercgi.pwd",
        "password filename and path.",
        nullptr
    },
    {
        'l',
        advgetopt::GETOPT_FLAG_COMMAND_LINE,
        "list",
        nullptr,
        "list existing users.",
        nullptr
    },
    {
        'u',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
        "username",
        nullptr,
        "specify the name of user.",
        nullptr
    },
    {
        'p',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
        "password",
        nullptr,
        "specify the password of user.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_END,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    }
};



// until we have C++20 remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_snappassword_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_options = g_snappassword_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>] ...\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPMANAGER_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2013-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop


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
    : f_opt(g_snappassword_options_environment, argc, argv)
{
    if(f_opt.is_defined("version"))
    {
        std::cout << SNAPMANAGER_VERSION_STRING << std::endl;
        exit(0);
    }
    if(f_opt.is_defined("help"))
    {
        std::cerr << f_opt.usage();
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
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
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
