// Snap Websites Server -- C-like parser and running environment
// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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

// libexcept
//
#include    <libexcept/exception.h>


// libdbproxy
//
//#include    <libdbproxy/value.h>
//#include    <libdbproxy/context.h>


// Qt
//
#include <QMap>
#include <QSharedPointer>


// C++
//
#include    <cmath>



namespace snap
{
namespace snap_expr
{


DECLARE_MAIN_EXCEPTION(snap_expr_exception);

DECLARE_LOGIC_ERROR(snap_expr_logic);

DECLARE_EXCEPTION(snap_expr_exception, snap_expr_exception_unknown_function);
DECLARE_EXCEPTION(snap_expr_exception, snap_expr_exception_invalid_number_of_parameters);
DECLARE_EXCEPTION(snap_expr_exception, snap_expr_exception_invalid_parameter_type);
DECLARE_EXCEPTION(snap_expr_exception, snap_expr_exception_invalid_parameter_value);
DECLARE_EXCEPTION(snap_expr_exception, snap_expr_exception_not_accessible);
DECLARE_EXCEPTION(snap_expr_exception, snap_expr_exception_not_ready);
DECLARE_EXCEPTION(snap_expr_exception, snap_expr_exception_invalid_data);
DECLARE_EXCEPTION(snap_expr_exception, snap_expr_exception_division_by_zero);





constexpr double pi_number()
{
    return std::acos(-1.0);
}



class variable_t
{
public:
    typedef QMap<QString, variable_t> variable_map_t;
    typedef QVector<variable_t> variable_vector_t;

    enum class variable_type_t
    {
        // Variables
        // WARNING: the order is VERY important
        EXPR_VARIABLE_TYPE_NULL,
        EXPR_VARIABLE_TYPE_BOOL,
        EXPR_VARIABLE_TYPE_INT8,
        EXPR_VARIABLE_TYPE_UINT8,
        EXPR_VARIABLE_TYPE_INT16,
        EXPR_VARIABLE_TYPE_UINT16,
        EXPR_VARIABLE_TYPE_INT32,
        EXPR_VARIABLE_TYPE_UINT32,
        EXPR_VARIABLE_TYPE_INT64,
        EXPR_VARIABLE_TYPE_UINT64,
        EXPR_VARIABLE_TYPE_FLOAT,
        EXPR_VARIABLE_TYPE_DOUBLE,
        EXPR_VARIABLE_TYPE_STRING,
        EXPR_VARIABLE_TYPE_BINARY
    };

                                        variable_t(QString const & name = "");

    QString                             get_name() const;

    variable_type_t                     get_type() const;
    void                                set_value(variable_type_t type, libdbproxy::value const & value);
    void                                set_value(); // set to NULL
    void                                set_value(bool value);
    void                                set_value(char value);
    void                                set_value(signed char value);
    void                                set_value(unsigned char value);
    void                                set_value(int16_t value);
    void                                set_value(uint16_t value);
    void                                set_value(int32_t value);
    void                                set_value(uint32_t value);
    void                                set_value(int64_t value);
    void                                set_value(uint64_t value);
    void                                set_value(float value);
    void                                set_value(double value);
    void                                set_value(char const * value);
    void                                set_value(wchar_t const * value);
    void                                set_value(QString const & value);
    void                                set_value(QByteArray const & value);

    libdbproxy::value const &           get_value() const;
    bool                                get_bool(QString const & name) const;
    int64_t                             get_integer(QString const & name) const;
    double                              get_floating_point(QString const & name) const;
    QString                             get_string(QString const & name) const;

    bool                                is_true() const;

    QString                             to_string() const;

private:
    QString                             f_name = QString();
    variable_type_t                     f_type = variable_type_t::EXPR_VARIABLE_TYPE_NULL;
    libdbproxy::value                   f_value = libdbproxy::value();
};


class functions_t
{
public:
    typedef void (*function_call_t)(variable_t & result, variable_t::variable_vector_t const & parameters);
    struct function_call_table_t
    {
        char const *    f_name = nullptr;
        function_call_t f_function = function_call_t();
    };
    typedef QMap<QString, function_call_t> expr_node_functions_map_t;

    void add_functions(function_call_table_t const *functions)
    {
        for(; functions->f_name != nullptr; ++functions)
        {
#ifdef DEBUG
            if(f_functions.contains(functions->f_name))
            {
                throw snap_expr_logic(
                          "functions_t::add_functions() function \""
                        + std::string(functions->f_name)
                        + "\" already defined");
            }
#endif
            f_functions[functions->f_name] = functions->f_function;
        }
    }

    function_call_t get_function(QString const & name)
    {
        if(f_functions.contains(name))
        {
            return f_functions[name];
        }
        return nullptr;
    }

    //set_request_internal_functions(int flags)
    //{
    //    f_flags = flags;
    //    // and when adding internal functions, check the flags and
    //    // add this or that accordingly (i.e. trigonometry,
    //    // statistics, strings, etc.)
    //    if((flags & ...) != 0)
    //    {
    //        
    //    }
    //}

    void set_has_internal_functions()
    {
        f_has_internal_functions = true;
    }

    bool get_has_internal_functions() const
    {
        return f_has_internal_functions;
    }

private:
    expr_node_functions_map_t   f_functions = expr_node_functions_map_t();
    bool                        f_has_internal_functions = false;
};



class expr_node_base
{
public:
    virtual ~expr_node_base() {}
};


class expr
{
public:
    typedef QSharedPointer<expr>            expr_pointer_t;
    typedef QMap<QString, expr_pointer_t>   expr_map_t;

    bool            compile(QString const & expression);
    QByteArray      serialize() const;
    void            unserialize(QByteArray const & serialized_code);
    void            execute(variable_t & result, variable_t::variable_map_t & variables, functions_t & functions);

    // some Snap! specialization
    static void     set_cassandra_context(libdbproxy::context::pointer_t context);

private:
    QSharedPointer<expr_node_base>          f_program_tree = QSharedPointer<expr_node_base>();
};




} // namespace snap_expr
} // namespace snap
// vim: ts=4 sw=4 et
