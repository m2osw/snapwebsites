// Snap Websites Server -- verify and manage version and names in filenames
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

#include "snapwebsites/snap_version.h"

#include "snapwebsites/minmax.h"
#include "snapwebsites/not_reached.h"
#include "snapwebsites/snap_string_list.h"

#include "snapwebsites/poison.h"

namespace snap
{
namespace snap_version
{


namespace
{
/** \brief List of operators.
 *
 * Operators are 1 or 2 characters. This string lists all the operators.
 * It is safe because the operators are used from a limited_auto_init
 * variable. Each string here is exactly 3 characters and thus we
 * can use the index x 3 to access the string operator.
 */
char const * g_operators = "\0\0\0"  // OPERATOR_UNORDERED
                           "=\0\0"   // OPERATOR_EQUAL
                           "!=\0"    // OPERATOR_EXCEPT
                           "<\0\0"   // OPERATOR_EARLIER
                           ">\0\0"   // OPERATOR_LATER
                           "<=\0"    // OPERATOR_EARLIER_OR_EQUAL
                           ">=\0";   // OPERATOR_LATER_OR_EQUAL
} // no name namespace


/** \brief Find the extension used from a list of extension.
 *
 * This function checks the end of the \p filename for a match with one
 * of the specified extensions and returns the extension that matches.
 *
 * Note that the list of extensions MUST be sorted from the longest
 * extension first to the shortest last.
 *
 * The extensions are all expected
 *
 * \param[in] filename  The name of the file to check.
 * \param[in] extensions  A nullptr terminated array of extensions.
 *
 * \return The pointer of the extensions that matched or nullptr.
 */
char const * find_extension(QString const& filename, char const **extensions)
{
#ifdef DEBUG
    // we expect at least one extensions in the list
    size_t length(strlen(extensions[0]));
#endif
    do
    {
#ifdef DEBUG
        size_t const l(strlen(*extensions));
        if(l > length)
        {
            throw snap_logic_exception(QString("Extension \"%1\" is longer than the previous extension \"%2\".")
                    .arg(*extensions).arg(extensions[-1]));
        }
        length = l;
#endif
        if(filename.endsWith(*extensions))
        {
            return *extensions;
        }

        // not that one, move on
        ++extensions;
    }
    while(*extensions != nullptr);

    return nullptr;
}


/** \brief Verify that the specified name is valid.
 *
 * A basic name must be composed of letters ([a-z]), digits ([0-9]), and
 * dashes (-). This function makes sure that the name only includes
 * those characters. The minimum size must also be two characters.
 *
 * Also, the name cannot have two or more dashes in a row, it also must
 * start with a letter and cannot end with a dash.
 *
 * Note that only lower case ASCII letters are accepted ([a-z]).
 *
 * The following is the regular expression representing a basic name:
 *
 * \code
 *  [a-z][-a-z0-9]*[a-z0-9]
 * \endcode
 *
 * \param[in] name  The name being checked.
 * \param[out] error  A string where the error is saved if one is discovered.
 *
 * \return true if the name is considered valid, false if an error is
 *         found and the error string is set to that error.
 */
bool validate_basic_name(QString const & name, QString & error)
{
    error.clear();

    // length constraint
    int const max_length(name.length());
    if(max_length < 2)
    {
        // name must be at least 2 characters
        error = QString("the name or browser in a versioned filename must be at least two characters. \"%1\" is not valid.").arg(name);
        return false;
    }

    // first character constraint
    ushort c(name[0].unicode());
    if(c < 'a' || c > 'z')
    {
        // name cannot start with dash (-) or a digit ([0-9])
        error = QString("the name or browser of a versioned filename must start with a letter [a-z]. \"%1\" is not valid.").arg(name);
        return false;
    }

    // inside constraints
    ushort p(c);
    for(int i(0); i < max_length; ++i, p = c)
    {
        c = name.at(i).unicode();
        if(c == '-')
        {
            // two '-' in a row constraint
            if(p == '-')
            {
                // found two '-' in a row
                error = QString("a name or browser versioned filename cannot include two dashes (--) one after another. \"%1\" is not valid.").arg(name);
                return false;
            }
        }
        else if((c < '0' || c > '9')
             && (c < 'a' || c > 'z'))
        {
            // name can only include [a-z0-9] and dashes (-)
            error = QString("a name or browser versioned filename can only include letters (a-z), digits (0-9), and dashes (-). \"%1\" is not valid.").arg(name);
            return false;
        }
    }

    // no ending '-' constraint
    if(c == '-') // c and p have the value, the last character
    {
        error = QString("a versioned name or browser cannot end with a dash (-) or a colon (:). \"%1\" is not valid.").arg(name);
        return false;
    }

    return true;
}


/** \brief Verify that the name or browser strings are valid.
 *
 * The \p name parameter is checked for validity. It may be composed
 * of a namespace and a name separated by the namespace scope operator (::).
 *
 * If no scope operator is found in the name, then no namespace is defined
 * and that parameter remains empty.
 *
 * \code
 *      [<namespace>::]<name>
 * \endcode
 *
 * The namespace and name parts must both be valid basic names.
 *
 * The names must exclusively be composed of lowercase letters. This will
 * allow, one day, to run Snap! on computers that do not distinguish
 * between case (i.e. Mac OS/X.)
 *
 * \note
 * This function is used to verify the name and the browser strings.
 *
 * \param[in,out] name  The name to be checked against the name pattern.
 * \param[out] error  The error message in case an error occurs.
 * \param[out] namespace_string  The namespace if one was defined.
 *
 * \return true if the name matches, false otherwise.
 *
 * \sa validate_basic_name()
 */
bool validate_name(QString & name, QString & error, QString & namespace_string)
{
    error.clear();
    namespace_string.clear();

    int const pos(name.indexOf("::"));
    if(pos != -1)
    {
        // name includes a namespace
        namespace_string = name.mid(0, pos);
        name = name.mid(pos + 2);
        if(!validate_basic_name(namespace_string, error))
        {
            return false;
        }
    }

    return validate_basic_name(name, error);
}


/** \brief Validate a version.
 *
 * This function validates a version string and returns the result.
 *
 * The validation includes three steps:
 *
 * \li Parse the input \p version_string parameter in separate numbers.
 * \li Save those numbers in the \p version vector.
 * \li Canonicalized the \p version vector by removing ending zeroes.
 *
 * The function only supports sets of numbers in the version. Something
 * similar to 1.2.3. The regex of \p version_string looks like this:
 *
 * \code
 * [0-9]+(\.[0-9]+)*
 * \endcode
 *
 * The versions are viewed as:
 *
 * \li Major Release Version (public)
 * \li Minor Release Version (public)
 * \li Patch Version (still public)
 * \li Development or Build Version (not public)
 *
 * While in development, each change should be reflected by incrementing
 * the development (or build) version number by 1. That way your browser
 * will automatically reload the new file.
 *
 * Once the development is over and a new version is to be released,
 * remove the development version or reset it to zero and increase the
 * Patch Version, or one of the Release Versions as appropriate.
 *
 * If you are trying to install a 3rd party JavaScript library which uses
 * a different scheme for their version, learn of their scheme and adapt
 * it to our versions. For example, a version defined as:
 *
 * \code
 * <major-version>.<minor-version>[<patch>]
 * \endcode
 *
 * where \<patch\> is a letter, can easily be converted to a 1.2.3 type of
 * version where the letters are numbered starting at 1 (if no patch letter,
 * use zero.)
 *
 * In the end the function returns an array of integers in the \p version
 * parameter. This parameter is used by subsequent compare() calls.
 *
 * \note
 * The version "0" is considered valid although maybe not useful (We
 * suggest that you do not use it, use at least 0.0.0.1)
 *
 * \note
 * Although we only mention 4 numbers in a version, this function does
 * not enforce a limit. So you could use 5 or more numbers in your
 * version definitions.
 *
 * \param[in,out] version_string  The version to be parsed.
 * \param[out] version  The array of numbers.
 * \param[out] error  The error message if an error occurs.
 *
 * \return true if the version is considered valid.
 */
bool validate_version(QString const & version_string, version_numbers_vector_t & version, QString & error)
{
    version.clear();

    int const max_length(version_string.length());
    if(max_length < 1)
    {
        error = "The version in a versioned filename is required after the name. \"" + version_string + "\" is not valid.";
        return false;
    }
    if(version_string.at(max_length - 1).unicode() == '.')
    {
        error = "The version in a versioned filename cannot end with a period. \"" + version_string + "\" is not valid.";
        return false;
    }

    for(int i(0); i < max_length;)
    {
        // force the version to have a digit at the start
        // and after each period
        ushort c(version_string.at(i).unicode());
        if(c < '0' || c > '9')
        {
            error = "The version of a versioned filename is expected to have a number at the start and after each period. \"" + version_string + "\" is not valid.";
            return false;
        }
        int value(c - '0');
        // start with ++i because we just checked character 'i'
        for(++i; i < max_length;)
        {
            c = version_string.at(i).unicode();
            ++i;
            if(c < '0' || c > '9')
            {
                if(c != '.')
                {
                    error = "The version of a versioned filename is expected to be composed of numbers and periods (.) only. \"" + version_string + "\" is not valid.";
                    return false;
                }
                if(i == max_length)
                {
                    throw snap_logic_exception("The version_string was already checked for an ending '.' and yet we reached that case later in the function.");
                }
                break;
            }
            value = value * 10 + c - '0';
        }
        version.push_back(value);
    }

    // canonicalize the array
    while(version.size() > 1 && 0 == version[version.size() - 1])
    {
        version.pop_back();
    }

    return true;
}


/** \brief Validate an operator string.
 *
 * This function validates an operator string and converts it to an operator
 * enumeration.
 *
 * The function clears the error string by default. If the operator cannot
 * be converted, then an error message is saved in that string.
 *
 * Supported operators are:
 *
 * \li '=' or '=='
 * \li '!=' or '<>'
 * \li '<'
 * \li '<='
 * \li '>'
 * \li '>='
 *
 * Note that '==' and '<>' are extension. These are accepted by the
 * canonicalized version are '=' and '!=' respectively.
 *
 * \note
 * Internally the strings get canonicalized in the version_operator
 * object. The get_operator_string() function always returns a canonicalized
 * version of the operator.
 *
 * \param[in] operator_string  The operator to convert.
 * \param[out] op  The new operator.
 * \param[out] error  An error string if the operator is not valid.
 *
 * \return true if the operator is valid.
 */
bool validate_operator(QString const & operator_string, operator_t & op, QString & error)
{
    op = operator_t::OPERATOR_UNORDERED;
    error.clear();

    if(operator_string == "=" || operator_string == "==")
    {
        op = operator_t::OPERATOR_EQUAL;
    }
    else if(operator_string == "!=" || operator_string == "<>")
    {
        op = operator_t::OPERATOR_EXCEPT;
    }
    else if(operator_string == "<")
    {
        // support << as well, like in Debian?
        op = operator_t::OPERATOR_EARLIER;
    }
    else if(operator_string == ">")
    {
        // support >> as well, like in Debian?
        op = operator_t::OPERATOR_LATER;
    }
    else if(operator_string == "<=")
    {
        op = operator_t::OPERATOR_EARLIER_OR_EQUAL;
    }
    else if(operator_string == ">=")
    {
        op = operator_t::OPERATOR_LATER_OR_EQUAL;
    }
    else
    {
        error = "Operator " + operator_string + " is not recognized as a valid operator.";
        return false;
    }

    return true;
}


/** \fn name::clear()
 * \brief Clear the name.
 *
 * This is the only way to clear a name object. This function clears the
 * name (makes it an empty name) and clears the error message if there was
 * one.
 *
 * Note that set_name() cannot be used with an empty string because that is
 * not a valid entry. Names have to be at least two characters.
 *
 * By default, when a name object is constructed, the name is empty.
 */


/** \brief Set the name of the string.
 *
 * Set the name of the item. This function verifies that the name is valid,
 * if so the function returns true and saves the new name in the name object.
 * Otherwise it doesn't change anything and returns false.
 *
 * This function clears the error by default so that way if no error occurs
 * the get_error() function returns an empty string.
 *
 * \param[in] name_string  The name to save in this object.
 *
 * \return true if the name was valid.
 */
bool name::set_name(QString const& name_string)
{
    QString namespace_string;
    QString the_name(name_string);
    bool const r(validate_name(the_name, f_error, namespace_string));
    if(r)
    {
        f_name = the_name;
        f_namespace = namespace_string;
    }
    return r;
}


/** \fn name::get_name() const
 * \brief Retrieve the name.
 *
 * This function returns the last name that was set with the set_name()
 * function and was valid.
 *
 * This means only valid names or empty names are returned.
 *
 * \return The last name set with set_name().
 */


/** \fn name::is_valid() const
 * \brief Check whether this name is considered valid.
 *
 * Although the set_name() function does not change the old value when it
 * fails, it is considered invalid if the new value was invalid (had a
 * character that is not considered valid in a name, was too short, etc.)
 *
 * This function returns true if the last set_name() generated no error.
 * Note that a new empty name (or after a call to the clear() function)
 * is considered valid even though in most cases a name is mandatory.
 *
 * \return true if the name is currently considered valid.
 */


/** \fn name::get_error() const
 * \brief Retrieve the error.
 *
 * If the set_name() function returns false, then the error message will
 * be set to what happened (why the name was refused.) This error message
 * can be retrieved using this function.
 *
 * The clear() function empties the error message as well as the name.
 *
 * \return The last error message.
 */


/** \brief Compare two names together.
 *
 * This function compares two names together and returns one of the
 * following:
 *
 * \li COMPARE_INVALID -- if the current name is considered invalid
 * \li COMPARE_SMALLER -- if this is smaller than \p rhs
 * \li COMPARE_EQUAL -- if this is equal \p rhs
 * \li COMPARE_LARGER -- if this is larger than \p rhs
 *
 * The special name "any" is viewed as a pattern that matches any
 * name. This comparing "any" against, say, "editor" returns
 * COMPARE_EQUAL. "any" can appear in this name or the rhs name.
 *
 * \param[in] rhs  The right hand side name to compare with.
 *
 * \return one of the COMPARE_... values (-2, -1, 0, 1).
 */
compare_t name::compare(name const& rhs) const
{
    if(!is_valid() || !rhs.is_valid())
    {
        return compare_t::COMPARE_INVALID;
    }

    if(f_name == "any" || rhs.f_name == "any")
    {
        return compare_t::COMPARE_EQUAL;
    }

    if(f_name < rhs.f_name)
    {
        return compare_t::COMPARE_SMALLER;
    }
    if(f_name > rhs.f_name)
    {
        return compare_t::COMPARE_LARGER;
    }
    return compare_t::COMPARE_EQUAL;
}


/** \brief Set the specified version string as the new version.
 *
 * This function parses the supplied version string in an array of
 * version numbers saved internally.
 *
 * If an error occurs, the current version is not modified and
 * an error message is saved internally. The message can be retrieved
 * with the get_error() function.
 *
 * The error messages is cleared on entry so if no errore are discovered
 * in \p version_string then get_error() returns an empty string.
 *
 * \param[in] version_string  The version string to parse in a series of
 *                            numbers.
 *
 * \return true if the version is considered valid.
 */
bool version::set_version_string(QString const& version_string)
{
    version_numbers_vector_t version_vector;
    bool const r(validate_version(version_string, version_vector, f_error));
    if(r)
    {
        set_version(version_vector);
    }
    return r;
}


/** \brief Set a new version from an array of numbers.
 *
 * This function can be used to set the version directly from a set of
 * numbers. The function canonicalize the version array by removing
 * any ending zeroes.
 *
 * \param[in] version_vector  The value of the new version.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
void version::set_version(version_numbers_vector_t const& version_vector)
{
#pragma GCC diagnostic pop
    f_error.clear();    // no error possible in this case

    // copy and then canonicalize the array
    f_version = version_vector;
    while(f_version.size() > 1 && 0 == f_version[f_version.size() - 1])
    {
        f_version.pop_back();
    }

    // make sure that if we have a new version we get a new version
    // string that corresponds one to one
    f_version_string.clear();
}


/** \brief Set the version operator.
 *
 * By default a version has operator Unordered. In general, a version is
 * 'unordered' when not part of an expression (i.e. in a filename, the
 * version is just that and no operator is defined.) In a list of versions
 * of a dependency, the version is always defined with an operator although
 * by default the \>= operator is not specified.
 *
 * \param[in] op  The version operator.
 */
void version::set_operator(version_operator const& op)
{
    f_operator = op;
}


/** \fn version::get_version() const
 * \brief Retrieve the version as an array of numbers.
 *
 * This function returns the array of numbers representing this version.
 * The array will have been canonicalized, which means it will not end
 * with extra zeroes (it may be zero, if composed of one element.)
 *
 * By default, a version object is empty which means "no version".
 *
 * \return An array of version numbers.
 */


/** \fn version::get_version_string() const
 * \brief Retrieve the version as a canonicalized string.
 *
 * This function returns the version as a canonicalized string. The version
 * is canonnicalized by removing all .0 from the end of a version. So version
 * 1.2 and 1.2.0 will both return string "1.2".
 *
 * \return The canonicalized version string.
 */
QString const& version::get_version_string() const
{
    if(f_version_string.isEmpty() && !f_version.isEmpty())
    {
        // create the version string
        f_version_string = QString("%1").arg(static_cast<int>(f_version[0]));
        int const max_size(f_version.size());
        for(int i(1); i < max_size; ++i)
        {
            f_version_string += QString(".%1").arg(static_cast<int>(f_version[i]));
        }
    }

    return f_version_string;
}


/** \brief Return the operator and version as a string.
 *
 * This function returns the operator followed by the version both
 * separated by a space. If the operator is OPERATOR_UNORDERED then
 * just the version string is returned, just as if you called the
 * get_version_string() function.
 *
 * \note
 * Dependencies are always expected to include an operator along their
 * version. When a version is specified without an operator, the
 * default OPERATOR_LATER_OR_EQUAL is used.
 *
 * \return The operator and version that this version object represent.
 */
QString version::get_opversion_string() const
{
    QString v(get_version_string());

    if(f_operator.get_operator() != operator_t::OPERATOR_UNORDERED)
    {
        v = f_operator.get_operator_string() + (" " + v);
    }

    return v;
}


/** \fn version::get_error() const
 * \brief Get errors
 *
 * The function retrieve the last error message that happened when you
 * called the set_version() function.
 *
 * The set_version() functions clear the error message out to represent
 * a "no error state."
 *
 * \return The error message or an empty string if none.
 */


/** \brief Compare two versions against each others.
 *
 * This function compares this version against \p rhs and returns one of
 * the following:
 *
 * \li COMPARE_INVALID -- if the current name is considered invalid
 * \li COMPARE_SMALLER -- if this is smaller than \p rhs
 * \li COMPARE_EQUAL -- if this is equal \p rhs
 * \li COMPARE_LARGER -- if this is larger than \p rhs
 *
 * \param[in] rhs  The right hand side name to compare with.
 *
 * \return One of the COMPARE_... values (-2, -1, 0, 1).
 */
compare_t version::compare(version const& rhs) const
{
    if(!is_valid() || !rhs.is_valid())
    {
        return compare_t::COMPARE_INVALID;
    }

    int const max_size(std::max SNAP_PREVENT_MACRO_SUBSTITUTION (f_version.size(), rhs.f_version.size()));
    for(int i(0); i < max_size; ++i)
    {
        int l(i >=     f_version.size() ? 0 : static_cast<int>(    f_version[i]));
        int r(i >= rhs.f_version.size() ? 0 : static_cast<int>(rhs.f_version[i]));
        if(l < r)
        {
            return compare_t::COMPARE_SMALLER;
        }
        if(l > r)
        {
            return compare_t::COMPARE_LARGER;
        }
    }

    return compare_t::COMPARE_EQUAL;
}





/** \brief Set the operator from a string.
 *
 * This function defines a version operator from a string as found in a
 * dependency string.
 *
 * The valid operators are:
 *
 * \li = or == -- OPERATOR_EQUAL, canonicalized as "="
 * \li != or <> -- OPERATOR_EXCEPT, canonicalized as "!="
 * \li < -- OPERATOR_EARLIER
 * \li > -- OPERATOR_LATER
 * \li <= -- OPERATOR_EARLIER_OR_EQUAL
 * \li >= -- OPERATOR_LATER_OR_EQUAL (TBD should this one be canonicalized
 *           as an empty string?)
 *
 * By default a version operator object is set to OPERATOR_UNORDERED which
 * pretty much means it was not set yet.
 *
 * Note that Debian supported \<\< and \>\> as an equivalent to \<= and \>=
 * respectively. We do not support those operators since (1) Debian
 * deprecated them, and (2) they are definitively confusing.
 *
 * You can also use the set_operator() function which access an OPERATOR_...
 * enumeration.
 *
 * Note that it is possible to create a range with a shortcut in a dependency
 * declaration:
 *
 * \code
 * <smaller version> < <larger version>
 * <smaller version> <= <larger version>
 *
 * // for example:
 * 1.3.4 < 1.4.0
 * 1.3.4 <= 1.4.0
 * \endcode
 *
 * Once compiled in, this is represented using two version operators and
 * the operator is changed from \< to > and \<, and from \<= to >= and
 * \<= respectively, so the previous example becomes:
 *
 * \code
 * // This range:
 * my_lib (1.3.4 < 1.4.0)
 *
 * // is equivalent to those two entries
 * my_lib (> 1.3.4)
 * my_lib (< 1.4)
 *
 * // And that range:
 * my_lib (1.3.4 <= 1.4.0)
 *
 * // is equivalent to those two entries
 * my_lib (>= 1.3.4)
 * my_lib (<= 1.4)
 * \endcode
 *
 * \param[in] operator_string  The string to be checked.
 *
 * \return true if the operator string represents a valid operator.
 */
bool version_operator::set_operator_string(QString const& operator_string)
{
    operator_t op;
    bool const r(validate_operator(operator_string, op, f_error));
    if(r)
    {
        // valid, save it
        set_operator(op);
    }
    return r;
}


/** \brief Set the operator using the enumeration.
 *
 * This function sets the operator using one of the enumeration values.
 *
 * \note
 * You may use this function to reset the version operator back to
 * OPERATOR_UNORDERED. In that case the operator string becomes
 * the empty string ("").
 *
 * \param[in] op  The new operator for this version operator object.
 *
 * \return true if op is a valid operator.
 */
bool version_operator::set_operator(operator_t op)
{
    switch(op)
    {
    case operator_t::OPERATOR_UNORDERED:
    case operator_t::OPERATOR_EQUAL:
    case operator_t::OPERATOR_EXCEPT:
    case operator_t::OPERATOR_EARLIER:
    case operator_t::OPERATOR_LATER:
    case operator_t::OPERATOR_EARLIER_OR_EQUAL:
    case operator_t::OPERATOR_LATER_OR_EQUAL:
        f_operator = op;
        return true;

    default:
        return false;

    }
}


/** \brief Retrieve the canonicalized operator string.
 *
 * This function returns the string representing the operator. The
 * string is canonicalized, which means that it has one single
 * representation (i.e. we accept "==" which is represented as "="
 * when canonicalized.)
 *
 * \return The canonicalized operator string.
 */
char const * version_operator::get_operator_string() const
{
    return g_operators + static_cast<int>(static_cast<operator_t>(f_operator)) * 3;
}


/** \fn version_operator::get_operator() const
 * \brief Retrieve the operator.
 *
 * This function retrieves the operator as a number. The operator is used
 * to compare versions between each others while searching for dependencies.
 *
 * \return The operator, may be OPERATOR_UNORDERED if not initialized.
 */





/** \brief Initialize a versioned filename object.
 *
 * The versioned filename class initializes the versioned filename object
 * with an extension which is mandatory and unique.
 *
 * \note
 * The period in the extension is optional. However, the extension cannot
 * be the empty string.
 *
 * \param[in] extension  The expected extension (i.e. ".js" for JavaScript files).
 */
versioned_filename::versioned_filename(QString const & extension)
    //: f_valid(false) -- auto-init
    //, f_error("") -- auto-init
    : f_extension(extension)
    //, f_name("") -- auto-init
    //, f_version_string("") -- auto-init
    //, f_version() -- auto-init
{
    if(f_extension.isEmpty())
    {
        throw snap_version_exception_invalid_extension("the extension of a versioned filename cannot be the empty string");
    }

    // make sure the extension includes the period
    if(f_extension.at(0).unicode() != '.')
    {
        f_extension = "." + f_extension;
    }
}


/** \brief Set the name of a file through the parser.
 *
 * This function is used to setup a versioned filename from a full filename.
 * The input filename can include a path. It must end with the valid
 * extension (as defined when creating the versioned_filename object.)
 * Assuming the function returns true, the get_filename() function
 * returns the basename (i.e. the filename without the path nor the
 * extension, although you can get the extension if you ask for it.)
 *
 * The filename is then broken up in a name, a version, and browser, all of
 * which are checked for validity. If invalid, the function returns false.
 *
 * \code
 * .../some/path/name_version_browser.ext
 * \endcode
 *
 * Note that the browser part is optional. In general, if not indicated
 * it means the file is compatible with all browsers.
 *
 * \note
 * This function respects the contract: if the function returns false,
 * then the name, version, and browser information are not changed.
 *
 * However, on entry the value of f_error is set to the empty string and
 * the value of f_valid is set to false. So most of the functions will
 * continue to return the old value of the versioned filename, except
 * the compare() and relational operators.
 *
 * \param[in] filename  The filename to be parsed for a version.
 *
 * \return true if the filename was a valid versioned filename.
 */
bool versioned_filename::set_filename(QString const & filename)
{
    f_error.clear();

    // the extension must be exactly "extension"
    if(!filename.endsWith(f_extension))
    {
        f_error = "this filename must end with \"" + f_extension + "\" in lowercase. \"" + filename + "\" is not valid.";
        return false;
    }

    int const max_length(filename.length() - f_extension.length());

    int start(filename.lastIndexOf('/'));
    if(start == -1)
    {
        start = 0;
    }
    else
    {
        ++start;
    }

    // now break the name in two or three parts: <name> and <version> [and <browser>]
    int p1(filename.indexOf('_', start));
    if(p1 == -1 || p1 > max_length)
    {
        f_error = "a versioned filename is expected to include an underscore (_) as the name and version separator. \"" + filename + "\" is not valid.";
        return false;
    }
    // and check whether the <browser> part is specified
    int p2(filename.indexOf('_', p1 + 1));
    if(p2 > max_length || p2 == -1)
    {
        p2 = max_length;
    }
    else
    {
        // filename ends with an underscore?
        if(p2 + 1 >= max_length)
        {
            f_error = QString("a browser name must be specified in a versioned filename if you include two underscores (_). \"%1\" is not valid.").arg(filename);
            return false;
        }
    }

    // name
    QString const name(filename.mid(start, p1 - start));
    QString name_string(name);
    // TBD: can we really allow a namespace in a filename?
    QString namespace_string;
    if(!validate_name(name_string, f_error, namespace_string))
    {
        return false;
    }

    // version
    ++p1;
    QString const version_string(filename.mid(p1, p2 - p1));
    version_numbers_vector_t version;
    if(!validate_version(version_string, version, f_error))
    {
        return false;
    }

    // browser
    QString browser;
    if(p2 < max_length)
    {
        // validate only if not empty (since it is optional, empty is okay)
        browser = filename.mid(p2 + 1, max_length - p2 - 1);
        if(!validate_basic_name(browser, f_error))
        {
            return false;
        }
    }

    // save the results
    f_name.set_name(name);
    f_version.set_version_string(version_string);
    if(browser.isEmpty())
    {
        // browser info is optional and if not defined we need to clear
        // the name (because a set_name("") generates an error)
        f_browser.clear();
    }
    else
    {
        f_browser.set_name(browser);
    }

    return true;
}


/** \brief Set the name of the versioned filename object.
 *
 * A versioned filename is composed of a name, a version, and an optional
 * browser reference. This function is used to replace the name.
 *
 * The name is checked using the \p validate_name() function.
 *
 * \param[in] name  The new file name.
 *
 * \return true if the name is valid.
 */
bool versioned_filename::set_name(QString const& name)
{
    QString namespace_string;
    QString name_string(name);
    bool const r(validate_name(name_string, f_error, namespace_string));
    if(r)
    {
        f_name.set_name(name);
    }

    return r;
}


/** \brief Set the version of the versioned filename.
 *
 * This function sets the version of the versioned filename. Usually, you
 * will call the set_filename() function which sets the name, the version,
 * and the optional browser all at once and especially let the parsing
 * work to the versioned_filename class.
 *
 * \param[in] version_string  The version in the form of a string.
 *
 * \return true if the version was considered valid.
 */
bool versioned_filename::set_version(QString const& version_string)
{
    version_numbers_vector_t version_vector;
    bool r(validate_version(version_string, version_vector, f_error));
    if(r)
    {
        f_version.set_version_string(version_string);
    }

    return r;
}


/** \brief Return the canonicalized filename.
 *
 * This function returns the canonicalized filename. This means all version
 * numbers have leading 0's removed, ending .0 are all removed, and the
 * path is removed.
 *
 * The \p extension flag can be used to get the extension appended or not.
 *
 * \param[in] extension  Set to true to get the extension.
 *
 * \return The canonicalized filename.
 */
QString versioned_filename::get_filename(bool extension) const
{
    if(!is_valid())
    {
        return "";
    }
    return f_name.get_name()
         + "_" + f_version.get_version_string()
         + (f_browser.get_name().isEmpty() ? "" : "_" + f_browser.get_name())
         + (extension ? f_extension : "");
}


/** \brief Compare two versioned_filename's against each others.
 *
 * This function first makes sure that both filenames are considered
 * valid, if not, the function returns COMPARE_INVALID (-2).
 *
 * Assuming the two filenames are valid, the function returns:
 *
 * \li COMPARE_SMALLER (-1) if this filename is considered to appear before rhs
 * \li COMPARE_EQUAL (0) if both filenames are considered equal
 * \li COMPARE_LARGER (1) if this filename is considered to appear after rhs
 *
 * The function first compares the name (get_name()) of each object.
 * If not equal, return COMPARE_SMALLER or COMPARE_LARGER.
 *
 * When the name are equal, the function compares the browser (get_browser())
 * of each object. If not equal, rethrn COMPARE_SMALLER or COMPARE_LARGER.
 *
 * When the name and the browser are equal, then the function compares the
 * versions starting with the major release number. If a version array is
 * longer than the other, the missing values in the smaller array are
 * considered to be zero. That way "1.2.3" > "1.2" because "1.2" is the
 * same as "1.2.0" and 3 > 0.
 *
 * \param[in] rhs  The right hand side to compare against this versioned
 *                 filename.
 *
 * \return -2, -1, 0, or 1 depending on the order (or unordered status)
 */
compare_t versioned_filename::compare(versioned_filename const& rhs) const
{
    if(!is_valid())
    {
        return compare_t::COMPARE_INVALID;
    }

    compare_t c(f_name.compare(rhs.f_name));
    if(c != compare_t::COMPARE_EQUAL)
    {
        return c;
    }
    c = f_browser.compare(rhs.f_browser);
    if(c != compare_t::COMPARE_EQUAL)
    {
        return c;
    }

    return f_version.compare(rhs.f_version);
}


/** \brief Compare two filenames for equality.
 *
 * This function returns true if both filenames are considered equal
 * (i.e. if the compare() function returns 0.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered equal.
 */
bool versioned_filename::operator == (versioned_filename const & rhs) const
{
    compare_t r(compare(rhs));
    return r == compare_t::COMPARE_EQUAL;
}


/** \brief Compare two filenames for differences.
 *
 * This function returns true if both filenames are not considered equal
 * (i.e. if the compare() function returns -1 or 1.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered different.
 */
bool versioned_filename::operator != (versioned_filename const& rhs) const
{
    compare_t r(compare(rhs));
    return r == compare_t::COMPARE_SMALLER || r == compare_t::COMPARE_LARGER;
}


/** \brief Compare two filenames for inequality.
 *
 * This function returns true if this filename is considered to appear before
 * \p rhs filename (i.e. if the compare() function returns -1.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered inequal.
 */
bool versioned_filename::operator <  (versioned_filename const& rhs) const
{
    compare_t r(compare(rhs));
    return r == compare_t::COMPARE_SMALLER;
}


/** \brief Compare two filenames for inequality.
 *
 * This function returns true if this filename is considered to appear before
 * \p rhs filename or both are equal (i.e. if the compare() function
 * returns -1 or 0.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered inequal.
 */
bool versioned_filename::operator <= (versioned_filename const& rhs) const
{
    compare_t r(compare(rhs));
    return r == compare_t::COMPARE_SMALLER || r == compare_t::COMPARE_EQUAL;
}


/** \brief Compare two filenames for inequality.
 *
 * This function returns true if this filename is considered to appear after
 * \p rhs filename (i.e. if the compare() function returns 1.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered inequal.
 */
bool versioned_filename::operator >  (versioned_filename const& rhs) const
{
    compare_t r(compare(rhs));
    return r > compare_t::COMPARE_EQUAL;
}


/** \brief Compare two filenames for inequality.
 *
 * This function returns true if this filename is considered to appear before
 * \p rhs filename or both are equal (i.e. if the compare() function
 * returns 0 or 1.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered inequal.
 */
bool versioned_filename::operator >= (versioned_filename const& rhs) const
{
    compare_t r(compare(rhs));
    return r >= compare_t::COMPARE_EQUAL;
}


/** \brief Define a dependency from a string.
 *
 * This function is generally called to transform a dependency string
 * in a name, a list of versions and operators, and a list of browsers.
 *
 * The format of the string is as follow in simplified yacc:
 *
 * \code
 * dependency: name
 *           | name versions
 *           | name versions browsers
 *           | name browsers
 *
 * name: NAME
 *
 * versions: '(' version_list ')'
 *
 * version_list: version_range
 *             | version_list ',' version_range
 *
 * version_range: version
 *              | version '<=' version
 *              | version '<' version
 *              | op version
 *
 * version: VERSION
 *
 * op: '='
 *   | '!='
 *   | '<'
 *   | '<='
 *   | '>'
 *   | '>='
 *
 * browsers: '[' browser_list ']'
 *
 * browser_list: name
 *             | browser_list ',' name
 * \endcode
 */
bool dependency::set_dependency(QString const& dependency_string)
{
    f_error.clear();

    // trim spaces
    QString const d(dependency_string.simplified());

    // get the end of the name
    int space_pos(d.indexOf(' '));
    if(space_pos < 0)
    {
        space_pos = d.length();
    }
    int paren_pos(d.indexOf('('));
    if(paren_pos < 0)
    {
        paren_pos = d.length();
    }
    int bracket_pos(d.indexOf('['));
    if(bracket_pos < 0)
    {
        bracket_pos = d.length();
    }
    if(paren_pos != d.length() && paren_pos > bracket_pos)
    {
        // cannot have versions after browsers
        f_error = "version dependency syntax error, '[' found before '('";
        return false;
    }
    int pos(std::min SNAP_PREVENT_MACRO_SUBSTITUTION (space_pos, std::min SNAP_PREVENT_MACRO_SUBSTITUTION (paren_pos, bracket_pos)));
    QString const dep_name(d.left(pos));
    QString namespace_string;
    QString name_string(dep_name);
    if(!validate_name(name_string, f_error, namespace_string))
    {
        return false;
    }
    f_name.set_name(dep_name);

    // skip the spaces (because of the simplied there should be 1 at most)
    while(pos < d.length() && isspace(d.at(pos).unicode()))
    {
        ++pos;
    }

    // read list of versions and operators
    if(pos < d.length() && d.at(pos) == QChar('('))
    {
        ++pos;
        int end(d.indexOf(')', pos));
        if(end == -1)
        {
            f_error = "version dependency syntax error, ')' not found";
            return false;
        }
        QString const version_list(d.mid(pos, end - pos));
        snap_string_list version_strings(version_list.split(',', QString::SkipEmptyParts));
        int const max_versions(version_strings.size());
        for(int i(0); i < max_versions; ++i)
        {
            QString const vonly(version_strings[i].trimmed());
            if(vonly.isEmpty())
            {
                // this happens because of the trimmed()
                continue;
            }
            version_operator vo;
            QByteArray utf8(vonly.toUtf8());
            char const *s(utf8.data());
            char const *start(s);
            for(; (*s >= '0' && *s <= '9') || *s == '.'; ++s);
            if(s == start)
            {
                // we assume an operator at the start
                for(; (*s < '0' || *s > '9') && *s != '\0'; ++s);
                QString const op(QString::fromUtf8(start, static_cast<int>(s - start)).trimmed());
                if(!vo.set_operator_string(op))
                {
                    f_error = vo.get_error();
                    return false;
                }
                // skip spaces after the operator
                for(; isspace(*s); ++s);
                start = s;
                for(; (*s >= '0' && *s <= '9') || *s == '.'; ++s);
            }

            // got a version, verify it
            QString const version_string(QString::fromUtf8(start, static_cast<int>(s - start)));
            version v;
            if(!v.set_version_string(version_string))
            {
                f_error = v.get_error();
                return false;
            }
            for(; isspace(*s); ++s);
            if(*s != '\0')
            {
                // not end of the version, check for an operator
                if(vo.get_operator() != operator_t::OPERATOR_UNORDERED)
                {
                    f_error = "a version specification in a dependency can only include one operator, two found in \"" + vonly + "\" (missing ',' or ')' maybe?)";
                    return false;
                }
                // we assume an operator in between two versions
                // (i.e. version <= version)
                start = s;
                for(; (*s < '0' || *s > '9') && *s != '\0'; ++s);
                QString const op(QString::fromUtf8(start, static_cast<int>(s - start)).trimmed());
                if(!vo.set_operator_string(op))
                {
                    f_error = vo.get_error();
                    return false;
                }
                operator_t const range_op(vo.get_operator());
                switch(range_op)
                {
                case operator_t::OPERATOR_UNORDERED:
                case operator_t::OPERATOR_EQUAL:
                case operator_t::OPERATOR_EXCEPT:
                    f_error = "unsupported operator \"" + QString(vo.get_operator_string()) + "\" for a range";
                    return false;

                default:
                    // otherwise we're good
                    break;

                }
                // skip spaces after the operator
                for(; isspace(*s); ++s);
                start = s;
                for(; (*s >= '0' && *s <= '9') || *s == '.'; ++s);
                if(*s != '\0')
                {
                    f_error = "a version range can have two versions separated by an operator, \"" + vonly + "\" is not valid";
                    return false;
                }
                QString const rhs_version(QString::fromUtf8(start, static_cast<int>(s - start)));
                version rhs_v;
                if(!rhs_v.set_version_string(rhs_version))
                {
                    f_error = rhs_v.get_error();
                    return false;
                }
                switch(range_op)
                {
                case operator_t::OPERATOR_EARLIER:
                    vo.set_operator(operator_t::OPERATOR_LATER);
                    v.set_operator(vo);
                    f_versions.push_back(v);
                    vo.set_operator(operator_t::OPERATOR_EARLIER);
                    rhs_v.set_operator(vo);
                    f_versions.push_back(rhs_v);
                    if(v.compare(rhs_v) >= compare_t::COMPARE_EQUAL)
                    {
                        f_error = "versions are not in the correct order in range \"" + vonly + "\" since " + v.get_version_string() + " >= " + rhs_v.get_version_string();
                        return false;
                    }
                    break;

                case operator_t::OPERATOR_EARLIER_OR_EQUAL:
                    vo.set_operator(operator_t::OPERATOR_LATER_OR_EQUAL);
                    v.set_operator(vo);
                    f_versions.push_back(v);
                    vo.set_operator(operator_t::OPERATOR_EARLIER_OR_EQUAL);
                    rhs_v.set_operator(vo);
                    f_versions.push_back(rhs_v);
                    if(v.compare(rhs_v) >= compare_t::COMPARE_EQUAL)
                    {
                        f_error = "versions are not in the correct order in range \"" + vonly + "\" since " + v.get_version_string() + " >= " + rhs_v.get_version_string();
                        return false;
                    }
                    break;

                case operator_t::OPERATOR_LATER:
                    vo.set_operator(operator_t::OPERATOR_LATER);
                    rhs_v.set_operator(vo);
                    f_versions.push_back(rhs_v);
                    vo.set_operator(operator_t::OPERATOR_EARLIER);
                    v.set_operator(vo);
                    f_versions.push_back(v);
                    if(v.compare(rhs_v) <= compare_t::COMPARE_EQUAL)
                    {
                        f_error = "versions are not in the correct order in range \"" + vonly + "\" since " + v.get_version_string() + " <= " + rhs_v.get_version_string();
                        return false;
                    }
                    break;

                case operator_t::OPERATOR_LATER_OR_EQUAL:
                    vo.set_operator(operator_t::OPERATOR_LATER_OR_EQUAL);
                    rhs_v.set_operator(vo);
                    f_versions.push_back(rhs_v);
                    vo.set_operator(operator_t::OPERATOR_EARLIER_OR_EQUAL);
                    v.set_operator(vo);
                    f_versions.push_back(v);
                    if(v.compare(rhs_v) <= compare_t::COMPARE_EQUAL)
                    {
                        f_error = "versions are not in the correct order in range \"" + vonly + "\" since " + v.get_version_string() + " <= " + rhs_v.get_version_string();
                        return false;
                    }
                    break;

                default:
                    throw snap_logic_exception("unexpected operators in this second switch(range_op) statement");

                }
            }
            else
            {
                if(vo.get_operator() == operator_t::OPERATOR_UNORDERED)
                {
                    // default operator is '>='
                    vo.set_operator(operator_t::OPERATOR_LATER_OR_EQUAL);
                }
                v.set_operator(vo);
                f_versions.push_back(v);
            }
        }

        // skip the version including the ')'
        pos = end + 1;

        // skip the spaces (because of the simplied there should be 1 at most)
        while(pos < d.length() && isspace(d.at(pos).unicode()))
        {
            ++pos;
        }
    }

    // read list of browsers
    if(pos < d.length() && d.at(pos) == '[')
    {
        ++pos;
        int end(d.indexOf(']'));
        if(end == -1)
        {
            // we could just emit some kind of a warning but then we may not
            // be able to support additional features later...
            f_error = "Invalid browser dependency list, the list of browsers must end with a ']'";
            return false;
        }
        QString const browsers(d.mid(pos, end - pos));
        snap_string_list const browser_list(browsers.split(',', QString::SkipEmptyParts));
        int const max_size(browser_list.size());
        for(int i(0); i < max_size; ++i)
        {
            QString const bn(browser_list[i].trimmed());
            if(bn.isEmpty())
            {
                // this happens because of the trimmed()
                continue;
            }
            name_string = bn;
            if(!validate_name(name_string, f_error, namespace_string))
            {
                return false;
            }
            name browser;
            browser.set_name(bn);
            f_browsers.push_back(browser);
        }

        pos = end + 1;

        // skip the spaces (because of the simplied there should be none here)
        while(pos < d.length() && isspace(d.at(pos).unicode()))
        {
            ++pos;
        }
    }

    if(pos != d.length())
    {
        f_error = QString("left over data at the end of the dependency string \"%1\"").arg(dependency_string);
        return false;
    }

    return true;
}


/** \brief Get the canonicalized dependency string.
 *
 * When you set the dependency string with set_dependency() the string
 * may miss some spaces or include additional spaces, some versions may
 * end with ".0" or some numbers start with 0 (i.e. "5.03") and
 * additional commas may be found in lists of versions and browsers.
 *
 * This function returned a fully cleaned up string with the dependency
 * information as intended by the specification.
 */
QString dependency::get_dependency_string() const
{
    QString dep;
    
    if(!f_name.get_namespace().isEmpty())
    {
        dep = QString("%1::").arg(f_name.get_namespace());
    }

    dep += f_name.get_name();

    int const max_versions(f_versions.count());
    if(max_versions > 0)
    {
        dep += " (" + f_versions[0].get_opversion_string();
        for(int i(1); i < max_versions; ++i)
        {
            dep += ", " + f_versions[i].get_opversion_string();
        }
        dep += ")";
    }

    int const max_browsers(f_browsers.count());
    if(max_browsers > 0)
    {
        dep += " [" + f_browsers[0].get_name();
        for(int i(1); i < max_browsers; ++i)
        {
            dep += ", " + f_browsers[i].get_name();
        }
        dep += "]";
    }

    return dep;
}


/** \brief Check the validity of a dependency declaration.
 *
 * This function retrieves the validity of the dependency.
 *
 * This includes the validity of the dependency object itself, the name,
 * all the versions, and all the browser names.
 *
 * \return true if all the information is considered valid.
 */
bool dependency::is_valid() const
{
    if(!f_error.isEmpty() || !f_name.is_valid())
    {
        return false;
    }

    // loop through all the versions
    int const max_versions(f_versions.size());
    for(int i(0); i < max_versions; ++i)
    {
        if(!f_versions[i].is_valid())
        {
            return false;
        }
    }

    // loop through all the browsers
    int const max_browsers(f_browsers.size());
    for(int i(0); i < max_browsers; ++i)
    {
        if(!f_browsers[i].is_valid())
        {
            return false;
        }
    }

    return true;
}


/** \brief Function to quickly find the Version and Browsers fields.
 *
 * This function initializes the Quick Find Version in Source object
 * with a pointer to the input data and the size of the buffer.
 *
 * The find_version() function is expected to be called afterward to
 * get the Version and Browsers fields. The validity of those fields
 * is also getting checked when found. The Browsers field is optional,
 * however the Version field is mandatory.
 *
 * The source is expected to be UTF-8.
 */
quick_find_version_in_source::quick_find_version_in_source()
    //: f_data(nullptr) -- auto-init
    //, f_end(nullptr) -- auto-init
    //, f_name() -- auto-init
    //, f_layout() -- auto-init
    //, f_version("") -- auto-init
    //, f_browsers("") -- auto-init
    //, f_error("") -- auto-init
    //, f_description("") -- auto-init
    //, f_depends() -- auto-init
{
}


/** \brief Search for the Version and other fields.
 *
 * This function reads the file. It must start with a C-like comment (a.
 * slash (/) and an asterisk (*)).
 *
 * The C-like comment can include any number of fields. On a line you want
 * to include a field name, a colon, followed by a value. For example, the
 * version field is defined as:
 *
 * Version: 1.2.3
 *
 * And the browsers field is defined as a list of browser names:
 *
 * Browsers: ie, firefox, opera
 *
 * The list of browsers is used to select code using a C-like preprocessor
 * in the .js and .css files. That allows us to not have to use tricks to
 * support different browsers with very similar but different enough code
 * for different browsers.
 *
 * \param[in] data  The pointer to the string to be parsed.
 * \param[in] size  The size (number of bytes) of the string to be parsed.
 *
 * \return true if the function succeeded, false otherwise and an error is
 *         set which can be retrieved with get_error()
 */
bool quick_find_version_in_source::find_version(char const *data, int const size)
{
    class field_t
    {
    public:
        field_t(char const *name)
            : f_name(name)
        {
        }

        bool check(QString const & line, QString & value)
        {
            // find the field name if available

            // skip spaces at the beginning of the line
            unsigned int const max_length(line.length());
            unsigned int pos;
            for(pos = 0; pos < max_length; ++pos)
            {
                if(!isspace(line.at(pos).unicode()))
                {
                    break;
                }
            }

            // C-like comments most often have " * " at the start of the line
            if(line.at(pos).unicode() == '*')
            {
                // skip spaces after the '*'
                for(++pos; pos < max_length; ++pos)
                {
                    if(!isspace(line.at(pos).unicode()))
                    {
                        break;
                    }
                }
            }

            // compare with the name of this field
            char const *n(f_name);
            for(; pos < max_length && *n != '\0'; ++pos, ++n)
            {
                int c(line.at(pos).unicode());
                if(c >= 'a' && c <= 'z')
                {
                    // uppercase
                    c &= 0x5F;
                }
                if(c != *n)
                {
                    // no equal...
                    return false;
                }
            }

            // make sure there is a colon after the name
            if(pos >= max_length
            || line.at(pos).unicode() != ':')
            {
                return false;
            }

            // got a field, save it in value
            value = line.mid(pos + 1).simplified();

            return true;
        }

    private:
        char const *    f_name;
    };

    f_data = data;
    f_end = data + size;

    QString l(get_line());
    if(!l.startsWith("/*"))
    {
        // C comment must appear first
        f_error = "file does not start with a C-like comment";
        return false;
    }

    // note: field names are case insensitive
    field_t field_name("NAME");
    field_t field_layout("LAYOUT");
    field_t field_version("VERSION");
    field_t field_browsers("BROWSERS");
    field_t field_description("DESCRIPTION");
    field_t field_depends("DEPENDS");
    for(;;)
    {
        QString value;
        if(field_name.check(l, value))
        {
            if(!f_name.get_name().isEmpty())
            {
                f_error = "name field cannot be defined more than once";
                return false;
            }
            if(!f_name.set_name(value))
            {
                f_error = f_name.get_error();
                return false;
            }
        }
        if(field_layout.check(l, value))
        {
            if(!f_layout.get_name().isEmpty())
            {
                f_error = "layout field cannot be defined more than once";
                return false;
            }
            if(!f_layout.set_name(value))
            {
                f_error = f_layout.get_error();
                return false;
            }
        }
        else if(field_version.check(l, value))
        {
            if(!f_version.get_version_string().isEmpty())
            {
                // more than one Version field
                f_error = "version field cannot be defined more than once";
                return false;
            }
            if(!f_version.set_version_string(value))
            {
                f_error = f_version.get_error();
                return false;
            }
        }
        else if(field_browsers.check(l, value))
        {
            if(!f_browsers.isEmpty())
            {
                // more than one Browsers field
                f_error = "browser field cannot be defined more than once";
                return false;
            }
            snap_string_list browser_list(value.split(','));
            int const max_size(browser_list.size());
            for(int i(0); i < max_size; ++i)
            {
                name browser;
                if(!browser.set_name(browser_list[i].trimmed()))
                {
                    f_error = browser.get_error();
                    return false;
                }
                f_browsers.push_back(browser);
            }
        }
        else if(field_description.check(l, value))
        {
            if(!f_description.isEmpty())
            {
                // more than one Description field
                f_error = "description field cannot be defined more than once";
                return false;
            }
            // description can be anything
            f_description = value;
        }
        else if(field_depends.check(l, value))
        {
            if(!f_depends.isEmpty())
            {
                // more than one Depends field
                f_error = "depends field cannot be defined more than once";
                return false;
            }
            // parse dependencies one by one
            if(!value.isEmpty())
            {
                int paren(0);
                int brack(0);
                QChar const *start(value.data());
                for(QChar const *s(start);; ++s)
                {
                    switch(s->unicode())
                    {
                    case '(':
                        ++paren;
                        break;

                    case ')':
                        --paren;
                        break;

                    case '[':
                        ++brack;
                        break;

                    case ']':
                        --brack;
                        break;

                    case ',':
                    case '\0':
                        if(paren == 0 && brack == 0)
                        {
                            // got one!
                            size_t const len(s - start);
                            if(len > 0) // ignore empty entries
                            {
                                dependency d;
                                QString dep(start, static_cast<int>(s - start));
                                d.set_dependency(dep);
                                f_depends.push_back(d);
                            }
                            start = s;
                            if(s->unicode() == ',')
                            {
                                // skip the comma
                                ++start;
                            }
                        }
                        else if(s->unicode() == '\0')
                        {
                            // parenthesis or brackets mismatched
                            f_error = "depends field () or [] mismatch";
                            return false;
                        }
                        break;

                    }
                    // by doing this test here we avoid having to duplicate
                    // the case when we save a dependency
                    if(s->unicode() == '\0')
                    {
                        break;
                    }
                }
            }
        }
        if(l.contains("*/"))
        {
            // stop with the end of the comment
            // return true only if the version was specified
            bool result(!f_version.get_version_string().isEmpty());
            if(result && f_browsers.isEmpty())
            {
                // always have some browsers, "all" if nothing else
                name browser;
                browser.set_name("all");
                f_browsers.push_back(browser);
            }
            return result;
        }
        l = get_line();
    }
}


/** \brief Get one character from the input file.
 *
 * This function reads one byte from the file and returns it.
 *
 * \return The next character or EOF at the end of the file.
 */
int quick_find_version_in_source::getc()
{
    if(f_data >= f_end)
    {
        return EOF;
    }
    // note: UTF-8 can be ignored because what we are interested
    //       in is using ASCII only
    return *f_data++;
}


/** \brief Get a line of text.
 *
 * This function reads the next line. Empty lines are skipped and not
 * returned unless the end of the file is reached. In that case, the
 * function returns anyway.
 *
 * \return The line just read or an empty string if the end of the line.
 */
QString quick_find_version_in_source::get_line()
{
    int c(0);
    for(;;)
    {
        std::string raw;
        for(;;)
        {
            c = getc();
            if(c == '\n' || c == '\r' || c == EOF)
            {
                break;
            }
            raw += static_cast<char>(c);
        }
        // we need to support UTF-8 properly for descriptions
        QString line(QString::fromUtf8(raw.c_str()).trimmed());
        if(!line.isEmpty() || c == EOF)
        {
            // do not return empty line unless we reached the
            // end of the file
            return line;
        }
    }
    NOTREACHED();
    return "";
}


/** \brief Check whether the object is valid.
 *
 * This function returns true if all the data in this object is valid.
 */
bool quick_find_version_in_source::is_valid() const
{
    // first check internal values
    if(!f_error.isEmpty()
    || !f_name.is_valid()
    || !f_version.is_valid())
    {
        return false;
    }

    // check each browser name
    int const max_size(f_browsers.size());
    for(int i(0); i < max_size; ++i)
    {
        if(!f_browsers[i].is_valid())
        {
            return false;
        }
    }

    return true;
}


} // namespace snap_version
} // namespace snap
// vim: ts=4 sw=4 et
