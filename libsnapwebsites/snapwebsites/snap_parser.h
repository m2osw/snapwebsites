// Snap Websites Server -- advanced parser
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
#pragma once

// libexcept lib
//
#include    <libexcept/exception.h>


// Qt lib
//
#include    <QVariant>
#include    <QVector>
#include    <QSharedPointer>



namespace snap
{
namespace parser
{

DECLARE_MAIN_EXCEPTION(snap_parser_exception);

DECLARE_EXCEPTION(snap_parser_exception, snap_parser_no_current_choices);
DECLARE_EXCEPTION(snap_parser_exception, snap_parser_state_has_children);
DECLARE_EXCEPTION(snap_parser_exception, snap_parser_unexpected_token);



enum class token_t
{
    TOKEN_ID_NONE_ENUM = 0,     // "not a token" (also end of input)

    TOKEN_ID_INTEGER_ENUM,
    TOKEN_ID_FLOAT_ENUM,
    TOKEN_ID_IDENTIFIER_ENUM,
    TOKEN_ID_KEYWORD_ENUM,
    TOKEN_ID_STRING_ENUM,
    TOKEN_ID_LITERAL_ENUM,      // literal character(s)

    TOKEN_ID_EMPTY_ENUM,        // special empty token
    TOKEN_ID_CHOICES_ENUM,      // pointer to a choices object
    TOKEN_ID_RULES_ENUM,        // pointer to a choices object (see rules operator |() )
    TOKEN_ID_NODE_ENUM,         // pointer to a node object
    TOKEN_ID_ERROR_ENUM         // an error occured
};

struct token_id { token_id(token_t t) : f_type(t) {} operator token_t () const { return f_type; } private: token_t f_type; };
struct token_id_none_def       : public token_id { token_id_none_def()       : token_id(token_t::TOKEN_ID_NONE_ENUM      ) {} };
struct token_id_integer_def    : public token_id { token_id_integer_def()    : token_id(token_t::TOKEN_ID_INTEGER_ENUM   ) {} };
struct token_id_float_def      : public token_id { token_id_float_def()      : token_id(token_t::TOKEN_ID_FLOAT_ENUM     ) {} };
struct token_id_identifier_def : public token_id { token_id_identifier_def() : token_id(token_t::TOKEN_ID_IDENTIFIER_ENUM) {} };
struct token_id_keyword_def    : public token_id { token_id_keyword_def()    : token_id(token_t::TOKEN_ID_KEYWORD_ENUM   ) {} };
struct token_id_string_def     : public token_id { token_id_string_def()     : token_id(token_t::TOKEN_ID_STRING_ENUM    ) {} };
struct token_id_literal_def    : public token_id { token_id_literal_def()    : token_id(token_t::TOKEN_ID_LITERAL_ENUM   ) {} };
struct token_id_empty_def      : public token_id { token_id_empty_def()      : token_id(token_t::TOKEN_ID_EMPTY_ENUM     ) {} };

extern token_id_none_def        TOKEN_ID_NONE;
extern token_id_integer_def     TOKEN_ID_INTEGER;
extern token_id_float_def       TOKEN_ID_FLOAT;
extern token_id_identifier_def  TOKEN_ID_IDENTIFIER;
extern token_id_keyword_def     TOKEN_ID_KEYWORD;
extern token_id_string_def      TOKEN_ID_STRING;
extern token_id_literal_def     TOKEN_ID_LITERAL;
extern token_id_empty_def       TOKEN_ID_EMPTY;




class token
{
public:
                    token(token_t id = TOKEN_ID_NONE) : f_id(id) {}
                    token(token const & t) : f_id(t.f_id), f_value(t.f_value) {}
                    token & operator = (token const & t)
                    {
                        if(this != &t)
                        {
                            f_id = t.f_id;
                            f_value = t.f_value;
                        }
                        return *this;
                    }

    // polymorphic type so user data works as expected
    virtual         ~token() {}

    void            set_id(token_t id) { f_id = id; }
    token_t         get_id() const { return f_id; }

    void            set_value(QVariant const & value) { f_value = value; }
    QVariant        get_value() const { return f_value; }

    QString         to_string() const;

private:
    token_t         f_id = token_t::TOKEN_ID_NONE_ENUM;
    QVariant        f_value = QVariant();
};
typedef QVector<QSharedPointer<token> >    vector_token_t;

class keyword;

class lexer
{
public:
    enum class lexer_error_t
    {
        LEXER_ERROR_NONE,

