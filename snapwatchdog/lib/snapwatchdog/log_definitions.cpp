// Snap Websites Server -- snap watchdog library
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
#include "./log_definitions.h"


// snapwebsites lib
//
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/log.h>
#include <snapwebsites/qdomhelpers.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// Qt lib
//
#include <QFile>


// C lib
//
#include <grp.h>
#include <pwd.h>


// last include
//
#include <snapdev/poison.h>




namespace snap
{



namespace
{





/** \brief Load a log definition XML file.
 *
 * This function loads one log definition XML file and transform it in a
 * watchdog_log_t structure.
 *
 * Note that one file may include many log definitions.
 *
 * \param[in] log_definitions_filename  The name of an XML file representing
 *                                      log definitions.
 * \param[in,out] result  The vector of log definitions grown on each call.
 */
void load_xml(QString log_definitions_filename, watchdog_log_t::vector_t * result)
{
    QFile input(log_definitions_filename);
    if(input.open(QIODevice::ReadOnly))
    {
        QDomDocument doc;
        if(doc.setContent(&input, false)) // TODO: add error handling for debug
        {
            // we got the XML loaded
            //
            QDomNodeList logs(doc.elementsByTagName("log"));
            int const max(logs.size());
            for(int idx(0); idx < max; ++idx)
            {
                QDomNode p(logs.at(idx));
                if(!p.isElement())
                {
                    continue;
                }
                QDomElement log(p.toElement());
                QString const name(log.attribute("name"));
                if(name.isEmpty())
                {
                    throw log_definitions_exception_invalid_parameter("the name of a log definition cannot be the empty string");
                }

                bool const mandatory(log.hasAttribute("mandatory"));

                auto it(std::find_if(
                              result->begin()
                            , result->end()
                            , [name](auto const & l)
                            {
                                return name == l.get_name();
                            }));
                if(it != result->end())
                {
                    throw log_definitions_exception_invalid_parameter("found log definition named \"" + name + "\" twice.");
                }

                watchdog_log_t wl(name, mandatory);

                if(log.hasAttribute("secure"))
                {
                    wl.set_secure(true);
                }

                QDomNodeList path_tags(log.elementsByTagName("path"));
                if(path_tags.size() > 0)
                {
                    QDomNode path_node(path_tags.at(0));
                    if(path_node.isElement())
                    {
                        QDomElement path(path_node.toElement());
                        wl.set_path(path.text());
                    }
                }

                QDomNodeList pattern_tags(log.elementsByTagName("pattern"));
                int const max_patterns(pattern_tags.size());
                for(int pat(0); pat < max_patterns; ++pat)
                {
                    QDomNode pattern_node(pattern_tags.at(pat));
                    if(pattern_node.isElement())
                    {
                        QDomElement pattern(pattern_node.toElement());
                        wl.add_pattern(pattern.text());
                    }
                }

                QDomNodeList user_name_tags(log.elementsByTagName("user-name"));
                if(user_name_tags.size() > 0)
                {
                    QDomNode user_name_node(user_name_tags.at(0));
                    if(user_name_node.isElement())
                    {
                        QDomElement user_name(user_name_node.toElement());
                        wl.set_user_name(user_name.text());
                    }
                }

                QDomNodeList max_size_tags(log.elementsByTagName("max-size"));
                if(max_size_tags.size() > 0)
                {
                    QDomNode max_size_node(max_size_tags.at(0));
                    if(max_size_node.isElement())
                    {
                        QDomElement max_size_tag(max_size_node.toElement());
                        QString const max_size_str(max_size_tag.text().trimmed());
                        int const size(max_size_str.length());
                        int max_size(0);
                        int multiplicator(1);
                        for(int j(0); j < size; ++j)
                        {
                            short c(max_size_str[j].unicode());
                            if(c >= '0' && c <= '9')
                            {
                                max_size = max_size * 10 + c - '0';
                            }
                            else
                            {
                                // the digits can be followed by spaces and
                                // a size specification
                                //
                                if(std::isspace(c))
                                {
                                    ++j;
                                    if(j < size)
                                    {
                                        c = max_size_str[j].unicode();
                                    }
                                    else
                                    {
                                        c = ' ';
                                    }
                                }

                                // c must be one of KMG in lower or upper case
                                // (lower means x 1000, upper means x 1024)
                                //
                                switch(c)
                                {
                                case 'K':
                                    multiplicator = 1024;
                                    break;

                                case 'M':
                                    multiplicator = 1024 * 1024;
                                    break;

                                case 'G':
                                    multiplicator = 1024 * 1024 * 1024;
                                    break;

                                case 'k':
                                    multiplicator = 1000;
                                    break;

                                case 'm':
                                    multiplicator = 1000 * 1000;
                                    break;

                                case 'g':
                                    multiplicator = 1000 * 1000 * 1000;
                                    break;

                                case ' ':
                                    // no size multiplicator is acceptable
                                    break;

                                default:
                                    throw log_definitions_exception_invalid_parameter(QString("invalid size character \"%1\" defining the size, should be one of K, M, G, k, m, or g.").arg(c));

                                }
                                ++j;
                                if(j < size)
                                {
                                    c = max_size_str[j].unicode();
                                    if(c != 'b'
                                    && c != 'B')
                                    {
                                        throw log_definitions_exception_invalid_parameter(QString("invalid size character \"%1\" after KMG, should be B or b.").arg(c));
                                    }

                                    // the b or B is just noise, ignore it
                                    //
                                    ++j;
                                    if(j < size)
                                    {
                                        throw log_definitions_exception_invalid_parameter("left over characters after the size definition are not allowed");
                                    }
                                }
                                break;
                            }
                        }
                        max_size *= multiplicator;

                        wl.set_max_size(max_size);
                    }
                }

                QDomNodeList mode_tags(log.elementsByTagName("mode"));
                if(mode_tags.size() > 0)
                {
                    QDomNode mode_node(mode_tags.at(0));
                    if(mode_node.isElement())
                    {
                        QDomElement mode_tag(mode_node.toElement());
                        QString const mode_str(mode_tag.text().trimmed());
                        int const size(mode_str.length());
                        if(size > 0)
                        {
                            int mode(0);
                            int mode_mask(0);
                            int j(0);
                            short c(mode_str[j].unicode());
                            if(c >= '0' && c <= '8')
                            {
                                // octal is read as is
                                //
                                do
                                {
                                    mode = mode * 8 + c - '0';
                                    ++j;
                                    if(j >= size)
                                    {
                                        break;
                                    }
                                    c = mode_str[j].unicode();
                                }
                                while(c >= '0' && c <= '8');
                                if(j < size
                                && c == '/')
                                {
                                    for(++j; j < size; ++j)
                                    {
                                        c = mode_str[j].unicode();
                                        if(c < '0' || c > '8')
                                        {
                                            throw log_definitions_exception_invalid_parameter("a numeric mode must have a numeric mask");
                                        }
                                        mode_mask = mode_mask * 8 + c - '0';
                                    }
                                }
                            }
                            else
                            {
                                // accept letters instead:
                                //      u -- owner (user)
                                //      g -- group
                                //      o -- other
                                //      a -- all (user, group, other)
                                //
                                // the a +-= operator to add remove or
                                // set to exactly that value
                                //
                                // then the permissions are:
                                //      r -- read
                                //      w -- write
                                //      x -- execute (access directory)
                                //      s -- set user/group ID
                                //      t -- sticky bit
                                //
                                int flags(0);
                                int op(0);
                                for(;;)
                                {
                                    switch(c)
                                    {
                                    case 'u':
                                        flags |= 0700;
                                        break;

                                    case 'g':
                                        flags |= 0070;
                                        break;

                                    case 'o':
                                        flags |= 0007;
                                        break;

                                    case 'a':
                                        flags |= 0777;
                                        break;

                                    case '+':
                                    case '-':
                                    case '=':
                                        if(op != 0)
                                        {
                                            throw log_definitions_exception_invalid_parameter("only one operator can be used in a mode");
                                        }

                                        // default is 'a' if undefined
                                        //
                                        if(flags == 0)
                                        {
                                            flags = 0777;
                                        }

                                        // this is our operator
                                        //
                                        op = c;
                                        break;

                                    default:
                                        throw log_definitions_exception_invalid_parameter("unknown character for mode, expected one or more of u, g, o, or a");

                                    }
                                    ++j;
                                    if(j >= size
                                    || op != 0)
                                    {
                                        break;
                                    }
                                    c = mode_str[j].unicode();
                                }
                                // the r/w/... flags now
                                //
                                int upper_mode(0);
                                for(; j < size; ++j)
                                {
                                    c = mode_str[j].unicode();
                                    switch(c)
                                    {
                                    case 'r':
                                        mode |= 0004;
                                        break;

                                    case 'w':
                                        mode |= 0002;
                                        break;

                                    case 'x':
                                        mode |= 0001;
                                        break;

                                    case 's':
                                        upper_mode |= 06000;
                                        break;

                                    case 't':
                                        upper_mode |= 01000;
                                        break;

                                    default:
                                        throw log_definitions_exception_invalid_parameter("unknown character for mode, expected one or more of r, w, x, s, or t");

                                    }
                                }

                                // repeat the mode as defined in the left
                                // hand side
                                //
                                mode *= flags;

                                // add the upper mode as required
                                //
                                if((upper_mode & 01000) != 0)
                                {
                                    mode |= 01000; // 't'
                                }
                                if((upper_mode & 06000) != 0)
                                {
                                    mode |= ((flags & 0700) != 0 ? 04000 : 0)   // user 's'
                                          | ((flags & 0070) != 0 ? 02000 : 0);  // group 's'
                                }

                                // now the operator defines the mode versus mask
                                //
                                switch(op)
                                {
                                case '+':
                                    // '+' means that the specified flags must
                                    // be set, but others may be set or not
                                    //
                                    mode_mask = mode;
                                    break;

                                case '-':
                                    // '-' means that the specified flags must
                                    // not be set, but others may be set or not
                                    //
                                    mode_mask = mode;
                                    mode ^= 0777; // we can't set mode to zero--but this is bogus if the user expects all the flags to be zero (which should not be something sought)
                                    break;

                                case '=':
                                    //'=' means that the specified flags must
                                    // be exactly as specified
                                    //
                                    mode_mask = 07777;
                                    break;

                                default:
                                    throw std::logic_error("unexpected operator, did we add one and are not properly handling it?");

                                }
                            }

                            wl.set_mode(mode);
                            wl.set_mode_mask(mode_mask == 0 ? 07777 : mode_mask);
                        }
                    }
                }

                result->push_back(wl);
            }
        }
    }
}



}
// no name namespace







watchdog_log_t::search_t::search_t(QString const & regex, QString const & report_as)
    : f_regex(regex)
    , f_report_as(report_as)
{
}



QString const & watchdog_log_t::search_t::get_regex() const
{
    return f_regex;
}


QString const & watchdog_log_t::search_t::get_report_as() const
{
    return f_report_as;
}




/** \class watchdog_log_t
 * \brief Class used to record the list of logs to check.
 *
 * Objects of type watchdog_log_t are read from XML files.
 *
 * The watchdog log plugin checks log files for sizes and various content
 * to warn the administrators of problems it discovers.
 */



watchdog_log_t::watchdog_log_t(QString const & name, bool mandatory)
    : f_name(name)
    , f_mandatory(mandatory)
{
}


void watchdog_log_t::set_mandatory(bool mandatory)
{
    f_mandatory = mandatory;
}


void watchdog_log_t::set_secure(bool secure)
{
    f_secure = secure;
}


void watchdog_log_t::set_path(QString const & path)
{
    f_path = path;
}


void watchdog_log_t::set_user_name(QString const & user_name)
{
    f_uid = -1;

    if(!user_name.isEmpty())
    {
        struct passwd const * pwd(getpwnam(user_name.toUtf8().data()));
        if(pwd == nullptr)
        {
            SNAP_LOG_WARNING("user name \"")
                            (user_name)
                            ("\" does not exist on this system. A log file can't be named after it.");
        }
        else
        {
            f_uid = pwd->pw_uid;
        }
    }
}


void watchdog_log_t::set_group_name(QString const & group_name)
{
    f_gid = -1;

    if(!group_name.isEmpty())
    {
        struct group const * grp(getgrnam(group_name.toUtf8().data()));
        if(grp == nullptr)
        {
            SNAP_LOG_WARNING("group name \"")
                            (group_name)
                            ("\" does not exist on this system. A log file can't be named after it.");
        }
        else
        {
            f_gid = grp->gr_gid;
        }
    }
}


void watchdog_log_t::set_mode(int mode)
{
    f_mode = mode;
}


void watchdog_log_t::set_mode_mask(int mode_mask)
{
    f_mode_mask = mode_mask;
}


void watchdog_log_t::add_pattern(QString const & pattern)
{
    if(f_first_pattern)
    {
        f_first_pattern = false;

        f_patterns.clear();
    }
    f_patterns.push_back(pattern);
}


void watchdog_log_t::set_max_size(size_t size)
{
    f_max_size = size;
}


void watchdog_log_t::add_search(search_t const & search)
{
    f_searches.push_back(search);
}


QString const & watchdog_log_t::get_name() const
{
    return f_name;
}


bool watchdog_log_t::is_mandatory() const
{
    return f_mandatory;
}


bool watchdog_log_t::is_secure() const
{
    return f_secure;
}


QString const & watchdog_log_t::get_path() const
{
    return f_path;
}


uid_t watchdog_log_t::get_uid() const
{
    return f_uid;
}


gid_t watchdog_log_t::get_gid() const
{
    return f_gid;
}


mode_t watchdog_log_t::get_mode() const
{
    return f_mode;
}


mode_t watchdog_log_t::get_mode_mask() const
{
    return f_mode_mask;
}


snap::snap_string_list const & watchdog_log_t::get_patterns() const
{
    return f_patterns;
}


size_t watchdog_log_t::get_max_size() const
{
    return f_max_size;
}


watchdog_log_t::search_t::vector_t watchdog_log_t::get_searches() const
{
    return f_searches;
}








/** \brief Load the list of watchdog log definitions.
 *
 * This function loads the XML files from the watchdog and other packages.
 *
 * \param[in] log_definitions_path  The path to the list of XML files
 *            declaring log definitions that should be running.
 *
 * \return A vector of watchdog_log_t objects each representing a log
 *         definition.
 */
watchdog_log_t::vector_t watchdog_log_t::load(QString log_definitions_path)
{
    watchdog_log_t::vector_t result;

    // get the path to the log definition XML files
    //
    if(log_definitions_path.isEmpty())
    {
        log_definitions_path = "/usr/share/snapwebsites/snapwatchdog/log-definitions";
    }

    glob_dir const log_filenames(log_definitions_path + "/*.xml", GLOB_NOSORT | GLOB_NOESCAPE, true);
    log_filenames.enumerate_glob(std::bind(&load_xml, std::placeholders::_1, &result));

    return result;
}



}
// snap namespace
// vim: ts=4 sw=4 et
