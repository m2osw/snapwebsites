// Snap Websites Server -- plugin loader
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

// ourselves
//
#include "snapwebsites/plugins.h"

// snapwebsites lib
//
#include "snapwebsites/log.h"
#include "snapwebsites/not_used.h"
#include "snapwebsites/snapwebsites.h"
#include "snapwebsites/qstring_stream.h"

// Qt lib
//
#include <QDir>
#include <QMap>
#include <QFileInfo>

// C lib
//
#include <dlfcn.h>
#include <link.h>
#include <sys/stat.h>

// C++ lib
#include <sstream>

// included last
//
#include "snapwebsites/poison.h"


namespace snap
{
namespace plugins
{

plugin_map_t        g_plugins;
plugin_vector_t     g_ordered_plugins;
QString             g_next_register_name;
QString             g_next_register_filename;


/** \brief Load a complete list of available plugins.
 *
 * This is used in the administrator screen to offer users a complete list of
 * plugins that can be installed.
 *
 * \param[in] plugin_paths  The paths to all the Snap plugins.
 *
 * \return A list of plugin names.
 */
snap_string_list list_all(QString const & plugin_paths)
{
    // note that we expect the plugin directory to be clean
    // (we may later check the validity of each directory to make 100% sure
    // that it includes a corresponding .so file)
    snap_string_list const paths(plugin_paths.split(':'));
    snap_string_list filters;
    filters << "*.so";
    snap_string_list result;
    for(auto const & p : paths)
    {
        QDir dir(p);

        dir.setSorting(QDir::Name | QDir::IgnoreCase);

        // TBD: while in development, plugins are in sub-directories
        //      once installed, they are not...
        //      maybe we should have some sort of flag to skip on the
        //      sub-directories once building a package?
        //
        snap_string_list const sub_dirs(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot));
        for(auto const & s : sub_dirs)
        {
            QDir sdir(QString("%1/%2").arg(p).arg(s));

            sdir.setSorting(QDir::Name | QDir::IgnoreCase);
            sdir.setNameFilters(filters);

            result << sdir.entryList(QDir::Files);
        }

        dir.setNameFilters(filters);

        result << dir.entryList(QDir::Files);
    }

    // clean up the list
    for(int p(result.size() - 1); p >= 0; --p)
    {
        if(result[p].length() < 4
        || !result[p].endsWith(".so"))
        {
            // this should never happen
            result.removeAt(p);
        }
        else if(result[p].length() > 6
             && result[p].startsWith("lib"))
        {
            // remove the "lib" and ".so"
            result[p] = result[p].mid(3, result[p].length() - 6);
        }
        else
        {
            // remove the ".so"
            result[p] = result[p].left(result[p].length() - 3);
        }
    }

    result << "server";

    result.sort();

