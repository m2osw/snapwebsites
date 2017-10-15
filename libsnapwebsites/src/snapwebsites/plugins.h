// Snap Servers -- plugins loader
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

#include "snapwebsites/snap_exception.h"
#include "snapwebsites/snap_string_list.h"

#include <memory>

#include <QVector>
#include <QMap>


namespace snap
{

// bootstrap() references snap_child as a pointer
class snap_child;

namespace plugins
{

class plugin_exception : public snap_exception
{
public:
    explicit plugin_exception(char const *        what_msg) : snap_exception("plugin", what_msg) {}
    explicit plugin_exception(std::string const & what_msg) : snap_exception("plugin", what_msg) {}
    explicit plugin_exception(QString const &     what_msg) : snap_exception("plugin", what_msg) {}
};

class plugin_exception_invalid_order : public plugin_exception
{
public:
    explicit plugin_exception_invalid_order(char const *        what_msg) : plugin_exception(what_msg) {}
    explicit plugin_exception_invalid_order(std::string const & what_msg) : plugin_exception(what_msg) {}
    explicit plugin_exception_invalid_order(QString const &     what_msg) : plugin_exception(what_msg) {}
};


class plugin_signal
{
public:
    plugin_signal(const char * name);
};




class plugin
{
public:
                                        plugin();
    virtual                             ~plugin() {}

    void                                set_version(int version_major, int version_minor);
    int                                 get_major_version() const;
    int                                 get_minor_version() const;
    QString                             get_plugin_name() const;
    int64_t                             last_modification() const;
    virtual QString                     icon() const;
    virtual QString                     description() const = 0;
    virtual QString                     plugin_categorization_tags() const;
    virtual QString                     help_uri() const;
    virtual QString                     settings_path() const;
    virtual QString                     dependencies() const = 0;
    virtual void                        bootstrap(snap_child * snap) = 0;
    virtual int64_t                     do_update(int64_t last_updated);
    virtual int64_t                     do_dynamic_update(int64_t last_updated);

private:
    QString const                       f_name;
    QString const                       f_filename;
    mutable int64_t                     f_last_modification = 0;
    int32_t                             f_version_major = 0;
    int32_t                             f_version_minor = 0;
};

typedef std::shared_ptr<plugin>                 plugin_ptr_t;
typedef QMap<QString, plugin *>                 plugin_map_t;
typedef QVector<plugin *>                       plugin_vector_t;


class plugin_info
{
public:
                        plugin_info(QString const & plugin_paths, QString const & name);

