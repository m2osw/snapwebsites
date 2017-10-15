// Snap Websites Server -- log services
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

#include "snapwebsites/log.h"

#include "snapwebsites/not_reached.h"
#include "snapwebsites/not_used.h"
#include "snapwebsites/snap_exception.h"
#include "snapwebsites/snapwebsites.h"

#include <syslog.h>

#include <boost/algorithm/string/replace.hpp>

// log4cplus wants to be compatible with old compilers and thus uses
// std::auto_ptr<>() which throws an error in our code
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/logger.h>
#include <log4cplus/socketappender.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/syslogappender.h>
#pragma GCC diagnostic pop

#include <QFileInfo>

#include "snapwebsites/poison.h"


/** \file
 * \brief Handle logging in the Snap environment.
 *
 * The snap::logging namespace defines a set of functions and classes used
 * to setup the snap logger that can be easily accessed with the following
 * macros:
 *
 * \li SNAP_LOG_FATAL -- output what is viewed as fatal error
 * \li SNAP_LOG_ERROR -- output an error
 * \li SNAP_LOG_WARNING -- output a warning
 * \li SNAP_LOG_INFO -- output some information
 * \li SNAP_LOG_DEBUG -- output debug information
 * \li SNAP_LOG_TRACE -- output trace information
 *
 * The macros should be used so that way you include the filename and line
 * number of where the message is generated from. That information is then
 * available to be printed in the logs.
 *
 * The macros define a logger object that accepts messages with either the
 * << operator or the () operator, both of which support null terminated
 * C strings (char and wchar_t), QString, std::string, std::wstring,
 * and all basic types (integers and floats).
 *
 * The () operator also accepts the security enumeration as input, so you
 * can change the level to SECURE at any time when you generate a log.
 *
 * \code
 *      SNAP_LOG_INFO("User password is: ")
 *              (snap::logging::log_security_t::LOG_SECURITY_SECURE)
 *              (password);
 *
 *      SNAP_LOG_FATAL("We could not read resources: ") << filename;
 * \endcode
 *
 * Try to remember that the \\n character is not necessary. The logger
 * will automatically add a newline at the end of each log message.
 *
 * \note
 * The newer versions of the log4cplus library offer a very similar set
 * of macros. These macro, though, do not properly check out all of
 * our flags and levels so you should avoid them for now.
 *
 * To setup the logging system, the snapserver makes use of the following
 * files:
 *
 * \li log.properties
 * \li snapcgi.properties
 *
 * \code
 *      log_config=/etc/snapwebsites/logger/log.properties
 * \endcode
 *
 * The backends run just like the snapserver so they get the same logger
 * settings.
 *
 * The snap.cgi tool, however, has its own setup. It first checks the
 * command line, and if no configuration is defined on the command
 * line it uses the log_config=... parameter from the snapcgi.conf
 * file. The default file is snapcgi.properties.
 *
 * \code
 *      log_config=/etc/snapwebsites/logger/snapcgi.properties
 * \endcode
 *
 * \sa log4cplus/include/log4cplus/loggingmacros.h
 */

