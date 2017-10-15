// Snap Websites Server -- check the lexer & parser of the Snap library
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

#include <snapwebsites/snap_parser.h>
#include <snapwebsites/qstring_stream.h>

#include <iostream>

#include <stdio.h>
#include <stdlib.h>

////////////////////////////////////////////
//   LEXER                                //
////////////////////////////////////////////
void check_lexer()
{
    struct literal_t {
        snap::parser::token_t   f_token;
        QVariant::Type          f_type;
        const char *            f_result;
    };
    static const literal_t results[] = {
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "+" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "++" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "+=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "-" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "--" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "-=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "*" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "*=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "**" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "**=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "/" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "/=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "%" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "%=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "~" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "~=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "&" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "&=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "&&" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "&&=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "|" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "|=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "||" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "||=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "^" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "^=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "^^" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "^^=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "!" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "!=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "!==" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "!<" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "!>" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "?" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "?=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "==" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "===" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "<" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "<=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "<<" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "<<=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "<?" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "<?=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   ">" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, ">=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, ">>" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, ">>>" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, ">>=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, ">>>=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, ">?" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, ">?=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   ":" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, ":=" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, "::" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "(" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   ")" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "{" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "}" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "," },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   ";" },
        { snap::parser::TOKEN_ID_STRING,  QVariant::String, "this is a string" },
        { snap::parser::TOKEN_ID_STRING,  QVariant::String, "<this+is-a*string>" },
        { snap::parser::TOKEN_ID_INTEGER, QVariant::ULongLong, "1234" },
        { snap::parser::TOKEN_ID_FLOAT,   QVariant::Double, "55.123" },
        { snap::parser::TOKEN_ID_IDENTIFIER, QVariant::String, "an_identifier" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "." },
        { snap::parser::TOKEN_ID_IDENTIFIER, QVariant::String, "_id" },
        { snap::parser::TOKEN_ID_IDENTIFIER, QVariant::String, "_1" },
        { snap::parser::TOKEN_ID_IDENTIFIER, QVariant::String, "i123" },
        { snap::parser::TOKEN_ID_KEYWORD, QVariant::Int, "333" }, // procedure
        { snap::parser::TOKEN_ID_IDENTIFIER, QVariant::String, "go" },
        { snap::parser::TOKEN_ID_KEYWORD, QVariant::Int, "7777" }, // is
        { snap::parser::TOKEN_ID_KEYWORD, QVariant::Int, "4761" }, // begin
        { snap::parser::TOKEN_ID_KEYWORD, QVariant::Int, "54" }, // if
        { snap::parser::TOKEN_ID_KEYWORD, QVariant::Int, "1" }, // true
        { snap::parser::TOKEN_ID_KEYWORD, QVariant::Int, "1078" }, // then
        { snap::parser::TOKEN_ID_IDENTIFIER, QVariant::String, "a" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::String, ":=" },
        { snap::parser::TOKEN_ID_INTEGER, QVariant::ULongLong, "56" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   ";" },
        { snap::parser::TOKEN_ID_KEYWORD, QVariant::Int, "5611" }, // else
        { snap::parser::TOKEN_ID_IDENTIFIER, QVariant::String, "a" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   "=" },
        { snap::parser::TOKEN_ID_IDENTIFIER, QVariant::String, "last_value" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   ";" },
        { snap::parser::TOKEN_ID_KEYWORD, QVariant::Int, "9000" }, // end
        { snap::parser::TOKEN_ID_KEYWORD, QVariant::Int, "54" }, // if
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   ";" },
        { snap::parser::TOKEN_ID_KEYWORD, QVariant::Int, "9000" }, // end
        { snap::parser::TOKEN_ID_IDENTIFIER, QVariant::String, "go" },
        { snap::parser::TOKEN_ID_LITERAL, QVariant::Char,   ";" },
    };

    struct keywords_t {
        const char *    f_identifier;
        int                f_value;
    };
    static const keywords_t keywords[] = {
        // here I use "random" numbers so we make sure we get the correct result
        // in a regular grammar you'd probably use numbers starting at 1 and
        // incrementing 1 by 1
        { "if",            54 },
        { "then",        1078 },
        { "else",        5611 },
        { "begin",        4761 },
        { "end",        9000 },
        { "while",        32 },
        { "until",        35 },
        { "do",            3030 },
        { "is",            7777 },
        { "true",        1 },
        { "false",        0 },
        { "function",    761 },
        { "procedure",    333 }
    };

    // the test string
    static const char *input = "+ ++ +=\n"
        " - -- -=\n"
        " * *= ** **=\n"
        " / /= /* and C like comments */ // or C++ like comments\n"
        " % %=\n"
        " ~ ~=\n"
        " & &= && &&=\n"
        " | |= || ||=\r" // Mac new line
        " ^ ^= ^^ ^^=\n"
        " ! != !== !< !>\n"
        " ? ?=\n"
        " = == ===\r\n" // Windows new line
        " < <= << <<= <? <?=\n"
        " > >= >> >>> >>= >>>= >? >?=\n"
        " : := ::\n"
        " ( ) { } , ;\n"
        " \"this is a string\"\n"
        " \"<this+is-a*string>\" // content of the string not detected as literals\n"
        " 1234\n"
        " 55.123\n"
        " an_identifier._id _1 i123\n"
        " procedure go is begin if true then a := 56; else a = last_value; end if; end go;\n"
    ;

    snap::parser::lexer l;
    l.set_input(input);
    for(size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
        snap::parser::keyword k(l, keywords[i].f_identifier, keywords[i].f_value);
        // we don't need to keep those in this case, so we destroy the
        // keyword, it does not remove them from the lexer though
    }
