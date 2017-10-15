// Snap Websites Server -- path canonicalization
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

#include "snapwebsites/snap_parser.h"
#include "snapwebsites/snap_string_list.h"

#include <QtSerialization/QSerializationReader.h>
#include <QtSerialization/QSerializationFieldTag.h>
#include <QtSerialization/QSerializationWriter.h>

#include <QMap>

namespace snap
{

class snap_uri_exception : public snap_exception
{
public:
    explicit snap_uri_exception(char const *        what_msg) : snap_exception("snap_uri", what_msg) {}
    explicit snap_uri_exception(std::string const & what_msg) : snap_exception("snap_uri", what_msg) {}
    explicit snap_uri_exception(QString const &     what_msg) : snap_exception("snap_uri", what_msg) {}
};

class snap_uri_exception_invalid_uri : public snap_uri_exception
{
public:
    explicit snap_uri_exception_invalid_uri(char const *        what_msg) : snap_uri_exception(what_msg) {}
    explicit snap_uri_exception_invalid_uri(std::string const & what_msg) : snap_uri_exception(what_msg) {}
    explicit snap_uri_exception_invalid_uri(QString const &     what_msg) : snap_uri_exception(what_msg) {}
};

class snap_uri_exception_invalid_parameter : public snap_uri_exception
{
public:
    explicit snap_uri_exception_invalid_parameter(char const *        what_msg) : snap_uri_exception(what_msg) {}
    explicit snap_uri_exception_invalid_parameter(std::string const & what_msg) : snap_uri_exception(what_msg) {}
    explicit snap_uri_exception_invalid_parameter(QString const &     what_msg) : snap_uri_exception(what_msg) {}
};

class snap_uri_exception_invalid_path : public snap_uri_exception
{
public:
    explicit snap_uri_exception_invalid_path(char const *        what_msg) : snap_uri_exception(what_msg) {}
    explicit snap_uri_exception_invalid_path(std::string const & what_msg) : snap_uri_exception(what_msg) {}
    explicit snap_uri_exception_invalid_path(QString const &     what_msg) : snap_uri_exception(what_msg) {}
};

class snap_uri_exception_out_of_bounds : public snap_uri_exception
{
public:
    explicit snap_uri_exception_out_of_bounds(char const *        what_msg) : snap_uri_exception(what_msg) {}
    explicit snap_uri_exception_out_of_bounds(std::string const & what_msg) : snap_uri_exception(what_msg) {}
    explicit snap_uri_exception_out_of_bounds(QString const &     what_msg) : snap_uri_exception(what_msg) {}
};

class snap_uri_exception_exclusive_parameters : public snap_uri_exception
{
public:
    explicit snap_uri_exception_exclusive_parameters(char const *        what_msg) : snap_uri_exception(what_msg) {}
    explicit snap_uri_exception_exclusive_parameters(std::string const & what_msg) : snap_uri_exception(what_msg) {}
    explicit snap_uri_exception_exclusive_parameters(QString const &     what_msg) : snap_uri_exception(what_msg) {}
};







// Helper class to handle URIs
// http://tools.ietf.org/html/rfc3986
class snap_uri
{
public:
    // types used by this class
    typedef QMap<QString, QString>    snap_uri_options_t;

    // constructors
                        snap_uri();
                        snap_uri(QString const & uri);

    // URI handling
    bool                set_uri(QString const & uri);
    QString const &     get_original_uri() const;
    QString             get_uri(bool use_hash_bang = false) const;
    QString             get_website_uri(bool include_port = false) const;

    // get a part by name
    QString             get_part(QString const & name, int part = -1) const;

    // protocol handling
    void                set_protocol(QString const & uri_protocol);
    QString const &     protocol() const;

    // domain & sub-domains handling
    void                set_domain(QString const & full_domain_name);
    QString             full_domain() const;
    QString const &     top_level_domain() const;
    QString const &     domain() const;
    QString             sub_domains() const;
    int                 sub_domain_count() const;
    QString             sub_domain(int part) const;
    snap_string_list const & sub_domains_list() const;

    // port handling
    void                set_port(QString const & port);
    void                set_port(int port);
    int                 get_port() const;

    // path handling
    void                set_path(QString uri_path);
    QString             path(bool encoded = true) const;
    int                 path_count() const;
    QString             path_folder_name(int part) const;
    snap_string_list const & path_list() const;