namespace snap
{

namespace logging
{

namespace
{

std::string         g_progname;
QString             g_log_config_filename;
QString             g_log_output_filename;
messenger_t         g_log_messenger;
log4cplus::Logger   g_logger;
log4cplus::Logger   g_secure_logger;
log4cplus::Logger   g_messenger_logger;
bool                g_messenger_logger_initialized = false;

enum class logging_type_t
    { UNCONFIGURED_LOGGER
    , CONSOLE_LOGGER
    , FILE_LOGGER
    , CONFFILE_LOGGER
    , SYSLOG_LOGGER
    , MESSENGER_LOGGER
    };

logging_type_t      g_logging_type( logging_type_t::UNCONFIGURED_LOGGER );
logging_type_t      g_last_logging_type( logging_type_t::UNCONFIGURED_LOGGER );



class logger_stub
        : public logger
{
public:
                    logger_stub(log_level_t const log_level, char const * file, char const * func, int const line)
                        : logger(log_level, file, func, line)
                    {
                        f_ignore = true;
                    }

                    logger_stub(logger const & l)
                        : logger(l)
                    {
                    }

    logger &        operator () ()                              { return *this; }
    logger &        operator () (log_security_t const v)        { NOTUSED(v); return *this; }
    logger &        operator () (char const * s)                { NOTUSED(s); return *this; }
    logger &        operator () (wchar_t const * s)             { NOTUSED(s); return *this; }
    logger &        operator () (std::string const & s)         { NOTUSED(s); return *this; }
    logger &        operator () (std::wstring const & s)        { NOTUSED(s); return *this; }
    logger &        operator () (QString const & s)             { NOTUSED(s); return *this; }
    logger &        operator () (snap::snap_config::snap_config_parameter_ref const & s) { NOTUSED(s); return *this; }
    logger &        operator () (char const v)                  { NOTUSED(v); return *this; }
    logger &        operator () (signed char const v)           { NOTUSED(v); return *this; }
    logger &        operator () (unsigned char const v)         { NOTUSED(v); return *this; }
    logger &        operator () (signed short const v)          { NOTUSED(v); return *this; }
    logger &        operator () (unsigned short const v)        { NOTUSED(v); return *this; }
    logger &        operator () (signed int const v)            { NOTUSED(v); return *this; }
    logger &        operator () (unsigned int const v)          { NOTUSED(v); return *this; }
    logger &        operator () (signed long const v)           { NOTUSED(v); return *this; }
    logger &        operator () (unsigned long const v)         { NOTUSED(v); return *this; }
    logger &        operator () (signed long long const v)      { NOTUSED(v); return *this; }
    logger &        operator () (unsigned long long const v)    { NOTUSED(v); return *this; }
    logger &        operator () (float const v)                 { NOTUSED(v); return *this; }
    logger &        operator () (double const v)                { NOTUSED(v); return *this; }
    logger &        operator () (bool const v)                  { NOTUSED(v); return *this; }
};


class MessengerAppender
        : public log4cplus::Appender
{
public:
    MessengerAppender()
    {
    }

    MessengerAppender(log4cplus::helpers::Properties const & props)
        : Appender(props)
    {
    }

    virtual ~MessengerAppender() override
    {
        destructorImpl();
    }

    virtual void close() override
    {
    }

    static void registerAppender()
    {
        // The registration must run just once and static variables
        // are initialized just once
        //
        static bool const g_registered = []()
        {
            log4cplus::spi::AppenderFactoryRegistry & reg(log4cplus::spi::getAppenderFactoryRegistry());

            // there are macros to do the following, but it uses the log4cplus
            // namespace which is not terribly useful in our case
            //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            reg.put(std::auto_ptr<log4cplus::spi::AppenderFactory>(
                    new log4cplus::spi::FactoryTempl<snap::logging::MessengerAppender, log4cplus::spi::AppenderFactory>(
                                LOG4CPLUS_TEXT("snap::logging::")
                                LOG4CPLUS_TEXT("MessengerAppender"))));
#pragma GCC diagnostic pop

            return false;
        }();

        NOTUSED(g_registered);
    }

protected:
    virtual void append(const log4cplus::spi::InternalLoggingEvent& event) override
    {
        auto messenger( g_log_messenger.lock() );
        if( messenger ) // silently fail if the shared object has been deleted...
        {
            char const * level_str("unknown");
            switch( event.getLogLevel() )
            {
                case log4cplus::FATAL_LOG_LEVEL:
                    level_str = "fatal error";
                    break;

                case log4cplus::ERROR_LOG_LEVEL:
                    level_str = "error";
                    break;

                case log4cplus::WARN_LOG_LEVEL:
                    level_str = "warning";
                    break;

                case log4cplus::INFO_LOG_LEVEL:
                    level_str = "info";
                    break;

                case log4cplus::DEBUG_LOG_LEVEL:
                    level_str = "debug";
                    break;

                case log4cplus::TRACE_LOG_LEVEL:
                    level_str = "trace";
                    break;
            }

            // Send the log to snapcommunicator, and eventually to snaplog.
            //
            snap::snap_communicator_message request;
            request.set_command("SNAPLOG");
            request.set_service("snaplog");
            request.add_parameter( "cache",   "ttl=60"                                       );
            request.add_parameter( "level",   level_str                                      );
            request.add_parameter( "file",    QString::fromUtf8(event.getFile().c_str())     );
            request.add_parameter( "func",    QString::fromUtf8(event.getFunction().c_str()) );
            request.add_parameter( "line",    event.getLine()                                );
            request.add_parameter( "message", QString::fromUtf8(event.getMessage().c_str())  );

            messenger->send_message(request);
        }
    }

private:
};


/** \brief Check whether the logger exists.
 *
 * Although it should never happens, the alloc_dc() function throws
 * a logic_error exception if called when it is already initialized.
 * For this reason we have this function which makes sure that the
 * exception does not occur (in part because it is called from the
 * logger destructor)
 *
 * \param[in] name  The name of the logger to retrieve.
 *
 * \return true if the logger exists, false otherwise.
 */
bool logger_exists(char const * name)
{
    try
    {
        return log4cplus::Logger::exists(name);
    }
    catch(std::logic_error const & )
    {
        // no logging for this error, we are in the logger and it failed!
        return false;
    }
}


}
// no name namespace


/** \brief Set the name of the program.
 *
 * This function is used to setup the logger progname parameter.
 * Although we had a server::instance()->servername() call, that
 * would not work with tools that do not start the server code,
 * so better have a function to do that setup.
 *
 * \param[in] progname  The name of the program initializing the logger.
 *
 * \sa get_progname()
 */
void set_progname( std::string const & progname )
{
    g_progname = progname;
}