    return result;
}


/** \brief Load all the plugins.
 *
 * Someone who wants to remove a plugin simply deletes it or its
 * softlink at least.
 *
 * \warning
 * This function CANNOT use glob() to read all the plugins in a directory.
 * At this point we assume that each website will use more or less of
 * the installed plugins and thus loading them all is not the right way of
 * handling the loading. Thus we now get a \p list_of_plugins parameter
 * with the name of the plugins we want to dlopen().
 *
 * \todo
 * Look into the shared pointers and unloading plugins, if that ever
 * happens (I don't think it does.)
 *
 * \param[in] plugin_paths  The colon (:) separated list of paths to
 *                          directories with plugins.
 * \param[in] snap  A pointer to the child object loading these plugins.
 *                  This pointer gets passed to the bootstrap() signal.
 * \param[in] server  A pointer to the server to register it as a plugin.
 * \param[in] list_of_plugins  The list of plugins to load.
 *
 * \return true if all the modules were loaded.
 */
bool load(QString const & plugin_paths, snap_child * snap, plugin_ptr_t server, snap_string_list const & list_of_plugins)
{
    g_plugins.insert("server", server.get());

    snap_string_list const paths(plugin_paths.split(':'));

    bool good(true);
    for(snap_string_list::const_iterator it(list_of_plugins.begin());
                                    it != list_of_plugins.end();
                                    ++it)
    {
        QString const name(*it);

        // the Snap server is already added to the list under that name!
        //
        if(name == "server")
        {
            SNAP_LOG_ERROR("error: a plugin cannot be called \"server\".");
            good = false;
            continue;
        }

        // in case we get multiple calls to this function we must make sure that
        // all plugins have a distinct name (i.e. a plugin factory could call
        // this function to load sub-plugins!)
        //
        if(exists(name))
        {
            SNAP_LOG_ERROR("error: two plugins cannot be named the same, found \"")(name)("\" twice.");
            good = false;
            continue;
        }

        // make sure the name is one we consider valid; we may end up
        // using plugin names in scripts and thus want to only support
        // a small set of characters; any other name is refused by
        // the verify_plugin_name() function (which prints an error
        // message already so no need to another one here)
        //
        if(!verify_plugin_name(name))
        {
            good = false;
            continue;
        }

        // check that the file exists, if not we generate an error
        //
        QString const filename(find_plugin_filename(paths, name));
        if(filename.isEmpty())
        {
            SNAP_LOG_ERROR("plugin named \"")(name)("\" not found in the plugin directory. (paths: ")(plugin_paths)(")");
            good = false;
            continue;
        }

        // TBD: Use RTLD_NOW instead of RTLD_LAZY in DEBUG mode
        //      so we discover missing symbols would be nice, only
        //      that would require loading in the correct order...
        //      (see dlopen() call below)
        //

        // load the plugin; the plugin will register itself
        //
        // use some really ugly globals because dlopen() does not give us
        // a way to pass parameters to the plugin factory constructor
        //
        g_next_register_name = name;
        g_next_register_filename = filename;
        void const * const h(dlopen(filename.toUtf8().data(), RTLD_LAZY | RTLD_GLOBAL));
        if(h == nullptr)
        {
            int const e(errno);
            SNAP_LOG_ERROR("error: cannot load plugin file \"")(filename)("\" (errno: ")(e)(", ")(dlerror())(")");
            good = false;
            continue;
        }
        g_next_register_name.clear();
        g_next_register_filename.clear();
//SNAP_LOG_ERROR("note: registering plugin: \"")(name)("\"");
    }

    // set the g_ordered_plugins with the default order as alphabetical,
    // although we check dependencies to properly reorder as expected
    // by what each plugin tells us what its dependencies are
    //
    // TODO: refactor this embedded raw loop to take advantage of
    // C++11's lambda functions. Specifically, we should use std::find_if(),
    // and remove the goto + label. My suggested code is in the #if 0 block
    // below.
    //
    // Alexis Wilke, [07.06.17 14:46]
    // "If you can guarantee that the order won't be affected, we can do it
    // either way. It's sorted alphabetically, then by priority. The priority
    // has precedence over the alphabetical order. But we always end up with a
    // strong order (i.e. if the priority and names do not change plugin A will
    // always be before plugin B if it were before on the previous load)."
    //
    for(auto const & p : g_plugins)
    {
        QString const column_name(QString("|%1|").arg(p->get_plugin_name()));
#if 0
        // TODO: test this new code out to remove the raw loop below...
        auto found_iter = std::find_if( std::begin(g_ordered_plugins), std::end(g_ordered_plugins),
                [&column_name]( auto const & plugin )
                {
                    return plugin->dependencies().indexOf(column_name) >= 0;
                });
        if( found_iter != std::end(g_ordered_plugins) )
        {
            g_ordered_plugins.insert( found_iter, p );
        }
        else
        {
            g_ordered_plugins.push_back(p);
        }
#else
        for(plugin_vector_t::iterator sp(g_ordered_plugins.begin());
                                      sp != g_ordered_plugins.end();
                                      ++sp)
        {
            if((*sp)->dependencies().indexOf(column_name) >= 0)
            {
                g_ordered_plugins.insert(sp, p);
                goto inserted;
            }
        }
        // if not before another plugin, insert at the end by default
        g_ordered_plugins.push_back(p);
inserted:;
#endif
    }

    // bootstrap() functions have to be called in order to get all the
    // signals registered in order! (YES!!! This one for() loop makes
    // all the signals work as expected by making sure they are in a
    // very specific order)
    //
    for(auto const & p : g_ordered_plugins)
    {
        p->bootstrap(snap);
    }

    return good;
}


/** \brief Try to find the plugin using the list of paths.
 *
 * This function searches for a plugin in each one of the specified
 * paths and as:
 *
 * \code
 *    <path>/<name>.so
 *    <path>/lib<name>.so
 *    <path>/<name>/<name>.so
 *    <path>/<name>/lib<name>.so
 * \endcode
 *
 * \todo
 * We may change the naming convention to make use of the ${PROJECT_NAME}
 * in the CMakeLists.txt files. In that case we'd end up with names
 * that include the work plugin as in:
 *
 * \code
 *    <path>/libplugin_<name>.so
 * \endcode
 *
 * \param[in] plugin_paths  The list of paths to check with.
 * \param[in] name  The name of the plugin being searched.
 *
 * \return The full path and filename of the plugin or empty if not found.
 */
QString find_plugin_filename(snap_string_list const & plugin_paths, QString const & name)
{
    int const max_paths(plugin_paths.size());
    for(int i(0); i < max_paths; ++i)
    {
        QString const path(plugin_paths[i]);
        QString filename(QString("%1/%2.so").arg(path).arg(name));
        if(QFile::exists(filename))
        {
            return filename;
        }

        //
        // If not found, see if it has a "lib" at the front of the file:
        //
        filename = QString("%1/lib%2.so").arg(path).arg(name);
        if(QFile::exists(filename))
        {
            return filename;
        }

        //
        // If not found, see if it lives under a named folder:
        //
        filename = QString("%1/%2/%2.so").arg(path).arg(name);
        if(QFile::exists(filename))
        {
            return filename;
        }

        //
        // Last test: check plugin names starting with "lib" in named folder:
        //
        filename = QString("%1/%2/lib%2.so").arg(path).arg(name);
        if(QFile::exists(filename))
        {
            return filename;
        }
    }

    return QString();
}


/** \brief Verify that a name is a valid plugin name.
 *
 * This function checks a string to know whether it is a valid plugin name.
 *
 * A valid plugin name is a string of letters (A-Z or a-z), digits (0-9),
 * and the underscore (_), dash (-), and period (.). Although the name
 * cannot start or end with a dash or a period.
 *
 * \todo
 * At this time this function prints errors out in stderr. This may
 * change later and errors will be sent to the logger. However, we
 * need to verify that the logger is ready when this function gets
 * called.
 *
 * \param[in] name  The name to verify.
 *
 * \return true if the name is considered valid.
 */
bool verify_plugin_name(QString const & name)
{
    if(name.isEmpty())
    {
        SNAP_LOG_ERROR() << "error: an empty plugin name is not valid.";
        return false;
    }
    for(QString::const_iterator p(name.begin()); p != name.end(); ++p)
    {
        if((*p < 'a' || *p > 'z')
        && (*p < 'A' || *p > 'Z')
        && (*p < '0' || *p > '9')
        && *p != '_' && *p != '-' && *p != '.')
        {
            SNAP_LOG_ERROR("error: plugin name \"")(name)("\" includes forbidden characters.");
            return false;
        }
    }
    // Note: we know that name is not empty
    QChar const first(name[0]);
    if(first == '.'
    || first == '-'
    || (first >= '0' && first <= '9'))
    {
        SNAP_LOG_ERROR("error: plugin name \"")(name)("\" cannot start with a digit (0-9), a period (.), or dash (-).");
        return false;
    }
    // Note: we know that name is not empty
    QChar const last(name[name.length() - 1]);
    if(last == '.' || last == '-')
    {
        SNAP_LOG_ERROR("error: plugin name \"")(name)("\" cannot end with a period (.) or dash (-).");
        return false;
    }

    return true;
}


/** \brief Check whether a plugin was loaded.
 *
 * This function searches the list of loaded plugins and returns true if the
 * plugin with the speficied \p name exists.
 *
 * \param[in] name  The name of the plugin to check for.
 *
 * \return true if the plugin is loaded, false otherwise.
 */
bool exists(QString const & name)
{
    return g_plugins.contains(name);
}


/** \brief Register a plugin in the list of plugins.
 *
 * This function is called by plugin factories to register new plugins.
 * Do not attempt to call this function directly or you'll get an
 * exception.
 *
 * \exception plugin_exception
 * If the name is empty, the name does not correspond to the plugin
 * being loaded, or the plugin is being loaded for the second time,
 * then this exception is raised.
 *
 * \param[in] name  The name of the plugin being added.
 * \param[in] p  A pointer to the plugin being added.
 */
void register_plugin(QString const & name, plugin * p)
{
    if(name.isEmpty())
    {
        throw plugin_exception("plugin name missing when registering... expected \"" + name + "\".");
    }
    if(name != g_next_register_name)
    {
        throw plugin_exception("it is not possible to register a plugin (" + name + ") other than the one being loaded (" + g_next_register_name + ").");
    }
#ifdef DEBUG
    // this is not possible if you use the macro, but in case you create
    // your own factory instance by hand, it is a requirement too
    //
    if(name != p->get_plugin_name())
    {
        throw plugin_exception("somehow your plugin factory name is \"" + p->get_plugin_name() + "\" when we were expecting \"" + name + "\".");
    }
#endif
    if(exists(name))
    {
        // this should not happen except if the plugin factory was attempting
        // to register the same plugin many times in a row
        throw plugin_exception("it is not possible to register a plugin more than once (" + name + ").");
    }
    g_plugins.insert(name, p);
}


/** \brief Initialize a plugin.
 *
 * This function initializes the plugin with its filename.
 */
plugin::plugin()
    : f_name(g_next_register_name)
    , f_filename(g_next_register_filename)
    //, f_last_modification(0) -- auto-init
    //, f_version_major(0) -- auto-init
    //, f_version_minor(0) -- auto-init
{
}


/** \brief Define the version of the plugin.
 *
 * This function saves the version of the plugin in the object.
 * This way other systems can access the version.
 *
 * In general you never call that function. It is automatically
 * called by the SNAP_PLUGIN_START() macro. Note that the
 * function cannot be called more than once and the version
 * cannot be zero or negative.
 *
 * \param[in] version_major  The major version of the plugin.
 * \param[in] version_minor  The minor version of the plugin.
 */
void plugin::set_version(int version_major, int version_minor)
{
    if(f_version_major != 0
    || f_version_minor != 0)
    {
        // version was already defined; it cannot be set again
        throw plugin_exception(QString("version of plugin \"%1\" already defined.").arg(f_name));
    }

    if(version_major < 0
    || version_minor < 0
    || (version_major == 0 && version_minor == 0))
    {
        // version cannot be negative or null
        throw plugin_exception(QString("version of plugin \"%1\" cannot be zero or negative (%2.%3).")
                    .arg(f_name).arg(version_major).arg(version_minor));
    }

    f_version_major = version_major;
    f_version_minor = version_minor;
}


/** \brief Retrieve the major version of this plugin.
 *
 * This function returns the major version of this plugin. This is the
 * same version as defined in the plugin factory.
 *
 * \return The major version of the plugin.
 */
int plugin::get_major_version() const
{
    return f_version_major;
}


/** \brief Retrieve the minor version of this plugin.
 *
 * This function returns the minor version of this plugin. This is the
 * same version as defined in the plugin factory.
 *
 * \return The minor version of the plugin.
 */
int plugin::get_minor_version() const
{
    return f_version_minor;
}


/** \brief Retrieve the name of the plugin as defined on creation.
 *
 * This function returns the name of the plugin as defined on
 * creation of the plugin. It is not possible to modify the
 * name for safety.
 *
 * \return The name of the plugin as defined in your SNAP_PLUGIN_START() instantiation.
 */
QString plugin::get_plugin_name() const
{
    return f_name;
}


/** \brief Get the last modification date of the plugin.
 *
 * This function reads the modification date on the plugin file to determine
 * when it was last modified. This date can be used to determine whether the
 * plugin was modified since the last time we ran snap with this website.
 *
 * \return The last modification date and time in micro seconds.
 */
int64_t plugin::last_modification() const
{
    if(0 == f_last_modification)
    {
        // read the info only once
        struct stat s;
        if(stat(f_filename.toUtf8().data(), &s) == 0)
        {
            // should we make use of the usec mtime when available?
            f_last_modification = static_cast<int64_t>(s.st_mtime * 1000000LL);
        }
        // else TBD: should we throw here?
    }

    return f_last_modification;
}


/** \brief Return the URL to an icon representing your plugin.
 *
 * Each plugin can be assigned an icon. The path to that icon has
 * to be returned by this function. It us used whenever we build
 * lists representing plugins.
 *
 * The default function returns the path to a default plugin image.
 *
 * The image must be a 64x64 picture. The CSS will enforce the size
 * so it will possibly get stretched in weird ways if you did not
 * use the correct size to start withicon.
 *
 * \return The path to the default plugin icon.
 */
QString plugin::icon() const
{
    return "/images/snap/plugin-icon-64x64.png";
}


/** \fn QString plugin::description() const;
 * \brief Return a string describing this plugin.
 *
 * This function must be overloaded. It is expected to return a description
 * of the plugin. The string may include HTML. It should be limited to
 * inline HTML tags, although it can include header tags and paragraphs
 * too.
 *
 * The description should be relatively brief. The plugins also offer a
 * help URI which sends users to our snapwebsites.org website (or the
 * author of the plugin wbesite) so we do not need to go crazy in the
 * description. That external help page can go at length.
 *
 * \return A string representing the plugin description.
 */


/** \brief Comma separated list of tags.
 *
 * This function returns a list of comma separated tags. It is used
 * to categorize a plugin so that way it makes it easier to find
 * in the large list presented to users under the Plugin Selector.
 *
 * For example, plugins that are used to test other plugins and
 * other parts of the system can be assigned the tag "test".
 *
 * By default plugins are not assigned any tags. This means they
 * will appear under the "others" special tag. You can always
 * make this explicit or offer the "others" tag along with
 * more useful tags, for example: "image,others".
 *
 * \return A list of tags, comma separated.
 */
QString plugin::plugin_categorization_tags() const
{
    return "";
}


/** \brief Return the URI to the help page for this plugin.
 *
 * Each plugin can be assigned a help page. By default, we
 * define the URL as:
 *
 * \code
 *      http://snapwebsites.org/help/plugin/<plugin-name>
 * \endcode
 *
 * If you program your own plugin, you are expected to overload
 * this function and send users to your own website.
 *
 * \return The URI to the help page for this plugin.
 */
QString plugin::help_uri() const
{
    return QString("http://snapwebsites.org/help/plugin/%1").arg(f_name);
}


/** \brief Return the path to the settings page for this plugin.
 *
 * Each plugin can be assigned a settings page. By default, this
 * function returns an empty string meaning that no settings are
 * available.
 *
 * The function has to return an absolute path to a site borne
 * page that will allow an administrator to change various
 * settings that this plugin is handling.
 *
 * Some plugins may not themelves offer settings, but they may
 * still return a settings path to another plugin that is setup
 * to handle their settings (i.e. the info plugin handles many
 * other lower level plugins settings.)
 *
 * By default the function returns an empty path, meaning that the
 * settings button needs to be disabled in the plugin selector.
 *
 * \return The path to the settings page for this plugin.
 */
QString plugin::settings_path() const
{
    return QString();
}


/** \fn QString plugin::dependencies() const;
 * \brief Return a list of required dependencies.
 *
 * This function returns a list of dependencies, plugin names written
 * between pipes (|). All plugins have at least one dependency since
 * most plugins will not work without the base plugin (i.e. "|server|"
 * is the bottom most base you can use in your plugin).
 *
 * At this time, the "content" and "test_plugin_suite" plugins have no
 * dependencies.
 *
 * \note
 * Until "links" is merged with "content", it will depend on "content"
 * so that way "links" signals are registered after "content" signals.
 *
 * \return A list of plugin names representing all dependencies.
 */


/** \fn void plugin::bootstrap(snap_child * snap)
 * \brief Bootstrap this plugin.
 *
 * The bootstrap virtual function is used to initialize the plugins. At
 * this point all the plugins are loaded, however, they are not yet
 * ready to receive signals because all plugins are not yet connected.
 * The bootstrap() function is actually used to get all the listeners
 * registered.
 *
 * Note that the plugin implementation loads all the plugins, sorts them,
 * then calls their bootstrap() function. Afterward, the init() function
 * is likely called. The bootstrap() registers signals and the server
 * init() signal can be used to send signals since at that point all the
 * plugins are properly installed and have all of their signals registered.
 *
 * \note
 * This is a pure virtual which is not implemented here so that way your
 * plugin will crash the server if you did not implement this function
 * which is considered mandatory.
 *
 * \param[in,out] snap  The snap child process.
 */


/** \brief Run an update.
 *
 * This function is a stub that does nothing. It is here so any plug in that
 * does not need an update does not need to define an "empty" function.
 *
 * At this time the function ignores the \p last_updated parameter and 
 * it always returns the same date: Jan 1, 1990 at 00:00:00.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t plugin::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    // in a complete implementation you have entries like this one:
    //SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Run a dynamic update.
 *
 * This function is called after the do_update(). This very version is
 * a stub that does nothing. It can be overloaded to create content in
 * the database after the content.xml was installed fully. In other
 * words, the dynamic update can make use of data that the content.xml
 * will be adding ahead of time.
 *
 * At this time the function ignores the \p last_updated parameter and 
 * it always returns the same date: Jan 1, 1990 at 00:00:00.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t plugin::do_dynamic_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    // in a complete implementation you have entries like this one:
    //SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, dynamic_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Retrieve a pointer to an existing plugin.
 *
 * This function returns a pointer to a plugin that was previously loaded
 * with the load() function. If you only need to test whether a plugin
 * exists, then you should use exists() instead.
 *
 * \note
 * This function should not be called until your plugin bootstrap() function
 * is called. Before then, there are no guarantees that the plugin was already
 * loaded.
 *
 * \param[in] name  The name of the plugin to retrieve.
 *
 * \return This function returns a pointer to the named plugin, or nullptr
 * if the plugin was not loaded.
 *
 * \sa load()
 * \sa exists()
 */
plugin * get_plugin(QString const & name)
{
    return g_plugins.value(name, nullptr);
}


/** \brief Retrieve the list of plugins.
 *
 * This function returns the list of plugins that were loaded in this
 * session. Remember that plugins are loaded each time a client accesses
 * the server.
 *
 * This means that the list is complete only once you are in the snap
 * child and after the plugins were initialized. If you are in a plugin,
 * this means the list is not complete in the constructor. It is complete
 * anywhere else.
 *
 * \return List of plugins in a map indexed by plugin name
 *         (i.e. alphabetical order).
 */
plugin_map_t const & get_plugin_list()
{
    return g_plugins;
}


/** \brief Retrieve the list of plugins.
 *
 * This function returns the list of plugins that were sorted, once
 * loaded, using their dependencies. This is a vector since we need
 * to keep a very specific order of the plugins.
 *
 * This list is empty until all the plugins were loaded.
 *
 * This list should be empty when your plugins constructors are called.
 *
 * \return The sorted list of plugins in a vector.
 */
plugin_vector_t const & get_plugin_vector()
{
    return g_ordered_plugins;
}


/** \brief Read a plugin information.
 *
 * This constructor reads all the available information from the named
 * plugin.
 */
plugin_info::plugin_info(QString const & plugin_paths, QString const & name)
{
    if(name == "server")
    {
        // this is a special case, the user is requesting information about
        // the snapserver (snapwebsites.cpp) and not a plugin per-se.
        //
        f_name =                name;
        f_filename =            "snapserver";
        f_last_modification =   0;
        f_icon =                "/images/snap/snap-logo-64x64.png";
        f_description =         "The Snap! Websites server defines the base plugin used by the snap system.";
        f_categorization_tags = "core";
        f_help_uri =            "http://snapwebsites.org/help/plugin/server";
        f_settings_path =       "/admin/plugins";
        f_dependencies =        "";
        f_version_major =       SNAPWEBSITES_VERSION_MAJOR;
        f_version_minor =       SNAPWEBSITES_VERSION_MINOR;

        // too bad that dlopen() returns opaque handles only...
        // that being said, we can still walk the walk and
        // find the full path to the currently running binary
        //
        struct unknown_structures  // <- yuck
        {
            void *                      f_unused[3];
            struct unknown_structures * f_pointer;
        };
        struct unknown_structures * p(reinterpret_cast<struct unknown_structures *>(dlopen(nullptr, RTLD_LAZY | RTLD_GLOBAL)));
        if(p != nullptr)
        {
            p = p->f_pointer;
            if(p != nullptr)
            {
                struct link_map * map(reinterpret_cast<struct link_map *>(p->f_pointer));
                if(map != nullptr)
                {
                    if(map->l_name != nullptr)
                    {
                        // full path to the snapwebsites.so library
                        f_filename = QString::fromUtf8(map->l_name);

                        if(!f_filename.isEmpty())
                        {
                            struct stat s;
                            if(stat(f_filename.toUtf8().data(), &s) == 0)
                            {
                                // should we make use of the usec mtime when available?
                                f_last_modification = static_cast<int64_t>(s.st_mtime * 1000000LL);
                            }
                        }
                    }
                }
            }
        }
        return;
    }

    snap_string_list const paths(plugin_paths.split(':'));
    QString const filename(find_plugin_filename(paths, name));
    if(filename.isEmpty())
    {
        throw plugin_exception(QString("plugin named \"%1\" not found.").arg(name));
    }

    // "normal" load of the plugin... (We do not really have a choice)
    //
    // Note that this is the normal low level load, that means the plugin
    // will not get its bootstrap() and other initialization functions
    // called... we will be limited to a very small number of functions.
    //
    // XXX: allow for the non-adding to the global list?
    //      in general we should be fine since their signals will not
    //      be installed, however, the plugin_exists() function will
    //      return 'true', which is not correct
    //
    g_next_register_name = name;
    g_next_register_filename = filename;
    void const * const h(dlopen(filename.toUtf8().data(), RTLD_LAZY | RTLD_GLOBAL));
    if(h == nullptr)
    {
        int const e(errno);
        std::stringstream ss;
        ss << "error: cannot load plugin file \"" << filename << "\" (errno: " << e << ", " << dlerror() << ")";
        throw plugin_exception(ss.str());
    }
    g_next_register_name.clear();
    g_next_register_filename.clear();

    plugin * p(get_plugin(name));
    if(p == nullptr)
    {
        std::stringstream ss;
        ss << "error: cannot find plugin \"" << name << "\", even though the loading was successful.";
        throw plugin_exception(ss.str());
    }

    f_name =                name;
    f_filename =            filename;
    f_last_modification =   p->last_modification();
    f_icon =                p->icon();
    f_description =         p->description();
    f_categorization_tags = p->plugin_categorization_tags();
    f_help_uri =            p->help_uri();
    f_settings_path =       p->settings_path();
    f_dependencies =        p->dependencies();
    f_version_major =       p->get_major_version();
    f_version_minor =       p->get_minor_version();
}


/** \brief Retrieve the name of the plugin.
 *
 * This function returns the name of this plugin.
 *
 * \return The name of this plugin.
 */
QString const & plugin_info::get_name() const
{
    return f_name;
}


/** \brief Get the filename of the plugin.
 *
 * Advanced informaton, the plugin path on the server. This is really
 * only useful for developers and administrators to make sure things
 * are in the right place.
 *
 * \return The path to this plugin.
 */
QString const & plugin_info::get_filename() const
{
    return f_filename;
}


/** \brief Get the last modification time of this plugin.
 *
 * This function reads the mtime in microseconds of the file attached
 * to this plugin and returns that value.
 *
 * Assuming people do not temper with the modification of the file,
 * this represents the last time the plugin was compiled and packaged.
 *
 * \return The time of the last modifications of this plugin in microseconds.
 */
int64_t plugin_info::get_last_modification() const
{
    return f_last_modification;
}


/** \brief Retrieve path to the icon.
 *
 * This function returns the local path to the icon representing this plugin.
 *
 * \return A path to the plugin icon.
 */
QString const & plugin_info::get_icon() const
{
    return f_icon;
}


/** \brief Retrieve URI to the help page for this plugin.
 *
 * This function returns the URI to the help page for this plugin.
 * In most cases this is a URI to an external website that describes
 * the plugin in details: what it does, what it does not do, what
 * the settings are for, how to set it up in this way or that way, etc.
 *
 * \return The URI to the help page.
 */
QString const & plugin_info::get_help_uri() const
{
    return f_help_uri;
}


/** \brief Retrieve path to the settings page for this plugin.
 *
 * This function returns the path to the main settings of this plugin.
 *
 * \return The path to the settings page.
 */
QString const & plugin_info::get_settings_path() const
{
    return f_settings_path;
}


/** \brief Retrieve the plugin description.
 *
 * Each plugin has a small description defined in the code. This
 * description is shown when you have to deal with the plugin
 * in such places as the page allowing you to install / uninstall
 * the plugin.
 *
 * \return The description of the plugin.
 */
QString const & plugin_info::get_description() const
{
    return f_description;
}


/** \brief Retrieve the plugin last of tags used for categorization.
 *
 * Each plugin can be given a set of tags, comma separated, to define
 * its category. For example, core plugins are generally marked with
 * the "core" tag. The "base" tag is generally used by the set of
 * plugins that one cannot uninstall because otherwise the system
 * would break. Etc.
 *
 * \return The list of comma separated tags.
 */
QString const & plugin_info::get_plugin_categorization_tags() const
{
    return f_categorization_tags;
}


/** \brief Retrieve the plugin dependencies.
 *
 * Most plugins have a set of dependencies which must be
 * initialized before they themselves get initialized. This
 * way they will receive events after those dependencies.
 * In other words, we have a system which is well defined
 * (deterministic in terms of who receives what and when.)
 *
 * This function returns the raw string defined in the plugins
 * which means the list of plugin names are written between
 * pipes (|) characters.
 *
 * \return The dependencies of the plugin.
 */
QString const & plugin_info::get_dependencies() const
{
    return f_dependencies;
}


/** \brief Retrieve the major version of the plugin.
 *
 * This function returns the major version of the plugin as a number.
 *
 * \return The major version of this plugin.
 */
int32_t plugin_info::get_version_major() const
{
    return f_version_major;
}


/** \brief Retrieve the minor version of the plugin.
 *
 * This function returns the minor version of the plugin as a number.
 *
 * \return The minor version of this plugin.
 */
int32_t plugin_info::get_version_minor() const
{
    return f_version_minor;
}




} // namespace plugins
} // namespace snap
// vim: ts=4 sw=4 et
