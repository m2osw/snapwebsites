// Snap Websites Server -- all the user content and much of the system content
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


/** \file
 * \brief The implementation of the content plugin field_search class.
 *
 * This file contains the field_search class implementation.
 */

#include "content.h"

#include <snapwebsites/log.h>
#include <snapwebsites/qdomhelpers.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(content)



/** \class field_search
 * \brief Retrieve one or more parameters from one or more path.
 *
 * This function is used to search for a parameter in one or more paths
 * in your existing database tree.
 *
 * In many cases, the parameter exists in the specified path (i.e. the
 * "modified" parameter). In some other cases, the parameter only
 * exists in a child, a parent, the template, or a settings page.
 * This function is very easy to use and it will return said parameter
 * from wherever it is first found.
 *
 * If you are creating an administrative screen (and in some other
 * circumstances) it may be useful to find all instances of the parameter.
 * In that case you can request all instances. Note that this case is
 * considered SLOW and it should not be used lightly while generating
 * a page!
 *
 * The following shows you an example of a tree that this function can
 * search. Say that the input path represents B. If your search setup
 * asks for SELF, its CHILDREN with a depth limit of 2, a template (assuming
 * its template is D,) its type found using LINK (and assuming its type is
 * F) and the PARENTS of that type with a limit on C then the search can
 * check the following nodes in that order:
 *
 * \li B
 * \li E (switched to children)
 * \li H (switched to children; last time because depth is limited to 2)
 * \li I
 * \li J
 * \li D (switched to template)
 * \li F (switched to page type)
 * \li C (switched to parent, stop on C)
 *
 * Pages A, K and G are therefore ignored.
 *
 *                +-------+       +------+       +-------+
 *          +---->| B     |+----->| E    |+-+--->| H     |
 *          |     +-------+       +------+  |    +-------+
 *          |                               |
 *          |                               |
 *          |                     +------+  |    +-------+     +------+
 *          |     +-------+  +--->| F    |  +--->| I     |+--->| K    |
 *          +---->| C     |+-+    +------+  |    +-------+     +------+
 *  +----+  |     +-------+  |              |
 *  | A  |+-+                |              |
 *  +----+  |                |    +------+  |
 *          |                +--->| G    |  |    +-------+
 *          |     +-------+       +------+  +--->| J     |
 *          +---->| D     |                      +-------+
 *                +-------+
 *
 * View: http://www.asciiflow.com/#1357940162213390220
 * Edit: http://www.asciiflow.com/#1357940162213390220/1819073096
 *
 * This type of search can be used to gather pretty much all the
 * necessary parameters used in a page to display that page.
 *
 * Note that this function is not used by the permissions because in
 * that case all permission links defined in a page are sought. Whereas
 * here we're interested in the content of a field in a page.
 *
 * Note that when searching children we first search all the children at
 * a given depth, then repeat the search at the next level. So in our
 * example, if we had a search depth of 3, we would end up searching
 * K after J, not between I and J.
 *
 * Since the cmd_info_t object is like a mini program, it is possible
 * to do things such as change the name of the field being sought as
 * the different parts of the tree are searched. So a parameter named
 * "created" in SELF, could change to "modified" when searching the
 * PARENT, and "primary-date" when searching the TYPE. It may, however,
 * not be a good idea as in most situations you probably want to use
 * just and only "modified". This being said, when you try to determine
 * the modification date, you could try the "modified" date first, then
 * try the "updated" and finally "created" and since "created" is
 * mandatory you know you'll always find it (and it not, there is no
 * other valid default):
 *
 * \code
 * // WARNING: this code is not up to date...
 * //          especially, the SELF function returns a QtCassandraValue
 * //          and the run() function is automatically called on destruction
 * snap_string_list result(cmd_info_t()
 *      (PARAMETER_OPTION_FIELD_NAME, "modified")
 *      (PARAMETER_OPTION_SELF, path)
 *      (PARAMETER_OPTION_FIELD_NAME, "updated")
 *      (PARAMETER_OPTION_SELF, path)
 *      (PARAMETER_OPTION_FIELD_NAME, "created")
 *      (PARAMETER_OPTION_SELF, path)
 *      .run(PARAMETER_OPTION_MODE_FIRST)); // run
 * \endcode
 *
 * In this example notice that we just lose the cmd_info_t object. It is
 * temporarily created on the stack, initialized, used to gather the
 * first match, then return a list of strings our of which we expect
 * either nothing (empty list) or one entry (the first parameter found.)
 */


/** \class cmd_info_t
 * \brief Instructions about the search to perform.
 *
 * This sub-class is used by the parameters_t class as an instruction:
 * what to search next to find a given parameter.
 */


/** \brief Create an empty cmd_info_t object.
 *
 * To be able to create cmd_info_t objects in a vector we have to create
 * a constructor with no parameters. This creates an invalid command
 * object.
 */