//printf("Parsing [%s]\n", input);
    const int max = static_cast<int>(sizeof(results) / sizeof(results[0]));
    for(int i = 0; i < max; ++i) {
        snap::parser::token t(l.next_token());
        if(t.get_id() == snap::parser::token_t::TOKEN_ID_ERROR_ENUM) {
            fprintf(stderr, "error: token #%d returned an error\n", i);
            exit(1);
        }
        if(t.get_id() != results[i].f_token) {
            int const id(static_cast<int>(t.get_id()));
            int const tok(static_cast<int>(results[i].f_token));
            fprintf(stderr, "error: token #%d returned an unexpected token identifier (%d instead of %d)\n",
                    i, id, tok);
            exit(1);
        }
        if(t.get_value().toString() != results[i].f_result) {
            fprintf(stderr, "error: token #%d returned an unexpected token value (\"%s\" instead of \"%s\")\n",
                    i, t.get_value().toString().toUtf8().data(), results[i].f_result);
            exit(1);
        }
    }
    printf("%d lines tokenized successfully.\n", l.line());
}



////////////////////////////////////////////
//   PARSER                               //
////////////////////////////////////////////
class domain_variable : public snap::parser::parser_user_data
{
public:
    enum domain_variable_type_t {
        DOMAIN_VARIABLE_TYPE_STANDARD = 0,
        DOMAIN_VARIABLE_TYPE_WEBSITE,
        DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT,
        DOMAIN_VARIABLE_TYPE_FLAG_NO_DEFAULT
    };

    domain_variable(domain_variable_type_t type, const QString& name, const QString& value)
        : f_type(type),
          f_name(name),
          f_value(value)
          //f_default("") -- auto-init
          //f_required(false) -- auto-init
    {
    }

    void set_default(const QString& default_value)
    {
        f_default = default_value;
    }

    void set_required(bool required = true)
    {
        f_required = required;
    }

    domain_variable_type_t get_type() const { return f_type; }
    const QString& get_name() const { return f_name; }
    const QString& get_value() const { return f_value; }
    const QString& get_default() const { return f_default; }
    bool get_required() const { return f_required; }

private:
    domain_variable_type_t          f_type;
    QString                         f_name;
    QString                         f_value;