    // option handling
    void                set_option(QString const & name, QString const & value);
    void                unset_option(QString const & name);
    QString             option(QString const & name) const;
    int                 option_count() const;
    QString             option(int part, QString & name) const;
    snap_uri_options_t const & options_list() const;

    // query string handling
    void                set_query_option(QString const & name, QString const & value);
    void                unset_query_option(QString const & name);
    void                set_query_string(QString const & uri_query_string);
    QString             query_string() const;
    bool                has_query_option(QString const & name) const;
    void                clear_query_options();
    QString             query_option(QString const & name) const;
    int                 query_option_count() const;
    QString             query_option(int part, QString & name) const;
    snap_uri_options_t const & query_string_list() const;

    // anchor handling (note: "#!" is not considered an anchor)
    void                set_anchor(QString const & uri_anchor);
    QString const &     anchor() const;

    // operators
    bool                operator == (snap_uri const & rhs) const;
    bool                operator != (snap_uri const & rhs) const;
    bool                operator <  (snap_uri const & rhs) const;
    bool                operator <= (snap_uri const & rhs) const;
    bool                operator >  (snap_uri const & rhs) const;
    bool                operator >= (snap_uri const & rhs) const;

    static QString      urlencode(QString const & uri, char const * accepted = "");
    static QString      urldecode(QString const & uri, bool relax = false);
    static int          protocol_to_port(QString const & uri_protocol);

private:
    bool                process_domain(QString const & full_domain_name, snap_string_list & sub_domain_names, QString & domain_name, QString & tld);

    // f_original is the unchanged source (from constructor or
    // last set_uri() call)
    QString                         f_original;
    QString                         f_protocol = "http";
    QString                         f_username;
    QString                         f_password;
    int                             f_port = 80;
    QString                         f_domain;
    QString                         f_top_level_domain;
    snap_string_list                f_sub_domains;
    snap_string_list                f_path;
    snap_uri_options_t              f_options;
    snap_uri_options_t              f_query_strings;
    QString                         f_anchor;
};




// The following is used to compile rules as defined for the domains
// and websites; the result is a set of regular expressions we can
// use to parse the URI when we receive a hit
//
// See http://snapwebsites.org/implementation/basic-concept-url-website/url-test
class snap_uri_rules
{
public:
    // handling of rule scripts
    bool            parse_domain_rules(QString const & script, QByteArray & result);
    bool            parse_website_rules(QString const & script, QByteArray & result);

    // processing of URIs
    //QString         process_uri(snap_uri & uri);

    QString const & errmsg() const { return f_errmsg; }

private:
    QString        f_errmsg;
};


/** \brief Domain variables
 *
 * This class is used to hold the domain variable information.
 * Each variable has a type that defines how the variable is used
 * (standard, website, or flag, and when defined as a flag whether
 * a default was defined.)
 */
class domain_variable : public parser::parser_user_data
{
public:
    /** \brief Define the type of this variable.
     *
     * This enumaration is used to define the exact type of the variable.
     * This is used to know how to use the variable later. That information
     * is actually saved in the resulting compiled data.
     *
     * \warning
     * The variable type is saved as is in the database (i.e. as a number)
     * which means that the order defined below CANNOT CHANGE. If we need
     * more types, add them afterward. If a type becomes obsolete, do not
     * delete it nor reuse its position.
     */
    typedef int domain_variable_type_t;
        // WARNING: saved as a number in the database

    /** \brief Standard variable type.
     *
     * The standard variable type is used as is to define a sub-domain
     * name. It is given a name and a regular expression. It may be
     * optional, but it does not otherwise required anything specific.
     *
     * The result is whatever the regular expression matches.
     */
    static domain_variable_type_t const DOMAIN_VARIABLE_TYPE_STANDARD = 0;
        // WARNING: saved as a number in the database DO NOT CHANGE

    /** \brief Website variable type.
     *
     * This type defines a value which is a regular expression, exactly
     * the same as the standard variable. However, the result is always
     * set to the default value (canonicalization.)
     */
    static domain_variable_type_t const DOMAIN_VARIABLE_TYPE_WEBSITE = 1;
        // WARNING: saved as a number in the database DO NOT CHANGE