field_search::cmd_info_t::cmd_info_t()
    //: f_cmd(command_t::COMMAND_UNKNOWN) -- auto-init
    //, f_value() -- auto-init
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd)
    : f_cmd(cmd)
    //, f_value(str_value)
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case command_t::COMMAND_PARENT_ELEMENT:
    case command_t::COMMAND_ELEMENT_TEXT:
    case command_t::COMMAND_RESET:
    case command_t::COMMAND_SELF:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for an instruction without parameters").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] str_value  The string value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QString const & str_value)
    : f_cmd(cmd)
    , f_value(str_value)
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case command_t::COMMAND_FIELD_NAME:
    case command_t::COMMAND_PATH:
    case command_t::COMMAND_PARENTS:
    case command_t::COMMAND_LINK:
    case command_t::COMMAND_DEFAULT_VALUE:
    case command_t::COMMAND_DEFAULT_VALUE_OR_NULL:
    case command_t::COMMAND_PATH_ELEMENT:
    case command_t::COMMAND_CHILD_ELEMENT:
    case command_t::COMMAND_NEW_CHILD_ELEMENT:
    case command_t::COMMAND_ELEMENT_ATTR:
    case command_t::COMMAND_SAVE:
    case command_t::COMMAND_SAVE_FLOAT64:
    case command_t::COMMAND_SAVE_INT64:
    case command_t::COMMAND_SAVE_INT64_DATE:
    case command_t::COMMAND_SAVE_INT64_DATE_AND_TIME:
    case command_t::COMMAND_SAVE_PLAIN:
    case command_t::COMMAND_SAVE_XML:
    case command_t::COMMAND_WARNING:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a string (%2)").arg(static_cast<int>(cmd)).arg(str_value));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] int_value  The integer value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, int64_t int_value)
    : f_cmd(cmd)
    , f_value(int_value)
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case command_t::COMMAND_CHILDREN:
    case command_t::COMMAND_DEFAULT_VALUE:
    case command_t::COMMAND_DEFAULT_VALUE_OR_NULL:
    case command_t::COMMAND_LABEL:
    case command_t::COMMAND_GOTO:
    case command_t::COMMAND_IF_FOUND:
    case command_t::COMMAND_IF_NOT_FOUND:
    case command_t::COMMAND_IF_ELEMENT_NULL:
    case command_t::COMMAND_IF_NOT_ELEMENT_NULL:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for an integer (%2)").arg(static_cast<int>(cmd)).arg(int_value));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. MODE.)
 * \param[in] int_value  The integer value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, mode_t mode)
    : f_cmd(cmd)
    , f_value(static_cast<int32_t>(mode))
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case command_t::COMMAND_MODE:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a mode (%2)").arg(static_cast<int>(cmd)).arg(static_cast<int>(mode)));

    }
}


/** \brief Initialize a cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] value  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QtCassandra::QCassandraValue& value)
    : f_cmd(cmd)
    , f_value(value)
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case command_t::COMMAND_DEFAULT_VALUE:
    case command_t::COMMAND_DEFAULT_VALUE_OR_NULL:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a QCassandraValue").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] element  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QDomElement element)
    : f_cmd(cmd)
    //, f_value() -- auto-init
    , f_element(element)
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case command_t::COMMAND_ELEMENT:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a QDomElement").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize a cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] doc  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QDomDocument doc)
    : f_cmd(cmd)
    //, f_value() -- auto-init
    , f_element(doc.documentElement())
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case command_t::COMMAND_ELEMENT:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a QDomDocument").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in,out] result  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, search_result_t& result)
    : f_cmd(cmd)
    //, f_value() -- auto-init
    //, f_element(element)
    , f_result(&result)
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case command_t::COMMAND_RESULT:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a search_result_t").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. COMMAND_PATH_INFO, etc.)
 * \param[in] ipath  A full defined path to a page.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, path_info_t const & ipath)
    : f_cmd(cmd)
    //, f_value() -- auto-init
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    , f_path_info(ipath)
{
    switch(cmd)
    {
    case command_t::COMMAND_PATH_INFO_GLOBAL:
    case command_t::COMMAND_PATH_INFO_BRANCH:
    case command_t::COMMAND_PATH_INFO_REVISION:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for an ipath (%2)").arg(static_cast<int>(cmd)).arg(ipath.get_cpath()));

    }
}


/** \brief Initialize a field search object.
 *
 * This constructor saves the snap child pointer in the field_search so
 * it can be referenced later to access pages.
 */
field_search::field_search(char const *filename, char const *func, int line, snap_child *snap)
    : f_filename(filename)
    , f_function(func)
    , f_line(line)
    , f_snap(snap)
    //, f_program() -- auto-init
{
}


/** \brief Generate the data and then destroy the field_search object.
 *
 * The destructor makes sure that the program runs once, then it cleans
 * up the object. This allows you to create a tempoary field_search object
 * on the stack and at the time it gets deleted, it runs the program.
 */
field_search::~field_search()
{
    try
    {
        run();
    }
    catch(QtCassandra::QCassandraException const &)
    {
    }
}