    QString                         f_default;  // this may be the default (flag) or forced (website) value

    bool                            f_required = false;
};
class domain_info : public snap::parser::parser_user_data
{
public:
    void add_var(QSharedPointer<domain_variable> var)
    {
        f_vars.push_back(var);
    }

    void set_name(const QString& name)
    {
        f_name = name;
    }

    const QString& get_name() const
    {
        return f_name;
    }

    int size() const
    {
        return f_vars.size();
    }

    QSharedPointer<domain_variable> operator [] (int idx) const
    {
        return f_vars[idx];
    }

private:
    QString                            f_name;

    QVector<QSharedPointer<domain_variable> >        f_vars;
};
class domain_rules : public snap::parser::parser_user_data
{
public:
    void add_info(QSharedPointer<domain_info> info)
    {
        f_info.push_back(info);
    }
    int size() const
    {
        return f_info.size();
    }
    QSharedPointer<domain_info> operator [] (int idx) const
    {
        return f_info[idx];
    }

private:
    QVector<QSharedPointer<domain_info> >            f_info;
};


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void set_new_qualified_name(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
fprintf(stderr, "reducing simple name... %p (%s)\n", reinterpret_cast<void *>(&(*t)), (*t)[0]->get_value().toString().toUtf8().data());
}
void set_qualified_name(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
fprintf(stderr, "reducing qualified name! (%s::%s)\n", (*t)[0]->get_value().toString().toUtf8().data(), (*t)[2]->get_value().toString().toUtf8().data());
    QSharedPointer<snap::parser::token_node> n(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[0]));
    (*t)[0]->set_value((*n)[0]->get_value().toString() + "::" + (*t)[2]->get_value().toString());
}
void set_flag_empty(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
//fprintf(stderr, "*** got empty flag ***\n");
}
void set_opt_flag(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
//fprintf(stderr, "*** GOT OPT FLAG ***\n");
}
void set_var(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
fprintf(stderr, "Checking set_var p1 (%d)", static_cast<int>(t->size()));
    QSharedPointer<snap::parser::token_node> n(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[0]));
fprintf(stderr, " n = %p", reinterpret_cast<void *>(n.data()));
    QSharedPointer<domain_variable> v(new domain_variable(domain_variable::DOMAIN_VARIABLE_TYPE_STANDARD, (*n)[0]->get_value().toString(), (*t)[2]->get_value().toString()));
    t->set_user_data(v);
fprintf(stderr, " line %d - Reducing variable rule!!! [%s] = [%s]\n",
        t->get_line(),
        v->get_name().toUtf8().data(),
        (*t)[2]->get_value().toString().toUtf8().data());
}
void set_website_var(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
    QSharedPointer<snap::parser::token_node> n(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[0]));
fprintf(stderr, "Checking set_website_var p1 (%d) ", static_cast<int>(t->size()));
    QSharedPointer<domain_variable> v(new domain_variable(domain_variable::DOMAIN_VARIABLE_TYPE_WEBSITE, (*n)[0]->get_value().toString(), (*t)[4]->get_value().toString()));
    v->set_default((*t)[6]->get_value().toString());
    t->set_user_data(v);
fprintf(stderr, "line %d - Reducing website rule!!! [%s] = [%s/%s]\n",
        t->get_line(),
        (*n)[0]->get_value().toString().toUtf8().data(),
        (*t)[4]->get_value().toString().toUtf8().data(),
        (*t)[6]->get_value().toString().toUtf8().data()
        );
}
void set_flag_var(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
    QSharedPointer<snap::parser::token_node> n(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[0]));
    QSharedPointer<snap::parser::token_node> o(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[5]));
    bool is_empty = (*o)[0]->get_id() == snap::parser::token_t::TOKEN_ID_EMPTY_ENUM;