    QString const &     get_name() const;
    QString const &     get_filename() const;
    int64_t             get_last_modification() const;
    QString const &     get_icon() const;
    QString const &     get_description() const;
    QString const &     get_plugin_categorization_tags() const;
    QString const &     get_help_uri() const;
    QString const &     get_settings_path() const;
    QString const &     get_dependencies() const;
    int32_t             get_version_major() const;
    int32_t             get_version_minor() const;

private:
    QString             f_name;
    QString             f_filename;
    int64_t             f_last_modification = 0;
    QString             f_icon;
    QString             f_description;
    QString             f_categorization_tags;
    QString             f_help_uri;
    QString             f_settings_path;
    QString             f_dependencies;
    int32_t             f_version_major = 0;
    int32_t             f_version_minor = 0;
};


snap_string_list        list_all(QString const & plugin_path);
bool                    load(QString const & plugin_path, snap_child * snap, plugin_ptr_t server, snap_string_list const & list_of_plugins);
QString                 find_plugin_filename(snap_string_list const & plugin_paths, QString const & name);
bool                    exists(QString const & name);
void                    register_plugin(QString const & name, plugin * p);
plugin *                get_plugin(QString const & name);
plugin_map_t const &    get_plugin_list();
plugin_vector_t const & get_plugin_vector();
bool                    verify_plugin_name(QString const & name);

/** \brief Initialize a plugin by creating a mini-factory.
 *
 * The factory is used to create a new instance of the plugin.
 *
 * Remember that we cannot have a plugin register all of its signals
 * in its constructor. This is because many of the other plugins
 * will otherwise not already be loaded and if missing at the time
 * we try to connect, the software breaks. This is why we have the
 * bootstrap() virtual function (very much like an init() function!)
 * and this macro automatically ensures that it gets called.
 *
 * The use of the macro is very simple, it is expected to appear
 * at the beginning in your filter implementation (.cpp) file:
 *
 * \code
 * SNAP_PLUGIN_START(plugin_name, 1, 0)
 * \endcode
 *
 * Replace \<plugin_name> with the name of your plugin. The following
 * numbers represent the major and minor version numbers of the plugin.
 * They are used to show the user what he's running with. The system
 * does not otherwise do much with that information at this point.
 *
 * The name of your plugin factory is:
 *
 * \code
 * plugin_\<name>_factory
 * \endcode
 *
 * And in order to get an instance, it defines a global variable named:
 *
 * \code
 * g_plugin_\<name>_factory
 * \endcode
 *
 * The factory has a few public functions you can call:
 * \li name *instance() -- It returns a pointer to your plugin (i.e. the
 * same as 'this' in your plugin functions.)
 * \li int version_major() const -- The major version number
 * \li int version_minor() const -- The minor version number
 * \li QString version() const -- The version as a string: "<major>.<minor>"
 *
 * The constructor is automatically called when dlopen() loads
 * your plugin and the destructor is automatically called whenever
 * dl unloads the plugins.
 *
 * At the end of your implementation you have to add the
 * SNAP_PLUGIN_END() macro to close the namespaces that the start
 * macro automatically adds.
 *
 * Note that the factory initialization also takes care of
 * initializing the Qt resources of the plugin (resources
 * are mandatory in all plugins because you are expected
 * to call the add_xml() function, although a few low
 * level plugins won't do that, they still need resources.)
 *
 * \note
 * If you create multiple .cpp files to implement your plugin
 * only one can use the SNAP_PLUGIN_START()/SNAP_PLUGIN_END()
 * macros. Otherwise you'd create multiple factories. In the
 * other files, make use of the SNAP_PLUGIN_EXTENSION_START()
 * and SNAP_PLUGIN_EXTENSION_END() macros instead.
 *
 * \todo
 * Look in a way to avoid the qInitResources_... if the plugin
 * has no resources.
 *
 * \param[in] name  The name of the plugin.
 * \param[in] major  The major version of the plugin.
 * \param[in] minor  The minor version of the plugin.
 */
#define SNAP_PLUGIN_START(name, major, minor) \
    extern int qInitResources_##name(); \
    namespace snap { namespace name { \
    class plugin_##name##_factory { \
    public: plugin_##name##_factory() : f_plugin(new name()) { \
        qInitResources_##name(); \
        f_plugin->set_version(major, minor); \
        ::snap::plugins::register_plugin(#name, f_plugin); } \
    virtual ~plugin_##name##_factory() { delete f_plugin; } \
    name * instance() { return f_plugin; } \
    virtual int version_major() const { return major; } \
    virtual int version_minor() const { return minor; } \
    virtual QString version() const { return #major "." #minor; } \
    private: name * f_plugin; \
    } g_plugin_##name##_factory;


/** \brief The counter part of the SNAP_PLUGIN_START() macro.
 *
 * This macro is used to close the namespaces opened by the
 * SNAP_PLUGIN_START() macro. It is used at the end of your
 * plugin implementation.
 */
#define SNAP_PLUGIN_END() \
    }} // close namespaces


/** \brief Initialize an extension (.cpp) file of a plugin.
 *
 * At time a plugin is really large. In order to write the plugin
 * in multiple .cpp files, you need to have a start and an end in
 * each file. Only one of the files can define the plugin factory
 * and use the SNAP_PLUGIN_START() macro. The other files want
 * to use this macro:
 *
 * \code
 * SNAP_PLUGIN_EXTENSION_START(plugin_name)
 * \endcode
 *
 * Replace \<plugin_name> with the name of your plugin. It has to
 * be the same in all the files.
 *
 * At the end of your implementation extension you have to add the
 * SNAP_PLUGIN_EXTENSION_END() macro to close the namespaces that the
 * start macro automatically adds.
 *
 * \param[in] name  The name of the plugin.
 */
#define SNAP_PLUGIN_EXTENSION_START(name) \
    namespace snap { namespace name {


/** \brief End the extension (.cpp) file of a plugin.
 *
 * When you started a .cpp file with the SNAP_PLUGIN_EXTENSION_START()
 * then you want to end it with the SNAP_PLUGIN_EXTENSION_END() macro.
 *
 * \code
 * SNAP_PLUGIN_EXTENSION_END()
 * \endcode
 */
#define SNAP_PLUGIN_EXTENSION_END() \
    }} // close namespaces


/** \brief Conditionally listen to a signal.
 *
 * This function checks whether a given plugin was loaded and if so
 * listen to one of its signals.
 *
 * The macro accepts the name of the listener plugin (it must be
 * 'this'), the name of the emitter plugin, and the name of the
 * signal to listen to.
 *
 * The listener must have a function named on_\<name of signal>.
 * The emitter is expected to define the signal using the
 * SNAP_SIGNAL() macro so the signal is called
 * signal_listen_\<name of signal>.
 *
 * \param[in] name  The name of the plugin connecting.
 * \param[in] emitter_name  The name of the plugin emitting this signal.
 * \param[in] emitter_class  The class with qualifiers if necessary of the plugin emitting this signal.
 * \param[in] signal  The name of the signal to listen to.
 * \param[in] args  The list of arguments to that signal.
 */
#define SNAP_LISTEN(name, emitter_name, emitter_class, signal, args...) \
    if(::snap::plugins::exists(emitter_name)) \
        emitter_class::instance()->signal_listen_##signal( \
                    boost::bind(&name::on_##signal, this, ##args));

#define SNAP_LISTEN0(name, emitter_name, emitter_class, signal) \
    if(::snap::plugins::exists(emitter_name)) \
        emitter_class::instance()->signal_listen_##signal( \
                    boost::bind(&name::on_##signal, this));

/** \brief Compute the number of days in the month of February.
 * \private
 *
 * The month of February is used to adjust the date by 1 day over leap
 * years. Years are leap years when multiple of 4, but not if multiple
 * of 100, except if it is also a multiple of 400.
 *
 * The computation of a leap year is documented on Wikipedia:
 * http://www.wikipedia.org/wiki/Leap_year
 *
 * \param[in] year  The year of the date to convert.
 *
 * \return 28 or 29 depending on whether the year is a leap year
 */
#define _SNAP_UNIX_TIMESTAMP_FDAY(year) \
    (((year) % 400) == 0 ? 29LL : \
        (((year) % 100) == 0 ? 28LL : \
            (((year) % 4) == 0 ? 29LL : \
                28LL)))

/** \brief Compute the number of days in a year.
 * \private
 *
 * This macro returns the number of day from the beginning of the
 * year the (year, month, day) value represents.
 *
 * \param[in] year  The 4 digits year concerned.
 * \param[in] month  The month (1 to 12).
 * \param[in] day  The day of the month (1 to 31)
 *
 * \return The number of days within that year (starting at 1)
 */
#define _SNAP_UNIX_TIMESTAMP_YDAY(year, month, day) \
    ( \
        /* January */    static_cast<qint64>(day) \
        /* February */ + ((month) >=  2 ? 31LL : 0LL) \
        /* March */    + ((month) >=  3 ? _SNAP_UNIX_TIMESTAMP_FDAY(year) : 0LL) \
        /* April */    + ((month) >=  4 ? 31LL : 0LL) \
        /* May */      + ((month) >=  5 ? 30LL : 0LL) \
        /* June */     + ((month) >=  6 ? 31LL : 0LL) \
        /* July */     + ((month) >=  7 ? 30LL : 0LL) \
        /* August */   + ((month) >=  8 ? 31LL : 0LL) \
        /* September */+ ((month) >=  9 ? 31LL : 0LL) \
        /* October */  + ((month) >= 10 ? 30LL : 0LL) \
        /* November */ + ((month) >= 11 ? 31LL : 0LL) \
        /* December */ + ((month) >= 12 ? 30LL : 0LL) \
    )

/** \brief Compute a Unix date from a hard coded date.
 *
 * This macro is used to compute a Unix date from a date defined as 6 numbers:
 * year, month, day, hour, minute, second. Each number is expected to be an
 * integer although it could very well be an expression. The computation takes
 * the year and month in account to compute the year day which is used by
 * the do_update() functions.
 *
 * The year is expected to be written as a 4 digit number (1998, 2012, etc.)
 *
 * Each number is expected to represent a valid date. If a number is out of
 * range, then the date is still computed. It will just represent a valid
 * date, just not exactly what you wrote down.
 *
 * The math used in this macro comes from a FreeBSD implementation of mktime
 * (http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15)
 *
 * \code
 * tm_sec + tm_min*60 + tm_hour*3600 + tm_yday*86400 +
 *   (tm_year-70)*31536000 + ((tm_year-69)/4)*86400 -
 *   ((tm_year-1)/100)*86400 + ((tm_year+299)/400)*86400
 * \endcode
 *
 * Note that we expect the year to be 1970 and not 0, 2000 and not 30, etc.
 * For this reason our macro subtract values from the year that are different
 * from those shown in the FreeBSD sample code.
 *
 * Also the month must be a number from 1 to 12 and not 0 to 11 as used
 * in various Unix structures.
 *
 * \param[in] year  The year representing this Unix timestamp.
 * \param[in] month  The month representing this Unix timestamp.
 * \param[in] day  The day representing this Unix timestamp.
 * \param[in] hour  The hour representing this Unix timestamp.
 * \param[in] minute  The minute representing this Unix timestamp.
 * \param[in] second  The second representing this Unix timestamp.
 */
#define SNAP_UNIX_TIMESTAMP(year, month, day, hour, minute, second) \
    ( /* time */ static_cast<qint64>(second) \
                + static_cast<qint64>(minute) * 60LL \
                + static_cast<qint64>(hour) * 3600LL \
    + /* year day (month + day) */ (_SNAP_UNIX_TIMESTAMP_YDAY(year, month, day) - 1) * 86400LL \
    + /* year */ (static_cast<qint64>(year) - 1970LL) * 31536000LL \
                + ((static_cast<qint64>(year) - 1969LL) / 4LL) * 86400LL \
                - ((static_cast<qint64>(year) - 1901LL) / 100LL) * 86400LL \
                + ((static_cast<qint64>(year) - 1601LL) / 400LL) * 86400LL )

/** \brief Initialize the do_update() function.
 *
 * This macro is used to initialize the do_update() function by creating a
 * variable that is going to be given a date.
 */
#define SNAP_PLUGIN_UPDATE_INIT() int64_t last_plugin_update(SNAP_UNIX_TIMESTAMP(1990, 1, 1, 0, 0, 0) * 1000000LL);

/** \brief Create an update entry in your on_update() signal implementation.
 *
 * This macro is used to generate the necessary code to test the latest
 * update date and the date of the specified update.
 *
 * The function is called if the last time the website was updated it
 * was before this update. The function is then called with its own
 * date in micro-seconds (usec).
 *
 * \warning
 * The parameter to the on_update() function must be named last_updated for
 * this macro to compile as expected.
 *
 * \param[in] year  The year representing this Unix timestamp.
 * \param[in] month  The month representing this Unix timestamp.
 * \param[in] day  The day representing this Unix timestamp.
 * \param[in] hour  The hour representing this Unix timestamp.
 * \param[in] minute  The minute representing this Unix timestamp.
 * \param[in] second  The second representing this Unix timestamp.
 * \param[in] function  The name of the function to call if the update is required.
 */
#define SNAP_PLUGIN_UPDATE(year, month, day, hour, minute, second, function) \
    if(last_plugin_update > SNAP_UNIX_TIMESTAMP(year, month, day, hour, minute, second) * 1000000LL) { \
        throw plugins::plugin_exception_invalid_order("the updates in your do_update() functions must appear in increasing order in regard to date and time"); \
    } \
    last_plugin_update = SNAP_UNIX_TIMESTAMP(year, month, day, hour, minute, second) * 1000000LL; \
    if(last_updated < last_plugin_update) { \
        function(last_plugin_update); \
    }

/** \brief End the plugin update function.
 *
 * Use the macro as the very last line in your plugin do_update() function.
 * It returns the lastest update of your plugin.
 *
 * This function adds a return statement so anything appearing after it
 * will be ignored (not reached.)
 */
#define SNAP_PLUGIN_UPDATE_EXIT() return last_plugin_update;


} // namespace plugins
} // namespace snap
// vim: ts=4 sw=4 et