    /** \brief Flag variable with a default value.
     *
     * This value is used whenever a sub-domain entry is an option which
     * means it does not participate to the website canonicalization. Any
     * matches are removed from the result.
     *
     * This flag defines a default. If there is no match, then use the
     * default instead.
     */
    static domain_variable_type_t const DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT = 2;
        // WARNING: saved as a number in the database DO NOT CHANGE

    /** \brief Flag variable without a default value.
     *
     * This flag is similar to the DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT
     * except that no default was defined. This means the flag is either
     * required, or completely ignored (when marked as optional.)
     *
     * In this case, the default value may be defined somewhere else. For
     * example, the language information is expected to be sent by the
     * client browser and can be accessed from there instead.
     */
    static domain_variable_type_t const DOMAIN_VARIABLE_TYPE_FLAG_NO_DEFAULT = 3;
        // WARNING: saved as a number in the database DO NOT CHANGE

    /** \brief Initialize a domain variable.
     *
     * This function initializes the domain variable with its type, name
     * and value. It is not possible to change the type or name of the
     * variable at a later time. The value can be changed with the
     * set_value() function.
     *
     * The default value is marked as undefined. It can be modified with
     * the set_default() function.
     *
     * Values and default values are always strings. Values are expected
     * to be regular expressions.
     *
     * The required flag is set to false (optional.) The set_required()
     * function can be used later to change that flag.
     *
     * \param[in] type  The type of the new domain variable.
     * \param[in] name  The name of the variable. The name can be qualified.
     * \param[in] value  The value of the variable.
     *
     * \sa set_value()
     * \sa set_default()
     * \sa set_required()
     */
    domain_variable(domain_variable_type_t type, QString const & name, QString const & value)
        : f_type(type)
        , f_name(name)
        , f_value(value)
        //, f_default("") -- auto-init
        //, f_required(false) -- auto-init
    {
        switch(type)
        {
        case DOMAIN_VARIABLE_TYPE_STANDARD:
        case DOMAIN_VARIABLE_TYPE_WEBSITE:
        case DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT:
        case DOMAIN_VARIABLE_TYPE_FLAG_NO_DEFAULT:
            break;

        default:
            throw std::runtime_error("unknown type specified in domain_variable constructor");

        }
    }

    /** \brief Retrieve the variable type.
     *
     * This function returns the type of the variable.
     *
     * \return The type of this variable.
     */
    domain_variable_type_t get_type() const
    {
        return f_type;
    }

    /** \brief Retrieve the variable name.
     *
     * This function returns the variable name. The name is read-only.
     *
     * The only way to set the name of a variable is to define it in
     * the constructor. It cannot be changed later. This function
     * returns the fully qualified name of the variable.
     *
     * \return The variable name in a QString.
     */
    QString const & get_name() const
    {
        return f_name;
    }

    /** \brief Get the value of the variable.
     *
     * The value of the domain variables is expected to be a valid
     * regular expression. The return value is read-only. To change
     * the value, use the set_value() function instead.
     *
     * \return The value of the variable in a QString.
     *
     * \sa set_value()
     */
    QString const & get_value() const
    {
        return f_value;
    }

    /** \brief Change the value of the variable.
     *
     * This function can be used to change the value of this variable.
     *
     * \param[in] value  The new value to set this variable to.
     */
    void set_value(QString const & value)
    {
        f_value = value;
    }

    /** \brief Get the default value of the variable.
     *
     * This function can be used to read the default value of the
     * variable. Note that if the variable is not of a type that
     * supports a default, the return value is always an empty
     * string.
     *
     * The value returned by this function is read-only. To modify
     * the default value, use the set_default() function instead.
     *
     * \return The default value of this variable.
     */
    QString const & get_default() const
    {
        return f_default;
    }

    /** \brief Set the default value of the variable.
     *
     * This function is used to set the default value of this variable.
     * By default the default value is an empty string. At this time
     * there is no function one can use to know whether the default
     * value was defined, however, the flag uses a different type
     * when a default or no defaults are defined.
     *
     * When the type is DOMAIN_VARIABLE_TYPE_FLAG_NO_DEFAULT then the
     * default is expected to never be defined.
     *
     * \exception std::runtime_error
     * The runtime error is raised if the default is set when the type
     * of the variable doesn't allow it. This is an internal error since
     * the system should never have to raise this error.
     *
     * \param[in] default_value  The new default value.
     */
    void set_default(QString const & default_value)
    {
        switch(f_type)
        {
        case DOMAIN_VARIABLE_TYPE_WEBSITE:
        case DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT:
            f_default = default_value;
            break;

        default:
            throw std::runtime_error("cannot define a default value for this type of domain variable");

        }
    }

