// Snap Websites Server -- snapwebsites library -- flags handling
// Copyright (c) 2018-2019  Made to Order Software Corp.  All Rights Reserved
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


// self
//
#include "./flags.h"


// snapwebsites lib
//
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/log.h>
#include <snapwebsites/snap_config.h>
#include <snapwebsites/snapwebsites.h>


// snapdev lib
//
#include <snapdev/not_used.h>
#include <snapdev/tokenize_string.h>


// boost lib
//
#include <boost/algorithm/string.hpp>


// last include
//
#include <snapdev/poison.h>




namespace snap
{



namespace
{



}
// no name namespace





/** \brief Initialize a "new" flag.
 *
 * This function creates a new flag in memory.
 *
 * New flags are generally created using one of the SNAP_FLAG_UP()
 * or the SNAP_FLAG_DOWN() macros, which will automatically
 * initialize the flag, especially the source filename, the function name,
 * and the line number where the flag is being created, and the status
 * which the macro describes.
 *
 * All the names must match the following macro:
 *
 * \code
 *      [a-zA-Z][-a-zA-Z0-9]*
 * \endcode
 *
 * The underscore is not included in a name because we want to be able to
 * separate multiple names using the underscore, which is what is used
 * when building the filename from this information.
 *
 * \param[in] unit  The name of the unit creating this flag. For example,
 *            the core plugins would use "core-plugin".
 * \param[in] section  The name of the section creating this flag. In case
 *            of the core plugin, this should be the name of the plugin.
 * \param[in] name  The actual name of the flag. This is expected to
 *            somewhat describe what the flag is being for.
 */
snap_flag::snap_flag(std::string const & unit, std::string const & section, std::string const & name)
    : f_unit(unit)
    , f_section(section)
    , f_name(name)
{
    valid_name(f_unit);
    valid_name(f_section);
    valid_name(f_name);
}


/** \brief Load a flag from file.
 *
 * When this constructor is used, the flag gets loaded from file.
 * Flags use a snap_config file to handle their permanent data.
 *
 * The fields offered in the snap_flag object are translated
 * in a configuration field. This function reads the data with
 * a snap_file object and converts it to snap_flag data.
 *
 * \param[in] filename  The nane of the file to load.
 */
snap_flag::snap_flag(std::string const & filename)
    : f_filename(filename)
{
    if(f_filename.empty())
    {
        throw flags_exception_invalid_parameter("the filename must be defined (i.e. not empty) when using the flag constructor with a filename");
    }

    snap_config flag(get_filename());

    if(!flag.has_parameter("unit")
    || !flag.has_parameter("section")
    || !flag.has_parameter("name")
    || !flag.has_parameter("message"))
    {
        throw flags_exception_invalid_parameter("a flag file is expecteda unit, section, and name field, along with a message field. Other fields are optional.");
    }

    f_unit = flag.get_parameter("unit");
    f_section = flag.get_parameter("section");
    f_name = flag.get_parameter("name");

    if(flag.has_parameter("source_file"))
    {
        f_source_file = flag.get_parameter("source_file");
    }

    if(flag.has_parameter("function"))
    {
        f_function = flag.get_parameter("function");
    }

    if(flag.has_parameter("line"))
    {
        f_line = std::stol(flag.get_parameter("line"));
    }

    f_message = flag.get_parameter("message");

    if(flag.has_parameter("priority"))
    {
        f_priority = std::stol(flag.get_parameter("priority"));
    }

    if(flag.has_parameter("manual_down"))
    {
        f_manual_down = flag.get_parameter("manual_down") == "yes";
    }

    if(flag.has_parameter("date"))
    {
        f_date = std::stol(flag.get_parameter("date"));
    }

    if(flag.has_parameter("modified"))
    {
        f_modified = std::stol(flag.get_parameter("modified"));
    }

    if(flag.has_parameter("tags"))
    {
        // here we use an intermediate tag_list vector so the tokenize_string
        // works then add those string in the f_tags parameter
        //
        std::string const tags(flag.get_parameter("tags"));
        std::vector<std::string> tag_list;
        tokenize_string(tag_list
                      , tags
                      , ","
                      , true
                      , " \n\r\t");
        f_tags.insert(tag_list.begin(), tag_list.end());
    }

    if(flag.has_parameter("hostname"))
    {
        f_hostname = flag.get_parameter("hostname");
    }

    if(flag.has_parameter("count"))
    {
        f_count = std::stol(flag.get_parameter("count"));
    }

    if(flag.has_parameter("version"))
    {
        f_version = flag.get_parameter("version");
    }
}


/** \brief Set the state of the flag.
 *
 * At the moment, the flag can be UP or DOWN. By default it is UP meaning
 * that there is an error, something the administrator has to take care of
 * to make sure the system works as expected. For example, the antivirus
 * backend will detect that the clamav package is not installed and install
 * it as required.
 *
 * \param[in] state  The new state.
 *
 * \return A reference to this.
 */
snap_flag & snap_flag::set_state(state_t state)
{
    f_state = state;

    return *this;
}


/** \brief Set the name of the source file.
 *
 * The name of the source where the flag is being raised is added using
 * this function.
 *
 * \param[in] source_file  The source file name.
 *
 * \return A reference to this.
 */
snap_flag & snap_flag::set_source_file(std::string const & source_file)
{
    f_source_file = source_file;

    return *this;
}


/** \brief Set the name of the function raising the flag.
 *
 * For debug purposes, we save the name of the function that called
 * the manager function to save the function. It should help us,
 * long term, to find flags and maintain them as required.
 *
 * \param[in] function  The name of the concerned function.
 *
 * \return A reference to this.
 */
snap_flag & snap_flag::set_function(std::string const & function)
{
    f_function = function;

    return *this;
}


/** \brief Set the line number at which the event happened.
 *
 * This parameter is used to save the line at which the function
 * used one of the snap_flag macros.
 *
 * By default the value is set to zero. If never called, then this
 * is a way to know that no line number was defined.
 *
 * \param[in] line  The new line number at which this flag is being raised.
 *
 * \return A reference to this.
 */
snap_flag & snap_flag::set_line(int line)
{
    f_line = line;

    return *this;
}


/** \brief Set the error message.
 *
 * A flag is always accompagned by an error message of some sort.
 * For example, the sendmail backend checks whether postfix is
 * installed on that computer. If not, it raises a flag with an
 * error message saying something like this: "The sendmail backend
 * expects Postfix to be installed on the same computer. snapmta
 * is not good enough to support the full mail server."
 *
 * \param[in] message  The message explaining why the flag is raised.
 *
 * \return A reference to this.
 */
snap_flag & snap_flag::set_message(std::string const & message)
{
    f_message = message;

    return *this;
}


/** \brief Set the error message.
 *
 * A flag is always accompagned by an error message of some sort.
 * For example, the sendmail backend checks whether postfix is
 * installed on that computer. If not, it raises a flag with an
 * error message saying something like this: "The sendmail backend
 * expects Postfix to be installed on the same computer. snapmta
 * is not good enough to support the full mail server."
 *
 * \param[in] message  The message explaining why the flag is raised.
 *
 * \return A reference to this.
 */
snap_flag & snap_flag::set_message(QString const & message)
{
    f_message = message.toUtf8().data();

    return *this;
}


/** \brief Set the error message.
 *
 * A flag is always accompagned by an error message of some sort.
 * For example, the sendmail backend checks whether postfix is
 * installed on that computer. If not, it raises a flag with an
 * error message saying something like this: "The sendmail backend
 * expects Postfix to be installed on the same computer. snapmta
 * is not good enough to support the full mail server."
 *
 * \param[in] message  The message explaining why the flag is raised.
 *
 * \return A reference to this.
 */
snap_flag & snap_flag::set_message(char const * message)
{
    if(message == nullptr)
    {
        f_message.clear();
    }
    else
    {
        f_message = message;
    }

    return *this;
}


/** \brief Set the error message.
 *
 * A flag is always accompagned by an error message of some sort.
 * For example, the sendmail backend checks whether postfix is
 * installed on that computer. If not, it raises a flag with an
 * error message saying something like this: "The sendmail backend
 * expects Postfix to be installed on the same computer. snapmta
 * is not good enough to support the full mail server."
 *
 * The default priority is 5. It can be reduced or increased. It
 * is expected to be between 0 and 100.
 *
 * \param[in] priority  The error priority.
 *
 * \return A reference to this.
 */
snap_flag & snap_flag::set_priority(int priority)
{
    if(priority < 0)
    {
        f_priority = 0;
    }
    else if(priority > 100)
    {
        f_priority = 100;
    }
    else
    {
        f_priority = priority;
    }

    return *this;
}


/** \brief Mark whether a manual down is required for this flag.
 *
 * Some flags may be turned ON but never turned OFF. These are called
 * _manual flags_, because you have to turn them off manually.
 *
 * \todo
 * At some point, the Watchdog interface in the snapmanager.cgi will
 * allow you to click a link to manually take a flag down.
 *
 * \param[in] manual  Whether the flag is considered manual or not.
 *
 * \return A reference to this.
 */
snap_flag & snap_flag::set_manual_down(bool manual)
{
    f_manual_down = manual;

    return *this;
}


/** \brief Add a tag to the list of tags of this flag.
 *
 * You can assign tags to a flag so as to group it with other flags
 * that reuse the same tag.
 *
 * The names must be valid names (as the unit, section, and flag names.)
 * Your name must validate against this regular expression:
 *
 * \code
 *      [a-zA-Z][-a-zA-Z0-9]*
 * \endcode
 *
 * So a-z, A-Z, 0-9, and dash. The first character must be a letter.
 *
 * Note that the underscore (_) is not included because we use those
 * to separate each word in a filename.
 *
 * \param[in] tag  The name of a tag to add to this flag.
 *
 * \return A reference to this.
 */
snap_flag & snap_flag::add_tag(std::string const & tag)
{
    f_tags.insert(tag);

    return *this;
}


/** \brief Get the current state.
 *
 * Get the state of the flag file.
 *
 * A flag file can be UP or DOWN. When DOWN, a save() will delete the
 * file. When UP, the file gets created.
 *
 * \return The current state of the flag.
 */
snap_flag::state_t snap_flag::get_state() const
{
    return f_state;
}


/** \brief Get the unit name.
 *
 * Flags are made unique by assigning them unit and section names.
 *
 * The unit name would be something such as "core-plugins" for the
 * main snapserver core plugins.
 *
 * \return The unit name.
 *
 * \sa get_section()
 * \sa get_name()
 */
std::string const & snap_flag::get_unit() const
{
    return f_unit;
}


/** \brief Get the section name.
 *
 * The section name defines the part of the software that has a problem.
 * For example, for the core plugins, you may want to use the name of the
 * plugin.
 *
 * \return The name of the section raising the flag.
 *
 * \sa get_unit()
 * \sa get_name()
 */
std::string const & snap_flag::get_section() const
{
    return f_section;
}


/** \brief Name of the flag.
 *
 * This parameter defines the name of the flag. The reason for the error
 * is often what is used here. The name must be short and ASCII only, but
 * should still properly define why the error occurs.
 *
 * A more detailed error message is returned by get_message().
 *
 * The get_unit() and get_section() define more generic names than this
 * one.
 *
 * For example, the attachment plugin checks for virus infected attachments.
 * This requires the clamav package to be installed. If not installed, it
 * raises a flag. That flag is part of the "core-plugins" (unit name),
 * and it gets raised in the "attachment" (section name) plugin, and it gets
 * raised because of "clamav-missing" (flag name)
 *
 * \return The name of the flag.
 *
 * \sa get_message()
 */
std::string const & snap_flag::get_name() const
{
    return f_name;
}


/** \brief Retrieve the name of the source file.
 *
 * This function retrieves the source filename. This is set using the
 * macros. It helps finding the reason for the falg being raised if
 * the message is not clear enough.
 *
 * \return The source file name.
 */
std::string const & snap_flag::get_source_file() const
{
    return f_source_file;
}


/** \brief Get the filename.
 *
 * This function returns the filename used to access this flag.
 *
 * If you loaded the flag files, then this is defined from the constructor.
 *
 * If you created a snap_flag object from scratch, then the filename
 * is built from the unit, section, and flag names as follow:
 *
 * \code
 *      <unit> + '_' + <section> + '_' + <flag name> + ".flag"
 * \endcode
 *
 */
std::string const & snap_flag::get_filename() const
{
    if(f_filename.empty())
    {
        snap_config server_config("snapserver");
        std::string path("/var/lib/snapwebsites/flags");
        if(server_config.has_parameter("flag_path"))
        {
            path = server_config["flag_path"];
        }
        f_filename = path + "/" + f_unit + "_" + f_section + "_" + f_name + ".flag";
    }
    return f_filename;
}


/** \brief Retrieve the function name.
 *
 * The function name defines the name of the function where the macro
 * was used. It can be useful for debugging where aproblem happens.
 *
 * \return The name of the function we are that was tike
 */
std::string const & snap_flag::get_function() const
{
    return f_function;
}


/** \brief Retrieve the line number at which it was first called.
 *
 * This is for debug purposes so one can easily find exactly what code
 * generated which flag.
 *
 * \return The line number where th snap_flag object is created.
 */
int snap_flag::get_line() const
{
    return f_line;
}


/** \brief The actual error message of this flag.
 *
 * A flag is used to tell the snapwatchdog flag plugin that something
 * is wrong and requires the administrator to fix it up.
 *
 * The message should be plain text. It may include newline characters.
 *
 * \return The message of this flag.
 */
std::string const & snap_flag::get_message() const
{
    return f_message;
}


/** \brief Retrieve the flag priority.
 *
 * The function returns the priority of the flag. By default it is set to 5.
 * If you want to increase the priority so the error shows up in an email,
 * increase the priority to at least 50. Remember that a really high priority
 * (close to 100) will increase the number of emails. Watch out as it could
 * both the admninistrators.
 *
 * When displaying the flags, the highest priority is used and a single
 * message is sent for all the priorities.
 *
 * \return This flag priority.
 */
int snap_flag::get_priority() const
{
    return f_priority;
}


/** \brief Check whether the flag is considered manual or automatic.
 *
 * A _manual down_ flag is a flag that the administrator has to turn
 * off manually once the problem was taken cared off.
 *
 * By default, a snap_flag is considered automatic, which means
 * that the process that raises the flag up for some circumstances
 * will also know how to bring that flag down once the circumstances
 * disappear.
 *
 * This function returns true if the process will never bring its
 * flag down automatically. This is particularly true if the process
 * use the SNAP_FLAG_UP() macro but never uses the corresponding
 * SNAP_FLAG_DOWN()--corresponding as in with the same first
 * three strings (unit, section, name.)
 *
 * \return true if the flag has to be taken down (deleted) manually.
 *
 * \sa set_manual_down()
 */
bool snap_flag::get_manual_down() const
{
    return f_manual_down;
}


/** \brief Retrieve the date when the flag was first raised.
 *
 * The function returns the date when the flag was first raised. A flag
 * that often goes up and down will have the date when it last went up.
 *
 * See get_modified() to get the date when the flag was last checked.
 * In some cases, checks are done once each time a command is run.
 * In other cases, checks are performed by the startup code of a
 * daemon so the modification date is likely to not change for a while.
 *
 * \return This date when this flag was last raised.
 */
time_t snap_flag::get_date() const
{
    return f_date;
}


/** \brief Retrieve the date when the flag was last checked.
 *
 * The function returns the date when the code raising this flag was last
 * run.
 *
 * This indicates when the flag was last updated. If very recent then we
 * know that the problem that the flag raised is likely still in force.
 * A modified date which is really old may mean that the code testing
 * this problem does not automatically take the flag down (a bug in itself).
 *
 * Note that some flags are checked only once at boot time, or once
 * when a service starts. So it is not abnormal to see a raised flag
 * modification date remain the same for a very long time.
 *
 * \return This date when this flag's problem was last checked.
 */
time_t snap_flag::get_modified() const
{
    return f_modified;
}


/** \brief Return a reference to the list of tags.
 *
 * A flag can be given a list of tags in order to group it with other
 * flags that may not be of the same unit or section.
 *
 * \return The set of tags attached to this flag.
 */
snap_flag::tag_list_t const & snap_flag::get_tags() const
{
    return f_tags;
}


/** \brief The name of the computer on which this flag was generated.
 *
 * In order to be able to distinguish on which computer the flag was
 * raised, the save() function includes the hostname of the computer
 * in the flag file.
 *
 * \return The hostname of the computer that saved this flag file.
 */
std::string const & snap_flag::get_hostname() const
{
    return f_hostname;
}


/** \brief Retrieve the number of times this flag was raised.
 *
 * Each time a flag gets raised this counter is increased by 1. It starts
 * at 0 so the very first time it gets saved, the counter will be 1.
 *
 * This is an indicator of how many times the flag situation was found to
 * be true. In most cases this should be a really small number.
 *
 * \return The number of times the flag file was saved.
 */
int snap_flag::get_count() const
{
    return f_count;
}


/** \brief Get the version used to create this flag file.
 *
 * When the flag file gets saved, the current version of snapwebsites gets
 * saved in the file as the "version" field. This function returns that
 * value.
 *
 * Note that if the file gets updated, then the version of the newest write
 * is used in the file.
 *
 * \return The version used to save the flag file.
 */
std::string const & snap_flag::get_version() const
{
    return f_version;
}


/** \brief Save the data to file.
 *
 * This function is used to save the flag to file.
 *
 * Note that if  the status is DOWN, then the file gets deleted.
 *
 * The file format used is the same as our configuration files:
 *
 * \code
 *      <varname>=<value>
 * \endcode
 *
 * These files should not be edited by administrators, though, since
 * they are just handled automatically by the code that generates this
 * data.
 *
 * \attention
 * Your implementation of the flags must make sure to use the
 * SNAP_FLAG_UP() when an error is detected and use
 * the SNAP_FLAG_DOWN() when the error is not detected
 * anymore. This is important since the file needs to disappear
 * once the problem was resolved.
 *
 * \return true if the save succeeded.
 */
bool snap_flag::save()
{
    bool result(true);

    if(f_state == state_t::STATE_UP)
    {
        // create and config object
        //
        snap_config flag(get_filename());

        // if the file exists, check whether a "date" is defined
        //
        bool const exists(flag.configuration_file_exists());
        bool has_date(false);
        bool has_count(false);
        if(exists)
        {
            has_date = flag.has_parameter("date");
            has_count = flag.has_parameter("count");
        }

        // do a first save in case the file did not yet exist
        //
        // TODO: find a way to avoid the throw when no .conf exists yet
        //       it should be as easy as adding  a flag in the snap_config
        //       which tells the system not to throw and return empty to
        //       all queries; it's just not done yet
        //
        flag.save(false);

        std::string const now(std::to_string(time(nullptr)));

        // setup all the fields as required
        // (note that setting up the first one will read the file if it exists...)
        //
        flag["unit"]        = f_unit;
        flag["section"]     = f_section;
        flag["name"]        = f_name;
        flag["source_file"] = f_source_file;
        flag["function"]    = f_function;
        flag["line"]        = std::to_string(f_line);
        flag["message"]     = f_message;
        flag["priority"]    = std::to_string(f_priority);
        flag["manual_down"] = f_manual_down ? "yes" : "no";
        if(!has_date)
        {
            flag["date"]    = now;
        }
        flag["modified"]    = now;
        flag["tags"]        = boost::algorithm::join(f_tags, ",");
        flag["hostname"]    = snap::server::get_server_name();
        flag["version"]     = SNAPWEBSITES_VERSION_STRING;

        if(has_count)
        {
            // increment existing counter by 1
            //
            flag["count"] = std::to_string(std::stol(flag["count"]) + 1);
        }
        else
        {
            flag["count"] = "1";
        }

        // now save that data to file
        //
        result = flag.save(false);
    }
    else
    {
        // state is down, delete the file if it still exists
        //
        result = unlink(get_filename().c_str()) == 0;
        if(!result && errno == ENOENT)
        {
            // deleting a flag that does not exist "works" every time
            //
            result = true;
        }
    }

    return result;
}



/** \brief Load all the flag files.
 *
 * This function is used to load all the flag files from disk.
 *
 * \note
 * It is expected that the number of flags is always going to be relatively
 * small. The function make sure that if more than 100 are defined, only
 * the first 100 are read and another is created warning about the large
 * number of available flags.
 *
 * \return The vector of flag files read from disk.
 */
snap_flag::vector_t snap_flag::load_flags()
{
    // get the path to read with glob_dir
    //
    snap_config server_config("snapserver");
    std::string path("/var/lib/snapwebsites/flags");
    if(server_config.has_parameter("flag_path"))
    {
        path = server_config["flag_path"];
    }

    // read the list of files
    //
    glob_dir const flag_filenames(path + "/*.flag", GLOB_NOSORT | GLOB_NOESCAPE, true);
    snap_flag::vector_t result;
    try
    {
        flag_filenames.enumerate_glob(std::bind(&load_flag, std::placeholders::_1, &result));
    }
    catch(flags_exception_too_many_flags const &)
    {
        // that error means we have over 100 flags raised
        //
        // we raise a "dynamic" flag about this error and ignore the
        // additional entries in the directory (the ignore happens because
        // of the throw in the load_flag() function.)
        //
        auto flag(SNAP_FLAG_UP("snap_flag"
                             , "flag"
                             , "too-many-flags"
                             , "too many flag were raised, showing only the first 100,"
                               " others can be viewed on this system at \"" + path + "\""));
        flag->set_priority(97);
        flag->add_tag("flag");
        flag->add_tag("too-many");
        result.push_back(flag);
    }

    return result;
}


/** \brief Callback function that receives each flag filename.
 *
 * When we use the load_flags() function, it calls this callback for
 * each filename found in the flag_path directory and ending
 * with the ".flag" extension.
 *
 * \param[in] filename  The name of the flag file found in the flag_path
 *                      directory.
 * \param[in,out] result  The vector where the new snap_flag object is
 *                        pushed.
 */
void snap_flag::load_flag(std::string const & filename, snap_flag::vector_t * result)
{
    if(result->size() >= 100)
    {
        throw flags_exception_too_many_flags("too many flags");
    }

    result->push_back(std::make_shared<snap_flag>(filename));
}


/** \brief Validate a name so we make sure they are as expected.
 *
 * Verify that the name is composed of letters (a-z, A-Z), digits (0-9),
 * and dashes (-) only.
 *
 * Also, it doesn't accept names that start with a digit or a dash.
 *
 * Note that the input is read/write because any upper case letters
 * will be transformed to lowercase (A-Z become a-z).
 *
 * Further, the name cannot have two dashes in a row nor a dash at
 * the end of the name.
 *
 * Finally, an empty name is also considered invalid.
 *
 * \param[in] name  The name to be checked.
 */
void snap_flag::valid_name(std::string & name)
{
    if(name.empty())
    {
        throw flags_exception_invalid_parameter("unit, section, name, tags cannot be empty");
    }

    bool first(true);
    char last_char('\0');
    std::string::size_type const max(name.length());
    for(std::string::size_type idx(0); idx < max; ++idx)
    {
        char c(name[idx]);
        switch(c)
        {
        case '-':
            if(first)
            {
                throw flags_exception_invalid_parameter("unit, section, name, tags cannot start with a dash (-)");
            }
            if(last_char == '-')
            {
                throw flags_exception_invalid_parameter("unit, section, name, tags cannot have two dashes (--) in a row");
            }
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(first)
            {
                throw flags_exception_invalid_parameter("unit, section, name, tags cannot start with a digit (-)");
            }
            break;

        default:
            if(c >= 'A' && c <= 'Z')
            {
                // force all to lowercase
                //
                c |= 0x20;
                name[idx] = c;
            }
            else if(c < 'a' || c > 'z')
            {
                throw flags_exception_invalid_parameter("name cannot include characters other than a-z, 0-9, and dashes (-)");
            }
            break;

        }
        first = false;
        last_char = c;
    }

    if(last_char == '-')
    {
        throw flags_exception_invalid_parameter("unit, section, name, tags cannot end with a dash (-)");
    }
}





}
// snap namespace
// vim: ts=4 sw=4 et
