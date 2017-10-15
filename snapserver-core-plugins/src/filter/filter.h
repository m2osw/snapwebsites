// Snap Websites Server -- filter
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

// other plugins
//
#include "../content/content.h"

// snapwebsites lib
//
#include <snapwebsites/not_reached.h>


namespace snap
{
namespace filter
{

class filter_exception : public snap_exception
{
public:
    explicit filter_exception(char const *        what_msg) : snap_exception("filter", what_msg) {}
    explicit filter_exception(std::string const & what_msg) : snap_exception("filter", what_msg) {}
    explicit filter_exception(QString const &     what_msg) : snap_exception("filter", what_msg) {}
};

class filter_exception_invalid_arguement : public filter_exception
{
public:
    explicit filter_exception_invalid_arguement(char const *        what_msg) : filter_exception(what_msg) {}
    explicit filter_exception_invalid_arguement(std::string const & what_msg) : filter_exception(what_msg) {}
    explicit filter_exception_invalid_arguement(QString const &     what_msg) : filter_exception(what_msg) {}
};




class filter
        : public plugins::plugin
{
public:
    enum class token_t
    {
        TOK_UNDEFINED,
        TOK_IDENTIFIER,
        TOK_STRING,
        TOK_INTEGER,
        TOK_REAL,
        TOK_SEPARATOR,
        TOK_INVALID
    };

    // TODO: change that to a class with private vars...
    struct parameter_t
    {
        token_t                 f_type = token_t::TOK_UNDEFINED;
        QString                 f_name;
        QString                 f_value;

        parameter_t()
            : f_type(token_t::TOK_UNDEFINED)
            //, f_name("") -- auto-init
            //, f_value("") -- auto-init
        {
        }

        bool is_null() const
        {
            return f_type == token_t::TOK_UNDEFINED || f_type == token_t::TOK_INVALID;
        }

        void reset()
        {
            f_type = token_t::TOK_INVALID;
            f_name = "";
            f_value = "";
        }

        bool operator == (parameter_t const & rhs) const
        {
            return f_name == rhs.f_name;
        }
        bool operator < (parameter_t const & rhs) const
        {
            return f_name < rhs.f_name;
        }

        static char const * type_name(token_t type)
        {
            switch(type)
            {
            case token_t::TOK_UNDEFINED:
                return "undefined";

            case token_t::TOK_IDENTIFIER:
                return "identifier";

            case token_t::TOK_STRING:
                return "string";

            case token_t::TOK_INTEGER:
                return "integer";

            case token_t::TOK_REAL:
                return "real";

            case token_t::TOK_SEPARATOR:
                return "separator";

            case token_t::TOK_INVALID:
                return "invalid";

            }
            NOTREACHED();
        }
    };

    // TODO: change that in a class
    struct token_info_t
    {
        QString                     f_name;
        QVector<parameter_t>        f_parameters;
        bool                        f_found = false;
        bool                        f_error = false;
        bool                        f_name_used = false;
        QString                     f_replacement;
        QDomDocument &              f_xml;

        token_info_t(QDomDocument & xml)
            : f_xml(xml)
        {
        }

        bool is_namespace(char const *name)
        {
            // we expect 'name' to end with '::'
            return f_name.startsWith(name);
        }

        bool is_token(char const * name)
        {
            // in a way, once marked as found a token is viewed as used up
            // and thus it does not match anymore; same with errors
            bool const result(!f_found && !f_error && f_name == name);
            if(result)
            {
                f_found = true;
            }
            return result;
        }

        bool verify_args(int min, int max)
        {
            if(min < 0 || max < -1)
            {
                throw filter_exception_invalid_arguement(QString("detected a minimum (%1) or maximum (%2) smaller than zero in token_info_t::args()")
                                .arg(min).arg(max));
            }
            if(min > max && max != -1)
            {
                throw filter_exception_invalid_arguement(QString("detected a minimum (%1) larger than the maximum (%2) in token_info_t::args()")
                                .arg(min).arg(max));
            }
            const int size(f_parameters.size());
            const bool valid(size >= min && (max == -1 || size <= max));
            if(!valid)
            {
                f_found = true;
                QString msg(f_name + " expects ");
                if(min == max)
                {
                    if(min == 0)
                    {
                        msg += "no arguments";
                    }
                    else if(min == 1)
                    {
                        msg += "exactly 1 argument";
                    }
                    else
                    {
                        msg += QString("exactly %1 arguments").arg(min);
                    }
                }
                else
                {
                    if(min == 0)
                    {
                        if(max == 1)
                        {
                            msg += "at most 1 argument";
                        }
                        else
                        {
                            msg += QString("at most %1 arguments").arg(max);
                        }
                    }
                    else if(max == -1)
                    {
                        if(min == 1)
                        {
                            msg += "at least 1 argument";
                        }
                        else
                        {
                            msg += QString("at least %1 arguments").arg(min);
                        }
                    }
                    else
                    {
                        msg += QString("between %1 and %2 arguments").arg(min).arg(max);
                    }
                }
                error(msg);
            }
            return valid;
        }

        bool has_arg(QString const & name, int position = -1)
        {
            QVector<parameter_t>::const_iterator it(f_parameters.end());
            if(!name.isEmpty())
            {
                parameter_t p;
                p.f_name = name;
                it = std::find(f_parameters.begin(), f_parameters.end(), p);
                if(it == f_parameters.end() && position == -1)
                {
                    return false;
                }
            }
            if(it == f_parameters.end() && position != -1 && !f_name_used)
            {
                if(position >= 0 && position < f_parameters.size())
                {
                    it = f_parameters.begin() + position;
                    if(!it->f_name.isEmpty())
                    {
                        // if that parameter has a name, it cannot be
                        // valid (it should have matched the name
                        // in the previous if() block)
                        return false;
                    }
                }
            }
            return it != f_parameters.end();
        }

        parameter_t get_arg(QString const & name, int position = -1, token_t type = token_t::TOK_UNDEFINED)
        {
            parameter_t const null;
            QVector<parameter_t>::const_iterator it(f_parameters.end());
            if(!name.isEmpty())
            {
                parameter_t p;
                p.f_name = name;
                it = std::find(f_parameters.begin(), f_parameters.end(), p);
                if(it == f_parameters.end())
                {
                    if(position == -1)
                    {
                        error(QString("%1 is missing from the list of parameters, you may need to name your parameters.")
                                .arg(name));
                        return null;
                    }
                }
                else
                {
                    f_name_used = true;
                }
            }
            // we cannot switch between name and positioned
            // arguments; it fails in many ways...
            if(it == f_parameters.end() && position != -1 && !f_name_used)
            {
                if(position >= 0 && position < f_parameters.size())
                {
                    it = f_parameters.begin() + position;
                }
            }
            if(it == f_parameters.end())
            {
                error(QString("parameter \"%1\" (position: %2) was not found in the list.")
                        .arg(name).arg(position));
                return null;
            }
            if(type != token_t::TOK_UNDEFINED && it->f_type != type)
            {
                error(QString("parameter \"%1\" (position: %2) is a %3 not of the expected type: %4.")
                        .arg(name).arg(position).arg(parameter_t::type_name(it->f_type)).arg(parameter_t::type_name(type)));
                return null;
            }
            return *it;
        }

        void error(QString const & msg)
        {
            f_error = true;
            f_replacement = QString("<span class=\"filter-error\"><span class=\"filter-error-word\">error:</span> %1</span>")
                    .arg(filter::encode_text_for_html(msg));
        }

        void reset()
        {
            f_name = "";
            f_parameters.clear();
            f_found = false;
            f_replacement = "";
        }
    };

    class filter_text_t
    {
    public:
                                filter_text_t(content::path_info_t & ipath, QDomDocument & xml_document, QString const & text);

        void                    set_support_edit(bool support_edit);
        bool                    get_support_edit() const;

        content::path_info_t &  get_ipath();

        bool                    has_changed() const;

        QDomDocument &          get_xml_document() const;

        void                    set_text(QString const & text);
        QString const &         get_text() const;

    private:
        content::path_info_t &  f_ipath;
        QDomDocument &          f_xml_document;
        QString                 f_text; // i.e. input and result
        bool                    f_changed = false;
        bool                    f_support_edit = true;
    };

    class filter_teaser_info_t
    {
    public:
        void                        set_max_words(int words);
        int                         get_max_words() const;

        void                        set_max_tags(int tags);
        int                         get_max_tags() const;

        void                        set_end_marker(QString const & end_marker);
        QString const &             get_end_marker() const;

        void                        set_end_marker_uri(QString const & uri, QString const & title);
        QString const &             get_end_marker_uri() const;
        QString const &             get_end_marker_uri_title() const;

    private:
        int32_t                     f_words = 0;        // max. # of words
        int32_t                     f_tags = 0;         // max. # of tags
        QString                     f_end_marker;       // i.e. usually "..." or "[...]" or a "read more" link
        QString                     f_end_marker_uri;   // main page URI
        QString                     f_end_marker_uri_title;
    };

    class token_help_t
    {
    public:
                                    token_help_t();
        void                        add_token(QString const & token, QString const & help);
        QString                     result();

    private:
        QDomDocument                f_doc;
        QDomElement                 f_root_tag;
        QDomElement                 f_help_tag;
    };

                        filter();
                        ~filter();

    // plugins::plugin implementation
    static filter *     instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(::snap::snap_child * snap);

    // server signals
    void                on_xss_filter(QDomNode & node, QString const & accepted_tags, QString const & accepted_attributes);

    // this is currently a filter function, the signal is on_replace_token()
    void                on_token_filter(content::path_info_t & ipath, QDomDocument & xml);

    static bool         filter_uri(QString & uri);
    static QString      encode_text_for_html(QString const & text);
    static bool         body_to_teaser(QDomElement body, filter_teaser_info_t const & info);
    static bool         filter_filename(QString & filename, QString const & extension);

    SNAP_SIGNAL(replace_token, (content::path_info_t & ipath, QDomDocument & xml, token_info_t & token), (ipath, xml, token));
    SNAP_SIGNAL(token_help, (token_help_t & help), (help));
    SNAP_SIGNAL(filter_text, (filter_text_t & txt_filt), (txt_filt));

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};

} // namespace filter
} // namespace snap
// vim: ts=4 sw=4 et