fprintf(stderr, "Checking set_flag_var p1 (%d/%s)", static_cast<int>(t->size()), (*n)[0]->get_value().toString().toUtf8().data());
QString def;
    QSharedPointer<domain_variable> v(new domain_variable(is_empty ? domain_variable::DOMAIN_VARIABLE_TYPE_FLAG_NO_DEFAULT : domain_variable::DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT, (*n)[0]->get_value().toString(), (*t)[4]->get_value().toString()));
//int p = t->size() == 8 ? 6 : 5;
    if(!is_empty) {
        v->set_default((*o)[1]->get_value().toString());
def = (*o)[1]->get_value().toString();
    }
else def = "/*empty*/";
    t->set_user_data(v);
fprintf(stderr, "%d - Reducing flag rule!!! [%s] = [%s/%s]\n",
        t->get_line(),
        (*n)[0]->get_value().toString().toUtf8().data(),
        (*t)[4]->get_value().toString().toUtf8().data(),
        def.toUtf8().data()
        );
}
void set_var_required(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
    QSharedPointer<snap::parser::token_node> n(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[1]));
    QSharedPointer<domain_variable> v(qSharedPointerDynamicCast<domain_variable, snap::parser::parser_user_data>(n->get_user_data()));
    v->set_required();
    t->set_user_data(v);
fprintf(stderr, "%d - Reducing set_var_required (%s)\n", t->get_line(), v->get_name().toUtf8().data());
}
void set_var_optional(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
    QSharedPointer<snap::parser::token_node> n(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[1]));
    QSharedPointer<domain_variable> v(qSharedPointerDynamicCast<domain_variable, snap::parser::parser_user_data>(n->get_user_data()));
    v->set_required(false);
    t->set_user_data(v);
fprintf(stderr, "%d - Reducing set_var_optional (%s)\n", t->get_line(), v->get_name().toUtf8().data());
}
void set_new_domain_list(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
    QSharedPointer<snap::parser::token_node> n(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[0]));
    QSharedPointer<domain_variable> v(qSharedPointerDynamicCast<domain_variable, snap::parser::parser_user_data>(n->get_user_data()));
    QSharedPointer<domain_info> info(new domain_info);
    info->add_var(v);
    t->set_user_data(info);
fprintf(stderr, "%d - Reducing set_new_domain_list (%s)\n", t->get_line(), v->get_name().toUtf8().data());
}
void set_add_domain_list(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
    QSharedPointer<snap::parser::token_node> nl(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[0]));
    QSharedPointer<snap::parser::token_node> nr(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[1]));

    QSharedPointer<domain_info> info(qSharedPointerDynamicCast<domain_info, snap::parser::parser_user_data>(nl->get_user_data()));
    QSharedPointer<domain_variable> v(qSharedPointerDynamicCast<domain_variable, snap::parser::parser_user_data>(nr->get_user_data()));
    info->add_var(v);
    t->set_user_data(info);
fprintf(stderr, "%d - Reducing set_add_domain_list (%s)\n", t->get_line(), v->get_name().toUtf8().data());
}
void set_rule(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
    QSharedPointer<snap::parser::token_node> nr(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[2]));
    QSharedPointer<domain_info> info(qSharedPointerDynamicCast<domain_info, snap::parser::parser_user_data>(nr->get_user_data()));
    info->set_name((*t)[0]->get_value().toString());
    t->set_user_data(info);
fprintf(stderr, "%d - Reducing set_rule (%s)\n", t->get_line(), info->get_name().toUtf8().data());
}
void set_new_rule_list(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
    QSharedPointer<snap::parser::token_node> n(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[0]));
    QSharedPointer<domain_info> info(qSharedPointerDynamicCast<domain_info, snap::parser::parser_user_data>(n->get_user_data()));
    QSharedPointer<domain_rules> rules(new domain_rules);
    rules->add_info(info);
    t->set_user_data(rules);