/** \brief Retrieve the program name.
 *
 * This function returns the program name as set with set_progname().
 * If the program name was not set, then this function attempts to
 * define it from the server::instance()->servername() function. If
 * still empty, then the function throws so we (should) know right
 * away that something is wrong.
 *
 * \exception snap_exception
 * This exception is raised if the set_progname() is never called.
 *
 * \return The name of the program.
 *
 * \sa set_progname()
 */
std::string get_progname()
{
    if(g_progname.empty())
    {
        throw snap_exception( "g_progname undefined, please make sure to call set_progname() before calling any logger functions (even if with a fixed name at first)" );
    }

    return g_progname;
}



/** \brief Setup the messenger for the messenger appender.
 *
 * This function saves a copy of the smart pointer of the connection
 * to snapcommunicator in the logger.
 *
 * This connection will be used if available and a messenger logger
 * is setup.
 *
 * \param[in] messenger  A connection to snapcommunicator.
 */
void set_log_messenger( messenger_t messenger )
{
    if( !messenger.lock() )
    {
        throw snap_exception( "Snap communicator messenger must be allocated!" );
    }

    g_log_messenger = messenger;
}


/** \brief Unconfigure the logger and reset.
 *
 * This is an internal function which is here to prevent code duplication.
 *
 * \sa configure()
 */
void unconfigure()
{
    if( g_logging_type != logging_type_t::UNCONFIGURED_LOGGER )
    {
        // shutdown the previous version before re-configuring
        // (this is done after a fork() call.)
        //
        log4cplus::Logger::shutdown();
        g_logging_type = logging_type_t::UNCONFIGURED_LOGGER;
        //g_last_logging_type = ... -- keep the last valid configuration
        //  type so we can call reconfigure() and get it back "as expected"

        // TBD: should we clear the logger and secure logger instances?
        //g_logger = log4cplus::Logger();
        //g_secure_logger = log4cplus::Logger();
    }

    // register our appender
    //
    // Note: this function gets called by all the configure_...() functions
    //       so it is a fairly logical place to do that registration...
    //
    MessengerAppender::registerAppender();
}


/** \brief Configure log4cplus system to the console.
 *
 * This function is the default called in case the user has not specified
 * a configuration file to read.
 *
 * It sets up a default appender to the standard output.
 *
 * \note
 * This function marks that the logger was configured. The other functions
 * do not work (do nothing) until this happens. In case of the server,
 * configure() is called from the server::config() function. If no configuration
 * file is defined then the other functions will do nothing.
 *
 * Format documentation:
 * http://log4cplus.sourceforge.net/docs/html/classlog4cplus_1_1PatternLayout.html
 *
 * \sa fatal()
 * \sa error()
 * \sa warning()
 * \sa info()
 * \sa server::config()
 * \sa unconfigure()
 */
void configure_console()
{
    unconfigure();

    log4cplus::SharedAppenderPtr appender(new log4cplus::ConsoleAppender());
    appender->setName(LOG4CPLUS_TEXT("console"));
    const log4cplus::tstring pattern
                ( boost::replace_all_copy(get_progname(), "%", "%%").c_str()
                + log4cplus::tstring("[%i]:%b:%L:%h: %m%n")
                );
    //const log4cplus::tstring pattern( "%b:%L:%h: %m%n" );
// log4cplus only accepts std::auto_ptr<> which is deprecated in newer versions
// of g++ so we have to make sure the deprecation definition gets ignored
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    appender->setLayout( std::auto_ptr<log4cplus::Layout>( new log4cplus::PatternLayout(pattern) ) );
#pragma GCC diagnostic pop
    appender->setThreshold( log4cplus::TRACE_LOG_LEVEL );

    g_log_config_filename.clear();
    g_log_output_filename.clear();
    g_logging_type       = logging_type_t::CONSOLE_LOGGER;
    g_last_logging_type  = logging_type_t::CONSOLE_LOGGER;
    g_logger             = log4cplus::Logger::getInstance("snap");
    g_secure_logger      = log4cplus::Logger::getInstance("security");

    g_logger.addAppender( appender );
    g_secure_logger.addAppender( appender );
    set_log_output_level( log_level_t::LOG_LEVEL_INFO );
}


/** \brief Configure log4cplus system turning on the rolling file appender.
 *
 * This function is called when the user has specified to write logs to a file.
 *
 * \note
 * This function marks that the logger was configured. The other functions
 * do not work (do nothing) until this happens. In case of the server,
 * configure() is called from the server::config() function. If no configuration
 * file is defined then the other functions will do nothing.
 *
 * \param[in] logfile  The name of the configuration file.
 *
 * \sa fatal()
 * \sa error()
 * \sa warning()
 * \sa info()
 * \sa server::config()
 * \sa unconfigure()
 */