    /** \brief Get whether the required flag is true or false.
     *
     * This function is the representation of the REQUIRED (true) or
     * OPTIONAL (false) keywords used in front of variable names.
     *
     * \return true when the sub-domain is required.
     */
    bool get_required() const
    {
        return f_required;
    }

    /** \brief Set whether this variable was marked as required.
     *
     * The sub-domain rule defines whether this entry is required or not.
     * This function is used to reflect this information in this variable.
     *
     * \param[in] required  Set whether the variable was marked as required (true) or not (false).
     */
    void set_required(bool required = true)
    {
        f_required = required;
    }

    void read(QtSerialization::QReader & stream);
    void write(QtSerialization::QWriter & stream) const;

private:
    /** \brief The domain variable type.
     *
     * This field defines the type of the domain variable. It cannot be
     * modified once a variable was created.
     *
     * \sa get_type()
     */
    domain_variable_type_t f_type;

    /** \brief The domain variable name.
     *
     * This field represents the fully qualified name of the variable.
     * The name cannot be changed once the variable was created.
     *
     * Names are expected to be unique within one rule.
     *
     * \sa get_name()
     */
    QString f_name;

    /** \brief The value of this variable.
     *
     * This is the value of the variable. It is expected to be a valid
     * regular expression. The value is generally set at the time the
     * variable is created. It can be changed later with the set_value()
     * function.
     *
     * \sa get_value()
     * \sa set_value()
     */
    QString f_value;

    /** \brief The default value of the variable.
     *
     * This field holds the default value of the variable. This is used
     * for websites and flags with a default value. Other types do not
     * make use of a default value.
     *
     * The default value can be modified with the set_default()
     * function.
     *
     * \sa get_default()
     * \sa set_default()
     */
    QString f_default;

    /** \brief Wether the value is required.
     *
     * This value represents one or more sub-domains. If those sub-domains
     * are required, then this value will be true.
     *
     * This value is set using the REQUIRED or OPTIONAL keywords in the
     * sub_domain rule.
     *
     * \sa get_required()
     * \sa set_required()
     */
    bool f_required = false;
};


/** \brief Define the information of a domain.
 *
 * This class holds a set of sub-domain variables which together
 * define a domain. The domain is also given a domain.
 */
class domain_info : public parser::parser_user_data, public QtSerialization::QSerializationObject
{
public:
    /** \brief Add a domain variable to the domain info.
     *
     * This function adds a domain variable as defined by
     * the sub-domain rules to a domain information object.
     * The variable is expected to be valid, although at
     * this point the system doesn't check whether it is
     * unique or properly qualified. Those checks are done
     * afterward.
     *
     * \param[in] var  The variable to add to this domain.
     */
    void add_var(QSharedPointer<domain_variable> & var)
    {
        f_vars.push_back(var);
    }

    /** \brief Retrieve the name of this domain.
     *
     * This function returns a read-only reference to the name of
     * this domain definition.
     *
     * To change the name, use the set_name() function.
     *
     * \return A reference to the name of this domain.
     *
     * \sa set_name()
     */
    QString const & get_name() const
    {
        return f_name;
    }

    /** \brief Set the name of the domain.
     *
     * This function is used to set the name of the domain.
     * The name is expected to be unique among all the domain
     * definitions found in a block (i.e. per domain.)
     *
     * \param[in] name  The name of the domain.
     *
     * \sa get_name()
     */
    void set_name(QString const & name)
    {
        f_name = name;
    }

    /** \brief Retrieve the number of variables defined.
     *
     * This function returns the number of variables that were added
     * to this domain definition. The number may be zero if no variables
     * were added.
     *
     * \return The number of domain variables available in this object.
     */
    int size() const
    {
        return f_vars.size();
    }

    /** \brief Retrieve one of the domain variables.
     *
     * This function returns a domain variable shared pointer.
     *
     * The index is expected to be between 0 (inclusive) and size()
     * (exclusive.) If size() is zero, this function cannot be
     * used.
     *
     * \param[in] idx  The index of the variable.
     *
     * \return A shared pointer to the domain variable information.
     */
    QSharedPointer<domain_variable> get_variable(int idx) const
    {
        return f_vars[idx];
    }