        LEXER_ERROR_INVALID_STRING,
        LEXER_ERROR_INVALID_C_COMMENT,
        LEXER_ERROR_INVALID_NUMBER,

        LEXER_ERROR_max
    };

                    lexer() { f_pos = f_input.begin(); }
                    lexer(lexer const & rhs) = delete;
    lexer &         operator = (lexer const & rhs) = delete;
    bool            eoi() const { return f_pos == f_input.end(); }
    uint32_t        line() const { return f_line; }
    void            set_input(QString const & input);
    void            add_keyword(keyword & k);
    token           next_token();
    lexer_error_t   get_error_code() const { return f_error_code; }
    QString         get_error_message() const { return f_error_message; }
    uint32_t        get_error_line() const { return f_error_line; }

private:
    // list of keywords / identifiers
    typedef QMap<QString, int>      keywords_map_t;

    QString                         f_input = QString();
    QString::const_iterator         f_pos = QString::const_iterator();
    uint32_t                        f_line = 0;
    keywords_map_t                  f_keywords = keywords_map_t();
    lexer_error_t                   f_error_code = lexer_error_t::LEXER_ERROR_NONE;
    QString                         f_error_message = QString();
    uint32_t                        f_error_line = 0;
};


class keyword
{
public:
                    keyword() {}
                    keyword(lexer & parent, QString const & keyword_identifier, int index_number = 0);

    QString         identifier() const { return f_identifier; }
    int             number() const { return f_number; }

private:
    static int      g_next_number;

    int             f_number = 0;
    QString         f_identifier = QString();
};

class choices;
class token_node;

// TODO: remove these once we only have shared & weak pointers
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
class rule
{
public:
    typedef void (*reducer_t)(rule const & r, QSharedPointer<token_node> & t);

                    rule() : f_parent(nullptr), f_reducer(nullptr) {}
                    rule(choices& c);
                    rule(rule const & r);

    void            add_rules(choices& c); // choices of rules
    void            add_choices(choices& c); // sub-rule
    void            add_token(token_t token); // any value accepted
    void            add_literal(QString const& value);
    void            add_keyword(keyword const& k);
    void            set_reducer(reducer_t reducer) { f_reducer = reducer; }
    int             count() const { return f_tokens.count(); }

    class rule_ref
    {
    public:
                        rule_ref(rule const * r, int position)
                            : f_rule(r), f_position(position)
                        {
                        }
                        rule_ref(rule_ref const & ref)
                            : f_rule(ref.f_rule), f_position(ref.f_position)
                        {
                        }

        token           get_token() const { return f_rule->f_tokens[f_position].f_token; }
        QString         get_value() const { return f_rule->f_tokens[f_position].f_value; }
        keyword         get_keyword() const { return f_rule->f_tokens[f_position].f_keyword; }
        choices&        get_choices() const { return *f_rule->f_tokens[f_position].f_choices; }

    private:
        rule const *    f_rule = nullptr;
        int             f_position = 0;
    };

    rule_ref const  operator [] (int position) const
                    {
                        return rule_ref(this, position);
                    }

    void            reduce(QSharedPointer<token_node> & n) const
                    {
                        if(f_reducer != nullptr)
                        {
                            f_reducer(*this, n);
                        }
                    }

    rule&           operator >> (token_id const & token);
    rule&           operator >> (QString const & literal);
    rule&           operator >> (char const * literal);
    rule&           operator >> (keyword const & k);
    rule&           operator >> (choices & c);
    rule&           operator >= (rule::reducer_t function);

    QString         to_string() const;

private:
    struct rule_data_t
    {
        rule_data_t();
        rule_data_t(rule_data_t const & s);
        rule_data_t(choices & c);
        rule_data_t(token_t token);
        rule_data_t(QString const & value); // i.e. literal
        rule_data_t(keyword const & k);

        token_t             f_token = token_t::TOKEN_ID_NONE_ENUM;
        QString             f_value = QString();        // required value if not empty
        keyword             f_keyword = keyword();      // the keyword
        choices *           f_choices = nullptr;        // sub-rule if not null & token TOKEN_ID_CHOICES_ENUM
    };

