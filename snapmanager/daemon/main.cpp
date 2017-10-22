//
// File:        main.cpp
// Object:      Allow for applying functions on any computer.
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

#include "snapmanagerdaemon.h"


namespace snap
{

// definitions from the plugins so we can define the name and filename of
// the server plugin
namespace plugins
{

extern QString g_next_register_name;
extern QString g_next_register_filename;

} // plugin namespace

} // snap namespace


int main(int argc, char * argv[])
{
    try
    {
        // we need these globals to "properly" initializes the first
        // "plugin" (the core system or server)
        //
        snap::plugins::g_next_register_name = "server";
        snap::plugins::g_next_register_filename = "snapmanagercgi.cpp";

        snap_manager::manager_daemon::pointer_t daemon(new snap_manager::manager_daemon);

        snap::plugins::g_next_register_name.clear();
        snap::plugins::g_next_register_filename.clear();

        daemon->init(argc, argv);

        return daemon->run();
    }
    catch(snap::snap_exception const & e)
    {
        SNAP_LOG_ERROR("snap_exception in snapmanagerdaemon: ")(e.what());
    }
    catch(libdbproxy::exception const & e)
    {
        SNAP_LOG_ERROR("exception in snapmanagerdaemon: ")(e.what());
    }
    catch(std::runtime_error const & e)
    {
        // this should rarely happen!
        SNAP_LOG_ERROR("snapmanagerdaemon: caught a runtime exception: ")(e.what());
    }
    catch(std::logic_error const & e)
    {
        // this should never happen!
        SNAP_LOG_ERROR("snapmanagerdaemon: caught a logic exception: ")(e.what());
    }
    catch(std::exception const & e)
    {
        // we are in trouble, we cannot even answer!
        SNAP_LOG_ERROR("snapmanagerdaemon: standard exception: ")(e.what());
    }
    catch(...)
    {
        SNAP_LOG_ERROR("snapmanagerdaemon: caught an unknown exception!");
    }

    return 1;
}

// vim: ts=4 sw=4 et