void configure_logfile( QString const & logfile )
{
    unconfigure();

    if( logfile.isEmpty() )
    {
        throw snap_exception( "No output logfile specified!" );
    }

    QByteArray utf8_name(logfile.toUtf8());
    log4cplus::SharedAppenderPtr appender(new log4cplus::RollingFileAppender( utf8_name.data() ));
    appender->setName(LOG4CPLUS_TEXT("log_file"));
    log4cplus::tstring const pattern
                ( log4cplus::tstring("%d{%Y/%m/%d %H:%M:%S} %h ")
                + boost::replace_all_copy(get_progname(), "%", "%%").c_str()
                + log4cplus::tstring("[%i]: %m (%b:%L)%n")
                );
// log4cplus only accepts std::auto_ptr<> which is deprecated in newer versions
// of g++ so we have to make sure the deprecation definition gets ignored
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    appender->setLayout( std::auto_ptr<log4cplus::Layout>( new log4cplus::PatternLayout(pattern) ) );
#pragma GCC diagnostic pop
    appender->setThreshold( log4cplus::TRACE_LOG_LEVEL );

    g_log_config_filename.clear();
    g_log_output_filename = logfile;
    g_logging_type        = logging_type_t::FILE_LOGGER;
    g_last_logging_type   = logging_type_t::FILE_LOGGER;
    g_logger              = log4cplus::Logger::getInstance( "snap" );
    g_secure_logger       = log4cplus::Logger::getInstance( "security" );

    g_logger.addAppender( appender );
    g_secure_logger.addAppender( appender );
    set_log_output_level( log_level_t::LOG_LEVEL_INFO );
}


/** \brief Configure a messenger instance.
 *
 * Log entries are sent to snapcommunicator. The configured log level of
 * the "snap" logger is used to determine what to send "over the wire."
 * This is making the assumption that you have set up the "snap" logger
 * correctly.
 *
 * Note that in most cases you want to use configure_logfile() which
 * can define a messenger too, without the need to call this function.
 *
 * \warning
 * Make sure that you call the set_log_messenger() function with a
 * connection to snapcommunicator or this appender won't do anything.
 */
void configure_messenger()
{
    unconfigure();

    log4cplus::SharedAppenderPtr appender( new MessengerAppender );
    appender->setName( LOG4CPLUS_TEXT("snapcommunicator") );
    log4cplus::tstring const pattern
                ( log4cplus::tstring("%d{%Y/%m/%d %H:%M:%S} %h ")
                + boost::replace_all_copy(get_progname(), "%", "%%").c_str()
                + log4cplus::tstring("[%i]: %m (%b:%L)%n")
                );
// log4cplus only accepts std::auto_ptr<> which is deprecated in newer versions
// of g++ so we have to make sure the deprecation definition gets ignored
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    appender->setLayout( std::auto_ptr<log4cplus::Layout>( new log4cplus::PatternLayout(pattern) ) );
#pragma GCC diagnostic pop
    appender->setThreshold( log4cplus::TRACE_LOG_LEVEL );

    g_logging_type                 = logging_type_t::MESSENGER_LOGGER;
    g_last_logging_type            = logging_type_t::MESSENGER_LOGGER;
    g_messenger_logger             = log4cplus::Logger::getInstance( "messenger" );

    g_messenger_logger_initialized = true;

    g_messenger_logger.addAppender( appender );
    set_log_output_level( log_level_t::LOG_LEVEL_INFO );
}


/** \brief Configure log4cplus system to the syslog.
 *
 * Set up the logging to be routed to the syslog.
 *
 * \note
 * This function marks that the logger was configured. The other functions
 * do not work (do nothing) until this happens. In case of the snap server,
 * configure() is called from the server::config() function. If no
 * configuration file is defined then the other functions will do nothing.
 *
 * Format documentation:
 * http://log4cplus.sourceforge.net/docs/html/classlog4cplus_1_1PatternLayout.html
 *
 * \sa fatal()
 * \sa error()
 * \sa warning()
 * \sa info()
 * \sa server::config()
 * \sa unconfigure()
 */
void configure_syslog()
{
    unconfigure();

    log4cplus::SharedAppenderPtr appender( new log4cplus::SysLogAppender( get_progname() ) );
    log4cplus::tstring const pattern
                ( //boost::replace_all_copy(get_progname(), "%", "%%").c_str() -- this is added by syslog() already
                  log4cplus::tstring("[%i] %m (%b:%L)%n")
                );
// log4cplus only accepts std::auto_ptr<> which is deprecated in newer versions
// of g++ so we have to make sure the deprecated definition gets ignored
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    appender->setLayout( std::auto_ptr<log4cplus::Layout>( new log4cplus::PatternLayout(pattern) ) );
#pragma GCC diagnostic pop
    appender->setThreshold( log4cplus::TRACE_LOG_LEVEL );

    g_log_config_filename.clear();
    g_log_output_filename.clear();
    g_logging_type        = logging_type_t::SYSLOG_LOGGER;
    g_last_logging_type   = logging_type_t::SYSLOG_LOGGER;
    g_logger              = log4cplus::Logger::getInstance("snap");
    g_secure_logger       = log4cplus::Logger::getInstance("security");

    g_logger.addAppender( appender );
    g_secure_logger.addAppender( appender );
    set_log_output_level( log_level_t::LOG_LEVEL_INFO );
}