    choices *               f_parent = nullptr;
    QVector<rule_data_t>    f_tokens = QVector<rule_data_t>();
    reducer_t               f_reducer = reducer_t();
};
#pragma GCC diagnostic pop

// these have to be defined as friends of the class to enable
// all possible cases
rule & operator >> (token_id const & token_left, token_id const & token_right);
rule & operator >> (token_id const & token, QString const & literal);
rule & operator >> (token_id const & token, char const * literal);
rule & operator >> (token_id const & token, keyword const & k);
rule & operator >> (token_id const & token, choices & c);
rule & operator >> (QString const & literal, token_id const & token);
rule & operator >> (QString const & literal_left, QString const & literal_right);
rule & operator >> (QString const & literal, keyword const & k);
rule & operator >> (QString const & literal, choices & c);
rule & operator >> (keyword const & k, token_id const & token);
rule & operator >> (keyword const & k, QString const & literal);
rule & operator >> (keyword const & k_left, keyword const & k_right);
rule & operator >> (keyword const & k, choices & c);
rule & operator >> (choices & c, token_id const & token);
rule & operator >> (choices & c, QString const & literal);
rule & operator >> (choices & c, keyword const & k);
rule & operator >> (choices & c_left, choices & c_right);
rule & operator >> (char const * literal, choices & c);

// now a way to add a reducer function
rule & operator >= (token_id const & token, rule::reducer_t function);
rule & operator >= (QString const & literal, rule::reducer_t function);
rule & operator >= (keyword const & k, rule::reducer_t function);
rule & operator >= (choices & c, rule::reducer_t function);

rule & operator | (token_id const & token, rule & r_right);
rule & operator | (rule & r_left, token_id const & token);
rule & operator | (rule & r_left, keyword const & k);
rule & operator | (rule & r_left, rule & r_right);
rule & operator | (rule & r, choices & c);
// rule & operator | (choices & c, rule & r); -- defined in choices class

class grammar;

class choices
{
public:
                        choices(grammar * parent, char const * choice_name = "");
                        ~choices();

    QString const &     name() const { return f_name; }
    int                 count() { return f_rules.count(); }
    void                clear();

    choices &           operator = (const choices & rhs);

    choices &           operator >>= (token_id const & token);
    choices &           operator >>= (QString const & literal);
    choices &           operator >>= (keyword const & k);
    choices &           operator >>= (choices & rhs);
    choices &           operator >>= (rule & rhs);

    rule &              operator | (rule & r);

    void                add_rule(rule & r);
    rule const &        operator [] (int rule) const
                        {
                            return *f_rules[rule];
                        }

    // for debug purposes
    QString             to_string() const;

private:
    QString             f_name = QString();
    QVector<rule *>     f_rules = QVector<rule *>();
};
typedef QVector<choices *>            choices_array_t;


// base class that parsers derive from to create user data to be
// saved in token_node objects (see below)
// must always be used with QSharedPointer<>
class parser_user_data
{
public:
    virtual             ~parser_user_data() {}

private:
};


// token holder that can be saved in a tree like manner via the QObject
// child/parent functionality
class token_node : public token
{
// Q_OBJECT is not used because we don't have signals, slots or properties
public:
                                token_node() : token(token_t::TOKEN_ID_NODE_ENUM) {}

    void                        add_token(token & t) { f_tokens.push_back(QSharedPointer<token>(new token(t))); }
    void                        add_node(QSharedPointer<token_node> & n) { f_tokens.push_back(n); }
    vector_token_t &            tokens() { return f_tokens; }
    size_t                      size() const { return f_tokens.size(); }
    QSharedPointer<token>       operator [] (int index) { return f_tokens[index]; }
    QSharedPointer<token> const operator [] (int index) const { return f_tokens[index]; }
    void                        set_line(uint32_t line) { f_line = line; }
    uint32_t                    get_line() const { return f_line; }

    void                        set_user_data(QSharedPointer<parser_user_data> data) { f_user_data = data; }
    QSharedPointer<parser_user_data> get_user_data() const { return f_user_data; }

private:
    int32_t                             f_line = 0;
    vector_token_t                      f_tokens = vector_token_t();
    QSharedPointer<parser_user_data>    f_user_data = QSharedPointer<parser_user_data>();
};

class grammar
{
public:
                                grammar();

    void                        add_choices(choices & c);

    bool                        parse(lexer & input, choices & start);
    QSharedPointer<token_node>  get_result() const { return f_result; }

private:
    choices_array_t             f_choices = choices_array_t();
    QSharedPointer<token_node>  f_result = QSharedPointer<token_node>();
};



} // namespace parser
} // namespace snap
// vim: ts=4 sw=4 et
