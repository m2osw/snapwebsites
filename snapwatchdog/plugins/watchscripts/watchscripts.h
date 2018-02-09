// Snap Websites Server -- watchdog scripts
// Copyright (c) 2018  Made to Order Software Corp.  All Rights Reserved
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
    SNAP_NAME_WATCHDOG_WATCHSCRIPTS_PATH_DEFAULT
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
        , public process::process_output_callback
{
public:
                            watchscripts();
                            ~watchscripts();

    // plugins::plugin implementation
    static watchscripts *   instance();
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // server signals
    void                    on_process_watch(QDomDocument doc);

private:
    void                    process_script(QString script_filename);
    virtual bool            output_available(process * p, QByteArray const & output);
    static QString          format_date(time_t const t);

    snap_child *            f_snap = nullptr;
    bool                    f_new_script = false;
    char                    f_last_output_byte = '\n';
    QString                 f_log_path;
    QString                 f_log_subfolder;
    QString                 f_scripts_log;
    QString                 f_script_filename;
    std::shared_ptr<QFile>  f_file;
    time_t                  f_start_date;
    QString                 f_output;
    QString                 f_email;
};


} // namespace watchscripts
} // namespace snap
// vim: ts=4 sw=4 et