/** \brief Configure from a log4cplus header file.
 *
 * This function sends the specified \p filename to the log4cplus configurator
 * for initialization.
 *
 * If \p filename is empty (undefined in the server configuration file) then
 * the /etc/snapwebsites/log.conf file is used if it exists. If not, then
 * no configuration is created.
 *
 * \note
 * This function marks that the logger was configured. The other functions
 * do not work (do nothing) until this happens. In case of the server,
 * configure() is called from the server::config() function. If no configuration
 * file is defined then the other functions will do nothing.
 *
 * \todo
 * We may also want to get the progname so we can setup the system with that
 * name, although at this time we offer different configuration files for
 * each process.
 *
 * \param[in] filename  The name of the configuration file.
 *
 * \sa fatal()
 * \sa error()
 * \sa warning()
 * \sa info()
 * \sa server::config()
 * \sa unconfigure()
 */
void configure_conffile(QString const & filename)
{
    unconfigure();

    QFileInfo info(filename);
    if(!info.exists())
    {
        throw snap_exception( QObject::tr("Cannot open logger configuration file [%1].").arg(filename) );
    }

    g_log_config_filename   = filename;
    g_log_output_filename.clear();
    g_logging_type          = logging_type_t::CONFFILE_LOGGER;
    g_last_logging_type     = logging_type_t::CONFFILE_LOGGER;

    // note the doConfigure() may throw if the log.properties is invalid
    //
    log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_C_STR_TO_TSTRING(filename.toUtf8().data()));

    g_logger                = log4cplus::Logger::getInstance("snap");
    g_secure_logger         = log4cplus::Logger::getInstance("security");

    g_messenger_logger_initialized = logger_exists("messenger");
    if(g_messenger_logger_initialized)
    {
        g_messenger_logger = log4cplus::Logger::getInstance("messenger");
    }
}


/** \brief Ensure that the configuration is still in place.
 *
 * On a fork() the configuration of log4cplus is lost. We have to
 * call this function again before we can use the logs again.
 *
 * \note
 * TBD -- is it really necessary to reconfigure after a fork() or
 * would the logger know how to handle that case?
 */
void reconfigure()
{
    switch( g_last_logging_type )
    {
    case logging_type_t::CONSOLE_LOGGER:
        configure_console();
        break;

    case logging_type_t::FILE_LOGGER:
        configure_logfile( g_log_output_filename );
        break;

    case logging_type_t::CONFFILE_LOGGER:
        configure_conffile( g_log_config_filename );
        break;

    case logging_type_t::SYSLOG_LOGGER:
        configure_syslog();
        break;

    case logging_type_t::MESSENGER_LOGGER:
        configure_messenger();
        break;

    default:
        /* do nearly nothing */
        unconfigure();
        break;

    }
}


/** \brief Return the current configuration status.
 *
 * This function returns true if the log facility was successfully
 * configured, false otherwise.
 *
 * \return true if the configure() function was called with success.
 */
bool is_configured()
{
    return g_logging_type != logging_type_t::UNCONFIGURED_LOGGER;
}


/* \brief Set the current logging threshold.
 *
 * Tells log4cplus to limit the logging output to the specified threshold.
 *
 * \todo
 * The log level should be cached if this function gets called before
 * the logger is setup. Right now, we lose the information.
 */
void set_log_output_level( log_level_t level )
{
    if(!is_configured())
    {
        return;
    }

    log4cplus::LogLevel new_level = log4cplus::OFF_LOG_LEVEL;

    switch(level)
    {
    case log_level_t::LOG_LEVEL_OFF:
        new_level = log4cplus::OFF_LOG_LEVEL;
        return;

    case log_level_t::LOG_LEVEL_FATAL:
        new_level = log4cplus::FATAL_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_ERROR:
        new_level = log4cplus::ERROR_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_WARNING:
        new_level = log4cplus::WARN_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_INFO:
        new_level = log4cplus::INFO_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_DEBUG:
        new_level = log4cplus::DEBUG_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_TRACE:
        new_level = log4cplus::TRACE_LOG_LEVEL;
        break;

    }

    log4cplus::Logger::getRoot().setLogLevel( new_level );
    g_logger.setLogLevel( new_level );
    g_secure_logger.setLogLevel( new_level );
    if(g_messenger_logger_initialized)
    {
        g_messenger_logger.setLogLevel( new_level );
    }
}


