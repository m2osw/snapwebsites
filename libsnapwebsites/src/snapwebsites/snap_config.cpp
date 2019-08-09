// Snap Websites Server -- configuration reader
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include "snapwebsites/snap_config.h"

// snapwebsites lib
//
#include "snapwebsites/log.h"
#include "snapwebsites/qlockfile.h"
#include "snapwebsites/snap_thread.h"


// snapdev lib
//
#include <snapdev/not_reached.h>


// Qt lib
//
#include <QDateTime>


// boost lib
//
#include <boost/algorithm/string.hpp>


// C++ lib
//
#include <memory>
#include <sstream>


// C lib
//
#include <syslog.h>
#include <unistd.h>


// included last
//
#include "snapdev/poison.h"



namespace snap
{

namespace
{


/** \brief All the configurations are saved in one object.
 *
 * At this point we decided that there was no need for us to support
 * dynamic configurations, i.e. configurations that you can allocate,
 * load, tweak/use, then drop. The only reason why you'd want to
 * re-allocate a configuration would be to satisfy a RELOADCONFIG
 * event which we do not yet support (properly) in most cases
 * because we copy the configuration information in various places
 * (and at times these are used to do things like connect to another
 * server...)
 *
 * So at this point we do not allow such dynamism. Even if we were,
 * we would want you to make use of this interface instead.
 */
std::shared_ptr<snap_configurations>        g_configurations;


/** \brief Mutex used to make the configuration thread safe.
 *
 * This object is used whenever the configuration is accessed in
 * order to make it thread safe.
 */
std::shared_ptr<snap_thread::snap_mutex>    g_mutex;


/** \brief The path to the configuration files.
 *
 * This variable holds the path to the various configuration files.
 * The default is "/etc/snapwebsites". Most daemon will offer you
 * a way to change that value with a "--config" command line option.
 *
 * Once one configuration file was read, that parameter becomes
 * immutable.
 */
std::string                                 g_configurations_path = "/etc/snapwebsites";


/** \brief true once we started reading files.
 *
 * The parameter goes from false to true once we read the very first
 * configuration file. This allows us to prevent changing the path
 * to the configuration data past that call.
 *
 * The default is false.
 */
bool                                        g_configuration_has_started = false;



class snap_config_file
{
public:
    typedef std::shared_ptr<snap_config_file>       pointer_t;
    typedef std::map<std::string, pointer_t>        map_t;

                        snap_config_file(std::string const & configuration_filename, std::string const & override_filename);

    std::string const & get_configuration_filename() const;
    std::string const & get_override_filename() const;

    bool                exists();

    //void                clear();
    void                read_config_file();
    bool                write_config_file(bool override_file);

    std::string         get_parameter(std::string const & parameter_name) const;
    bool                has_parameter(std::string const & parameter_name) const;
    void                set_parameter(std::string const & parameter_name, std::string const & value);
    snap_configurations::parameter_map_t const &
                        get_parameters() const;
    void                set_parameters(snap_configurations::parameter_map_t const & params);

private:
    bool                actual_read_config_file(std::string const & filename, bool quiet);
    bool                actual_write_config_file(std::string const & filename);

