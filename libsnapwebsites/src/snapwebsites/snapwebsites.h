// Snap Websites Server -- snap websites server
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Main header file of the libsnapwebsites library.
 *
 * This header file defines the server class which is the main
 * class in the snapserver daemon. It executes the run() loop.
 *
 * See the .cpp file for more details.
 */

// self
//
#include    "snapwebsites/http_strings.h"
#include    "snapwebsites/snap_child.h"
#include    "snapwebsites/snap_expr.h"
#include    "snapwebsites/snap_pid.h"
#include    "snapwebsites/version.h"


// cppthread
//
#include    <cppthread/plugins.h>


// advgetopt
//
#include    <advgetopt/conf_file.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>
#include    <eventdispatcher/udp_server.h>


// advgetopt
//
#include    <advgetopt/advgetopt.h>


// Qt
//
#include    <QTranslator>




namespace snap
{

enum class name_t
{
    // low level names
    SNAP_NAME_SERVER,           // The name of the Snap! Server
    SNAP_NAME_CONTEXT,          // Cassandra Keyspace
    SNAP_NAME_INDEX,            // Row used for the domains & websites index
    SNAP_NAME_DOMAINS,          // Cassandra Table used for domains
    SNAP_NAME_WEBSITES,         // Cassandra Table used for websites
    SNAP_NAME_SITES,            // Cassandra Table used for sites (one site per row)
    SNAP_NAME_BACKEND,          // Cassandra Table used to know where we are with backends
    SNAP_NAME_MX,               // Cassandra Table used to cache which domains have an MX record and which do not