/* \brief Set the maximum logging threshold.
 *
 * Tells log4cplus to reduce the logging output to the specified threshold.
 * If the threshold is already that low or lower, nothing happens.
 *
 * \note
 * Our threshold levels are increasing when the log4cplus levels decrease...
 * Here we use "reduce" in the sense that we show more data and thus it
 * matches the log4cplus order.
 *
 * \todo
 * The conversion of our log level to the log4cplus level needs to be
 * in a separate function.
 */
void reduce_log_output_level( log_level_t level )
{
    if(!is_configured())
    {
        return;
    }

    log4cplus::LogLevel new_level = log4cplus::OFF_LOG_LEVEL;

    switch(level)
    {
    case log_level_t::LOG_LEVEL_OFF:
        new_level = log4cplus::OFF_LOG_LEVEL;
        return;

    case log_level_t::LOG_LEVEL_FATAL:
        new_level = log4cplus::FATAL_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_ERROR:
        new_level = log4cplus::ERROR_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_WARNING:
        new_level = log4cplus::WARN_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_INFO:
        new_level = log4cplus::INFO_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_DEBUG:
        new_level = log4cplus::DEBUG_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_TRACE:
        new_level = log4cplus::TRACE_LOG_LEVEL;
        break;

    }

    log4cplus::Logger::getRoot().setLogLevel( new_level );
    if( new_level < g_logger.getLogLevel() )
    {
        g_logger.setLogLevel( new_level );
    }
    if( new_level < g_secure_logger.getLogLevel() )
    {
        g_secure_logger.setLogLevel( new_level );
    }
    if(g_messenger_logger_initialized)
    {
        if( new_level < g_messenger_logger.getLogLevel() )
        {
            g_messenger_logger.setLogLevel( new_level );
        }
    }
}


/** \brief Create a log object with the specified information.
 *
 * This function generates a log object that can be used to generate
 * a log message with the () operator and then gets logged using
 * log4cplus on destruction.
 *
 * The level can be set to any one of the log levels available in
 * the log_level_t enumeration. The special LOG_LEVEL_OFF value can be
 * used to avoid the log altogether (can be handy when you support a
 * varying log level.)
 *
 * By default logs are not marked as secure. If you are creating a log
 * that should only go to the secure logger, then use the () operator
 * with the LOG_SECURITY_SECURE value as in:
 *
 * \code
 *   // use the "security" logger
 *   SNAP_LOG_FATAL(LOG_SECURITY_SECURE)("this is not authorized!");
 * \endcode
 *
 * \param[in] log_level  The level of logging.
 * \param[in] file  The name of the source file that log was generated from.
 * \param[in] func  The name of the function that log was generated from.
 * \param[in] line  The line number that log was generated from.
 */
logger::logger(log_level_t const log_level, char const * file, char const * func, int const line)
    : f_log_level(log_level)
    , f_file(file)
    , f_func(func)
    , f_line(line)
    , f_security(log_security_t::LOG_SECURITY_NONE)
    //, f_message() -- auto-init
    //, f_ignore(false) -- auto-init
{
}


/** \brief Create a copy of this logger instance.
 *
 * This function creates a copy of the logger instance. This happens when
 * you use the predefined fatal(), error(), warning(), ... functions since
 * the logger instantiated inside the function is returned and thus copied
 * once or twice (the number of copies will depend on the way the compiler
 * is capable of optimizing our work.)
 *
 * \note
 * The copy has a side effect on the input logger: it marks it as "please
 * ignore that copy" so its destructor does not print out anything.
 *
 * \param[in] l  The logger to duplicate.
 */
logger::logger(logger const & l)
    : f_log_level(l.f_log_level)
    , f_file(l.f_file)
    , f_func(l.f_func)
    , f_line(l.f_line)
    , f_security(l.f_security)
    , f_message(l.f_message)
{
    l.f_ignore = true;
}


/** \brief Output the log created with the () operators.
 *
 * The destructor of the log object is where things happen. This function
 * prints out the message that was built using the different () operators
 * and the parameters specified in the constructor.
 *
 * The snap log level is converted to a log4cplus log level (and a syslog
 * level in case log4cplus is not available.)
 *
 * If the () operator was used with LOG_SECURITY_SECURE, then the message
 * is sent using the "security" logger. Otherwise it uses the standard
 * "snap" logger.
 */