    /** \brief Retrieve one of the domain variables.
     *
     * This operator returns a domain variable shared pointer.
     *
     * The index is expected to be between 0 (inclusive) and size()
     * (exclusive.) If size() is zero, this operator cannot be
     * used.
     *
     * \param[in] idx  The index of the variable.
     *
     * \return A shared pointer to the domain variable information.
     */
    QSharedPointer<domain_variable> operator [] (int idx) const
    {
        return f_vars[idx];
    }

    void read(QtSerialization::QReader & r);
    virtual void readTag(QString const & name, QtSerialization::QReader & r);
    void write(QtSerialization::QWriter & w) const;

private:
    /** \brief The name of this domain definition.
     *
     * This field holds the name of the domain definition. The
     * name should be read-only, although it gets defined after
     * we create instances of domain_info...
     */
    QString f_name;

    /** \brief The list of variables attached to this domain.
     *
     * This array lists all the variables accepted by this domain.
     * Note that the variables are saved in the order in which
     * they appear in the source string. It is very important since
     * they need to be used to parse input URLs in the exact order
     * they are specified here.
     */
    QVector<QSharedPointer<domain_variable> > f_vars;
};


/** \brief Set of rules defined in a domain declaration.
 *
 * This class holds an array of domain information.
 */
class domain_rules : public parser::parser_user_data, public QtSerialization::QSerializationObject
{
public:
    /** \brief Add one domain information object.
     *
     * This function adds one domain information object to the
     * domain rules object. This is what the result is expected
     * to be.
     *
     * Note that the order is kept as is since the checks of the
     * domains is always done in order as specified by the user.
     * If a URL matches more than one domain, the first that matches
     * is used. The following will be ignored. In debug mode, URLs
     * may checked to all the domain information definitions in order
     * to determine whether there is a problem (i.e. a match that
     * should use Domain B instead of Domain A.) This can be done
     * on a backend system.
     *
     * \param[in] info  The domain information to add to this object.
     */
    void add_info(QSharedPointer<domain_info> & info)
    {
        f_info.push_back(info);
    }

    /** \brief Retrieve the number of domain information object.
     *
     * This function returns the number of domain_info objects
     * that were added to this domain rules object.
     *
     * This function will return zero until one or more domain info
     * was added to the domain rules.
     *
     * \return The number of domain_info objects defined in this domain rules.
     *
     * \sa operator [] ()
     * \sa add_info()
     */
    int size() const
    {
        return f_info.size();
    }

    /** \brief This operator can be used to retrieve a domain info.
     *
     * This operator returns the specified domain information from this
     * domain rules.
     *
     * The index must be between 0 (inclusive) and size() (exclusive).
     *
     * \param[in] idx  The index of the domain info to retrieve.
     *
     * \return A pointer to the specified domain_info object.
     *
     * \sa size()
     */
    QSharedPointer<domain_info> operator [] (int idx) const
    {
        return f_info[idx];
    }

    void read(QtSerialization::QReader & stream);
    virtual void readTag(QString const & name, QtSerialization::QReader & r);
    void write(QtSerialization::QWriter & stream) const;

private:
    /** \brief The array holding all the domain_info objects.
     *
     * This vector holds the domain information that are stored
     * by this domain rules object. The order in which the
     * domain_info where added to the domain_rules object
     * is preserved.
     */
    QVector<QSharedPointer<domain_info> > f_info;
};



// Websites data
class website_variable : public parser::parser_user_data
{
public:
        // WARNING: saved as a number in the database -- DO NOT CHANGE #'s
    typedef int website_variable_type_t;
    static website_variable_type_t const WEBSITE_VARIABLE_TYPE_STANDARD = 0;
    static website_variable_type_t const WEBSITE_VARIABLE_TYPE_WEBSITE = 1;
    static website_variable_type_t const WEBSITE_VARIABLE_TYPE_FLAG_WITH_DEFAULT = 2;
    static website_variable_type_t const WEBSITE_VARIABLE_TYPE_FLAG_NO_DEFAULT = 3;