fprintf(stderr, "%d - Reducing set_new_rule_list (%s)\n", t->get_line(), info->get_name().toUtf8().data());
}
void set_add_rule_list(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
    QSharedPointer<snap::parser::token_node> nl(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[0]));
    QSharedPointer<snap::parser::token_node> nr(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[1]));

    QSharedPointer<domain_rules> rules(qSharedPointerDynamicCast<domain_rules, snap::parser::parser_user_data>(nl->get_user_data()));
    QSharedPointer<domain_info> info(qSharedPointerDynamicCast<domain_info, snap::parser::parser_user_data>(nr->get_user_data()));
    rules->add_info(info);
    t->set_user_data(rules);
fprintf(stderr, "%d - Reducing set_add_rule_list (%s)\n", t->get_line(), info->get_name().toUtf8().data());
}
void set_start_result(const snap::parser::rule& r, QSharedPointer<snap::parser::token_node>& t)
{
    QSharedPointer<snap::parser::token_node> n(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*t)[0]));
    t->set_user_data(n->get_user_data());
}
#pragma GCC diagnostic pop



// check with one of the first grammar used to parse domains
// (the current may still be the same...)
using namespace snap::parser;
bool check_parser(const QString& script)
{
    printf("Parser test [%.32s]\n", script.toUtf8().data());

    // LEXER

    // lexer object
    //printf("  Setting up the lexer\n");
    lexer lexer;
    //printf("    input\n");
    lexer.set_input(script);
    //printf("    keywords\n");
    keyword keyword_flag(lexer, "flag");
    keyword keyword_optional(lexer, "optional");
    keyword keyword_required(lexer, "required");
    keyword keyword_website(lexer, "website");

    // GRAMMAR
    //printf("  Setting up the grammar\n");
    grammar g;

printf("\n\n");
    // qualified_name
    //printf("    flag optional parameter\n");
    choices qualified_name(&g, "qualified_name");
    qualified_name >>= TOKEN_ID_IDENTIFIER >= set_new_qualified_name
                           | qualified_name >> "::" >> TOKEN_ID_IDENTIFIER >= set_qualified_name
    ;

    printf("%s\n\n", qualified_name.to_string().toUtf8().data());

    // flag_opt_param
    //printf("    flag optional parameter\n");
    choices flag_opt_param(&g, "flag_opt_param");
    flag_opt_param >>= TOKEN_ID_EMPTY >= set_flag_empty
                     | "," >> TOKEN_ID_STRING >= set_opt_flag
    ;

    printf("%s\n\n", flag_opt_param.to_string().toUtf8().data());

    // sub_domain_var
    //printf("    sub-domain var\n");
    choices sub_domain_var(&g, "sub_domain_var");
    sub_domain_var >>= qualified_name >> "=" >> TOKEN_ID_STRING >= set_var
                     | qualified_name >> "=" >> keyword_website >> "(" >> TOKEN_ID_STRING >> "," >> TOKEN_ID_STRING >> ")" >= set_website_var
                     | qualified_name >> "=" >> keyword_flag >> "(" >> TOKEN_ID_STRING >> flag_opt_param >> ")" >= set_flag_var
    ;

    printf("%s\n\n", sub_domain_var.to_string().toUtf8().data());

    // sub_domain:
    //printf("    sub-domain\n");
    choices sub_domain(&g, "sub_domain");
    sub_domain >>= keyword_required >> sub_domain_var >> ";" >= set_var_required
                 | keyword_optional >> sub_domain_var >> ";" >= set_var_optional
    ;

    printf("%s\n\n", sub_domain.to_string().toUtf8().data());

    // sub_domain_list:
    //printf("    sub-domain list\n");
    choices sub_domain_list(&g, "sub_domain_list");
    sub_domain_list >>= sub_domain >= set_new_domain_list
                      | sub_domain_list >> sub_domain >= set_add_domain_list
    ;

    printf("%s\n\n", sub_domain_list.to_string().toUtf8().data());

    // rule
    //printf("    rule\n");
    choices rule(&g, "rule");
    rule >>= TOKEN_ID_IDENTIFIER >> "{" >> sub_domain_list >> "}" >> ";" >= set_rule;

    printf("%s\n\n", rule.to_string().toUtf8().data());

    // rule_list
    //printf("    rule list\n");
    choices rule_list(&g, "rule_list");
    rule_list >>= rule >= set_new_rule_list
                | rule_list >> rule >= set_add_rule_list
    ;

    printf("%s\n\n", rule_list.to_string().toUtf8().data());

    // start
    //printf("    start\n");
    choices start(&g, "start");
    start >>= rule_list >= set_start_result;

    printf("%s\n\n", start.to_string().toUtf8().data());

    printf("  Parse input\n");
    fflush(stdout);
    if(!g.parse(lexer, start)) {
        return false;
    }

    // it worked, manage the result (check it)
    QSharedPointer<token_node> r(g.get_result());
    //QSharedPointer<snap::parser::token_node> n1(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*r)[0]));
    //printf("result count = %d\n", (int)n1->size());
    //printf("result = %p\n", dynamic_cast<domain_rules *>(n1->get_user_data()));
    //QSharedPointer<snap::parser::token_node> n2(qSharedPointerDynamicCast<snap::parser::token_node, snap::parser::token>((*r)[0]));
    //printf("result count = %d\n", (int)n2->size());
    QSharedPointer<domain_rules> rr(qSharedPointerDynamicCast<domain_rules, snap::parser::parser_user_data>(r->get_user_data()));
    printf("result = %p, %d\n", reinterpret_cast<void *>(&(*rr)), rr->size());
    printf("got [%s] and [%s]\n", (*rr)[0]->get_name().toUtf8().data(), (*rr)[1]->get_name().toUtf8().data());
    if((*rr)[0]->get_name() != "testing") {
        fprintf(stderr, "error: result 0 was expected to be \"testing\". Got \"%s\" instead.\n",
                (*rr)[0]->get_name().toUtf8().data());
        exit(1);
    }
    if((*rr)[1]->get_name() != "advanced") {
        fprintf(stderr, "error: result 0 was expected to be \"advanced\". Got \"%s\" instead.\n",
                (*rr)[0]->get_name().toUtf8().data());
        exit(1);
    }
    for(int idx = 0; idx < 2; ++idx) {
        QSharedPointer<domain_info> info((*rr)[idx]);
        printf("+++ %s\n", info->get_name().toUtf8().data());
        for(int j = 0; j < info->size(); ++j) {
            static const char *types[] = { "standard", "website", "flag(def)", "flag(nodef)" };
            QSharedPointer<domain_variable> var((*info)[j]);
            printf("  %s var %3d - %s: [%s] = [%s]", var->get_required() ? "REQUIRED " : "Optional ", j + 1, types[var->get_type()], var->get_name().toUtf8().data(), var->get_value().toUtf8().data());
            switch(var->get_type()) {
            case domain_variable::DOMAIN_VARIABLE_TYPE_WEBSITE:
                printf(" WEBSITE = [%s]", var->get_default().toUtf8().data());
                break;

            case domain_variable::DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT:
                printf(" DEFAULT = [%s]", var->get_default().toUtf8().data());
                break;

            default:
                // ignore others, there's no default.
                break;

            }
            printf("\n");
        }
    }

    return true;
}

void check_parser_scripts()
{
    check_parser("testing { required global::language = \"(en|fr|de|es)\";\n"
                          " optional version = flag(\"[0-9]{1,3}\\.[0-9]{1,3}\", \"1.0\");\r"
                          " optional host = website(\"w{1,4}\", \"www\");\r\n"
                          " };\n"
                 "advanced { optional language = \"(en|fr|es)\";\r\n"
                          " required name = flag(\"[a-zA-Z0-9]+\");\r"
                          " optional host = website(\"w{0,4}\\.\", \"www\");\n"
                          " };\n");
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc, char *argv[])
{
    check_lexer();
    check_parser_scripts();
}
#pragma GCC diagnostic pop

// vim: ts=4 sw=4 et