/** \brief Add a command with no parameter.
 *
 * The following commands support this scheme:
 *
 * \li COMMAND_PARENT_ELEMENT
 * \li COMMAND_ELEMENT_TEXT
 * \li COMMAND_RESET
 * \li COMMAND_SELF
 *
 * \param[in] cmd  The command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd)
{
    cmd_info_t inst(cmd);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a "char const *".
 *
 * The following commands support the "char const *" value:
 *
 * \li COMMAND_FIELD_NAME
 * \li COMMAND_PATH
 * \li COMMAND_PARENTS
 * \li COMMAND_LINK
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 * \li COMMAND_PATH_ELEMENT
 * \li COMMAND_CHILD_ELEMENT
 * \li COMMAND_NEW_CHILD_ELEMENT
 * \li COMMAND_ELEMENT_ATTR
 * \li COMMAND_SAVE
 * \li COMMAND_SAVE_FLOAT64
 * \li COMMAND_SAVE_INT64
 * \li COMMAND_SAVE_INT64_DATE
 * \li COMMAND_SAVE_INT64_DATE_AND_TIME
 * \li COMMAND_SAVE_PLAIN
 * \li COMMAND_SAVE_XML
 * \li COMMAND_WARNING
 *
 * \param[in] cmd  The command.
 * \param[in] str_value  The string attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, char const * str_value)
{
    cmd_info_t inst(cmd, QString(str_value));
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QString.
 *
 * The following commands support the QString value:
 *
 * \li COMMAND_FIELD_NAME
 * \li COMMAND_PATH
 * \li COMMAND_PARENTS
 * \li COMMAND_LINK
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 * \li COMMAND_PATH_ELEMENT
 * \li COMMAND_CHILD_ELEMENT
 * \li COMMAND_NEW_CHILD_ELEMENT
 * \li COMMAND_ELEMENT_ATTR
 *
 * \param[in] cmd  The command.
 * \param[in] str_value  The string attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, QString const & str_value)
{
    cmd_info_t inst(cmd, str_value);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a 64 bit integer.
 *
 * The following commands support the integer:
 *
 * \li COMMAND_CHILDREN
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 * \li COMMAND_LABEL
 * \li COMMAND_GOTO
 * \li COMMAND_IF_FOUND
 * \li COMMAND_IF_NOT_FOUND
 * \li COMMAND_IF_ELEMENT_NULL
 * \li COMMAND_IF_NOT_ELEMENT_NULL
 *
 * \param[in] cmd  The command.
 * \param[in] value  The integer attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search & field_search::operator () (command_t cmd, int64_t int_value)
{
    cmd_info_t inst(cmd, int_value);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a 64 bit integer.
 *
 * The following commands support the integer:
 *
 * \li COMMAND_MODE
 *
 * \param[in] cmd  The command.
 * \param[in] value  The integer attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search & field_search::operator () (command_t cmd, mode_t mode)
{
    cmd_info_t inst(cmd, mode);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QCassandraValue.
 *
 * The following commands support the QCassandraValue:
 *
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 *
 * \param[in] cmd  The command.
 * \param[in] value  The value attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, QtCassandra::QCassandraValue value)
{
    cmd_info_t inst(cmd, value);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QDomElement.
 *
 * The following commands support the QDomElement:
 *
 * \li COMMAND_ELEMENT
 *
 * \param[in] cmd  The command.
 * \param[in] element  The element attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, QDomElement element)
{
    cmd_info_t inst(cmd, element);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QDomDocument.
 *
 * The following commands support the QDomDocument:
 *
 * \li COMMAND_ELEMENT
 *
 * \param[in] cmd  The command.
 * \param[in] element  The element attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, QDomDocument doc)
{
    cmd_info_t inst(cmd, doc);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a search_result_t reference.
 *
 * The following commands support the result reference:
 *
 * \li COMMAND_RESULT
 *
 * \param[in] cmd  The command.
 * \param[in] result  The result attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, search_result_t& result)
{
    cmd_info_t inst(cmd, result);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a path_info_t reference.
 *
 * The following commands support the path_info_t reference:
 *
 * \li COMMAND_PATH_INFO_GLOBAL
 * \li COMMAND_PATH_INFO_BRANCH
 * \li COMMAND_PATH_INFO_REVISION
 *
 * \param[in] cmd  The command.
 * \param[in] ipath  The ipath attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, path_info_t & ipath)
{
    cmd_info_t inst(cmd, ipath);
    f_program.push_back(inst);
    return *this;
}


/** \brief Run the search commands.
 *
 * This function runs the search commands over the data found in Cassandra.
 * It is somewhat similar to an XPath only it applies to a tree in Cassandra
 * instead of an XML tree.
 *
 * By default, you are expected to search for the very first instance of
 * the parameter sought. It is possible to transform the search in order
 * to search all the parameters that match.
 *
 * \return An array of QCassandraValue's.
 */