        // WARNING: saved as a number in the database -- DO NOT CHANGE #'s
    typedef int website_variable_part_t;
    static website_variable_part_t const WEBSITE_VARIABLE_PART_PATH = 0;
    static website_variable_part_t const WEBSITE_VARIABLE_PART_PORT = 1;
    static website_variable_part_t const WEBSITE_VARIABLE_PART_PROTOCOL = 2;
    static website_variable_part_t const WEBSITE_VARIABLE_PART_QUERY = 3;

    website_variable(website_variable_type_t type, QString const & name, QString const & value)
        : f_type(type)
        , f_part(WEBSITE_VARIABLE_PART_PATH) // this is the obvious default here, we could define an UNDEFINED = -1 too?
        , f_name(name)
        , f_value(value)
        //, f_default("") -- auto-init
        //, f_required(false) -- auto-init
    {
        switch(type)
        {
        case WEBSITE_VARIABLE_TYPE_STANDARD:
        case WEBSITE_VARIABLE_TYPE_WEBSITE:
        case WEBSITE_VARIABLE_TYPE_FLAG_WITH_DEFAULT:
        case WEBSITE_VARIABLE_TYPE_FLAG_NO_DEFAULT:
            break;

        default:
            throw std::runtime_error("unknown type specified in website_variable constructor");

        }
    }

    website_variable_type_t get_type() const
    {
        return f_type;
    }

    QString const & get_name() const
    {
        return f_name;
    }

    QString const & get_value() const
    {
        return f_value;
    }

    void set_value(QString const & value)
    {
        f_value = value;
    }

    QString const & get_default() const
    {
        return f_default;
    }

    void set_default(QString const & default_value)
    {
        switch(f_type)
        {
        case WEBSITE_VARIABLE_TYPE_WEBSITE:
        case WEBSITE_VARIABLE_TYPE_FLAG_WITH_DEFAULT:
            f_default = default_value;
            break;

        default:
            throw std::runtime_error("cannot define a default value for this type of website variable");

        }
    }

    bool get_required() const
    {
        return f_required;
    }

    void set_required(bool required = true)
    {
        f_required = required;
    }

    website_variable_part_t get_part() const
    {
        return f_part;
    }

    void set_part(website_variable_part_t part)
    {
        switch(part)
        {
        case WEBSITE_VARIABLE_PART_PATH:
        case WEBSITE_VARIABLE_PART_PORT:
        case WEBSITE_VARIABLE_PART_PROTOCOL:
        case WEBSITE_VARIABLE_PART_QUERY:
            break;

        default:
            throw std::runtime_error("unknown part specified in website_variable::set_part()");

        }
        f_part = part;
    }

    void read(QtSerialization::QReader & r);
    void write(QtSerialization::QWriter & w) const;

private:
    website_variable_type_t     f_type;
    website_variable_part_t     f_part;
    QString                     f_name;
    QString                     f_value;
    QString                     f_default;
    bool                        f_required = false;
};


class website_info : public parser::parser_user_data, public QtSerialization::QSerializationObject
{
public:
    void add_var(QSharedPointer<website_variable >& var)
    {
        f_vars.push_back(var);
    }

    QString const & get_name() const
    {
        return f_name;
    }

    void set_name(QString const & name)
    {
        f_name = name;
    }

    int size() const
    {
        return f_vars.size();
    }

    QSharedPointer<website_variable> get_variable(int idx) const
    {
        return f_vars[idx];
    }

    QSharedPointer<website_variable> operator [] (int idx) const
    {
        return f_vars[idx];
    }

    void read(QtSerialization::QReader & r);
    virtual void readTag(QString const & name, QtSerialization::QReader & r);
    void write(QtSerialization::QWriter & w) const;

private:
    QString                                     f_name;
    QVector<QSharedPointer<website_variable> >  f_vars;
};


class website_rules : public parser::parser_user_data, public QtSerialization::QSerializationObject
{
public:
    void            add_info(QSharedPointer<website_info> & info)
                    {
                        f_info.push_back(info);
                    }

    int             size() const
                    {
                        return f_info.size();
                    }

    QSharedPointer<website_info> operator [] (int idx) const
                    {
                        return f_info[idx];
                    }

    void            read(QtSerialization::QReader & r);
    virtual void    readTag(QString const & name, QtSerialization::QReader & r);
    void            write(QtSerialization::QWriter & w) const;

private:
    QVector<QSharedPointer<website_info> > f_info;
};


} // namespace snap
// vim: ts=4 sw=4 et