logger::~logger()
{
    if(f_ignore)
    {
        // someone made a copy, this version we ignore
        return;
    }

    log4cplus::LogLevel ll(log4cplus::FATAL_LOG_LEVEL);
    int sll(-1);  // syslog level if log4cplus not available (if -1 do not syslog() anything)
    bool console(false);
    char const * level_str(nullptr);
    switch(f_log_level)
    {
    case log_level_t::LOG_LEVEL_OFF:
        // off means we do not emit anything
        return;

    case log_level_t::LOG_LEVEL_FATAL:
        ll = log4cplus::FATAL_LOG_LEVEL;
        sll = LOG_CRIT;
        console = true;
        level_str = "fatal error";
        break;

    case log_level_t::LOG_LEVEL_ERROR:
        ll = log4cplus::ERROR_LOG_LEVEL;
        sll = LOG_ERR;
        console = true;
        level_str = "error";
        break;

    case log_level_t::LOG_LEVEL_WARNING:
        ll = log4cplus::WARN_LOG_LEVEL;
        sll = LOG_WARNING;
        console = true;
        level_str = "warning";
        break;

    case log_level_t::LOG_LEVEL_INFO:
        ll = log4cplus::INFO_LOG_LEVEL;
        sll = LOG_INFO;
        level_str = "info";
        break;

    case log_level_t::LOG_LEVEL_DEBUG:
        ll = log4cplus::DEBUG_LOG_LEVEL;
        level_str = "debug";
        break;

    case log_level_t::LOG_LEVEL_TRACE:
        ll = log4cplus::TRACE_LOG_LEVEL;
        level_str = "trace";
        break;
    }

    // TODO: instead of calling logger_exists() which is very expensive
    //       (because it uses a try/catch), we should instead have a flag
    //       to know whether a logger is properly configured; if so then
    //       we can use the else block.
    //
    if( (g_logging_type == logging_type_t::UNCONFIGURED_LOGGER)
    ||  !logger_exists(log_security_t::LOG_SECURITY_SECURE == f_security ? "security" : "snap"))
    {
        // if not even configured, return immediately
        if(sll != -1)
        {
            if(!f_file)
            {
                f_file = "unknown-file";
            }
            if(!f_func)
            {
                f_func = "unknown-func";
            }
            syslog(sll, "%s (%s:%s: %d)", f_message.toUtf8().data(), f_file, f_func, static_cast<int32_t>(f_line));
        }
    }
    else
    {
        if(f_func)
        {
            // TBD: how should we really include the function name to the log4cplus messages?
            //
            // Note: we permit ourselves to modify f_message since we are in the destructor
            //       about to leave this object anyway.
            f_message += QString(" (in function \"%1()\")").arg(f_func);
        }

        // actually emit the log
        if(log_security_t::LOG_SECURITY_SECURE == f_security)
        {
            // generally this at least goes in the /var/log/syslog
            // and it may also go in a secure log file (i.e. not readable by everyone)
            //
            g_secure_logger.log(ll, LOG4CPLUS_C_STR_TO_TSTRING(f_message.toUtf8().data()), f_file, f_line, f_func);
        }
        else
        {
            g_logger.log(ll, LOG4CPLUS_C_STR_TO_TSTRING(f_message.toUtf8().data()), f_file, f_line, f_func);

            // full logger used, do not report error in console, logger can
            // do it if the user wants to
            //
            console = false;
        }
    }

    if( g_messenger_logger_initialized )
    {
        g_messenger_logger.log( ll, LOG4CPLUS_C_STR_TO_TSTRING(f_message.toUtf8().data()), f_file, f_line, f_func );
    }

    if(console && isatty(STDERR_FILENO))
    {
        std::cerr << level_str << ":" << f_file << ":" << f_line << ": " << f_message.toUtf8().data() << std::endl;
    }
}


logger & logger::operator () ()
{
    // does nothing
    return *this;
}


logger & logger::operator () (log_security_t const v)
{
    f_security = v;
    return *this;
}


logger & logger::operator () (char const * s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    // TODO: change control characters to \xXX
    f_message += QString::fromUtf8(s);
    return *this;
}


logger & logger::operator () (wchar_t const * s)
{
    // TODO: change control characters to \xXX
    f_message += QString::fromWCharArray(s);
    return *this;
}


logger & logger::operator () (std::string const & s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    // TODO: change control characters to \xXX
    f_message += QString::fromUtf8(s.c_str());
    return *this;
}


logger & logger::operator () (std::wstring const & s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    // TODO: change control characters to \xXX
    f_message += QString::fromWCharArray(s.c_str());
    return *this;
}


logger & logger::operator () (QString const & s)
{
    // TODO: change control characters to \xXX
    f_message += s;
    return *this;
}


logger & logger::operator () (snap::snap_config::snap_config_parameter_ref const & s)
{
    f_message += QString(s);
    return *this;
}