void field_search::run()
{
    struct auto_search
    {
        auto_search(char const *filename, char const *func, int line, snap_child *snap, cmd_info_vector_t& program)
            : f_content_plugin(content::content::instance())
            , f_filename(filename)
            , f_function(func)
            , f_line(line)
            , f_snap(snap)
            , f_program(program)
            //, f_mode(mode_t::SEARCH_MODE_FIRST) -- auto-init
            , f_site_key(f_snap->get_site_key_with_slash())
            , f_revision_owner(f_content_plugin->get_plugin_name())
            //, f_field_name("") -- auto-init
            //, f_self("") -- auto-init
            , f_current_table(f_content_plugin->get_content_table())
            //, f_element() -- auto-init
            //, f_found_self(false) -- auto-init
            //, f_saved(false) -- auto-init
            //, f_result() -- auto-init
            //, f_variables() -- auto-init
            //, f_path_info() -- auto-init
        {
        }

        void cmd_field_name(QString const& field_name)
        {
            if(field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("COMMAND_FIELD_NAME cannot be set to an empty string");
            }
            f_field_name = field_name;
        }

        void cmd_field_name_with_vars(QString const& field_name)
        {
            if(field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("COMMAND_FIELD_NAME_WITH_VARS cannot be set to an empty string");
            }
            f_field_name.clear();
            QByteArray name(field_name.toUtf8());
            for(char const *n(name.data()); *n != '\0'; ++n)
            {
                if(*n == '$')
                {
                    if(n[1] != '{')
                    {
                        throw content_exception_invalid_sequence(QString("COMMAND_FIELD_NAME_WITH_VARS variable name \"%1\" must be enclosed in { and }.").arg(field_name));
                    }
                    QString varname;
                    for(n += 2; *n != '}'; ++n)
                    {
                        if(*n == '\0')
                        {
                            throw content_exception_invalid_sequence(QString("COMMAND_FIELD_NAME_WITH_VARS variable \"%1\" not ending with }.").arg(field_name));
                        }
                        varname += *n;
                    }
                    if(!f_variables.contains(varname))
                    {
                        throw content_exception_invalid_sequence(QString("COMMAND_FIELD_NAME_WITH_VARS variable \"%1\" is not defined.").arg(varname));
                    }
                    f_field_name += f_variables[varname];
                }
                else
                {
                    f_field_name += *n;
                }
            }
        }

        void cmd_mode(int64_t mode)
        {
            f_mode = static_cast<mode_t>(mode);
        }

        void cmd_branch_path(int64_t main_page)
        {
            // retrieve the path from this cell:
            //   content::revision_control::current_branch_key
            f_path_info.set_path(f_self);
            f_path_info.set_main_page(main_page != 0);
            cmd_path(f_path_info.get_branch_key());

            // make sure the current table is the branch table
            f_current_table = f_content_plugin->get_branch_table();
        }

        void cmd_revision_path(int64_t main_page)
        {
            // retrieve the path from this cell:
            //   content::revision_control::current_revision_key::<branch>::<locale>
            f_path_info.set_path(f_self);
            f_path_info.set_main_page(main_page != 0);
            cmd_path(f_path_info.get_revision_key());

            // make sure the current table is the revision table
            f_current_table = f_content_plugin->get_revision_table();
        }

        void cmd_table(QString const& name)
        {
            if(name == get_name(name_t::SNAP_NAME_CONTENT_TABLE))
            {
                f_current_table = f_content_plugin->get_content_table();
            }
            else if(name == get_name(name_t::SNAP_NAME_CONTENT_BRANCH_TABLE))
            {
                f_current_table = f_content_plugin->get_branch_table();
            }
            else if(name == get_name(name_t::SNAP_NAME_CONTENT_REVISION_TABLE))
            {
                f_current_table = f_content_plugin->get_revision_table();
            }
            else
            {
                throw content_exception_invalid_sequence("COMMAND_TABLE expected the name of the table to access: \"content\", \"branch\", or \"revision\"");
            }
        }

        void cmd_self(QString const & self)
        {
            // verify that a field name is defined
            if(f_field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("the field_search cannot check COMMAND_SELF without first being given a COMMAND_FIELD_NAME");
            }

            if(f_current_table->exists(self)
            && f_current_table->row(self)->exists(f_field_name))
            {
                f_found_self = true;

                // found a field, add it to result
                if(mode_t::SEARCH_MODE_PATHS == f_mode)
                {
                    // save the path(s) only
                    f_result.push_back(self);
                }
                else
                {
                    // save the value
                    f_result.push_back(f_current_table->row(self)->cell(f_field_name)->value());
                }
            }
        }

        void cmd_path(QString const & path)
        {
            f_found_self = false;

            // get the self path and add the site key if required
            // (it CAN be empty in case we are trying to access the home page
            f_self = path;
            if(f_self.isEmpty() || !f_self.startsWith(f_site_key))
            {
                // path does not yet include the site key
                f_snap->canonicalize_path(f_self);
                f_self = f_site_key + f_self;
            }
        }

        void cmd_path_info(path_info_t & ipath, content::param_revision_t mode)
        {
            switch(mode)
            {
            case content::param_revision_t::PARAM_REVISION_GLOBAL:
                cmd_path(ipath.get_cpath());
                f_current_table = f_content_plugin->get_content_table();
                break;

            case content::param_revision_t::PARAM_REVISION_BRANCH:
                cmd_path(ipath.get_branch_key());
                f_current_table = f_content_plugin->get_branch_table();
                break;

            case content::param_revision_t::PARAM_REVISION_REVISION:
                cmd_path(ipath.get_revision_key());
                f_current_table = f_content_plugin->get_revision_table();
                break;

            default:
                throw snap_logic_exception(QString("invalid mode (%1) in cmd_path_info.").arg(static_cast<int>(mode)));

            }
        }

        void cmd_children(int64_t depth)
        {
            // invalid depth?
            if(depth < 0)
            {
                throw content_exception_invalid_sequence("COMMAND_CHILDREN expects a depth of 0 or more");
            }
            if(depth == 0 || !f_found_self)
            {
                // no depth or no self
                return;
            }

            QString match;

            // last part is dynamic?
            // (later we could support * within the path and not just at the
            // very end...)
            if(f_self.endsWith("::*"))
            {
                int const pos = f_self.lastIndexOf('/');
                if(pos == -1)
                {
                    throw content_exception_invalid_name("f_self is expected to always include at least one slash, \"" + f_self + "\" does not");
                }
                // the match is everything except the '*'
                match = f_self.left(f_self.length() - 1);
                f_self = f_self.left(pos);
            }

            snap_string_list children;
            children += f_self;

            for(int i(0); i < children.size(); ++i, --depth)
            {
                // first loop through all the children of self for f_field_name
                // and if depth is larger than 1, repeat the process with those children
                path_info_t ipath;
                ipath.set_path(children[i]);
                links::link_info info(get_name(name_t::SNAP_NAME_CONTENT_CHILDREN), false, ipath.get_key(), ipath.get_branch());
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                links::link_info child_info;
                while(link_ctxt->next_link(child_info))
                {
                    QString const child(child_info.key());
                    if(match.isEmpty() || child.startsWith(match))
                    {
                        cmd_self(child);
                        if(!f_result.isEmpty() && mode_t::SEARCH_MODE_FIRST == f_mode)
                        {
                            return;
                        }

                        if(depth >= 2)
                        {
                            // record this child as its children will have to be tested
                            children += child;
                        }
                    }
                }
            }
        }

        void cmd_parents(QString limit_path)
        {
            // verify that a field name is defined in self or any parent
            if(f_field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("the field_search cannot check COMMAND_PARENTS without first being given a COMMAND_FIELD_NAME");
            }
            if(!f_found_self)
            {
                return;
            }

            // fix the parent limit
            if(!limit_path.startsWith(f_site_key) || limit_path.isEmpty())
            {
                // path does not yet include the site key
                f_snap->canonicalize_path(limit_path);
                limit_path = f_site_key + limit_path;
            }

            if(f_self.startsWith(limit_path))
            {
                // we could use the parent link from each page, but it is
                // a lot faster to compute it each time (no db access)
                snap_string_list parts(f_self.right(f_self.length() - f_site_key.length()).split('/'));
                while(!parts.isEmpty())
                {
                    parts.pop_back();
                    QString self(parts.join("/"));
                    cmd_self(f_site_key + self);
                    if((!f_result.isEmpty() && mode_t::SEARCH_MODE_FIRST == f_mode)
                    || self == limit_path)
                    {
                        return;
                    }
                }
            }
        }

        void cmd_link(QString const& link_name)
        {
            if(!f_found_self)
            {
                // no self, no link to follow
                return;
            }

            bool const unique_link(true);
            path_info_t ipath;
            ipath.set_path(f_self);
            links::link_info info(link_name, unique_link, ipath.get_key(), ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
            links::link_info type_info;
            if(link_ctxt->next_link(type_info))
            {
                f_self = type_info.key();
                cmd_self(f_self);
            }
            else
            {
                // no such link
                f_self.clear();
                f_found_self = false;
            }
        }

        void cmd_default_value(QtCassandra::QCassandraValue const& value, bool keep_null)
        {
            if(!value.nullValue() || keep_null)
            {
                f_result.push_back(value);
            }
        }

        void cmd_element(QDomElement element)
        {
            f_element = element;
        }

        // retrieve an element given a path, element must exist, if
        // not there it ends up being NULL; test with COMMAND_IF_ELEMENT_NULL
        // and COMMAND_IF_NOT_ELEMENT_NULL
        void cmd_path_element(QString const& child_name)
        {
            if(!f_element.isNull())
            {
                snap_string_list names(child_name.split("/"));
                int const max_names(names.size());
                for(int idx(0); idx < max_names && !f_element.isNull(); ++idx)
                {
                    QString name(names[idx]);
                    if(name.isEmpty())
                    {
                        // happens when child_name starts/ends with '/'
                        continue;
                    }
                    f_element = f_element.firstChildElement(name);
                }
            }
        }

        void cmd_child_element(QString const& child_name)
        {
            if(!f_element.isNull())
            {
                QDomElement child(f_element.firstChildElement(child_name));
                if(child.isNull())
                {
                    // it does not exist yet, add it
                    QDomDocument doc(f_element.ownerDocument());
                    child = doc.createElement(child_name);
                    f_element.appendChild(child);
                }
                f_element = child;
            }
        }

        void cmd_new_child_element(QString const& child_name)
        {
            if(!f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QDomElement child(doc.createElement(child_name));
                f_element.appendChild(child);
                f_element = child;
            }
        }

        void cmd_parent_element()
        {
            if(!f_element.isNull())
            {
                f_element = f_element.parentNode().toElement();
            }
        }

        void cmd_element_text()
        {
            if(!f_element.isNull())
            {
                f_result.push_back(f_element.text());
            }
        }

        void cmd_element_attr(QString const& attr)
        {
            if(!f_element.isNull())
            {
                snap_string_list a(attr.split('='));
                if(a.size() == 1)
                {
                    // checked="checked"
                    a += a[0];
                }
                f_element.setAttribute(a[0], a[1]);
            }
        }

        void cmd_reset(bool status)
        {
            f_saved = status;
            f_result.clear();
        }

        void cmd_result(search_result_t& result)
        {
            result = f_result;
        }

        void cmd_last_result_to_var(QString const& varname)
        {
            if(f_result.isEmpty())
            {
                throw content_exception_invalid_sequence(QString("no result to save in variable \"%1\"").arg(varname));
            }
            QtCassandra::QCassandraValue value(f_result.last());
            f_result.pop_back();
            f_variables[varname] = value.stringValue();
        }

        void cmd_save(QString const & child_name, command_t command)
        {
            struct parser
            {
                enum class token_t
                {
                    TOKEN_EOF,
                    TOKEN_OPEN_ATTR,
                    TOKEN_CLOSE_ATTR,
                    TOKEN_SLASH,
                    TOKEN_EQUAL,
                    TOKEN_IDENTIFIER
                };

                parser(QString const & child_name)
                    : f_child_name(child_name)
                    //, f_pos(0) -- auto-init
                    , f_length(f_child_name.length())
                    //, f_keep_result(false) -- auto-init
                {
                    if(f_length > 0 && f_child_name[0].unicode() == '*')
                    {
                        f_keep_result = true;
                        ++f_pos;
                    }
                }

                int getc()
                {
                    if(f_pos >= f_length)
                    {
                        return EOF;
                    }

                    int c(f_child_name[f_pos].unicode());
                    ++f_pos;
                    return c;
                }

                token_t get_token(QString& value)
                {
                    value = "";
                    int c(getc());
                    if(c == EOF)
                    {
                        return token_t::TOKEN_EOF;
                    }

                    switch(c)
                    {
                    case '[':
                        return token_t::TOKEN_OPEN_ATTR;

                    case ']':
                        return token_t::TOKEN_CLOSE_ATTR;

                    case '/':
                        return token_t::TOKEN_SLASH;

                    case '=':
                        return token_t::TOKEN_EQUAL;

                    case '\'':
                    case '"':
                        // we got a string, read all up to the closing quote
                        {
                            int const quote(c);
                            for(;;)
                            {
                                c = getc();
                                if(c == EOF)
                                {
                                    throw content_exception_invalid_sequence(QString("invalid string definition, missing closing quote (%1).").arg(quote));
                                }
                                if(c == quote)
                                {
                                    return token_t::TOKEN_IDENTIFIER;
                                }
                                // that does not seem necessary
                                // (i.e. use &quot; and &#27; for quotes in strings)
                                //if(c == '\\')
                                //{
                                //    c = getc();
                                //    if(c == EOF)
                                //    {
                                //        throw content_exception_invalid_sequence("invalid string definition, missing escaped character.");
                                //    }
                                //}
                                value += c;
                            }
                        }
                        break;

                    default:
                        for(;;)
                        {
                            value += c;
                            c = getc();
                            if(c == EOF)
                            {
                                return token_t::TOKEN_IDENTIFIER;
                            }
                            if(c == '\''
                            || c == '"')
                            {
                                throw content_exception_invalid_sequence("invalid string definition appearing in the middle of nowhere.");
                            }
                            if(c == '['
                            || c == ']'
                            || c == '/'
                            || c == '=')
                            {
                                //ungetc();
                                --f_pos;
                                return token_t::TOKEN_IDENTIFIER;
                            }
                            // that does not seem necessary
                            //if(c == '\\')
                            //{
                            //    c = getc();
                            //    if(c == EOF)
                            //    {
                            //        throw content_exception_invalid_sequence("invalid identifier definition, missing escaped character");
                            //    }
                            //}
                        }
                        break;

                    }

                    NOTREACHED();
                }

                bool keep_result() const
                {
                    return f_keep_result;
                }

                QString const               f_child_name;
                uint32_t                    f_pos = 0;
                uint32_t                    f_length = -1;
                bool                        f_keep_result = false;
            };

            if(!f_result.isEmpty() && !f_element.isNull())
            {
                // supported syntax goes like this:
                //
                // path: segments
                //     | segments '/'
                //
                // segments: child
                //         | child attribute
                //         | segments '/' segments
                //
                // child: IDENTIFIER
                //
                // attribute: '[' IDENFITIER = value ']'
                //          | attribute attribute
                //
                // value: IDENTIFIER
                //      | "'" ANY "'"
                //      | '"' ANY '"'
                //
                // IDENTIFIER is any character except '[', ']', '=', '/',
                // '"', and "'".
                //
                // ANY represents any character except the ending quote.
                //
                // Example:
                //     desc[@type="filter"]/data
                //
                // Note that the '@' before the attribute name and the
                // quotation of "filter" are optional. At times an
                // attribute value includes slashes or square brackets.
                // In that case you must use quotes:
                //     formats[href="http://snapwebsites.org/"]/title
                //
                QDomDocument doc(f_element.ownerDocument());
                parser p(child_name);
                QDomElement parent(f_element);
                QDomElement child;
                QString v;
                parser::token_t t(p.get_token(v));
                while(t != parser::token_t::TOKEN_EOF)
                {
                    // we must have an identifier before attributes or '/'
                    //    <path>
                    if(t != parser::token_t::TOKEN_IDENTIFIER)
                    {
                        throw content_exception_invalid_sequence(QString("syntax error in field name \"%1\", expected a path name got token %2 instead").arg(child_name).arg(static_cast<int>(t)));
                    }
                    child = doc.createElement(v);
                    parent.appendChild(child);

                    // start an attribute?
                    //    '['
                    t = p.get_token(v);
                    while(t == parser::token_t::TOKEN_OPEN_ATTR)
                    {
                        // attribute name
                        //    <name>
                        QString attr_name;
                        t = p.get_token(attr_name);
                        if(t != parser::token_t::TOKEN_IDENTIFIER)
                        {
                            throw content_exception_invalid_sequence("attribute name expected after a '['");
                        }
                        // allow the attribute name to start with @
                        if(attr_name.length() > 0 && attr_name[0] == '@')
                        {
                            attr_name.remove(0, 1);
                        }
                        if(attr_name.isEmpty())
                        {
                            throw content_exception_invalid_sequence("the attribute must be given a valid name");
                        }

                        // got an attribute value?
                        //    '='
                        QString attr_value;
                        t = p.get_token(v);
                        if(t == parser::token_t::TOKEN_EQUAL)
                        {
                            // we have a value, we are setting the attribute
                            //     <value>
                            t = p.get_token(attr_value);
                            if(t != parser::token_t::TOKEN_IDENTIFIER)
                            {
                                throw content_exception_invalid_sequence("attribute name expected after an '='");
                            }
                            // move forward for
                            //     ']'
                            t = p.get_token(v);
                        }
                        else
                        {
                            // this is an attribute such as:
                            //    default="default"
                            attr_value = attr_name;
                        }
                        child.setAttribute(attr_name, attr_value);

                        // make sure we have a closing ']'
                        //     ']'
                        if(t != parser::token_t::TOKEN_CLOSE_ATTR)
                        {
                            throw content_exception_invalid_sequence(QString("attribute must end with ']' in %1, got token %2").arg(child_name).arg(static_cast<int>(t)));
                        }

                        t = p.get_token(v);
                    }

                    if(t != parser::token_t::TOKEN_EOF)
                    {
                        if(t != parser::token_t::TOKEN_SLASH)
                        {
                            throw content_exception_invalid_sequence(QString("expect a slash '/' instead of %1 to seperate each child name in \"%2\".").arg(static_cast<int>(t)).arg(child_name));
                        }
                        t = p.get_token(v);
                        parent = child;
                    }
                }
                if(child.isNull())
                {
                    throw content_exception_invalid_sequence("no name defined in the field name string, at least one is required for the save command");
                }
                switch(command)
                {
                case command_t::COMMAND_SAVE:
                    // the data is expected to be plain text
                    {
                        QDomText text(doc.createTextNode(f_result[0].stringValue()));
                        child.appendChild(text);
                    }
                    break;

                case command_t::COMMAND_SAVE_FLOAT64:
                    {
                        QDomText text(doc.createTextNode(QString("%1").arg(f_result[0].safeDoubleValue())));
                        child.appendChild(text);
                    }
                    break;

                case command_t::COMMAND_SAVE_INT64:
                    {
                        QDomText text(doc.createTextNode(QString("%1").arg(f_result[0].safeInt64Value())));
                        child.appendChild(text);
                    }
                    break;

                case command_t::COMMAND_SAVE_INT64_DATE:
                    {
                        QDomText text(doc.createTextNode(f_snap->date_to_string(f_result[0].safeInt64Value())));
                        child.appendChild(text);
                    }
                    break;

                case command_t::COMMAND_SAVE_INT64_DATE_AND_TIME:
                    {
                        QDomText text(doc.createTextNode(f_snap->date_to_string(f_result[0].safeInt64Value(), snap_child::date_format_t::DATE_FORMAT_LONG)));
                        child.appendChild(text);
                    }
                    break;

                case command_t::COMMAND_SAVE_PLAIN:
                    // the data is expected to be HTML that has to be saved as plain text
                    {
                        QDomText text(doc.createTextNode(snap_dom::remove_tags(f_result[0].stringValue())));
                        child.appendChild(text);
                    }
                    break;

                case command_t::COMMAND_SAVE_XML:
                    // the data is expected to be valid XML (XHTML)
                    snap_dom::insert_html_string_to_xml_doc(child, f_result[0].stringValue());
                    break;

                default:
                    throw content_exception_type_mismatch(QString("command %1 not supported in cmd_save()").arg(static_cast<int>(command)));

                }
                if(!p.keep_result())
                {
                    cmd_reset(true);
                }
            }
        }

        void cmd_if_found(int& i, int64_t label, bool equal)
        {
            if(f_result.isEmpty() == equal)
            {
                cmd_goto(i, label);
            }
        }

        void cmd_if_element_null(int& i, int64_t label, bool equal)
        {
            if(f_element.isNull() == equal)
            {
                cmd_goto(i, label);
            }
        }

        void cmd_goto(int& i, int64_t label)
        {
            int const max_size(f_program.size());
            for(int j(0); j < max_size; ++j)
            {
                if(f_program[j].get_command() == command_t::COMMAND_LABEL
                && f_program[j].get_int64() == label)
                {
                    // NOTE: the for() loop will do a ++i which is fine
                    //       since we're giving the label position here
                    i = j;
                    return;
                }
            }
            throw content_exception_invalid_sequence(QString("found unknown label %1 at %2").arg(label).arg(i));
        }

        void cmd_warning(QString const& warning_msg)
        {
            // XXX only problem is we do not get the right filename,
            //     line number, function name on this one...
            if(!f_saved)
            {
                SNAP_LOG_WARNING("in ")(f_filename)(":")(f_function)(":")(f_line)(": ")(warning_msg)(" (path: \"")(f_self)("\" and field name: \"")(f_field_name)("\")");
                f_saved = false;
            }
        }

        void run()
        {
            int const max_size(f_program.size());
            for(int i(0); i < max_size; ++i)
            {
                switch(f_program[i].get_command())
                {
                case command_t::COMMAND_RESET:
                    cmd_reset(false);
                    break;

                case command_t::COMMAND_FIELD_NAME:
                    cmd_field_name(f_program[i].get_string());
                    break;

                case command_t::COMMAND_FIELD_NAME_WITH_VARS:
                    cmd_field_name_with_vars(f_program[i].get_string());
                    break;

                case command_t::COMMAND_MODE:
                    cmd_mode(f_program[i].get_int32());
                    break;

                case command_t::COMMAND_BRANCH_PATH:
                    cmd_branch_path(f_program[i].get_int64());
                    break;

                case command_t::COMMAND_REVISION_PATH:
                    cmd_revision_path(f_program[i].get_int64());
                    break;

                case command_t::COMMAND_TABLE:
                    cmd_table(f_program[i].get_string());
                    break;

                case command_t::COMMAND_SELF:
                    cmd_self(f_self);
                    break;

                case command_t::COMMAND_PATH:
                    cmd_path(f_program[i].get_string());
                    break;

                case command_t::COMMAND_PATH_INFO_GLOBAL:
                    cmd_path_info(f_program[i].get_ipath(), content::param_revision_t::PARAM_REVISION_GLOBAL);
                    break;

                case command_t::COMMAND_PATH_INFO_BRANCH:
                    cmd_path_info(f_program[i].get_ipath(), content::param_revision_t::PARAM_REVISION_BRANCH);
                    break;

                case command_t::COMMAND_PATH_INFO_REVISION:
                    cmd_path_info(f_program[i].get_ipath(), content::param_revision_t::PARAM_REVISION_REVISION);
                    break;

                case command_t::COMMAND_CHILDREN:
                    cmd_children(f_program[i].get_int64());
                    break;

                case command_t::COMMAND_PARENTS:
                    cmd_parents(f_program[i].get_string());
                    break;

                case command_t::COMMAND_LINK:
                    cmd_link(f_program[i].get_string());
                    break;

                case command_t::COMMAND_DEFAULT_VALUE:
                    cmd_default_value(f_program[i].get_value(), true);
                    break;

                case command_t::COMMAND_DEFAULT_VALUE_OR_NULL:
                    cmd_default_value(f_program[i].get_value(), false);
                    break;

                case command_t::COMMAND_ELEMENT:
                    cmd_element(f_program[i].get_element());
                    break;

                case command_t::COMMAND_PATH_ELEMENT:
                    cmd_path_element(f_program[i].get_string());
                    break;

                case command_t::COMMAND_CHILD_ELEMENT:
                    cmd_child_element(f_program[i].get_string());
                    break;

                case command_t::COMMAND_NEW_CHILD_ELEMENT:
                    cmd_new_child_element(f_program[i].get_string());
                    break;

                case command_t::COMMAND_PARENT_ELEMENT:
                    cmd_parent_element();
                    break;

                case command_t::COMMAND_ELEMENT_TEXT:
                    cmd_element_text();
                    break;

                case command_t::COMMAND_ELEMENT_ATTR:
                    cmd_element_attr(f_program[i].get_string());
                    break;

                case command_t::COMMAND_RESULT:
                    cmd_result(*f_program[i].get_result());
                    break;

                case command_t::COMMAND_LAST_RESULT_TO_VAR:
                    cmd_last_result_to_var(f_program[i].get_string());
                    break;

                case command_t::COMMAND_SAVE:
                case command_t::COMMAND_SAVE_FLOAT64:
                case command_t::COMMAND_SAVE_INT64:
                case command_t::COMMAND_SAVE_INT64_DATE:
                case command_t::COMMAND_SAVE_INT64_DATE_AND_TIME:
                case command_t::COMMAND_SAVE_PLAIN:
                case command_t::COMMAND_SAVE_XML:
                    cmd_save(f_program[i].get_string(), f_program[i].get_command());
                    break;

                case command_t::COMMAND_LABEL:
                    // this is a nop
                    break;

                case command_t::COMMAND_IF_FOUND:
                    cmd_if_found(i, f_program[i].get_int64(), false);
                    break;

                case command_t::COMMAND_IF_NOT_FOUND:
                    cmd_if_found(i, f_program[i].get_int64(), true);
                    break;

                case command_t::COMMAND_IF_ELEMENT_NULL:
                    cmd_if_element_null(i, f_program[i].get_int64(), true);
                    break;

                case command_t::COMMAND_IF_NOT_ELEMENT_NULL:
                    cmd_if_element_null(i, f_program[i].get_int64(), false);
                    break;

                case command_t::COMMAND_GOTO:
                    cmd_goto(i, f_program[i].get_int64());
                    break;

                case command_t::COMMAND_WARNING:
                    cmd_warning(f_program[i].get_string());
                    break;

                default:
                    throw content_exception_invalid_sequence(QString("encountered an unknown instruction (%1)").arg(static_cast<int>(f_program[i].get_command())));

                }
                if(!f_result.isEmpty() && mode_t::SEARCH_MODE_FIRST == f_mode)
                {
                    return;
                }
            }
        }

        content *                                       f_content_plugin = nullptr;
        char const *                                    f_filename = nullptr;
        char const *                                    f_function = nullptr;
        int                                             f_line = 0;
        snap_child *                                    f_snap = nullptr;
        cmd_info_vector_t &                             f_program;
        mode_t                                          f_mode = mode_t::SEARCH_MODE_FIRST;
        QString const                                   f_site_key;
        QString                                         f_revision_owner;
        QString                                         f_field_name;
        QString                                         f_self;
        QtCassandra::QCassandraTable::pointer_t         f_current_table;
        QDomElement                                     f_element;
        bool                                            f_found_self = false;
        bool                                            f_saved = false;
        search_result_t                                 f_result;
        variables_t                                     f_variables;
        path_info_t                                     f_path_info;
    } search(f_filename, f_function, f_line, f_snap, f_program);

    search.run();
}


/** \brief This function is used by the FIELD_SEARCH macro.
 *
 * This function creates a field_search object and initializes it
 * with the information specified by the FIELD_SEARCH macro. The
 * result is a field_search that we can use to instantly run a
 * search program.
 *
 * \param[in] filename  The name of the file where the FIELD_SEARCH macro is used.
 * \param[in] func  The name of the function using the FIELD_SEARCH macro.
 * \param[in] line  The line where the FIELD_SEARCH macro can be found.
 * \param[in] snap  A pointer to the snap server.
 */
field_search create_field_search(char const *filename, char const *func, int line, snap_child *snap)
{
    field_search fs(filename, func, line, snap);
    return fs;
}



SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