    // names used by core (server & snap_child)
    SNAP_NAME_CORE_ADMINISTRATOR_EMAIL,
    SNAP_NAME_CORE_CANONICAL_DOMAIN,
    SNAP_NAME_CORE_CONTENT_DISPOSITION,
    SNAP_NAME_CORE_CONTENT_LANGUAGE,
    SNAP_NAME_CORE_CONTENT_TYPE_HEADER,
    SNAP_NAME_CORE_COOKIE_DOMAIN,
    SNAP_NAME_CORE_DATA_PATH,
    SNAP_NAME_CORE_DATE,
    SNAP_NAME_CORE_EMAIL_CONTENT_ENCODING_QUOTED_PRINTABLE,
    SNAP_NAME_CORE_EMAIL_CONTENT_TRANSFER_ENCODING,
    SNAP_NAME_CORE_EMAIL_FROM,
    SNAP_NAME_CORE_EMAIL_IMPORTANCE,
    SNAP_NAME_CORE_EMAIL_LIST_UNSUBSCRIBE,
    SNAP_NAME_CORE_EMAIL_MESSAGE_ID,
    SNAP_NAME_CORE_EMAIL_MIME_VERSION,
    SNAP_NAME_CORE_EMAIL_PRECEDENCE,
    SNAP_NAME_CORE_EMAIL_PRIORITY_BULK,
    SNAP_NAME_CORE_EMAIL_PRIORITY_HIGH,
    SNAP_NAME_CORE_EMAIL_PRIORITY_LOW,
    SNAP_NAME_CORE_EMAIL_PRIORITY_NORMAL,
    SNAP_NAME_CORE_EMAIL_PRIORITY_URGENT,
    SNAP_NAME_CORE_EMAIL_REPLY_TO,
    SNAP_NAME_CORE_EMAIL_SUBJECT,
    SNAP_NAME_CORE_EMAIL_TO,
    SNAP_NAME_CORE_EMAIL_X_PRIORITY,
    SNAP_NAME_CORE_EMAIL_X_MSMAIL_PRIORITY,
    SNAP_NAME_CORE_HTTP_ACCEPT_LANGUAGE,
    SNAP_NAME_CORE_HTTP_LINK_HEADER,
    SNAP_NAME_CORE_HTTP_USER_AGENT,
    SNAP_NAME_CORE_LAST_DYNAMIC_UPDATE,
    SNAP_NAME_CORE_LAST_UPDATED,
    SNAP_NAME_CORE_LIST_DATA_PATH,
    SNAP_NAME_CORE_LIST_DB_PATH,
    SNAP_NAME_CORE_LIST_JOURNAL_PATH,
    SNAP_NAME_CORE_LOCATION_HEADER,
    SNAP_NAME_CORE_MX_LAST_CHECKED,
    SNAP_NAME_CORE_MX_RESULT,
    SNAP_NAME_CORE_ORIGINAL_RULES,
    SNAP_NAME_CORE_PARAM_DEFAULT_PLUGINS,
    SNAP_NAME_CORE_PARAM_PLUGINS,
    SNAP_NAME_CORE_PARAM_PLUGINS_PATH,
    SNAP_NAME_CORE_PARAM_TABLE_SCHEMA_PATH,
    SNAP_NAME_CORE_PLUGINS,
    SNAP_NAME_CORE_PLUGIN_THRESHOLD,
    SNAP_NAME_CORE_REDIRECT,
    SNAP_NAME_CORE_REMOTE_ADDR,
    SNAP_NAME_CORE_REQUEST_METHOD,
    SNAP_NAME_CORE_REQUEST_URI,
    SNAP_NAME_CORE_RETRY_AFTER_HEADER,
    SNAP_NAME_CORE_RULES,
    SNAP_NAME_CORE_SERVER_PROTOCOL,
    SNAP_NAME_CORE_SITE_LONG_NAME,
    SNAP_NAME_CORE_SITE_NAME,
    SNAP_NAME_CORE_SITE_READY,
    SNAP_NAME_CORE_SITE_SECURE,
    SNAP_NAME_CORE_SITE_SHORT_NAME,
    SNAP_NAME_CORE_SITE_STATE,
    SNAP_NAME_CORE_SNAPBACKEND,
    SNAP_NAME_CORE_STATUS_HEADER,
    SNAP_NAME_CORE_TEST_SITE,
    SNAP_NAME_CORE_USER_COOKIE_NAME,
    SNAP_NAME_CORE_X_POWERED_BY_HEADER
};
char const * get_name(name_t name) __attribute__ ((const));



DECLARE_MAIN_EXCEPTION(snapwebsites_exception);

DECLARE_EXCEPTION(snapwebsites_exception, snapwebsites_exception_invalid_parameters);
DECLARE_EXCEPTION(snapwebsites_exception, snapwebsites_exception_parameter_not_available);
DECLARE_EXCEPTION(snapwebsites_exception, snapwebsites_exception_io_error);





class permission_error_callback
{
public:
    class error_by_mime_type
    {
    public:
        virtual                 ~error_by_mime_type() {}

        virtual void            on_handle_error_by_mime_type(snap_child::http_code_t err_code, QString const & err_name, QString const & err_description, QString const & path) = 0;
    };

    virtual                 ~permission_error_callback() {}

