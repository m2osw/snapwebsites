//
// File:        main.cpp
// Object:      Initialize and starts the snapmanager.cgi process.
//
// Copyright:   Copyright (c) 2016-2017 Made to Order Software Corp.
//              All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// ourselves
//
#include "snapmanagercgi.h"

// Qt lib
//
#include <QCoreApplication>

// last entry
//
#include <snapwebsites/poison.h>


namespace snap
{

// definitions from the plugins so we can define the name and filename of
// the server plugin
namespace plugins
{
extern QString g_next_register_name;
extern QString g_next_register_filename;
}

} // no name namespace


int main(int argc, char * argv[])
{
    // The Apache2 environment will pass parameters to us whenever the
    // end user enters a query string without an equal sign. For example:
    //
    //      http://www.example.com/cgi-bin/snapmanager.cgi?logout
    //
    // would add "logout" in argv[1]. That means hackers can pass any
    // parameter to us. (since `-` is a legal character in such query
    // string parameters.) So here we clear the list and force the count
    // to exactly 1 (i.e. we keep the program name only.)
    //
    argc = 1;
    argv[1] = nullptr;

    try
    {
        // Qt wants an application, we do not care too much ourselves,
        // we create it on the stack so it gets destroyed automatically
        //
        QCoreApplication app(argc, argv);

        // we need these globals to "properly" initializes the first
        // "plugin" (the core system or server)
        //
        snap::plugins::g_next_register_name = "server";
        snap::plugins::g_next_register_filename = "snapmanagercgi.cpp";

        snap_manager::manager_cgi::pointer_t cgi(new snap_manager::manager_cgi());

        snap::plugins::g_next_register_name.clear();
        snap::plugins::g_next_register_filename.clear();
        
        try
        {
            cgi->init(argc, argv);

            // mark that we started properly now that the logger is on
            //
            SNAP_LOG_INFO("--------------------------------- snapmanager.cgi v" SNAPMANAGERCGI_VERSION_STRING " started on ")(cgi->get_server_name());

            if(!cgi->verify())
            {
                // not acceptable, the verify() function already sent a
                // response, just exit with 1
                return 1;
            }
            return cgi->process();
        }
        catch(std::runtime_error const & e)
        {
            // this should rarely happen!
            return cgi->error("503 Service Unavailable", nullptr, ("The Snap! C++ CGI script caught a runtime exception: " + std::string(e.what()) + ".").c_str());
        }
        catch(std::logic_error const & e)
        {
            // this should never happen!
            return cgi->error("503 Service Unavailable", nullptr, ("The Snap! C++ CGI script caught a logic exception: " + std::string(e.what()) + ".").c_str());
        }
        catch(...)
        {
            return cgi->error("503 Service Unavailable", nullptr, "The Snap! C++ CGI script caught an unknown exception.");
        }
    }
    catch(std::exception const & e)
    {
        // we are in trouble, we cannot even answer!
        std::cerr << "snapmanager: initialization exception: " << e.what() << std::endl;
        return 1;
    }
}



// vim: ts=4 sw=4 et
