// Snap Websites Server -- advanced handling of Unix processes
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

// ourselves
//
#include "snapwebsites/snap_thread.h"
#include "snapwebsites/snap_string_list.h"

// Qt lib
//
#include <QVector>

// C++ lib
//
#include <map>
#include <memory>


// forward declarations to avoid including the readproc.h file in the header
//#include <proc/readproc.h>
extern "C" typedef struct proc_t proc_t;
extern "C" typedef struct PROCTAB PROCTAB;

namespace snap
{

class snap_process_exception : public snap_exception
{
public:
    explicit snap_process_exception(const char *        whatmsg) : snap_exception("snap_process", whatmsg) {}
    explicit snap_process_exception(const std::string & whatmsg) : snap_exception("snap_process", whatmsg) {}
    explicit snap_process_exception(const QString &     whatmsg) : snap_exception("snap_process", whatmsg) {}
};

class snap_process_exception_invalid_mode_error : public snap_process_exception
{
public:
    explicit snap_process_exception_invalid_mode_error(const char *        whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_invalid_mode_error(const std::string & whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_invalid_mode_error(const QString &     whatmsg) : snap_process_exception(whatmsg) {}
};

class snap_process_exception_already_initialized : public snap_process_exception
{
public:
    explicit snap_process_exception_already_initialized(const char *        whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_already_initialized(const std::string & whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_already_initialized(const QString &     whatmsg) : snap_process_exception(whatmsg) {}
};

class snap_process_exception_unknown_flag : public snap_process_exception
{
public:
    explicit snap_process_exception_unknown_flag(const char *        whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_unknown_flag(const std::string & whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_unknown_flag(const QString &     whatmsg) : snap_process_exception(whatmsg) {}
};

class snap_process_exception_openproc : public snap_process_exception
{
public:
    explicit snap_process_exception_openproc(const char *        whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_openproc(const std::string & whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_openproc(const QString &     whatmsg) : snap_process_exception(whatmsg) {}
};

class snap_process_exception_data_not_available : public snap_process_exception
{
public:
    explicit snap_process_exception_data_not_available(const char *        whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_data_not_available(const std::string & whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_data_not_available(const QString &     whatmsg) : snap_process_exception(whatmsg) {}
};

class snap_process_exception_initialization_failed : public snap_process_exception
{
public:
    explicit snap_process_exception_initialization_failed(const char *        whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_initialization_failed(const std::string & whatmsg) : snap_process_exception(whatmsg) {}
    explicit snap_process_exception_initialization_failed(const QString &     whatmsg) : snap_process_exception(whatmsg) {}
};




class process
{
public:
    typedef std::map<std::string, std::string> environment_map_t;

    enum class mode_t
    {
        PROCESS_MODE_COMMAND,
        PROCESS_MODE_INPUT,
        PROCESS_MODE_OUTPUT,
        PROCESS_MODE_INOUT,
        PROCESS_MODE_INOUT_INTERACTIVE
    };

    class process_output_callback
    {
    public:
        virtual bool                output_available(process * p, QByteArray const & output) = 0;
    };

                                process(QString const & name);

    QString const &             get_name() const;

    // setup the process
    void                        set_mode(mode_t mode);
    void                        set_forced_environment(bool forced = true);

    void                        set_command(QString const & name);
    void                        add_argument(QString const & arg);
    void                        add_environ(QString const & name, QString const & value);

    int                         run();

    // what is sent to the command stdin
    void                        set_input(QString const & input);
    void                        set_input(QByteArray const & input);

    // what is received from the command stdout
    QString                     get_output(bool reset = false);
    QByteArray                  get_binary_output(bool reset = false);
    void                        set_output_callback(process_output_callback * callback);

private:
    // prevent copies
                                process(process const & rhs) = delete;
    process &                   operator = (process const & rhs) = delete;

    QString const               f_name;
    mode_t                      f_mode = mode_t::PROCESS_MODE_COMMAND;
    QString                     f_command;
    snap_string_list            f_arguments;
    environment_map_t           f_environment;
    QByteArray                  f_input;
    QByteArray                  f_output;
    bool                        f_forced_environment = false;
    process_output_callback *   f_output_callback = nullptr;
    snap_thread::snap_mutex     f_mutex;
};


class process_list
{
public:
    typedef int         flags_t;

    enum class field_t
    {
        // current status
        MEMORY,
        STATUS,
        STATISTICS,

        // info on startup
        COMMAND_LINE,
        ENVIRON,

        // user/group info
        USER_NAME,
        GROUP_NAME,
        CGROUP,
        SUPPLEMENTARY_GROUP,

        // other
        OOM,
        WAIT_CHANNEL,
        NAMESPACE
    };

    class proc_info
    {
    public:
        typedef std::shared_ptr<proc_info>      pointer_t;

        pid_t                       get_pid() const;
        pid_t                       get_ppid() const;
        void                        get_page_faults(unsigned long & major, unsigned long & minor) const;
        unsigned                    get_pcpu() const;
        char                        get_status() const;
        void                        get_times(unsigned long long & utime, unsigned long long & stime, unsigned long long & cutime, unsigned long long & cstime) const;
        long                        get_priority() const;
        long                        get_nice() const;
        long                        get_total_size() const;
        long                        get_resident_size() const;
        std::string                 get_process_name() const;
        int                         get_args_size() const;
        std::string                 get_arg(int index) const;
        int                         get_tty() const;

    private:
        friend class process_list;

                                    proc_info(std::shared_ptr<proc_t> p, flags_t flags);
                                    proc_info(proc_info const &) = delete;
        proc_info&                  operator = (proc_info const &) = delete;

        std::shared_ptr<proc_t>     f_proc;
        flags_t                     f_flags = 0;
        mutable int32_t             f_count = -1;
    };

    bool                        get_field(field_t fld) const;
    void                        set_field(field_t fld);
    void                        clear_field(field_t fld);

    void                        rewind();
    proc_info::pointer_t        next();

private:
    int                         field_to_flag(field_t fld) const;

    std::shared_ptr<PROCTAB>    f_proctab;
    flags_t                     f_flags = 0;
};


} // namespace snap
// vim: ts=4 sw=4 et