    virtual void            on_error(snap_child::http_code_t const err_code, QString const & err_name, QString const & err_description, QString const & err_details, bool const err_by_mime_type) = 0;
    virtual void            on_redirect(QString const & err_name, QString const & err_description, QString const & err_details, bool err_security, QString const & path, snap_child::http_code_t const http_code) = 0;
};

// a simple specialization of the permission_error_callback that quiet
// the errors so they don't get in the way (quiet as in: the end users don't
// see them; it's going to be logged anyway)
class quiet_error_callback
    : public permission_error_callback
{
public:
                            quiet_error_callback(snap_child * snap, bool log);
                            quiet_error_callback(quiet_error_callback const & rhs) = delete;

    quiet_error_callback &  operator = (quiet_error_callback const & rhs) = delete;

    virtual void            on_error(snap_child::http_code_t const err_code, QString const & err_name, QString const& err_description, QString const & err_details, bool const err_by_mime_type) override;
    virtual void            on_redirect(QString const & err_name, QString const & err_description, QString const & err_details, bool err_security, QString const & path, snap_child::http_code_t const http_code) override;

    void                    clear_error();
    bool                    has_error() const;

private:
    snap_child *            f_snap = nullptr;
    bool                    f_log = false;
    bool                    f_error = false;
};


// implementation specific class
class listener_impl;


class server
    : public cppthread::plugin
{
public:
    typedef std::shared_ptr<server>             pointer_t;

    // TODO: remove once snapcommunicator is used
    typedef QSharedPointer<ed::udp_server>      udp_server_t;

    typedef uint32_t                            config_flags_t;

    static config_flags_t const                 SNAP_SERVER_CONFIG_OPTIONAL_SERVER_NAME = 0x01;

    class backend_action
    {
    public:
        virtual                 ~backend_action() {}
        virtual void            on_backend_action(QString const & action) = 0;
    };

    class backend_action_set
    {
    public:
        void                    add_action(QString const & action, cppthread::plugin * p);
        bool                    has_action(QString const & action) const;
        void                    execute_action(QString const & action);
        QString                 get_plugin_name(QString const & action);
        void                    display();

    private:
        typedef QMap<QString, backend_action *> actions_map_t;

        actions_map_t           f_actions = actions_map_t();
    };

    class accessible_flag_t
    {
    public:
                        accessible_flag_t()
                            //: f_accessible(false) -- auto-init
                            //, f_secure(false) -- auto-init
                        {
                        }

        bool            is_accessible() const { return f_accessible && !f_secure; }
        void            mark_as_accessible() { f_accessible = true; }
        void            mark_as_secure() { f_secure = true; }

    private:
        // prevent copies or a user could reset the flag!
                        accessible_flag_t(accessible_flag_t const & rhs) = delete;
                        accessible_flag_t & operator = (accessible_flag_t const & rhs) = delete;

        bool            f_accessible = false;
        bool            f_secure = false;
    };

    static pointer_t    instance();
    virtual             ~server();

    [[noreturn]] static void exit(int const code);

    static char const * version();
    static int          version_major();
    static int          version_minor();
    static int          version_patch();
    virtual void        show_version();

    static std::string  get_server_name();
    static void         verify_server_name(std::string & server_name);

    // cppthread::plugin implementation
    virtual void        bootstrap(void * snap) override;
    virtual int64_t     do_update(int64_t last_updated);

    [[noreturn]] void   usage();
    void                setup_as_backend();
    bool                is_debug() const { return f_debug; }
    bool                is_foreground() const { return f_foreground; }
    bool                is_backend() const { return f_backend; }
    static size_t       thread_count();
    void                set_config_filename(std::string const & filename);
    void                config( int argc, char * argv[] );
    void                set_translation(QString const xml_data);
    QString             get_parameter(QString const & param_name) const;
    void                set_parameter( QString const & param_name, QString const & value );
    void                prepare_qtapp( int argc, char * argv[] );
    bool                check_cassandra(QString const & mandatory_table, bool & timer_required);

    void                create_messenger_instance( bool const use_thread = false );

    void                detach();
    void                server_loop_ready();
    void                listen();
    void                backend();
    int                 snapdbproxy_port() const { return f_snapdbproxy_port; }
    QString const &     snapdbproxy_addr() const { return f_snapdbproxy_addr; }
    void                capture_zombies(pid_t child_pid);
    void                process_message(ed::message const & message);

    unsigned long       connections_count();

    std::string         servername() const;
    void                set_service_name(std::string const & service_name);
    std::string const & get_service_name() const;

    void                configure_messenger_logging(ed::tcp_client_permanent_message_connection::pointer_t ptr);

    void                udp_ping_server( QString const & service, QString const & uri );
    void                udp_rusage(QString const & process_name);
    static void         block_ip( QString const & uri, QString const & period = QString(), QString const & reason = QString() );

    advgetopt::conf_file::pointer_t
                        get_parameters() const;

#ifdef SNAP_NO_FORK
    bool nofork() const;
#endif

    SNAP_SIGNAL_WITH_MODE(init, (), (), NEITHER);
    SNAP_SIGNAL_WITH_MODE(update, (int64_t last_updated), (last_updated), NEITHER);
    SNAP_SIGNAL_WITH_MODE(process_cookies, (), (), NEITHER);
    SNAP_SIGNAL_WITH_MODE(attach_to_session, (), (), NEITHER);
    SNAP_SIGNAL_WITH_MODE(detach_from_session, (), (), NEITHER);
    SNAP_SIGNAL_WITH_MODE(user_status, (snap_child::user_status_t status, snap_child::user_identifier_t id), (status, id), NEITHER);
    SNAP_SIGNAL_WITH_MODE(define_locales, (http_strings::WeightedHttpString & locales), (locales), NEITHER);
    SNAP_SIGNAL_WITH_MODE(process_post, (QString const & url), (url), NEITHER);
    SNAP_SIGNAL_WITH_MODE(execute, (QString const & url), (url), NEITHER);
    SNAP_SIGNAL_WITH_MODE(register_backend_cron, (backend_action_set & actions), (actions), NEITHER);
    SNAP_SIGNAL_WITH_MODE(register_backend_action, (backend_action_set & actions), (actions), NEITHER);
    SNAP_SIGNAL_WITH_MODE(backend_process, (), (), NEITHER);
    SNAP_SIGNAL_WITH_MODE(save_content, (), (), NEITHER);
    SNAP_SIGNAL_WITH_MODE(xss_filter, (QDomNode & node,
                             QString const & accepted_tags,
                             QString const & accepted_attributes),
                            (node, accepted_tags, accepted_attributes), NEITHER);
    SNAP_SIGNAL_WITH_MODE(improve_signature, (QString const & path, QDomDocument doc, QDomElement & signature_tag), (path, doc, signature_tag), NEITHER);
    SNAP_SIGNAL(load_file, (snap_child::post_file_t & file, bool & found), (file, found));
    SNAP_SIGNAL_WITH_MODE(table_is_accessible, (QString const & table, accessible_flag_t & accessible), (table, accessible), NEITHER);
    SNAP_SIGNAL_WITH_MODE(add_snap_expr_functions, (snap_expr::functions_t & functions), (functions), NEITHER);
    SNAP_SIGNAL_WITH_MODE(output_result, (QString const & uri_path, QByteArray & output), (uri_path, output), NEITHER);

protected:
                                server();
    static server::pointer_t    get_instance();
    static server::pointer_t    set_instance(pointer_t server);

    // See TODO in server::prepare_cassandra()
    QString                     f_snapdbproxy_addr = QString();     // NO DEFAULT, if isEmpty() then we are not connected / cannot connect to snapdbproxy
    int32_t                     f_snapdbproxy_port = 0;
    bool                        f_snaplock = false;
    advgetopt::conf_file::pointer_t
                                f_parameters = advgetopt::conf_file::pointer_t();

private:
    typedef std::shared_ptr<advgetopt::getopt>    getopt_ptr_t;

    friend class server_interrupt;
    friend class listener_impl;

    static void                 sighandler( int sig );
    static void                 sigloghandler( int sig );

    void                        process_connection(ed::tcp_bio_client::pointer_t client);
    void                        stop_thread_func();
    void                        stop(bool quitting);

    static pointer_t            g_instance;

    QTranslator                 f_translator;
    QByteArray                  f_translation_xml = QByteArray();

    std::string                 f_servername = std::string();
    std::string                 f_service_name = std::string();
    snap_pid::pointer_t         f_pid_file = snap_pid::pointer_t();
    bool                        f_debug = false;
    bool                        f_foreground = false;
    bool                        f_backend = false;
    bool                        f_force_restart = false;
    bool                        f_firewall_is_active = false;
    bool                        f_firewall_up = false;
    //QMap<QString, bool>         f_created_table = QMap<QString, bool>();

    uint64_t                    f_connections_count = 0;
    snap_child_vector_t         f_children_running = snap_child_vector_t();
    snap_child_vector_t         f_children_waiting = snap_child_vector_t();

    getopt_ptr_t                f_opt = getopt_ptr_t();

#ifdef SNAP_NO_FORK
    bool                        f_nofork = false;
#endif
};

} // namespace snap
// vim: ts=4 sw=4 et