    std::string const                       f_configuration_filename;
    std::string const                       f_override_filename;
    snap_configurations::parameter_map_t    f_parameters = snap_configurations::parameter_map_t();
    bool                                    f_exists = false;
};



/** \brief A map of configurations.
 *
 * Most of our systems load configuration files with a "hard coded" filename
 * which can be accessed from many different locations. That 
 */
snap_config_file::map_t      g_config_files;



/** \brief The configuration file.
 *
 * The constructor saves the filename of the configuration file.
 * The filename cannot be modified later.
 *
 * \param[in] configuration_filename  The filename for this configuration file.
 * \param[in] override_filename  The name of the override, that file is
 *            checked first and if a field exists in it, that value is used.
 */
snap_config_file::snap_config_file(std::string const & configuration_filename, std::string const & override_filename)
    : f_configuration_filename(configuration_filename)
    , f_override_filename(override_filename)
{
    // empty
}


/** \brief Get the filename of this configuration file.
 *
 * This function gets the filename of this configuration file as was
 * defined on the constructor.
 *
 * \return The filename of this configuration file.
 */
std::string const & snap_config_file::get_configuration_filename() const
{
    return f_configuration_filename;
}


/** \brief Get the override_filename of this configuration file.
 *
 * This function gets the override_filename of this configuration file as was
 * defined on the constructor.
 *
 * \return The override_filename of this configuration file.
 */
std::string const & snap_config_file::get_override_filename() const
{
    return f_override_filename;
}


//void snap_config_file::clear()
//{
//    f_parameters.clear();
//}


/** \brief Return the value of the f_exists flag.
 *
 * This function lets you know whether the file exists or not. By default
 * it is set to false until the read_config_file() gets called. It may
 * remain set to false if the file is not found at that time.
 *
 * \return true if configuration file exists, false otherwise
 *
 * \sa read_config_file()
 */
bool snap_config_file::exists()
{
    return f_exists;
}


/** \brief Read the configuration file into memory.
 *
 * This function reads the configuration file from disk to memory.
 * It will stay there until the process leaves.
 *
 * The file is searched in the specified configuration path
 * and under a sub-directory of that configuration path named
 * "snapwebsites.d".
 *
 * \code
 *      <configuration path>/<configuration filename>
 *      <configuration path>/snapwebsites.d/<configuration filename>
 * \endcode
 *
 * This allows you to NOT modify the original .conf files, and instead
 * edit a version where you define just the few fields you want to
 * modify within the "snapwebsites.d" sub-directory.
 *
 * \note
 * Sets the f_exists flag.
 *
 * \sa actual_read_config_file()
 * \sa exists()
 */
void snap_config_file::read_config_file()
{
    // first use of the g_configurations_path variable
    // now the set_configuration_path() function cannot be called.
    //
    g_configuration_has_started = true;

    // if the filename includes any "." or "/", it is not one of our
    // files so we instead load the file as is
    //
    std::string::size_type const pos(f_configuration_filename.find_first_of("./"));
    if(pos != std::string::npos)
    {
        // we use 'true' (i.e. "keep quiet") here because in some cases
        // it is normal that the file does not exist
        //
        f_exists = actual_read_config_file(f_configuration_filename, true);

        // give a chance to other configuration files to have one
        // override
        //
        // TODO: later we want to support any number with an "'*' + sort"
        //       (like apache2 and other daemons do)
        //
        if(f_exists && !f_override_filename.empty())
        {
            actual_read_config_file(f_override_filename, true);
        }
    }
    else
    {
        f_exists = actual_read_config_file(g_configurations_path + "/" + f_configuration_filename + ".conf", false);

        // second try reading a file of the same name in a sub-directory named
        // "snapwebsites.d"; we have to do it last because we do overwrite
        // parameters (i.e. we keep the very last instance of each parameter
        // read from files.)
        //
        if(f_exists)
        {
            // TODO: allow for a different sub-directory name to be more
            //       versatiled
            //
            actual_read_config_file(g_configurations_path + "/snapwebsites.d/" + f_configuration_filename + ".conf", true);
        }
    }
}


/** \brief Read the configuration file itself.
 *
 * This is the function that actually reads the file. We use a sub-function
 * so that way we can read files in a sub-directory, such as "snapwebsites.d,"
 * with user modifications.
 *
 * This function attempts to read one file. The name of the sub-directory
 * is determined by the caller.
 *
 * \exception snap_configurations_exception_config_error
 * If there is an error reading the file or a parsing error, this exception
 * is raised.
 *
 * \param[in] filename  The name of the file to read from.
 * \param[in] quiet     Whether to keep quiet about missing files (do not
 *                      raise exception, just return false).
 *
 * \return true if read, false on failure to read file.
 *
 * \sa read_config_file()
 */
bool snap_config_file::actual_read_config_file(std::string const & filename, bool quiet)
{
    // read the configuration file now
    QFile c(QString::fromUtf8(filename.c_str()));
    //
    if( !c.exists() && quiet )
    {
        return false;
    }
    //
    if(!c.open(QIODevice::ReadOnly))
    {
        // if for nothing else we need to have the list of plugins so we always
        // expect to have a configuration file... if we're here we could not
        // read it, unfortunately
        std::stringstream ss;
        ss << "cannot read configuration file \"" << filename << "\"";
        SNAP_LOG_WARNING(ss.str())(".");
        syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
        //throw snap_configurations_exception_config_error(ss.str());
        return false;
    }

    // read the configuration file variables as parameters
    //
    std::string prefix;
    char buf[1024];
    for(int line(1); c.readLine(buf, sizeof(buf)) > 0; ++line)
    {
        // make sure the last byte is '\0'
        buf[sizeof(buf) - 1] = '\0';
        int len(static_cast<int>(strlen(buf)));
        if(len == 0 || (buf[len - 1] != '\n' && buf[len - 1] != '\r'))
        {
            std::stringstream ss;
            ss << "line " << line << " in \"" << filename << "\" is too long";
            SNAP_LOG_FATAL(ss.str())(".");
            syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
            throw snap_configurations_exception_config_error(ss.str());
        }
        buf[len - 1] = '\0';
        --len;
        while(len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        {
            --len;
            buf[len] = '\0';
        }
        if(len == 0)
        {
            // empty line
            continue;
        }
        char * n(buf);
        while(isspace(*n))
        {
            ++n;
        }
        if(*n == '#' || *n == ';' || *n == '\0')
        {
            // comment or empty line
            continue;
        }
        if(*n == '[')
        {
            // support for INI files starts here, we take the name between
            // [ and ] and save it as a "prefix" to our follow list of
            // names until another section appears
            //
            do
            {
                ++n;
            }
            while(isspace(*n));
            char * v(n);
            while(*v != ']' && *v != '\0' && !isspace(*v) && *v != ':')
            {
                ++v;
            }
            char * e(v);
            while(isspace(*v))
            {
                ++v;
            }
            // Note: we do not support "[]" to reset back to "global"
            //       variables; just place your global variables first
            //
            if(*v != ']' || n == e)
            {
                std::stringstream ss;
                ss << "invalid section on line " << line << " in \"" << filename << "\", no equal sign found";
                SNAP_LOG_FATAL(ss.str())(".");
                syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
                throw snap_configurations_exception_config_error(ss.str());
            }
            // right away add the "::" to the prefix so we can use it as is
            // when we find a variable
            //
            prefix = std::string(n, e - n) + "::";
        }
        else
        {
            char * v(n);
            while(*v != '=' && *v != '\0')
            {
                // TODO verify that the name is only ASCII? (probably not too
                //      important because if not it will be ignored anyway)
                //      Note that the layout expects names including colons (:)
                //      as a namespace separator: layout::layout, layout::theme.
                ++v;
            }
            if(*v != '=')
            {
                std::stringstream ss;
                ss << "invalid variable on line " << line << " in \"" << filename << "\", no equal sign found";
                SNAP_LOG_FATAL(ss.str())(".");
                syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
                throw snap_configurations_exception_config_error(ss.str());
            }
            char * e;
            for(e = v; e > n && isspace(e[-1]); --e);
            *e = '\0';
            do
            {
                ++v;
            }
            while(isspace(*v));
            for(e = v + strlen(v); e > v && isspace(e[-1]); --e);
            *e = '\0';
            if(v != e && ((v[0] == '\'' && e[-1] == '\'') || (v[0] == '"' && e[-1] == '"')))
            {
                // remove single or double quotes
                //
                v++;
                e[-1] = '\0';
            }

            // restore the escaped newlines if any
            // right now this is the only thing we escape, the rest can
            // stay as it is and still works
            //
            std::string value(v);
            boost::algorithm::replace_all(value, "\\n", "\n");

            // keep the last read value in that section
            //
            f_parameters[prefix + n] = value;
        }
    }

    return true;
}


/** \brief Write the data back to the configuration file.
 *
 * This function writes the existing data back to the configuration file.
 *
 * This function is somewhat dangerous in the sense that it destroys all
 * the comments, empty lines, etc. That information is not while reading
 * the input file, so when saving the file back, it saves raw data.
 *
 * It is expected that you use this function only for configuration files
 * used for things other than administrative configuration files.
 *
 * \warning
 * The \p override_file flag is ignored if the override_filename is
 * not defined in this configuration file. In other words, if you create
 * a configuration file without an override, this function cannot then
 * save the newdata in an override file.
 *
 * \warning
 * If the configuration filename does not include any period or slash,
 * it is considered to be a well known configuration filename and in that
 * case the \p override_file is ignored since the name is built using
 * the main configuration filename (by adding /snapwebsites.d/ to the
 * path of the configuration files.)
 *
 * \note
 * See also the manager::replace_configuration_value() function in
 * snapmanager/lib/installer.cpp (we may at some point want to move
 * that code to the main snap library or even a contrib library to
 * allow for easy editing of configuration files.)
 *
 * \param[in] override_file  Whether the override filename is used to save
 *            this configuration back to drive.
 *
 * \return true if the configuration was saved successfully.
 */
bool snap_config_file::write_config_file(bool override_file)
{
    // the g_configuration_has_started should already be true since
    // a read should always happen before a write, but just in case
    // set that variable to true now
    //
    g_configuration_has_started = true;

    // if the filename includes any "." or "/", it is not one of our
    // files so we instead load the file as is (i.e. filename is
    // expected to be a full name, so we ignore our path)
    //
    std::string::size_type const pos(f_configuration_filename.find_first_of("./"));
    if(pos != std::string::npos)
    {
        if(override_file && !f_override_filename.empty())
        {
            return actual_write_config_file(f_override_filename);
        }
        else
        {
            return actual_write_config_file(f_configuration_filename);
        }
    }
    else
    {
        if(override_file)
        {
            return actual_write_config_file(g_configurations_path + "/snapwebsites.d/" + f_configuration_filename + ".conf");
        }
        else
        {
            return actual_write_config_file(g_configurations_path + "/" + f_configuration_filename + ".conf");
        }
    }
}


/** \brief Write the configuration to file.
 *
 * This is the function that actually writes the configuration data to file.
 * We use a sub-function so that way we can handle multiple cases in a clear
 * manner in the main write_config_file() function.
 *
 * \param[in] filename  The name of the file to read from.
 *
 * \return true if written, false on failure to write file.
 *
 * \sa write_config_file()
 */
bool snap_config_file::actual_write_config_file(std::string const & filename)
{
    // write to the configuration file now
    //
    QLockFile c(QString::fromUtf8(filename.c_str()));
    if(!c.open(QIODevice::WriteOnly))
    {
        // could not write here, it may be an EPERM
        //
        int const e(errno);
        SNAP_LOG_WARNING("could not open \"")(filename)("\" for writing. (errno: ")(e)("--")(strerror(e))(")");
        return false;
    }

    QDateTime now(QDateTime::currentDateTime());

    // read the configuration file variables as parameters
    //
    c.write(QString("# This file was auto-generated by snap_config.cpp on %1 at %2.\n"
            "# Making modifications here is likely safe unless the tool handling this\n"
            "# configuration file is actively working on it while you do the edits.\n")
                    .arg(now.toString("yyyy/MM/dd"))
                    .arg(now.toString("HH:mm:ss"))
                    .toUtf8());

    // then write on line per parameter
    //
    for(auto p : f_parameters)
    {
        // make sure that the field name and content do not include newline
        // characters, instead we replace them with the same syntax as in
        // C/C++ so '\\' and 'n',
        //
        std::string::size_type const pos(p.first.find_first_of("./"));
        if(pos == std::string::npos)
        {
            QByteArray data(p.first.c_str());
            data += '=';
            QByteArray value(p.second.c_str());
            value.replace("\n", "\\n");
            data += value;
            data += '\n';  // our actual new line, do not replace this one
            c.write(data);
        }
        //else -- ignore parameters with invalid names
    }

    return true;
}


/** \brief Retrieve the value of this parameter.
 *
 * This function searches for the named parameter. If it exists, then
 * its value gets returned. If it does not exist, then an empty string
 * is returned.
 *
 * To know whether the parameter exists and its value is an empty string,
 * then call has_parameter().
 *
 * \param[in] parameter_name  The name of the parameter to retrieve.
 *
 * \return The value of the parameter or the empty string if the parameter
 *         is not defined.
 *
 * \sa has_parameter()
 */
std::string snap_config_file::get_parameter(std::string const & parameter_name) const
{
    auto const & it(f_parameters.find(parameter_name));
    if(f_parameters.end() != it)
    {
        return it->second;
    }

    return std::string();
}


/** \brief Check whether this configuration file has a certain parameter.
 *
 * This function searches for the specified parameter by name and if
 * found return true, otherwise false.
 *
 * \warning
 * If you set that parameter, then this function will return true whether
 * the parameter was found in the original file or not.
 *
 * \param[in] name  The name of the parameter to search.
 *
 * \return true if the parameter is defined in this file.
 */
bool snap_config_file::has_parameter( std::string const & name ) const
{
    return f_parameters.find( name ) != f_parameters.end();
}


/** \brief Replace or create a parameter.
 *
 * This function saves the specified value in the named parameter.
 *
 * If the parameter did not exist yet, it exists upon return.
 *
 * \param[in] parameter_name  The name of the parameter to retrieve.
 * \param[in] value  The parameter's value.
 *
 * \return The value of the parameter or the empty string if the parameter
 *         is not defined.
 *
 * \sa has_parameter()
 */
void snap_config_file::set_parameter(std::string const & parameter_name, std::string const & value)
{
    f_parameters[parameter_name] = value;
}


/** \brief Return a reference to all the parameters defined in this file.
 *
 * This function returns a reference to the parameter map defined in
 * this file. The map is not editable.
 *
 * \return The configuration file parameter map.
 */
snap_configurations::parameter_map_t const & snap_config_file::get_parameters() const
{
    return f_parameters;
}


/** \brief Add the specified params to the parameters.
 *
 * This function copies the specified parameters \p params to the
 * list of parameters of this config file.
 *
 * If the configuration file already had such a parameter, it gets
 * overwritten.
 *
 * \param[in] params  A map of parameters to save in this configuration file.
 */
void snap_config_file::set_parameters( snap_configurations::parameter_map_t const & params )
{
    f_parameters.insert(params.begin(), params.end());
}


/** \brief Get the named configuration file.
 *
 * This function retrieves the named configuration file. If the file is
 * not yet loaded, the function loads the file at this point.
 *
 * \param[in] configuration_filename  The name of the configuration file to retrieve.
 * \param[in] override_filename  The name of the override, that file is
 *            checked first and if a field exists in it, that value is used.
 * \param[in] quiet                   Silently fail if one cannot read the config file.
 *
 * \return The shared pointer to a snap_config_file object.
 */
snap_config_file::pointer_t get_configuration
    ( std::string const & configuration_filename
    , std::string const & override_filename
    , bool const quiet = false
    )
{
    auto const & it(std::find_if(
                      g_config_files.begin()
                    , g_config_files.end()
                    , [configuration_filename](auto const & configuration)
                    {
                        return configuration_filename == configuration.second->get_configuration_filename();
                    }));
    if(g_config_files.end() == it)
    {
        // we did not find that configuration, it was not yet loaded,
        // load it now
        //
        snap_config_file::pointer_t conf(std::make_shared<snap_config_file>(configuration_filename, override_filename));
        g_config_files[configuration_filename] = conf;
        conf->read_config_file();
        if( !quiet && !conf->exists() )
        {
            // Exit the process as we have a critical error.
            //
            throw snap_configurations_exception_config_error(
                      "loading configuration file \""
                    + configuration_filename
                    + "\" failed: File is missing.");
        }
        return conf;
    }

    if(it->second->get_override_filename() != override_filename)
    {
        // do not allow a configuration file to have varying overrides
        //
        std::stringstream ss;
        ss << "loading configuration file \""
           << configuration_filename
           << "\" with two different override filenames: \""
           << it->second->get_override_filename()
           << "\" and \""
           << override_filename
           << "\"";
        SNAP_LOG_FATAL(ss.str())(".");
        syslog( LOG_CRIT, "%s in snap_config.cpp: get_configuration()", ss.str().c_str() );
        throw snap_configurations_exception_config_error(ss.str());
    }

    return it->second;
}



}
// no name namespace




/** \brief Initialize the snap configuration object.
 *
 * The constructor is used to allow for deleting other constructors.
 */
snap_configurations::snap_configurations()
{
}


/** \brief Get an instance pointer to the configuration files.
 *
 * This function returns a shared pointer to the configuration
 * instance allocated for this process.
 *
 * Note that most of the configuration functions are not thread
 * safe. If you are working on a multithread application, make
 * sure to load all the configuration files you need at initialization
 * before you create threads, or make sure the other threads never
 * access the configuration data.
 *
 * \warning
 * The implementation of the snap_config objects is thread safe, but
 * only if you make sure that you call this function once before
 * you create any threads. In other words, this very function is
 * not actually thread safe.
 *
 * \return A pointer to the snap_configurations object.
 */
snap_configurations::pointer_t snap_configurations::get_instance()
{
    if(g_configurations == nullptr)
    {
        // WARNING: cannot use std::make_shared<>() because the singleton
        //          has a private constructor
        //

        // first do all allocations, so if one fails, it is exception safe
        //
        // WARNING: remember that shared_ptr<>() are not safe as is
        //          (see SNAP-507 for details)
        //
        std::shared_ptr<snap_configurations> configurations; // use reset(), see SNAP-507, can't use make_shared() because constructor is private
        configurations.reset(new snap_configurations());

        std::shared_ptr<snap_thread::snap_mutex> mutex(std::make_shared<snap_thread::snap_mutex>());

        // now that all allocations were done, save the results
        //
        g_configurations.swap(configurations);
        g_mutex.swap(mutex);
    }

    return g_configurations;
}


/** \brief Return a reference to the current configuration path.
 *
 * This function returns a reference to the configuration path used
 * by this process.
 *
 * \return A reference to the configuration path string.
 */
std::string const & snap_configurations::get_configuration_path() const
{
    snap_thread::snap_lock lock(*g_mutex);
    return g_configurations_path;
}


/** \brief Change the path to the configuration files.
 *
 * Some (should be all...) daemons may let the administrator specify
 * the path to the configuration files. This path has to be set early,
 * before you read any configuration file (after, it will throw.)
 *
 * The path is used to read all the files.
 *
 * \exception snap_configurations_exception_too_late
 * This exception is raised if the function gets called after one of
 * the functions that allows to read data from the configuration file.
 * Generally, you want to call this function very early on in your
 * initialization process.
 *
 * \param[in] path  The new path.
 */
void snap_configurations::set_configuration_path(std::string const & path)
{
    snap_thread::snap_lock lock(*g_mutex);

    // prevent changing the path once we started loading files.
    //
    if(g_configuration_has_started)
    {
        throw snap_configurations_exception_too_late("snap_configurations::set_configuration_path() cannot be called once a configuration file was read.");
    }

    // other functions will not deal with with "" as the current directory
    // so make sure we use "." instead
    //
    if(path.empty())
    {
        g_configurations_path = ".";
    }
    else
    {
        g_configurations_path = path;
    }
}


/** \brief Get a constant reference to all the parameters.
 *
 * Once in a while it may be useful to gain access to the entire list
 * of parameters defined in a configuration file. This function
 * gives you that ability.
 *
 * Since you get a reference, if you do not create a copy, you will
 * see any changes to parameters that are made by other functions.
 *
 * \param[in] configuration_filename  The name of the configuration file off of
 *                                which the parameters are returned.
 * \param[in] override_filename  The name of the override, that file is
 *            checked first and if a field exists in it, that value is used.
 *
 * \return A reference to the map of parameters of that configuration file.
 */
snap_configurations::parameter_map_t const & snap_configurations::get_parameters(std::string const & configuration_filename, std::string const & override_filename) const
{
    snap_thread::snap_lock lock(*g_mutex);
    auto const config(get_configuration(configuration_filename, override_filename));
    return config->get_parameters();
}


/** \brief Replace the parameters of this configuration file with new ones.
 *
 * This function replaces the existing parameters with the new specified
 * ones in \p params. This is most often used to copy the command line
 * parameters in the configuration file, as if the command line parameters
 * had been read from that configuration.
 *
 * The parameters specified to this function have precedence over the
 * parameters read from the file (i.e. they overwrite any existing
 * parameter.)
 *
 * \param[in] configuration_filename  The name of the configuration file concerned.
 * \param[in] override_filename  The name of the override, that file is
 *            checked first and if a field exists in it, that value is used.
 * \param[in] params  The new parameters.
 */
void snap_configurations::set_parameters(std::string const & configuration_filename, std::string const & override_filename, parameter_map_t const & params)
{
    snap_thread::snap_lock lock(*g_mutex);
    auto const config(get_configuration(configuration_filename, override_filename));
    config->set_parameters(params);
}


/** \brief Retreve a parameter from the configuration file.
 *
 * This function reads the specified \p configuration_filename file and then
 * searches for the specified \p parameter_name. If found, then its
 * value is returned, otherwise the function returns an empty string.
 *
 * To know whether a parameter is defined (opposed to being empty),
 * use the has_parameter() function instead.
 *
 * \param[in] configuration_filename  The name of the configuration file.
 * \param[in] override_filename  The name of the override, that file is
 *            checked first and if a field exists in it, that value is used.
 * \param[in] parameter_name  The name of the parameter to retrieve.
 *
 * \return The value of the parameter or the empty string.
 */
std::string snap_configurations::get_parameter(std::string const & configuration_filename, std::string const & override_filename, std::string const & parameter_name) const
{
    snap_thread::snap_lock lock(*g_mutex);
    auto const config(get_configuration(configuration_filename, override_filename));
    return config->get_parameter(parameter_name);
}


/** \brief Check whether the specified configuration file exists.
 *
 * This function searches for the configuration file and possibly an
 * override file. If neither exists, the function returns false. If at
 * least one exists, then the function returns false.
 *
 * \param[in] configuration_filename  The name of the configuration file to
 *            probe for existence.
 * \param[in] override_filename  The name of the override, that file is
 *            checked first and if a field exists in it, that value is used.
 *
 * \return true if a configuration file was found.
 */
bool snap_configurations::configuration_file_exists( std::string const & configuration_filename, std::string const &override_filename )
{
    snap_thread::snap_lock lock(*g_mutex);
    auto const config(get_configuration(configuration_filename, override_filename, true/*quiet*/));
    return config->exists();
}


/** \brief Check whether a certain configuration file has a certain parameter.
 *
 * This function reads the specified configuration file and then check
 * whether it defines the specified parameter. If so, it returns true.
 * If not, it returns false.
 *
 * \warning
 * Note that this function forces a read of the specified configuration
 * file since the only way to know whether that parameter exists in the
 * configuration is to read it.
 *
 * \param[in] configuration_filename  The name of the configuration file to read.
 * \param[in] override_filename  The name of the override, that file is
 *            checked first and if a field exists in it, that value is used.
 * \param[in] parameter_name  The parameter to check the presence of.
 */
bool snap_configurations::has_parameter(std::string const & configuration_filename, std::string const & override_filename, std::string const & parameter_name) const
{
    snap_thread::snap_lock lock(*g_mutex);
    auto const config(get_configuration(configuration_filename, override_filename));
    return config->has_parameter(parameter_name);
}


/** \brief Replace the value of one parameter.
 *
 * This function replaces the value of parameter \p parameter_name
 * in configuration file \p configuration_filename with \p value.
 *
 * \param[in] configuration_filename  The name of the configuration file to change.
 * \param[in] override_filename  The name of the override, that file is
 *            checked first and if a field exists in it, that value is used.
 * \param[in] parameter_name  The name of the parameter to modify.
 * \param[in] value  The new value of the parameter.
 */
void snap_configurations::set_parameter(std::string const & configuration_filename, std::string const & override_filename, std::string const & parameter_name, std::string const & value)
{
    snap_thread::snap_lock lock(*g_mutex);
    auto const config(get_configuration(configuration_filename, override_filename));
    config->set_parameter(parameter_name, value);
}


/** \brief Save fields back to the configuration file.
 *
 * This function can be used to save the configuration file back to file.
 * In most cases you want to use the override filename (so on the snap_config
 * you would use "true" as the first parameter.)
 *
 * Note that the in memory configuration parameters do NOT include any
 * comments. The saved file gets a comment at the top saying it was
 * auto-generated. This feature should only be used for configuration files
 * that administrators do not expected to update themselves or are being
 * updated in the override sub-folder.
 *
 * \note
 * The configuration_filename and override_filename parameters are used to
 * find the configuration file in our cache (or add it there if not already
 * present.) Since the corresponding filenames are already defined in the
 * snap_config_file object, it is not used to save the file per se, only
 * to find the snap_config_file that corresponds to the input parameters.
 *
 * \param[in] configuration_filename  The name of the configuration file to change.
 * \param[in] override_filename  The name of the override, that file is
 *            checked first and if a field exists in it, that value is used.
 * \param[in] override_file  Whether the save uses the \p configuration_filename
 *            or the \p override_filename to save this 
 */
bool snap_configurations::save(std::string const & configuration_filename, std::string const & override_filename, bool override_file)
{
    snap_thread::snap_lock lock(*g_mutex);
    auto const config(get_configuration(configuration_filename, override_filename, true));
    return config->write_config_file(override_file);
}


}
//namespace snap
// vim: ts=4 sw=4 et
