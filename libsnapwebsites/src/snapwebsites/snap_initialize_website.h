// Snap Websites Server -- snap websites server
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

#pragma once

#include <snapwebsites/snap_thread.h>

#include <libdbproxy/libdbproxy.h>

namespace snap
{


class snap_initialize_website_exception : public snap_exception
{
public:
    explicit snap_initialize_website_exception(char const *        whatmsg) : snap_exception("snap_initialize_website", whatmsg) {}
    explicit snap_initialize_website_exception(std::string const & whatmsg) : snap_exception("snap_initialize_website", whatmsg) {}
    explicit snap_initialize_website_exception(QString const &     whatmsg) : snap_exception("snap_initialize_website", whatmsg) {}
};

class snap_initialize_website_exception_invalid_parameter : public snap_initialize_website_exception
{
public:
    explicit snap_initialize_website_exception_invalid_parameter(char const *        whatmsg) : snap_initialize_website_exception(whatmsg) {}
    explicit snap_initialize_website_exception_invalid_parameter(std::string const & whatmsg) : snap_initialize_website_exception(whatmsg) {}
    explicit snap_initialize_website_exception_invalid_parameter(QString const &     whatmsg) : snap_initialize_website_exception(whatmsg) {}
};





class snap_initialize_website
{
public:
    typedef std::shared_ptr<snap_initialize_website>   pointer_t;

                    snap_initialize_website(QString const & snap_host, int snap_port, bool secure,
                                            QString const & website_uri, int destination_port,
                                            QString const & query_string, QString const & protocol);

    bool            start_process();
    bool            is_done() const;
    QString         get_status();

private:
    class snap_initialize_website_runner : public snap_thread::snap_runner
    {
    public:
                        snap_initialize_website_runner(snap_initialize_website * parent,
                                                       QString const & snap_host, int snap_port, bool secure,
                                                       QString const & website_uri, int destination_port,
                                                       QString const & query_string, QString const & protocol);

        // from class snap_thread
        virtual void    run();

        bool            is_done() const;
        QString         next_message();

    private:
        void            done();
        void            message(QString const & msg);
        void            send_init_command();

        snap_initialize_website *           f_parent = nullptr;
        mutable snap_thread::snap_mutex     f_mutex;
        bool                                f_done = false;
        QString const                       f_snap_host;
        int32_t                             f_snap_port = 0;
        bool                                f_secure = false;
        QString const                       f_website_uri;      // 'const' so it is mandatory anyway
        int32_t const                       f_destination_port; // 'const' so it is mandatory anyway
        QString const                       f_query_string;     // 'const' so it is mandatory anyway
        QString const                       f_protocol;         // 'const' so it is mandatory anyway
        std::deque<QString>                 f_message_queue;    // TODO: look into reusing the message queue from the thread with T = QString!?
    };

    std::unique_ptr<snap_initialize_website_runner>     f_website_runner;
    std::unique_ptr<snap_thread>                        f_process_thread;
};


}
// namespace snap
// vim: ts=4 sw=4 et
