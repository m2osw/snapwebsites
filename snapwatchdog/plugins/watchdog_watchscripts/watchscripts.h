// Snap Websites Server -- watchdog scripts
// Copyright (c) 2018-2021  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
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

// self
//
#include "snapwatchdog/snapwatchdog.h"

// snapwebsites lib
//
#include <snapwebsites/plugins.h>
#include <snapwebsites/process.h>
#include <snapwebsites/snap_child.h>

// Qt lib
//
#include <QDomDocument>
#include <QFile>


namespace snap
{
namespace watchscripts
{


enum class name_t
{
    SNAP_NAME_WATCHDOG_WATCHSCRIPTS_DEFAULT_LOG_SUBFOLDER,
    SNAP_NAME_WATCHDOG_WATCHSCRIPTS_LOG_SUBFOLDER,
    SNAP_NAME_WATCHDOG_WATCHSCRIPTS_OUTPUT,
    SNAP_NAME_WATCHDOG_WATCHSCRIPTS_OUTPUT_DEFAULT,
    SNAP_NAME_WATCHDOG_WATCHSCRIPTS_PATH,
    SNAP_NAME_WATCHDOG_WATCHSCRIPTS_PATH_DEFAULT,
    SNAP_NAME_WATCHDOG_WATCHSCRIPTS_WATCH_SCRIPT_STARTER,
    SNAP_NAME_WATCHDOG_WATCHSCRIPTS_WATCH_SCRIPT_STARTER_DEFAULT
};
char const * get_name(name_t name) __attribute__ ((const));



//class watchscripts_exception : public snap_exception
//{
//public:
//    watchscripts_exception(char const *        what_msg) : snap_exception("watchscripts_msg) {}
//    watchscripts_exception(std::string const & what_msg) : snap_exception("watchscripts_msg) {}
//    watchscripts_exception(QString const &     what_msg) : snap_exception("watchscripts_msg) {}
//};
//
//class watchscripts_exception_invalid_argument : public watchscripts_exception
//{
//public:
//    watchscripts_exception_invalid_argument(char const *        what_msg) : watchscripts_exception(what_msg) {}
//    watchscripts_exception_invalid_argument(std::string const & what_msg) : watchscripts_exception(what_msg) {}
//    watchscripts_exception_invalid_argument(QString const &     what_msg) : watchscripts_exception(what_msg) {}
//};





class watchscripts
    : public plugins::plugin
    , private process::process_output_callback
{
public:
                            watchscripts();
                            watchscripts(watchscripts const & rhs) = delete;
    virtual                 ~watchscripts() override;

    watchscripts &          operator = (watchscripts const & rhs) = delete;

    static watchscripts *   instance();

    // plugins::plugin implementation
    virtual QString         description() const override;
    virtual QString         dependencies() const override;
    virtual int64_t         do_update(int64_t last_updated) override;
    virtual void            bootstrap(snap_child * snap) override;

    // server signals
    void                    on_process_watch(QDomDocument doc);

private:
    void                    process_script(QString script_filename);
    static QString          format_date(time_t const t);

    // process::process_output_callback implementation
    //
    virtual bool            output_available(process * p, QByteArray const & output) override;
    virtual bool            error_available(process * p, QByteArray const & error) override;

    QString                 generate_header(QString const & type);

    watchdog_child *        f_snap = nullptr;
    QDomElement             f_watchdog = QDomElement();
    bool                    f_new_output_script = false;
    bool                    f_new_error_script = false;
    char                    f_last_output_byte = '\n';
    char                    f_last_error_byte = '\n';
    QString                 f_watch_script_starter = QString();
    QString                 f_log_path = QString();
    QString                 f_log_subfolder = QString();
    QString                 f_scripts_output_log = QString();
    QString                 f_scripts_error_log = QString();
    QString                 f_script_filename = QString();
    std::shared_ptr<QFile>  f_output_file = std::shared_ptr<QFile>();
    std::shared_ptr<QFile>  f_error_file = std::shared_ptr<QFile>();
    time_t                  f_start_date = time_t();
    QString                 f_output = QString();
    QString                 f_error = QString();
    QString                 f_email = QString();
};


} // namespace watchscripts
} // namespace snap
// vim: ts=4 sw=4 et