logger & logger::operator () (char const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger & logger::operator () (signed char const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger & logger::operator () (unsigned char const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger & logger::operator () (signed short const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger & logger::operator () (unsigned short const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger & logger::operator () (signed int const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger & logger::operator () (unsigned int const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger & logger::operator () (signed long const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger & logger::operator () (unsigned long const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger & logger::operator () (signed long long const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger & logger::operator () (unsigned long long const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger & logger::operator () (float const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger & logger::operator () (double const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger & logger::operator () (bool const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger & operator << ( logger & l, QString const & msg )
{
    return l( msg );
}


logger & operator << ( logger & l, std::basic_string<char> const & msg )
{
    return l( msg );
}


logger & operator << ( logger & l, snap::snap_config::snap_config_parameter_ref const & msg )
{
    return l( msg );
}


logger & operator << ( logger & l, std::basic_string<wchar_t> const & msg )
{
    return l( msg );
}


logger & operator << ( logger & l, char const * msg )
{
    return l( msg );
}


logger & operator << ( logger & l, wchar_t const * msg )
{
    return l( msg );
}



/** \brief This function checks whether the log level allows output.
 *
 * This function checks the user specified log level against
 * the current log level of the logger. If the log is to be
 * output, then the function returns true (i.e. user log level
 * is larger or equal to the logger's log level.)
 *
 * \todo
 * Unfortunately we cannot be sure, at this point, whether the
 * log will be secure or not. So we have to check with both
 * loggers and return true if either would log the data. Since
 * the secure logger is likely to have a higher log level and
 * we log way less secure data, we should be just fine.
 *
 * \return true if the log should be computed.
 */
bool is_enabled_for( log_level_t const log_level )
{
    // if still unconfigured, we just pretend the level is ON because
    // we do not really know for sure what the level is at this point
    //
    if( g_logging_type == logging_type_t::UNCONFIGURED_LOGGER )
    {
        return true;
    }

    log4cplus::LogLevel ll(log4cplus::FATAL_LOG_LEVEL);

    switch(log_level)
    {
    case log_level_t::LOG_LEVEL_OFF:
        // off means we do not emit anything so always return false
        return false;

    case log_level_t::LOG_LEVEL_FATAL:
        ll = log4cplus::FATAL_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_ERROR:
        ll = log4cplus::ERROR_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_WARNING:
        ll = log4cplus::WARN_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_INFO:
        ll = log4cplus::INFO_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_DEBUG:
        ll = log4cplus::DEBUG_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_TRACE:
        ll = log4cplus::TRACE_LOG_LEVEL;
        break;

    }

    // TODO: see whether we could have a better way to only
    //       return the one concerned (i.e. 2x the macros
    //       and specify secure right there?) -- although
    //       the likelihood is that g_logger is going to
    //       be used and the log level of that one is
    //       likely lower than g_secure_logger; but such
    //       a statement can always be all wrong...
    //
    return g_logger.isEnabledFor(ll)
        || g_secure_logger.isEnabledFor(ll)
        || (g_messenger_logger_initialized && g_messenger_logger.isEnabledFor(ll));
}



logger fatal(char const * file, char const * func, int line)
{
    if(is_enabled_for(log_level_t::LOG_LEVEL_FATAL))
    {
        logger l(log_level_t::LOG_LEVEL_FATAL, file, func, line);
        return l.operator () ("fatal error: ");
    }
    else
    {
        logger_stub l(log_level_t::LOG_LEVEL_FATAL, file, func, line);
        return l;
    }
}

logger error(char const * file, char const * func, int line)
{
    if(is_enabled_for(log_level_t::LOG_LEVEL_ERROR))
    {
        logger l(log_level_t::LOG_LEVEL_ERROR, file, func, line);
        return l.operator () ("error: ");
    }
    else
    {
        logger_stub l(log_level_t::LOG_LEVEL_ERROR, file, func, line);
        return l;
    }
}

logger warning(char const * file, char const * func, int line)
{
    if(is_enabled_for(log_level_t::LOG_LEVEL_WARNING))
    {
        logger l(log_level_t::LOG_LEVEL_WARNING, file, func, line);
        return l.operator () ("warning: ");
    }
    else
    {
        logger_stub l(log_level_t::LOG_LEVEL_WARNING, file, func, line);
        return l;
    }
}

logger info(char const * file, char const * func, int line)
{
    if(is_enabled_for(log_level_t::LOG_LEVEL_INFO))
    {
        logger l(log_level_t::LOG_LEVEL_INFO, file, func, line);
        return l.operator () ("info: ");
    }
    else
    {
        logger_stub l(log_level_t::LOG_LEVEL_INFO, file, func, line);
        return l;
    }
}

logger debug(char const * file, char const * func, int line)
{
    if(is_enabled_for(log_level_t::LOG_LEVEL_DEBUG))
    {
        logger l(log_level_t::LOG_LEVEL_DEBUG, file, func, line);
        return l.operator () ("debug: ");
    }
    else
    {
        logger_stub l(log_level_t::LOG_LEVEL_DEBUG, file, func, line);
        return l;
    }
}

logger trace(char const * file, char const * func, int line)
{
    if(is_enabled_for(log_level_t::LOG_LEVEL_TRACE))
    {
        logger l(log_level_t::LOG_LEVEL_TRACE, file, func, line);
        return l.operator () ("trace: ");
    }
    else
    {
        logger_stub l(log_level_t::LOG_LEVEL_TRACE, file, func, line);
        return l;
    }
}


} // namespace logging
} // namespace snap
// vim: ts=4 sw=4 et
