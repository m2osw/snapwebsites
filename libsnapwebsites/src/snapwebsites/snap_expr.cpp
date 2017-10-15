// Snap Websites Server -- C-like expression scripting support
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

//#define SHOW_COMMANDS

#include "snapwebsites/snap_expr.h"

#include "snapwebsites/snapwebsites.h"
#include "snapwebsites/not_reached.h"
#include "snapwebsites/not_used.h"
#include "snapwebsites/log.h"

// QtSerialization lib
//
#include <QtSerialization/QSerializationComposite.h>
#include <QtSerialization/QSerializationFieldString.h>
#include <QtSerialization/QSerializationFieldBasicTypes.h>

#include "snapwebsites/poison.h"


namespace snap
{
namespace snap_expr
{



namespace
{
/** \brief Context to access the database.
 *
 * Different functions available in this expression handling program let
 * you access the database. For example, the cell() function let you
 * read the content of a cell and save it in a variable, or compare
 * to a specific value, or use it as part of some key.
 *
 * The pointer is set using the set_cassandra_context() which is a
 * static function and should only be called once.
 */
QtCassandra::QCassandraContext::pointer_t   g_context;
} // no name namespace




variable_t::variable_t(QString const& name)
    : f_name(name)
    //, f_type(variable_type_t::EXPR_VARIABLE_TYPE_NULL) -- auto-init
    //, f_value() -- auto-init (empty, a.k.a. NULL)
{
}


QString variable_t::get_name() const
{
    return f_name;
}


variable_t::variable_type_t variable_t::get_type() const
{
    return f_type;
}


QtCassandra::QCassandraValue const& variable_t::get_value() const
{
    return f_value;
}


void variable_t::set_value(variable_type_t type, QtCassandra::QCassandraValue const & value)
{
    f_type = type;
    f_value = value;
}


void variable_t::set_value()
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_NULL;
    f_value.setNullValue();
}


void variable_t::set_value(bool value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_BOOL;
    f_value = value;
}


void variable_t::set_value(char value)
{
    // with g++ this is unsigned...
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_UINT8;
    f_value = value;
}


void variable_t::set_value(signed char value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_INT8;
    f_value = value;
}


void variable_t::set_value(unsigned char value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_UINT8;
    f_value = value;
}


void variable_t::set_value(int16_t value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_INT16;
    f_value = value;
}


void variable_t::set_value(uint16_t value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_UINT16;
    f_value = value;
}


void variable_t::set_value(int32_t value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_INT32;
    f_value = value;
}


void variable_t::set_value(uint32_t value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_UINT32;
    f_value = value;
}


void variable_t::set_value(int64_t value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_INT64;
    f_value = value;
}


void variable_t::set_value(uint64_t value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_UINT64;
    f_value = value;
}


void variable_t::set_value(float value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_FLOAT;
    f_value = value;
}


void variable_t::set_value(double value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE;
    f_value = value;
}


void variable_t::set_value(char const * value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_STRING;
    f_value.setStringValue(QString::fromUtf8(value));
}


void variable_t::set_value(wchar_t const * value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_STRING;
    f_value.setStringValue(QString::fromWCharArray(value));
}


void variable_t::set_value(QString const & value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_STRING;
    f_value.setStringValue(value);
}


void variable_t::set_value(QByteArray const & value)
{
    f_type = variable_type_t::EXPR_VARIABLE_TYPE_BINARY;
    f_value = value;
}


bool variable_t::is_true() const
{
    switch(f_type)
    {
    case variable_type_t::EXPR_VARIABLE_TYPE_NULL:
        return false;

    case variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
        return f_value.safeBoolValue();

    case variable_type_t::EXPR_VARIABLE_TYPE_INT8:
    case variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
        return f_value.safeSignedCharValue() != 0;

    case variable_type_t::EXPR_VARIABLE_TYPE_INT16:
    case variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
        return f_value.safeInt16Value() != 0;

    case variable_type_t::EXPR_VARIABLE_TYPE_INT32:
    case variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
        return f_value.safeInt32Value() != 0;

    case variable_type_t::EXPR_VARIABLE_TYPE_INT64:
    case variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
        return f_value.safeInt32Value() != 0;

    case variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        return f_value.safeFloatValue() != 0.0f;
#pragma GCC diagnostic pop

    case variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        return f_value.safeDoubleValue() != 0.0;
#pragma GCC diagnostic pop

    case variable_type_t::EXPR_VARIABLE_TYPE_STRING:
    case variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
        return !f_value.nullValue();

    }
    NOTREACHED();
}


bool variable_t::get_bool(QString const& name) const
{
    switch(get_type())
    {
    case variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
        return get_value().safeBoolValue();

    default:
        throw snap_expr_exception_invalid_parameter_type(QString("parameter for %1 must be a Boolean").arg(name));

    }
    NOTREACHED();
}


int64_t variable_t::get_integer(QString const& name) const
{
    switch(get_type())
    {
    case variable_type_t::EXPR_VARIABLE_TYPE_INT8:
        return get_value().safeSignedCharValue();

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
        return get_value().safeUnsignedCharValue();

    case variable_type_t::EXPR_VARIABLE_TYPE_INT16:
        return get_value().safeInt16Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
        return get_value().safeUInt16Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_INT32:
        return get_value().safeInt32Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
        return get_value().safeUInt32Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_INT64:
        return get_value().safeInt64Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
        return get_value().safeUInt64Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
        return static_cast<int64_t>(get_value().safeFloatValue());

    case variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
        return static_cast<int64_t>(get_value().safeDoubleValue());

    default:
        // although we allow floating point too...
        throw snap_expr_exception_invalid_parameter_type(QString("parameter for %1 must be an integer").arg(name));

    }
    NOTREACHED();
}


double variable_t::get_floating_point(QString const& name) const
{
    switch(get_type())
    {
    case variable_type_t::EXPR_VARIABLE_TYPE_INT8:
        return get_value().safeSignedCharValue();

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
        return get_value().safeUnsignedCharValue();

    case variable_type_t::EXPR_VARIABLE_TYPE_INT16:
        return get_value().safeInt16Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
        return get_value().safeUInt16Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_INT32:
        return get_value().safeInt32Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
        return get_value().safeUInt32Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_INT64:
        return get_value().safeInt64Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
        return get_value().safeUInt64Value();

    case variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
        return static_cast<int64_t>(get_value().safeFloatValue());

    case variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
        return static_cast<int64_t>(get_value().safeDoubleValue());

    default:
        // although we auto-convert integers too
        throw snap_expr_exception_invalid_parameter_type(QString("parameter for %1 must be a floating point").arg(name));

    }
    NOTREACHED();
}


QString variable_t::get_string(QString const & name) const
{
    switch(get_type())
    {
    case variable_type_t::EXPR_VARIABLE_TYPE_STRING:
        return get_value().stringValue();

    default:
        throw snap_expr_exception_invalid_parameter_type(QString("parameter for %1 must be a string (got type %2 instead)")
                            .arg(name)
                            .arg(static_cast<int>(get_type())));

    }
    NOTREACHED();
}


QString variable_t::to_string() const
{
    switch(f_type)
    {
    case variable_type_t::EXPR_VARIABLE_TYPE_NULL:
        return "(null)";

    case variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
        return f_value.safeBoolValue() ? "true" : "false";

    case variable_type_t::EXPR_VARIABLE_TYPE_INT8:
        return QString("%1").arg(f_value.safeSignedCharValue());

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
        return QString("%1").arg(static_cast<int>(f_value.safeUnsignedCharValue()));

    case variable_type_t::EXPR_VARIABLE_TYPE_INT16:
        return QString("%1").arg(f_value.safeInt16Value());

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
        return QString("%1").arg(f_value.safeUInt16Value());

    case variable_type_t::EXPR_VARIABLE_TYPE_INT32:
        return QString("%1").arg(f_value.safeInt32Value());

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
        return QString("%1").arg(f_value.safeUInt32Value());

    case variable_type_t::EXPR_VARIABLE_TYPE_INT64:
        return QString("%1").arg(f_value.safeInt64Value());

    case variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
        return QString("%1").arg(f_value.safeUInt64Value());

    case variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
        return QString("%1").arg(f_value.safeFloatValue());

    case variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
        return QString("%1").arg(f_value.safeDoubleValue());

    case variable_type_t::EXPR_VARIABLE_TYPE_STRING:
        return QString("\"%1\"").arg(f_value.stringValue());

    case variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
        return "#...TODO: binary...#";

    }
    NOTREACHED();
}







using namespace parser;


class expr_node
        : public expr_node_base
        , public parser_user_data
        , public QtSerialization::QSerializationObject
{
public:
    static int const LIST_TEST_NODE_MAJOR_VERSION = 1;
    static int const LIST_TEST_NODE_MINOR_VERSION = 0;

    typedef QSharedPointer<expr_node>           expr_node_pointer_t;
    typedef QVector<expr_node_pointer_t>        expr_node_vector_t;
    typedef QMap<QString, expr_node_pointer_t>  expr_node_map_t;

    static functions_t::function_call_table_t const internal_functions[];

    enum class node_type_t
    {
        NODE_TYPE_UNKNOWN, // used when creating a node without a type
        NODE_TYPE_LOADING, // used when loading

        // Operations
        NODE_TYPE_OPERATION_LIST, // comma operator
        NODE_TYPE_OPERATION_LOGICAL_NOT,
        NODE_TYPE_OPERATION_BITWISE_NOT,
        NODE_TYPE_OPERATION_NEGATE,
        NODE_TYPE_OPERATION_FUNCTION,
        NODE_TYPE_OPERATION_MULTIPLY,
        NODE_TYPE_OPERATION_DIVIDE,
        NODE_TYPE_OPERATION_MODULO,
        NODE_TYPE_OPERATION_ADD,
        NODE_TYPE_OPERATION_SUBTRACT,
        NODE_TYPE_OPERATION_SHIFT_LEFT,
        NODE_TYPE_OPERATION_SHIFT_RIGHT,
        NODE_TYPE_OPERATION_LESS,
        NODE_TYPE_OPERATION_LESS_OR_EQUAL,
        NODE_TYPE_OPERATION_GREATER,
        NODE_TYPE_OPERATION_GREATER_OR_EQUAL,
        NODE_TYPE_OPERATION_MINIMUM,
        NODE_TYPE_OPERATION_MAXIMUM,
        NODE_TYPE_OPERATION_EQUAL,
        NODE_TYPE_OPERATION_NOT_EQUAL,
        NODE_TYPE_OPERATION_BITWISE_AND,
        NODE_TYPE_OPERATION_BITWISE_XOR,
        NODE_TYPE_OPERATION_BITWISE_OR,
        NODE_TYPE_OPERATION_LOGICAL_AND,
        NODE_TYPE_OPERATION_LOGICAL_XOR,
        NODE_TYPE_OPERATION_LOGICAL_OR,
        NODE_TYPE_OPERATION_CONDITIONAL,
        NODE_TYPE_OPERATION_ASSIGNMENT, // save to variable
        NODE_TYPE_OPERATION_VARIABLE, // load from variable

        // Literals
        NODE_TYPE_LITERAL_BOOLEAN,
        NODE_TYPE_LITERAL_INTEGER,
        NODE_TYPE_LITERAL_FLOATING_POINT,
        NODE_TYPE_LITERAL_STRING,

        // Variable
        NODE_TYPE_VARIABLE
    };

    static char const *type_names[static_cast<size_t>(node_type_t::NODE_TYPE_VARIABLE) + 1];

    expr_node(node_type_t type)
        : f_type(type)
        //, f_name("")
        , f_variable("")
        //, f_children() -- auto-init
    {
    }

    virtual ~expr_node()
    {
    }

    node_type_t get_type() const
    {
        return f_type;
    }

    void set_name(QString const& name)
    {
        f_name = name;
    }

    QString get_name() const
    {
        return f_name;
    }

    variable_t const& get_variable() const
    {
        verify_variable();
        return f_variable;
    }

    void set_variable(variable_t const& variable)
    {
        verify_variable();
        f_variable = variable;
    }

    void add_child(expr_node_pointer_t child)
    {
        verify_children(-1);
        f_children.push_back(child);
    }

    void remove_child(int idx)
    {
        verify_children(idx);
        f_children.remove(idx);
    }

    void insert_child(int idx, expr_node_pointer_t child)
    {
        verify_children(idx, true);
        f_children.insert(idx, child);
    }

    int children_size() const
    {
        verify_children(-1);
        return f_children.size();
    }

    expr_node_pointer_t get_child(int idx)
    {
        verify_children(idx);
        return f_children[idx];
    }

    static QSharedPointer<expr_node> load(QtSerialization::QReader& r)
    {
        // create a "root" used only to load the data
        expr_node_pointer_t root(new expr_node(node_type_t::NODE_TYPE_LOADING));
        root->read(r);
#ifdef DEBUG
        if(root->children_size() != 1)
        {
            throw snap_logic_exception(QString("expr_node::load() did not return exactly one child in the root node"));
        }
//std::cout << "--- Loaded result:\n" << root->get_child(0)->toString() << "---\n";
#endif
        return root->get_child(0);
    }

    void read(QtSerialization::QReader& r)
    {
        // read the data from the reader
        QtSerialization::QComposite comp;
        qint32 type(static_cast<qint32>(static_cast<node_type_t>(node_type_t::NODE_TYPE_LOADING)));
        QtSerialization::QFieldInt32 node_type(comp, "type", type); // f_type is an enum...
        QtSerialization::QFieldString node_name(comp, "name", f_name);
        qint64 value_int(0);
        QtSerialization::QFieldInt64 node_int(comp, "int", value_int);
        double value_dbl(0.0);
        QtSerialization::QFieldDouble node_flt(comp, "flt", value_dbl);
        QString value_str; // ("") -- default value
        QtSerialization::QFieldString node_str(comp, "str", value_str);
        QtSerialization::QFieldTag vars(comp, "node", this);
        r.read(comp);
        f_type = static_cast<node_type_t>(type);
        switch(f_type)
        {
        case node_type_t::NODE_TYPE_UNKNOWN:
            throw snap_logic_exception("expr_node::read() loaded a node of type: node_type_t::NODE_TYPE_UNKNOWN");

        case node_type_t::NODE_TYPE_LITERAL_BOOLEAN:
            f_variable.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, static_cast<bool>(value_int));
            break;

        case node_type_t::NODE_TYPE_LITERAL_INTEGER:
            f_variable.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64, static_cast<int64_t>(value_int));
            break;

        case node_type_t::NODE_TYPE_LITERAL_FLOATING_POINT:
            f_variable.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE, value_dbl);
            break;

        case node_type_t::NODE_TYPE_LITERAL_STRING:
            f_variable.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value_str);
            break;

        default:
            // only literals have a value when serializing
            break;

        }
    }

    virtual void readTag(QString const& name, QtSerialization::QReader& r)
    {
        if(name == "node")
        {
            // create a node
            expr_node_pointer_t child(new expr_node(node_type_t::NODE_TYPE_LOADING));
            // read the data from the reader
            child->read(r);
            // add to the vector
            add_child(child);
        }
    }

    void write(QtSerialization::QWriter& w) const
    {
        QtSerialization::QWriter::QTag tag(w, "node");

        QtSerialization::writeTag(w, "type", static_cast<qint32>(static_cast<node_type_t>(f_type)));

        if(!f_name.isEmpty())
        {
            QtSerialization::writeTag(w, "name", f_name);
        }

        switch(f_type)
        {
        case node_type_t::NODE_TYPE_LITERAL_BOOLEAN:
            QtSerialization::writeTag(w, "int", static_cast<qint64>(f_variable.get_value().safeBoolValue()));
            break;

        case node_type_t::NODE_TYPE_LITERAL_INTEGER:
            QtSerialization::writeTag(w, "int", static_cast<qint64>(f_variable.get_value().safeInt64Value()));
            break;

        case node_type_t::NODE_TYPE_LITERAL_FLOATING_POINT:
            QtSerialization::writeTag(w, "flt", f_variable.get_value().safeDoubleValue());
            break;

        case node_type_t::NODE_TYPE_LITERAL_STRING:
            QtSerialization::writeTag(w, "str", f_variable.get_value().stringValue());
            break;

        default:
            // only literals have a value when serializing
            break;

        }

        int const max_children(f_children.size());
        for(int i(0); i < max_children; ++i)
        {
            f_children[i]->write(w);
        }
    }

    // SFINAE test
    // Source: http://stackoverflow.com/questions/257288/is-it-possible-to-write-a-c-template-to-check-for-a-functions-existence
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
    template<typename T>
    class has_function
    {
    private:
        typedef char yes[1];
        typedef char no [2];

        template <typename U, U> struct type_check;

        // integers() is always available at this point
        //template <typename C> static yes &test_integers(type_check<int64_t(&)(int64_t, int64_t), C::integers> *);
        //template <typename C> static no  &test_integers(...);

        template <typename C> static yes &test_floating_points(type_check<double(&)(double, double), C::floating_points> *);
        template <typename C> static no  &test_floating_points(...);

        template <typename C> static yes &test_string_integer(type_check<QString(&)(QString const&, int64_t), C::string_integer> *);
        template <typename C> static no  &test_string_integer(...);

        template <typename C> static yes &test_strings(type_check<QString(&)(QString const&, QString const&), C::strings> *);
        template <typename C> static no  &test_strings(...);

        // integers() is always available at this point
        //template <typename C> static yes &test_bool_integers(type_check<bool(&)(int64_t, int64_t), C::integers> *);
        //template <typename C> static no  &test_bool_integers(...);

        template <typename C> static yes &test_bool_floating_points(type_check<bool(&)(double, double), C::floating_points> *);
        template <typename C> static no  &test_bool_floating_points(...);

        template <typename C> static yes &test_bool_string_integer(type_check<bool(&)(QString const&, int64_t), C::string_integer> *);
        template <typename C> static no  &test_bool_string_integer(...);

        template <typename C> static yes &test_bool_strings(type_check<bool(&)(QString const&, QString const&), C::strings> *);
        template <typename C> static no  &test_bool_strings(...);

    public:
        //static bool const has_integers        = sizeof(test_integers       <T>(nullptr)) == sizeof(yes);
        static bool const has_floating_points = sizeof(test_floating_points<T>(nullptr)) == sizeof(yes);
        static bool const has_string_integer  = sizeof(test_string_integer <T>(nullptr)) == sizeof(yes);
        static bool const has_strings         = sizeof(test_strings        <T>(nullptr)) == sizeof(yes);
        //static bool const has_bool_integers        = sizeof(test_bool_integers       <T>(nullptr)) == sizeof(yes);
        static bool const has_bool_floating_points = sizeof(test_bool_floating_points<T>(nullptr)) == sizeof(yes);
        static bool const has_bool_string_integer  = sizeof(test_bool_string_integer <T>(nullptr)) == sizeof(yes);
        static bool const has_bool_strings         = sizeof(test_bool_strings        <T>(nullptr)) == sizeof(yes);
    };
#pragma GCC diagnostic pop

    // select structure depending on bool
    template<bool C, typename T = void>
    struct enable_if
    {
        typedef T type;
    };

    // if false, do nothing, whatever T
    template<typename T>
    struct enable_if<false, T>
    {
    };

    template<typename F>
    typename enable_if<has_function<F>::has_floating_points>::type do_float(variable_t & result, double a, double b)
    {
        QtCassandra::QCassandraValue value;
        value.setFloatValue(static_cast<float>(F::floating_points(a, b)));
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT, value);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    template<typename F>
    typename enable_if<!has_function<F>::has_floating_points>::type do_float(variable_t & result, double a, double b)
    {
    }
#pragma GCC diagnostic pop

    template<typename F>
    typename enable_if<has_function<F>::has_bool_floating_points>::type do_float_to_bool(variable_t& result, double a, double b)
    {
        QtCassandra::QCassandraValue value;
        value.setBoolValue(F::floating_points(a, b));
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    template<typename F>
    typename enable_if<!has_function<F>::has_bool_floating_points>::type do_float_to_bool(variable_t& result, double a, double b)
    {
    }
#pragma GCC diagnostic pop

    template<typename F>
    typename enable_if<has_function<F>::has_floating_points>::type do_double(variable_t& result, double a, double b)
    {
        QtCassandra::QCassandraValue value;
        value.setDoubleValue(F::floating_points(a, b));
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE, value);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    template<typename F>
    typename enable_if<!has_function<F>::has_floating_points>::type do_double(variable_t& result, double a, double b)
    {
    }
#pragma GCC diagnostic pop

    template<typename F>
    typename enable_if<has_function<F>::has_bool_floating_points>::type do_double_to_bool(variable_t & result, double a, double b)
    {
        QtCassandra::QCassandraValue value;
        value.setBoolValue(F::floating_points(a, b));
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    template<typename F>
    typename enable_if<!has_function<F>::has_bool_floating_points>::type do_double_to_bool(variable_t & result, double a, double b)
    {
    }
#pragma GCC diagnostic pop

    template<typename F>
    typename enable_if<has_function<F>::has_string_integer>::type do_string_integer(variable_t & result, QString const & a, int64_t b)
    {
        QtCassandra::QCassandraValue value;
        value.setStringValue(F::string_integer(a, b));
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    template<typename F>
    typename enable_if<!has_function<F>::has_string_integer>::type do_string_integer(variable_t & result, QString const & a, int64_t b)
    {
    }
#pragma GCC diagnostic pop

    template<typename F>
    typename enable_if<has_function<F>::has_bool_string_integer>::type do_string_integer_to_bool(variable_t & result, QString const & a, int64_t b)
    {
        QtCassandra::QCassandraValue value;
        value.setBoolValue(F::string_integer(a, b));
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    template<typename F>
    typename enable_if<!has_function<F>::has_bool_string_integer>::type do_string_integer_to_bool(variable_t & result, QString const & a, int64_t b)
    {
    }
#pragma GCC diagnostic pop

    template<typename F>
    typename enable_if<has_function<F>::has_strings>::type do_strings(variable_t & result, QString const & a, QString const & b)
    {
        QtCassandra::QCassandraValue value;
        value.setStringValue(F::strings(a, b));
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    template<typename F>
    typename enable_if<!has_function<F>::has_strings>::type do_strings(variable_t& result, QString const& a, QString const& b)
    {
    }
#pragma GCC diagnostic pop

    template<typename F>
    typename enable_if<has_function<F>::has_bool_strings>::type do_strings_to_bool(variable_t & result, QString const & a, QString const & b)
    {
        QtCassandra::QCassandraValue value;
        value.setBoolValue(F::strings(a, b));
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    template<typename F>
    typename enable_if<!has_function<F>::has_bool_strings>::type do_strings_to_bool(variable_t & result, QString const & a, QString const & b)
    {
    }
#pragma GCC diagnostic pop

    class op_multiply
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a * b; }
        static double floating_points(double a, double b) { return a * b; }
        static QString string_integer(QString const& a, int64_t b) { return a.repeated(static_cast<int>(b)); }
    };

    class op_divide
    {
    public:
        static int64_t integers(int64_t a, int64_t b)
        {
            if(b == 0)
            {
                throw snap_expr_exception_division_by_zero("expr_node::op_divide() called with integers and a denominator set to zero");
            }
            return a / b;
        }
        static double floating_points(double a, double b)
        {
            // in this case division by zero is well defined!
            return a / b;
        }
    };

    class op_modulo
    {
    public:
        static int64_t integers(int64_t a, int64_t b)
        {
            if(b == 0)
            {
                throw snap_expr_exception_division_by_zero("expr_node::op_modulo() called with integers and a denominator set to zero");
            }
            return a % b;
        }
    };

    class op_add
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a + b; }
        static double floating_points(double a, double b) { return a + b; }
        static QString strings(QString const & a, QString const & b) { return a + b; }
    };

    class op_subtract
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a - b; }
        static double floating_points(double a, double b) { return a - b; }
    };

    class op_shift_left
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a << b; }
    };

    class op_shift_right
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a >> b; }
        static int64_t integers(uint64_t a, int64_t b) { return a >> b; }
        static int64_t integers(int64_t a, uint64_t b) { return a >> b; }
        static int64_t integers(uint64_t a, uint64_t b) { return a >> b; }
    };

    class op_less
    {
    public:
        static bool integers(int64_t a, int64_t b) { return a < b; }
        static bool floating_points(double a, double b) { return a < b; }
        static bool strings(QString const& a, QString const& b) { return a < b; }
    };

    class op_less_or_equal
    {
    public:
        static bool integers(int64_t a, int64_t b) { return a <= b; }
        static bool floating_points(double a, double b) { return a <= b; }
        static bool strings(QString const& a, QString const& b) { return a <= b; }
    };

    class op_greater
    {
    public:
        static bool integers(int64_t a, int64_t b) { return a > b; }
        static bool floating_points(double a, double b) { return a > b; }
        static bool strings(QString const& a, QString const& b) { return a > b; }
    };

    class op_greater_or_equal
    {
    public:
        static bool integers(int64_t a, int64_t b) { return a >= b; }
        static bool floating_points(double a, double b) { return a >= b; }
        static bool strings(QString const& a, QString const& b) { return a >= b; }
    };

    class op_minimum
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a < b ? a : b; }
        static double floating_points(double a, double b) { return a < b ? a : b; }
        static QString strings(QString const& a, QString const& b) { return a < b ? a : b; }
    };

    class op_maximum
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a > b ? a : b; }
        static double floating_points(double a, double b) { return a > b ? a : b; }
        static QString strings(QString const& a, QString const& b) { return a > b ? a : b; }
    };

    class op_equal
    {
    public:
        static bool integers(int64_t a, int64_t b) { return a == b; }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        // TBD -- users should not compare floating points with == and !=
        //        can we do something about it here? (i.e. emit a warning
        //        in debug mode?)
        static bool floating_points(double a, double b) { return a == b; }
        static bool strings(QString const& a, QString const& b) { return a == b; }
#pragma GCC diagnostic pop
    };

    class op_not_equal
    {
    public:
        static bool integers(int64_t a, int64_t b) { return a != b; }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        // TBD -- users should not compare floating points with == and !=
        //        can we do something about it here? (i.e. emit a warning
        //        in debug mode?)
        static bool floating_points(double a, double b) { return a != b; }
        static bool strings(QString const& a, QString const& b) { return a != b; }
#pragma GCC diagnostic pop
    };

    class op_bitwise_and
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a & b; }
    };

    class op_bitwise_xor
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a ^ b; }
    };

    class op_bitwise_or
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a | b; }
    };

    class op_logical_and
    {
    public:
        static bool integers(int64_t a, int64_t b) { return a && b; }
    };

    class op_logical_xor
    {
    public:
        static bool integers(int64_t a, int64_t b) { return (a != 0) ^ (b != 0); }
    };

    class op_logical_or
    {
    public:
        static bool integers(int64_t a, int64_t b) { return a || b; }
    };

    void execute(variable_t & result, variable_t::variable_map_t & variables, functions_t & functions)
    {
#ifdef SHOW_COMMANDS
        switch(f_type)
        {
        case node_type_t::NODE_TYPE_UNKNOWN:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_UNKNOWN";
            break;

        case node_type_t::NODE_TYPE_LOADING:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_LOADING";
            break;

        case node_type_t::NODE_TYPE_OPERATION_LIST:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_LIST";
            break;

        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_NOT:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_LOGICAL_NOT";
            break;

        case node_type_t::NODE_TYPE_OPERATION_BITWISE_NOT:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_BITWISE_NOT";
            break;

        case node_type_t::NODE_TYPE_OPERATION_NEGATE:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_NEGATE";
            break;

        case node_type_t::NODE_TYPE_OPERATION_FUNCTION:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_FUNCTION -- " << f_name << "()";
            break;

        case node_type_t::NODE_TYPE_OPERATION_MULTIPLY:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_MULTIPLY (*)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_DIVIDE:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_DIVIDE (/)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_MODULO:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_MODULO (%)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_ADD:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_ADD (+)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_SUBTRACT:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_SUBTRACT (-)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_SHIFT_LEFT:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_SHIFT_LEFT (<<)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_SHIFT_RIGHT:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_SHIFT_RIGHT (>>)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_LESS:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_LESS (<)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_LESS_OR_EQUAL:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_LESS_OR_EQUAL (<=)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_GREATER:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_GREATER (>)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_GREATER_OR_EQUAL:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_GREATER_OR_EQUAL (>=)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_MINIMUM:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_MINIMUM (<?)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_MAXIMUM:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_MAXIMUM (>?)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_EQUAL:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_EQUAL (==)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_NOT_EQUAL:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_NOT_EQUAL (!=)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_BITWISE_AND:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_BITWISE_AND (&)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_BITWISE_XOR:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_BITWISE_XOR (^)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_BITWISE_OR:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_BITWISE_OR (|)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_AND:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_LOGICAL_AND (&&)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_XOR:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_LOGICAL_XOR (^^)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_OR:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_LOGICAL_OR (||)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_CONDITIONAL:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_CONDITIONAL (?:)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_ASSIGNMENT:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_ASSIGNMENT (" << f_name << ":= ...)";
            break;

        case node_type_t::NODE_TYPE_OPERATION_VARIABLE:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_OPERATION_VARIABLE (" << f_name
                << (variables.contains(f_name) ? " = " : "")
                << (variables.contains(f_name) ? variables[f_name].to_string() : "")
                << ")";
            break;

        case node_type_t::NODE_TYPE_LITERAL_BOOLEAN:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_LITERAL_BOOLEAN (" << (f_variable.get_bool(f_name) ? "true" : "false") << ")";
            break;

        case node_type_t::NODE_TYPE_LITERAL_INTEGER:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_LITERAL_INTEGER (" << f_variable.get_integer(f_name) << ")";
            break;

        case node_type_t::NODE_TYPE_LITERAL_FLOATING_POINT:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_LITERAL_FLOATING_POINT (" << f_variable.get_value().safeDoubleValue() << ")";
            break;

        case node_type_t::NODE_TYPE_LITERAL_STRING:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_LITERAL_STRING (" << f_variable.get_string(f_name) << ")";
            break;

        case node_type_t::NODE_TYPE_VARIABLE:
            SNAP_LOG_TRACE() << "execute: node_type_t::NODE_TYPE_VARIABLE";
            break;

        }
#endif

        variable_t::variable_vector_t sub_results;
        if(node_type_t::NODE_TYPE_OPERATION_CONDITIONAL != f_type)
        {
            // we don't do that for conditionals because only the left
            // or only the right shall be computed!
            int const max_children(f_children.size());
            for(int i(0); i < max_children; ++i)
            {
                variable_t cr;
                f_children[i]->execute(cr, variables, functions);
                sub_results.push_back(cr);
            }
        }

        switch(f_type)
        {
        case node_type_t::NODE_TYPE_UNKNOWN:
        case node_type_t::NODE_TYPE_LOADING:
            throw snap_logic_exception("expr_node::execute() called with an incompatible result type: node_type_t::NODE_TYPE_UNKNOWN or node_type_t::NODE_TYPE_LOADING");

        case node_type_t::NODE_TYPE_OPERATION_LIST:
            result = sub_results.last();
            break;

        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_NOT:
            logical_not(result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_BITWISE_NOT:
            bitwise_not(result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_NEGATE:
            negate(result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_FUNCTION:
            call_function(result, sub_results, functions);
            break;

        case node_type_t::NODE_TYPE_OPERATION_MULTIPLY:
            binary_operation<op_multiply>("*", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_DIVIDE:
            binary_operation<op_divide>("/", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_MODULO:
            binary_operation<op_modulo>("%", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_ADD:
            binary_operation<op_add>("+", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_SUBTRACT:
            binary_operation<op_subtract>("-", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_SHIFT_LEFT:
            binary_operation<op_shift_left>("<<", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_SHIFT_RIGHT:
            binary_operation<op_shift_right>(">>", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_LESS:
            bool_binary_operation<op_less>("<", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_LESS_OR_EQUAL:
            bool_binary_operation<op_less_or_equal>("<=", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_GREATER:
            bool_binary_operation<op_greater>(">", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_GREATER_OR_EQUAL:
            bool_binary_operation<op_greater_or_equal>(">=", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_MINIMUM:
            binary_operation<op_minimum>("<?", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_MAXIMUM:
            binary_operation<op_maximum>(">?", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_EQUAL:
            bool_binary_operation<op_equal>("==", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_NOT_EQUAL:
            bool_binary_operation<op_not_equal>("!=", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_BITWISE_AND:
            binary_operation<op_bitwise_and>("&", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_BITWISE_XOR:
            binary_operation<op_bitwise_xor>("^", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_BITWISE_OR:
            binary_operation<op_bitwise_or>("|", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_AND:
            bool_binary_operation<op_logical_and>("&&", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_XOR:
            bool_binary_operation<op_logical_xor>("^^", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_OR:
            bool_binary_operation<op_logical_or>("||", result, sub_results);
            break;

        case node_type_t::NODE_TYPE_OPERATION_CONDITIONAL:
            conditional(result, variables, functions);
            break;

        case node_type_t::NODE_TYPE_OPERATION_ASSIGNMENT:
            assignment(result, sub_results, variables);
            break;

        case node_type_t::NODE_TYPE_OPERATION_VARIABLE:
            load_variable(result, variables);
            break;

        case node_type_t::NODE_TYPE_LITERAL_BOOLEAN:
            result = f_variable;
            break;

        case node_type_t::NODE_TYPE_LITERAL_INTEGER:
            result = f_variable;
            break;

        case node_type_t::NODE_TYPE_LITERAL_FLOATING_POINT:
            result = f_variable;
            break;

        case node_type_t::NODE_TYPE_LITERAL_STRING:
            result = f_variable;
            break;

        case node_type_t::NODE_TYPE_VARIABLE:
            // a program cannot include variables as instructions
            throw snap_logic_exception(QString("expr_node::execute() called with an incompatible type: %1").arg(static_cast<int>(static_cast<node_type_t>(f_type))));

        }
    }

    void logical_not(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        verify_unary(sub_results);
        QtCassandra::QCassandraValue value;
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            value.setBoolValue(!sub_results[0].get_value().safeBoolValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            value.setBoolValue(sub_results[0].get_value().safeSignedCharValue() == 0);
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            value.setBoolValue(sub_results[0].get_value().safeUnsignedCharValue() == 0);
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            value.setBoolValue(sub_results[0].get_value().safeInt16Value() == 0);
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            value.setBoolValue(sub_results[0].get_value().safeUInt16Value() == 0);
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            value.setBoolValue(sub_results[0].get_value().safeInt32Value() == 0);
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            value.setBoolValue(sub_results[0].get_value().safeUInt32Value() == 0);
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            value.setBoolValue(sub_results[0].get_value().safeInt64Value() == 0);
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            value.setBoolValue(sub_results[0].get_value().safeUInt64Value() == 0);
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            value.setBoolValue(sub_results[0].get_value().safeFloatValue() == 0.0f);
#pragma GCC diagnostic pop
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            value.setBoolValue(sub_results[0].get_value().safeDoubleValue() == 0.0);
#pragma GCC diagnostic pop
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            value.setBoolValue(sub_results[0].get_value().stringValue().length() == 0);
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            value.setBoolValue(sub_results[0].get_value().binaryValue().size() == 0);
            break;

        default:
            throw snap_logic_exception(QString("expr_node::logical_not() called with an incompatible sub_result type: %1").arg(static_cast<int>(sub_results[0].get_type())));

        }
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

    void bitwise_not(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        verify_unary(sub_results);
        QtCassandra::QCassandraValue value;
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            // this is balently "wrong" as far as the source is concerned
            // and maybe we should throw instead of accepting such bad code?
            // (i.e. g++ and Coverity do not like it without the casts so
            // maybe we ought to throw on such and have the programmer fix
            // their source) CID 30192
            //
            value.setBoolValue(static_cast<bool>(~static_cast<int>(sub_results[0].get_value().safeBoolValue())));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            value.setSignedCharValue(static_cast<signed char>(~sub_results[0].get_value().safeSignedCharValue()));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            value.setUnsignedCharValue(static_cast<unsigned char>(~sub_results[0].get_value().safeUnsignedCharValue()));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            value.setInt16Value(static_cast<int16_t>(~sub_results[0].get_value().safeInt16Value()));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            value.setUInt16Value(static_cast<uint16_t>(~sub_results[0].get_value().safeUInt16Value()));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            value.setInt32Value(~sub_results[0].get_value().safeInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            value.setUInt32Value(~sub_results[0].get_value().safeUInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            value.setInt64Value(~sub_results[0].get_value().safeInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            value.setUInt64Value(~sub_results[0].get_value().safeUInt64Value());
            break;

        //case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            // TODO:
            // swap case (A -> a and A -> a)
        default:
            throw snap_logic_exception(QString("expr_node::bitwise_not() called with an incompatible sub_result type: %1").arg(static_cast<int>(sub_results[0].get_type())));

        }
        result.set_value(sub_results[0].get_type(), value);
    }

    void negate(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        verify_unary(sub_results);
        QtCassandra::QCassandraValue value;
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            value.setSignedCharValue(static_cast<signed char>(-sub_results[0].get_value().safeSignedCharValue()));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            value.setUnsignedCharValue(static_cast<unsigned char>(-sub_results[0].get_value().safeUnsignedCharValue()));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            value.setInt16Value(static_cast<int16_t>(-sub_results[0].get_value().safeInt16Value()));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            value.setUInt16Value(static_cast<uint16_t>(-sub_results[0].get_value().safeUInt16Value()));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            value.setInt32Value(-sub_results[0].get_value().safeInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            value.setUInt32Value(-sub_results[0].get_value().safeUInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            value.setInt64Value(-sub_results[0].get_value().safeInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            value.setUInt64Value(-sub_results[0].get_value().safeUInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            value.setFloatValue(-sub_results[0].get_value().safeFloatValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            value.setDoubleValue(-sub_results[0].get_value().safeDoubleValue());
            break;

        default:
            throw snap_logic_exception(QString("expr_node::negate() called with an incompatible sub_result type: %1").arg(static_cast<int>(sub_results[0].get_type())));

        }
        result.set_value(sub_results[0].get_type(), value);
    }

    static void call_cell(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(!g_context)
        {
            throw snap_expr_exception_not_ready("cell() function not available, g_context is NULL.");
        }
        if(sub_results.size() != 3)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call cell() expected exactly 3.");
        }
        QString const table_name(sub_results[0].get_string("cell(1)"));
        QString const row_name(sub_results[1].get_string("cell(2)"));
        QString const cell_name(sub_results[2].get_string("cell(3)"));
//SNAP_LOG_WARNING("cell(")(table_name)(", ")(row_name)(", ")(cell_name)(") -> ...");

        // verify whether reading the content is considered secure
        server::accessible_flag_t accessible;
        server::instance()->table_is_accessible(table_name, accessible);
        if(!accessible.is_accessible())
        {
            // TBD: should we just return a string with an error in it
            //      instead of throwing?
            //
            throw snap_expr_exception_not_accessible(
                    QString("cell() called with a set of parameters specifying access to a secure table (table \"%1\", row \"%2\", cell \"%3\"); no data will be returned.")
                            .arg(table_name).arg(row_name).arg(cell_name));
        }

        QtCassandra::QCassandraValue value(g_context->table(table_name)->row(row_name)->cell(cell_name)->value());
//std::cerr << "  -> value size: " << value.size() << "\n";
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY, value);
    }

    static void call_cell_exists(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(!g_context)
        {
            throw snap_expr_exception_not_ready("cell_exists() function not available, g_context is NULL");
        }
        if(sub_results.size() != 3)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call cell_exists(), expected exactly 3");
        }
        QString const table_name(sub_results[0].get_string("cell_exist(1)"));
        QString const row_name(sub_results[1].get_string("cell_exist(2)"));
        QString const cell_name(sub_results[2].get_string("cell_exist(3)"));
        QtCassandra::QCassandraValue value;
        value.setBoolValue(g_context->table(table_name)->row(row_name)->exists(cell_name));
//SNAP_LOG_WARNING("cell_exists(")(table_name)(", ")(row_name)(", ")(cell_name)(") -> ")(value.boolValue() ? "true" : "false");
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

    static void call_child(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(sub_results.size() != 2)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call child() expected exactly 2");
        }
        QString path(sub_results[0].get_string("child(1)"));
        QString child(sub_results[1].get_string("child(2)"));
        while(path.endsWith("/"))
        {
            path = path.left(path.length() - 1);
        }
        while(child.startsWith("/"))
        {
            child = child.right(child.length() - 1);
        }
        if(!child.isEmpty()
        && !path.isEmpty())
        {
            path = path + "/" + child;
        }
        QtCassandra::QCassandraValue value;
        value.setStringValue(path);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    // This cannot work right in an international website
    // The dates should be saved as int64_t values in the database instead
    //static void call_date_to_us(variable_t & result, variable_t::variable_vector_t const & sub_results)
    //{
    //    if(!g_context)
    //    {
    //        throw snap_expr_exception_not_ready("date_to_us() function not available, g_context is NULL");
    //    }
    //    if(sub_results.size() != 1)
    //    {
    //        throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call date_to_us(), expected exactly 1");
    //    }
    //    QString const date_str(sub_results[0].get_string("date_to_us(1)"));

    //    // we only support MM/DD/YYYY
    //    struct tm time_info;
    //    memset(&time_info, 0, sizeof(time_info));
    //    time_info.tm_mon = date_str.mid(0, 2).toInt() - 1;
    //    time_info.tm_mday = date_str.mid(3, 2).toInt();
    //    time_info.tm_year = date_str.mid(6, 4).toInt() - 1900;
    //    time_t t(mkgmtime(&time_info));

    //    QtCassandra::QCassandraValue value;
    //    value.setInt64Value(t * 1000000);
    //    result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    //}

    static void call_format(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(sub_results.size() < 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call format() expected at least 1, the format");
        }

        struct input
        {
            int const INPUT_LEFT_ALIGN = 0x0001;
            int const INPUT_ZERO_PAD   = 0x0002;
            int const INPUT_BLANK      = 0x0004;
            int const INPUT_SIGN       = 0x0008;
            int const INPUT_THOUSANDS  = 0x0010;

            input(variable_t::variable_vector_t const& sub_results)
                : f_sub_results(sub_results)
            {
                f_format = sub_results[0].get_string("format() function format string");
            }

            int getc()
            {
                if(f_position >= f_format.length())
                {
                    f_last = EOF;
                }
                else
                {
                    f_last = f_format[f_position].unicode();
                    ++f_position;
                }
                return f_last;
            }

            int get_flags()
            {
                int flags(0);
                for(int old_flags(-1); flags != old_flags; )
                {
                    old_flags = flags;
                    if(f_last == '\'')
                    {
                        flags |= INPUT_THOUSANDS;
                        getc();
                    }
                    if(f_last == ' ')
                    {
                        flags |= INPUT_BLANK;
                        getc();
                    }
                    if(f_last == '+')
                    {
                        flags |= INPUT_SIGN;
                        getc();
                    }
                    if(f_last == '-')
                    {
                        flags |= INPUT_LEFT_ALIGN;
                        getc();
                    }
                    if(f_last == '0')
                    {
                        flags |= INPUT_ZERO_PAD;
                        getc();
                    }
                }

                // mutually exclusive flags
                if(flags & INPUT_LEFT_ALIGN)
                {
                    flags &= ~INPUT_ZERO_PAD;
                }
                if(flags & INPUT_SIGN)
                {
                    flags &= ~INPUT_BLANK;
                }

                return flags;
            }

            int get_number()
            {
                int number(0);
                for(; f_last >= '0' && f_last <= '9'; getc())
                {
                    number = number * 10 + f_last - '0';
                }
                return number;
            }

            variable_t const & get_next_variable()
            {
                if(f_index >= f_sub_results.size())
                {
                    throw snap_expr_exception_invalid_data("invalid number of parameters to call format(), your format requires more parameters than is currently allowed");
                }
                variable_t const & r(f_sub_results[f_index]);
                ++f_index;
                return r;
            }

            QString get_integer()
            {
                int64_t v(get_next_variable().get_integer("format.get_integer()"));
                QString r(QString("%1").arg(v));
                if(f_flags & INPUT_THOUSANDS)
                {
                    int stop(0);
                    if(r[0] == '-')
                    {
                        stop = 1;
                    }
                    for(int p(r.length() - 3); p > stop; p -= 3)
                    {
                        // TODO: need to be able to specify the separator
                        //       (',', or '.', or ' ', or '_', ...)
                        //
                        r.insert(p, ',');
                    }
                }
                if((f_flags & INPUT_SIGN) && v >= 0)
                {
                    r = "+" + r;
                }
                else if((f_flags & INPUT_BLANK) && v >= 0)
                {
                    r = " " + r;
                }
                return r;
            }

            QString get_floating_point()
            {
                // TODO: floating points need to be properly formatted here
                //       (i.e. width and precision...)
                double v(get_next_variable().get_floating_point("format.get_floating_point()"));
                QString r(QString("%1").arg(v));
                if(f_flags & INPUT_THOUSANDS)
                {
                    int stop(0);
                    if(r[0] == '-')
                    {
                        stop = 1;
                    }
                    int p(r.indexOf('.'));
                    if(p == -1)
                    {
                        p = r.length() - 3;
                    }
                    for(; p > stop; p -= 3)
                    {
                        // TODO: need to be able to specify the separator
                        //       (',', or '.', or ' ', or '_', ...)
                        //
                        r.insert(p, ',');
                    }
                    // TBD: add thousand markers in the decimal part?
                }
                if((f_flags & INPUT_SIGN) && v >= 0)
                {
                    r = "+" + r;
                }
                else if((f_flags & INPUT_BLANK) && v >= 0)
                {
                    r = " " + r;
                }
                return r;
            }

            QString get_character()
            {
                // TODO: verify that the integer is a valid code point
                int64_t code(get_next_variable().get_integer("format.get_character()"));
                if(code < 0 || code > 0x110000
                || (code >= 0xD800 && code <= 0xDFFF))
                {
                    throw snap_expr_exception_invalid_data("invalid character code in format(), only valid Unicode characters are allowed");
                }
                if(QChar::requiresSurrogates(code))
                {
                    // this uses the old Unicode terminology
                    QChar high(QChar::highSurrogate(code));
                    QChar low(QChar::lowSurrogate(code));
                    return QString(high) + QString(low);
                }
                else
                {
                    // small characters can be used as is
                    QChar c(static_cast<uint>(code));
                    return QString(c);
                }
            }

            QString get_string()
            {
                QString r(QString("%1").arg(get_next_variable().get_string("format.get_string()")));
                if(r.isEmpty() && (f_flags & INPUT_BLANK))
                {
                    // blank against empty strings...
                    r = " ";
                }
                if(f_precision > 0)
                {
                    // truncate
                    // TODO: truncating a QString is wrong if it includes
                    //       UTF-16 surrogate characters!
                    r = r.left(f_precision);
                }
                f_flags &= ~INPUT_ZERO_PAD;
                return r;
            }

            QString format(QString value)
            {
                int align(f_width - value.length());
                if(align > 0)
                {
                    // need padding
                    if(f_flags & INPUT_LEFT_ALIGN)
                    {
                        // add padding to the right side
                        value += QString(" ").repeated(align);
                    }
                    else if(f_flags & INPUT_ZERO_PAD)
                    {
                        QChar c(value[0]);
                        QString const pad("0");
                        switch(c.unicode())
                        {
                        case '+':
                        case '-':
                            value = c + pad.repeated(align) + value.mid(1);
                            break;

                        case ' ':
                            if(f_flags & INPUT_BLANK)
                            {
                                // special case if we had the blank flag
                                value = c + pad.repeated(align) + value.mid(1);
                                break;
                            }
                            /*FALLTHROUGH*/
                        default:
                            value = pad.repeated(align) + value;
                            break;

                        }
                    }
                    else
                    {
                        value = QString(" ").repeated(align) + value;
                    }
                }
                return value;
            }

            void parse()
            {
                while(getc() != EOF)
                {
                    if(f_last == '%')
                    {
                        // skip the '%'
                        if(getc() == EOF)
                        {
                            return;
                        }
                        if(f_last == '%')
                        {
                            // literal percent
                            f_result += '%';
                        }
                        else
                        {
                            f_flags = get_flags();
                            if(f_last >= '1' && f_last <= '9')
                            {
                                f_width = get_number();
                            }
                            else
                            {
                                f_width = 0;
                            }
                            if(f_last == '.')
                            {
                                getc();
                                f_precision = get_number();
                            }
                            else
                            {
                                // default precision is 1, but we need to
                                // have -1 to distinguish in the case of
                                // a string
                                f_precision = -1;
                            }
                            switch(f_last)
                            {
                            case 'd': // integer formatting
                            case 'i':
                                f_result += format(get_integer());
                                break;

                            case 'f': // floating point formatting
                            case 'g':
                                f_result += format(get_floating_point());
                                break;

                            case 'c': // character formatting
                            case 'C':
                                f_result += format(get_character());
                                break;

                            case 's': // string formatting
                            case 'S':
                                f_result += format(get_string());
                                break;

                            }
                        }
                    }
                    else
                    {
                        f_result += f_last;
                    }
                }
            }

            variable_t::variable_vector_t const &   f_sub_results;
            QString                                 f_format;
            int32_t                                 f_position = 0;
            int32_t                                 f_index = 1;
            QString                                 f_result;
            int                                     f_last = EOF;
            int                                     f_flags = 0;
            int32_t                                 f_width = 0;
            int32_t                                 f_precision = 0;
        };
        input in(sub_results);
        in.parse();
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, in.f_result);
    }

    static void call_int16(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call int16() expected exactly 1");
        }
        int16_t r(0);
        QtCassandra::QCassandraValue const& v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
            //r = 0; -- an "empty" number...
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            r = v.safeBoolValue() ? 1 : 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            r = v.safeSignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            r = v.safeUnsignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            r = v.safeInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            r = v.safeUInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            r = static_cast<int16_t>(v.safeInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            r = static_cast<int16_t>(v.safeUInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            r = static_cast<int16_t>(v.safeInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            r = static_cast<int16_t>(v.safeUInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            r = static_cast<int16_t>(v.safeFloatValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            r = static_cast<int16_t>(v.safeDoubleValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            r = static_cast<int16_t>(v.stringValue().toLongLong());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            r = v.safeInt16Value();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setInt16Value(r);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16, value);
    }

    static void call_int32(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call int32() expected exactly 1");
        }
        int32_t r(0);
        QtCassandra::QCassandraValue const& v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
            //r = 0; -- an "empty" number...
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            r = v.safeBoolValue() ? 1 : 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            r = v.safeSignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            r = v.safeUnsignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            r = v.safeInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            r = v.safeUInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            r = v.safeInt32Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            r = v.safeUInt32Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            r = static_cast<int32_t>(v.safeInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            r = static_cast<int32_t>(v.safeUInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            r = static_cast<int32_t>(v.safeFloatValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            r = static_cast<int32_t>(v.safeDoubleValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            r = static_cast<int32_t>(v.stringValue().toLongLong());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            r = v.safeInt32Value();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setInt32Value(r);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32, value);
    }

    static void call_int64(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call int64() expected exactly 1");
        }
        int64_t r(0);
        QtCassandra::QCassandraValue const& v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
            //r = 0; -- an "empty" number...
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            r = v.safeBoolValue() ? 1 : 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            r = v.safeSignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            r = v.safeUnsignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            r = v.safeInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            r = v.safeUInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            r = v.safeInt32Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            r = v.safeUInt32Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            r = v.safeInt64Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            r = v.safeUInt64Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            r = static_cast<int64_t>(v.safeFloatValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            r = static_cast<int64_t>(v.safeDoubleValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            r = v.stringValue().toLongLong();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            r = v.safeInt64Value();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setInt64Value(r);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64, value);
    }

    static void call_int8(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call int8() expected exactly 1");
        }
        int8_t r(0);
        QtCassandra::QCassandraValue const & v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
            //r = 0; -- an "empty" number...
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            r = v.safeBoolValue() ? 1 : 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            r = v.safeSignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            r = static_cast<int8_t>(v.safeUnsignedCharValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            r = static_cast<int8_t>(v.safeInt16Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            r = static_cast<int8_t>(v.safeUInt16Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            r = static_cast<int8_t>(v.safeInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            r = static_cast<int8_t>(v.safeUInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            r = static_cast<int8_t>(v.safeInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            r = static_cast<int8_t>(v.safeUInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            r = static_cast<int8_t>(v.safeFloatValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            r = static_cast<int8_t>(v.safeDoubleValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            r = static_cast<int8_t>(v.stringValue().toLongLong());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            r = v.safeSignedCharValue();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setSignedCharValue(r);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8, value);
    }

    static void call_is_integer(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call is_integer() expected exactly 1");
        }
        bool r(false);
        QtCassandra::QCassandraValue const & v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            r = true;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            // make sure it is a valid integer
            v.stringValue().toLongLong(&r, 10);
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setBoolValue(r);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

    static void call_parent(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call parent() expected exactly 1");
        }
        QString path(sub_results[0].get_string("parent(1)"));
        if(path.endsWith("/"))
        {
            path = path.left(path.length() - 1);
        }
        int pos(path.lastIndexOf('/'));
        if(pos == -1)
        {
            path.clear();
        }
        else
        {
            path = path.mid(0, pos);
        }
        QtCassandra::QCassandraValue value;
        value.setStringValue(path);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    static void call_preg_replace(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        int const size(sub_results.size());
        if(size != 3)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call preg_replace() expected exactly 3");
        }
        QString pattern(sub_results[0].get_string("preg_replace(1)"));
        QString const replacement(sub_results[1].get_string("preg_replace(2)"));
        QString str(sub_results[2].get_string("preg_replace(3)"));
        QString flags;

        // pattern may include a set of flags after the ending '/'
        Qt::CaseSensitivity cs(Qt::CaseSensitive);
        if(pattern.length() >= 2
        && pattern[0] == '/')
        {
            int const end(pattern.lastIndexOf('/'));
            if(end <= 0)
            {
                throw snap_expr_exception_invalid_number_of_parameters("invalid pattern for preg_replace() if it starts with a '/' it must end with another '/'");
            }
            flags = pattern.mid(end + 1);
            pattern = pattern.mid(1, end - 2);
        }
        if(flags.contains("i"))
        {
            cs = Qt::CaseInsensitive;
        }
        QRegExp re(pattern, cs, QRegExp::RegExp2);
        str.replace(re, replacement); // this does the replacement in place
        QtCassandra::QCassandraValue value;
        value.setStringValue(str);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    static void call_row_exists(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(!g_context)
        {
            throw snap_expr_exception_not_ready("row_exists() function not available, g_context is NULL");
        }
        if(sub_results.size() != 2)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call row_exists() expected exactly 2");
        }
        QString const table_name(sub_results[0].get_string("row_exists(1)"));
        QString const row_name(sub_results[1].get_string("row_exists(2)"));
        QtCassandra::QCassandraValue value;
        value.setBoolValue(g_context->table(table_name)->exists(row_name));
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

    static void call_segment(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(sub_results.size() != 3)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call segment() expected exactly 3");
        }
        QString const str(sub_results[0].get_string("segment(1)"));
        QString const sep(sub_results[1].get_string("segment(2)"));
        int64_t const idx(sub_results[2].get_integer("segment(3)"));
        snap_string_list list(str.split(sep));
        if(idx >= 0 && idx < list.size())
        {
            result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, list[idx]);
        }
        else
        {
            result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, QString(""));
        }
    }

    static void call_string(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call string() expected exactly 1");
        }
        QString str;
        QtCassandra::QCassandraValue const & v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
            //str = ""; -- an empty string is the default
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            str = v.safeBoolValue() ? "true" : "false";
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            str = QString("%1").arg(static_cast<int>(v.safeSignedCharValue()));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            str = QString("%1").arg(static_cast<int>(v.safeUnsignedCharValue()));
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            str = QString("%1").arg(v.safeInt16Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            str = QString("%1").arg(v.safeUInt16Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            str = QString("%1").arg(v.safeInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            str = QString("%1").arg(v.safeUInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            str = QString("%1").arg(v.safeInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            str = QString("%1").arg(v.safeUInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            str = QString("%1").arg(v.safeFloatValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            str = QString("%1").arg(v.safeDoubleValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            str = v.stringValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            str = v.stringValue();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setStringValue(str);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    static void call_strlen(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call strlen() expected exactly 1");
        }
        QString const str(sub_results[0].get_string("strlen(1)"));
        QtCassandra::QCassandraValue value;
        value.setInt64Value(str.length());
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64, value);
    }

    static void call_strmatch(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        int const size(sub_results.size());
        if(size < 2 || size > 3)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call strmatch() expected 2 or 3");
        }
        QString const pattern(sub_results[0].get_string("submatch(1)"));
        QString const str(sub_results[1].get_string("submatch(2)"));
        QString flags;
        if(size == 3)
        {
            flags = sub_results[2].get_string("submatch(3)");
        }
        Qt::CaseSensitivity cs(Qt::CaseSensitive);
        if(flags.contains("i"))
        {
            cs = Qt::CaseInsensitive;
        }
        QRegExp re(pattern, cs, QRegExp::RegExp2);
        QtCassandra::QCassandraValue value;
//SNAP_LOG_WARNING("exact match pattern: \"")(pattern)("\", str \"")(str)("\".");
        value.setBoolValue(re.exactMatch(str));
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

    static void call_substr(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        int const size(sub_results.size());
        if(size < 2 || size > 3)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call substr() expected 2 or 3");
        }
        QString const str(sub_results[0].get_string("substr(1)"));
        int const start(static_cast<int>(sub_results[1].get_integer("substr(2)")));
        QtCassandra::QCassandraValue value;
        if(size == 3)
        {
            int const end(static_cast<int>(sub_results[2].get_integer("substr(3)")));
            value.setStringValue(str.mid(start, end));
        }
        else
        {
            value.setStringValue(str.mid(start));
        }
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    static void call_table_exists(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(!g_context)
        {
            throw snap_expr_exception_not_ready("table_exists() function not available, g_context is NULL");
        }
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call table_exists() expected exactly 1");
        }
        QString const table_name(sub_results[0].get_string("table_exists(1)"));
        QtCassandra::QCassandraValue value;
        value.setBoolValue(g_context->findTable(table_name) != nullptr);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

    static void call_tolower(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call tolower() expected exactly 1");
        }
        QString const str(sub_results[0].get_string("tolower(1)"));
        QtCassandra::QCassandraValue value;
        value.setStringValue(str.toLower());
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    static void call_toupper(variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call toupper() expected exactly 1");
        }
        QString const str(sub_results[0].get_string("toupper(1)"));
        QtCassandra::QCassandraValue value;
        value.setStringValue(str.toUpper());
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    static void call_uint16(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call uint16() expected exactly 1");
        }
        uint16_t r(0);
        QtCassandra::QCassandraValue const & v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
            //r = 0; -- an "empty" number...
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            r = v.safeBoolValue() ? 1 : 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            r = v.safeSignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            r = v.safeUnsignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            r = v.safeInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            r = v.safeUInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            r = static_cast<uint16_t>(v.safeInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            r = static_cast<uint16_t>(v.safeUInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            r = static_cast<uint16_t>(v.safeInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            r = static_cast<uint16_t>(v.safeUInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            r = static_cast<uint16_t>(v.safeFloatValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            r = static_cast<uint16_t>(v.safeDoubleValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            r = static_cast<uint16_t>(v.stringValue().toLongLong());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            r = v.safeUInt16Value();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setUInt16Value(r);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16, value);
    }

    static void call_uint32(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call uint32() expected exactly 1");
        }
        uint32_t r(0);
        QtCassandra::QCassandraValue const & v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
            //r = 0; -- an "empty" number...
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            r = v.safeBoolValue() ? 1 : 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            r = v.safeSignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            r = v.safeUnsignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            r = v.safeInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            r = v.safeUInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            r = v.safeInt32Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            r = v.safeUInt32Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            r = static_cast<uint32_t>(v.safeInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            r = static_cast<uint32_t>(v.safeUInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            r = static_cast<uint32_t>(v.safeFloatValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            r = static_cast<uint32_t>(v.safeDoubleValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            r = static_cast<uint32_t>(v.stringValue().toLongLong());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            r = v.safeUInt32Value();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setUInt32Value(r);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32, value);
    }

    static void call_uint64(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call uint64() expected exactly 1");
        }
        uint64_t r(0);
        QtCassandra::QCassandraValue const & v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
            //r = 0; -- an "empty" number...
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            r = v.safeBoolValue() ? 1 : 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            r = v.safeSignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            r = v.safeUnsignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            r = v.safeInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            r = v.safeUInt16Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            r = v.safeInt32Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            r = v.safeUInt32Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            r = v.safeInt64Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            r = v.safeUInt64Value();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            r = static_cast<int64_t>(v.safeFloatValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            r = static_cast<int64_t>(v.safeDoubleValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            r = v.stringValue().toLongLong();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            r = v.safeInt64Value();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setUInt64Value(r);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64, value);
    }

    static void call_uint8(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call uint8() expected exactly 1");
        }
        uint8_t r(0);
        QtCassandra::QCassandraValue const & v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
            //r = 0; -- an "empty" number...
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            r = v.safeBoolValue() ? 1 : 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            r = v.safeSignedCharValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            r = static_cast<uint8_t>(v.safeUnsignedCharValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            r = static_cast<uint8_t>(v.safeInt16Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            r = static_cast<uint8_t>(v.safeUInt16Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            r = static_cast<uint8_t>(v.safeInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            r = static_cast<uint8_t>(v.safeUInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            r = static_cast<uint8_t>(v.safeInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            r = static_cast<uint8_t>(v.safeUInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            r = static_cast<uint8_t>(v.safeFloatValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            r = static_cast<uint8_t>(v.safeDoubleValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            r = static_cast<uint8_t>(v.stringValue().toLongLong());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            r = v.safeUnsignedCharValue();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setUnsignedCharValue(r);
        result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8, value);
    }

    void call_function(variable_t & result, variable_t::variable_vector_t const & sub_results, functions_t & functions)
    {
        functions_t::function_call_t f(functions.get_function(f_name));
        if(f == nullptr)
        {
            // if we already initialized the internal functions,
            // there is nothing more to do here
            if(!functions.get_has_internal_functions())
            {
                // avoid recursivity (you never know!)
                functions.set_has_internal_functions();
                functions.add_functions(internal_functions);
                // also give a chance to plugins and other listeners to
                // register functions
                server::instance()->add_snap_expr_functions(functions);
                f = functions.get_function(f_name);
            }
            if(f == nullptr)
            {
                throw snap_expr_exception_unknown_function(QString("unknown function \"%1\" in list execution environment").arg(f_name));
            }
        }
        f(result, sub_results);
    }

    bool get_variable_value(variable_t const & var, int64_t & integer, double & floating_point, QString & string, bool & signed_integer, bool & is_floating_point, bool & is_string_type)
    {
        switch(var.get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            integer = static_cast<int64_t>(var.get_value().safeBoolValue() != 0);
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            integer = static_cast<int64_t>(var.get_value().safeSignedCharValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            signed_integer = false;
            integer = static_cast<int64_t>(var.get_value().safeUnsignedCharValue());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            integer = static_cast<int64_t>(var.get_value().safeInt16Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            signed_integer = false;
            integer = static_cast<int64_t>(var.get_value().safeUInt16Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            integer = static_cast<int64_t>(var.get_value().safeInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            signed_integer = false;
            integer = static_cast<int64_t>(var.get_value().safeUInt32Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            integer = static_cast<int64_t>(var.get_value().safeInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            signed_integer = false;
            integer = static_cast<int64_t>(var.get_value().safeUInt64Value());
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            is_floating_point = true;
            floating_point = var.get_value().safeDoubleValue();
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            is_string_type = true;
            string = var.get_value().stringValue();
            break;

        default:
            return false;

        }

        return true;
    }

    template<typename F>
    void binary_operation(char const * op, variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        verify_binary(sub_results);

        variable_t::variable_type_t type(std::max(sub_results[0].get_type(), sub_results[1].get_type()));

        bool lstring_type(false), rstring_type(false);
        bool lfloating_point(false), rfloating_point(false);
        bool lsigned_value(true), rsigned_value(true);
        QString ls, rs;
        double lf(0.0), rf(0.0);
        int64_t li(0), ri(0);

        bool valid(get_variable_value(sub_results[0], li, lf, ls, lsigned_value, lfloating_point, lstring_type));
        valid = valid && get_variable_value(sub_results[1], ri, rf, rs, rsigned_value, rfloating_point, rstring_type);

        if(valid)
        {
            QtCassandra::QCassandraValue value;
            switch(type)
            {
            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
                value.setSignedCharValue(static_cast<signed char>(F::integers(li, ri)));
                result.set_value(type, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
                value.setUnsignedCharValue(static_cast<unsigned char>(F::integers(li, ri)));
                result.set_value(type, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
                value.setInt16Value(static_cast<int16_t>(F::integers(li, ri)));
                result.set_value(type, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
                value.setUInt16Value(static_cast<uint16_t>(F::integers(li, ri)));
                result.set_value(type, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
                value.setInt32Value(static_cast<int32_t>(F::integers(li, ri)));
                result.set_value(type, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
                value.setUInt32Value(static_cast<uint32_t>(F::integers(li, ri)));
                result.set_value(type, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
                value.setInt64Value(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
                value.setUInt64Value(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
                if(has_function<F>::has_floating_points)
                {
                    if(!lfloating_point)
                    {
                        lf = static_cast<double>(lsigned_value ? li : static_cast<uint64_t>(li));
                    }
                    if(!rfloating_point)
                    {
                        rf = static_cast<double>(rsigned_value ? ri : static_cast<uint64_t>(ri));
                    }
                    do_float<F>(result, lf, rf);
                    return;
                }
                break;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
                if(has_function<F>::has_floating_points)
                {
                    if(!lfloating_point)
                    {
                        lf = static_cast<double>(lsigned_value ? li : static_cast<uint64_t>(li));
                    }
                    if(!rfloating_point)
                    {
                        rf = static_cast<double>(rsigned_value ? ri : static_cast<uint64_t>(ri));
                    }
                    do_double<F>(result, lf, rf);
                    return;
                }
                break;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
                if(has_function<F>::has_strings)
                {
                    if(!lstring_type)
                    {
                        ls = QString("%1").arg(lfloating_point ? lf : static_cast<double>(lsigned_value ? li : static_cast<uint64_t>(li)));
                    }
                    if(!rstring_type)
                    {
                        rs = QString("%1").arg(rfloating_point ? rf : static_cast<double>(rsigned_value ? ri : static_cast<uint64_t>(ri)));
                    }
                    // do "string" + "string"
                    do_strings<F>(result, ls, rs);
                    return;
                }
                else if(has_function<F>::has_string_integer
                     && variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING == sub_results[0].get_type()
                     && !rfloating_point && !rstring_type)
                {
                    // very special case to support: "string" * 123
                    do_string_integer<F>(result, ls, ri);
                    return;
                }
                break;

            default:
                // anything else is invalid at this point
                break;

            }
        }

        throw snap_logic_exception(QString("expr_node::binary_operation(\"%3\") called with incompatible sub_result types: %1 x %2")
                            .arg(static_cast<int>(sub_results[0].get_type()))
                            .arg(static_cast<int>(sub_results[1].get_type()))
                            .arg(op));
    }

    template<typename F>
    void bool_binary_operation(char const * op, variable_t & result, variable_t::variable_vector_t const & sub_results)
    {
        verify_binary(sub_results);

        variable_t::variable_type_t type(std::max(sub_results[0].get_type(), sub_results[1].get_type()));

        bool lstring_type(false), rstring_type(false);
        bool lfloating_point(false), rfloating_point(false);
        bool lsigned_value(true), rsigned_value(true);
        QString ls, rs;
        double lf(0.0), rf(0.0);
        int64_t li(0), ri(0);

        bool valid(get_variable_value(sub_results[0], li, lf, ls, lsigned_value, lfloating_point, lstring_type));
        valid = valid && get_variable_value(sub_results[1], ri, rf, rs, rsigned_value, rfloating_point, rstring_type);

        if(valid)
        {
            QtCassandra::QCassandraValue value;
            switch(type)
            {
            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
                return;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
                if(has_function<F>::has_bool_floating_points)
                {
                    if(!lfloating_point)
                    {
                        lf = static_cast<double>(lsigned_value ? li : static_cast<uint64_t>(li));
                    }
                    if(!rfloating_point)
                    {
                        rf = static_cast<double>(rsigned_value ? ri : static_cast<uint64_t>(ri));
                    }
                    do_float_to_bool<F>(result, lf, rf);
                    return;
                }
                break;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
                if(has_function<F>::has_bool_floating_points)
                {
                    if(!lfloating_point)
                    {
                        lf = static_cast<double>(lsigned_value ? li : static_cast<uint64_t>(li));
                    }
                    if(!rfloating_point)
                    {
                        rf = static_cast<double>(rsigned_value ? ri : static_cast<uint64_t>(ri));
                    }
                    do_double_to_bool<F>(result, lf, rf);
                    return;
                }
                break;

            case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
                if(has_function<F>::has_bool_strings)
                {
                    if(!lstring_type)
                    {
                        ls = QString("%1").arg(lfloating_point ? lf : static_cast<double>(lsigned_value ? li : static_cast<uint64_t>(li)));
                    }
                    if(!rstring_type)
                    {
                        rs = QString("%1").arg(rfloating_point ? rf : static_cast<double>(rsigned_value ? ri : static_cast<uint64_t>(ri)));
                    }
                    // do "string" + "string"
                    do_strings_to_bool<F>(result, ls, rs);
                    return;
                }
                else if(has_function<F>::has_bool_string_integer
                     && variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING == sub_results[0].get_type()
                     && !rfloating_point && !rstring_type)
                {
                    // very special case to support: "string" * 123
                    do_string_integer_to_bool<F>(result, ls, ri);
                    return;
                }
                break;

            default:
                // anything else is invalid at this point
                break;

            }
        }

        throw snap_logic_exception(QString("expr_node::binary_operation(\"%3\") called with incompatible sub_result types: %1 x %2")
                            .arg(static_cast<int>(sub_results[0].get_type()))
                            .arg(static_cast<int>(sub_results[1].get_type()))
                            .arg(op));
    }

    void conditional(variable_t& result, variable_t::variable_map_t& variables, functions_t& functions)
    {
        // IMPORTANT NOTE
        //   we compute the results directly in conditional() to
        //   avoid calculating f_children[1] and f_children[2]
#ifdef DEBUG
        if(f_children.size() != 3)
        {
            throw snap_logic_exception("expr_node::conditional() found a conditional operator with a number of results which is not 3");
        }
#endif
        // Note:
        // we put the result of f_children[0] in 'result' even though this
        // is not the result of the function, but it shouldn't matter, we'll
        // either throw or compute f_children[1] or f_children[2] as required
        f_children[0]->execute(result, variables, functions);
        bool r(false);
        switch(result.get_type())
        {
        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            // using the following to force the value ot 0 or 1
            r = result.get_value().safeSignedCharValue() != 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            r = result.get_value().safeSignedCharValue() != 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            r = result.get_value().safeUnsignedCharValue() != 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            r = result.get_value().safeInt16Value() != 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            r = result.get_value().safeUInt16Value() != 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            r = result.get_value().safeInt32Value() != 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            r = result.get_value().safeUInt32Value() != 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            r = result.get_value().safeInt64Value() != 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            r = result.get_value().safeUInt64Value() != 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            r = result.get_value().safeFloatValue() != 0.0f;
#pragma GCC diagnostic pop
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            r = result.get_value().safeDoubleValue() != 0.0;
#pragma GCC diagnostic pop
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            r = result.get_value().stringValue().length() != 0;
            break;

        case variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            r = result.get_value().binaryValue().size() != 0;
            break;

        default:
            throw snap_logic_exception(QString("expr_node::logical_not() called with an incompatible sub_result type: %1")
                        .arg(static_cast<int>(result.get_type())));

        }
        if(r)
        {
            f_children[1]->execute(result, variables, functions);
        }
        else
        {
            f_children[2]->execute(result, variables, functions);
        }
    }

    void assignment(variable_t & result, variable_t::variable_vector_t const & sub_results, variable_t::variable_map_t & variables)
    {
#ifdef DEBUG
        if(sub_results.size() != 1)
        {
            throw snap_logic_exception("expr_node::execute() found an assignment operator with a number of results which is not 1");
        }
#endif
        variables[f_name] = sub_results[0];
        // result is a copy of the variable contents
        result = variables[f_name];
    }

    void load_variable(variable_t & result, variable_t::variable_map_t & variables)
    {
        if(variables.contains(f_name))
        {
            result = variables[f_name];
        }
    }


    QString toString()
    {
        return internal_toString("");
    }


private:
    QString internal_toString(QString indent)
    {
        // start with the type
        QString result(QString("%1%2").arg(indent).arg(type_names[static_cast<int>(static_cast<node_type_t>(f_type))]));

        // add type specific data
        switch(f_type)
        {
        case node_type_t::NODE_TYPE_UNKNOWN:
        case node_type_t::NODE_TYPE_LOADING:
        case node_type_t::NODE_TYPE_OPERATION_LIST:
        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_NOT:
        case node_type_t::NODE_TYPE_OPERATION_BITWISE_NOT:
        case node_type_t::NODE_TYPE_OPERATION_NEGATE:
        case node_type_t::NODE_TYPE_OPERATION_MULTIPLY:
        case node_type_t::NODE_TYPE_OPERATION_DIVIDE:
        case node_type_t::NODE_TYPE_OPERATION_MODULO:
        case node_type_t::NODE_TYPE_OPERATION_ADD:
        case node_type_t::NODE_TYPE_OPERATION_SUBTRACT:
        case node_type_t::NODE_TYPE_OPERATION_SHIFT_LEFT:
        case node_type_t::NODE_TYPE_OPERATION_SHIFT_RIGHT:
        case node_type_t::NODE_TYPE_OPERATION_LESS:
        case node_type_t::NODE_TYPE_OPERATION_LESS_OR_EQUAL:
        case node_type_t::NODE_TYPE_OPERATION_GREATER:
        case node_type_t::NODE_TYPE_OPERATION_GREATER_OR_EQUAL:
        case node_type_t::NODE_TYPE_OPERATION_MINIMUM:
        case node_type_t::NODE_TYPE_OPERATION_MAXIMUM:
        case node_type_t::NODE_TYPE_OPERATION_EQUAL:
        case node_type_t::NODE_TYPE_OPERATION_NOT_EQUAL:
        case node_type_t::NODE_TYPE_OPERATION_BITWISE_AND:
        case node_type_t::NODE_TYPE_OPERATION_BITWISE_XOR:
        case node_type_t::NODE_TYPE_OPERATION_BITWISE_OR:
        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_AND:
        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_XOR:
        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_OR:
        case node_type_t::NODE_TYPE_OPERATION_CONDITIONAL:
            break;

        case node_type_t::NODE_TYPE_OPERATION_FUNCTION:
            result += QString(" (function name: %1)").arg(f_name);
            break;

        case node_type_t::NODE_TYPE_OPERATION_ASSIGNMENT:
        case node_type_t::NODE_TYPE_OPERATION_VARIABLE:
            result += QString(" (variable name: %1)").arg(f_name);
            break;

        case node_type_t::NODE_TYPE_LITERAL_BOOLEAN:
            result += QString(" (%1)").arg(f_variable.get_value().safeBoolValue() ? "true" : "false");
            break;

        case node_type_t::NODE_TYPE_LITERAL_INTEGER:
            result += QString(" (%1)").arg(f_variable.get_value().safeInt64Value());
            break;

        case node_type_t::NODE_TYPE_LITERAL_FLOATING_POINT:
            result += QString(" (%1)").arg(f_variable.get_value().safeDoubleValue());
            break;

        case node_type_t::NODE_TYPE_LITERAL_STRING:
            result += QString(" (%1)").arg(f_variable.get_value().stringValue());
            break;

        case node_type_t::NODE_TYPE_VARIABLE:
            result += "a program cannot include variables";
            break;

        }
        result += "\n";

        // add the children
        int const max_children(f_children.size());
        for(int i(0); i < max_children; ++i)
        {
            result += f_children[i]->internal_toString(indent + "  ");
        }

        return result;
    }

    void verify_variable() const
    {
#ifdef DEBUG
        switch(f_type)
        {
        case node_type_t::NODE_TYPE_OPERATION_ASSIGNMENT:
        case node_type_t::NODE_TYPE_LITERAL_BOOLEAN:
        case node_type_t::NODE_TYPE_LITERAL_INTEGER:
        case node_type_t::NODE_TYPE_LITERAL_FLOATING_POINT:
        case node_type_t::NODE_TYPE_LITERAL_STRING:
        case node_type_t::NODE_TYPE_OPERATION_VARIABLE:
            break;

        default:
            throw snap_logic_exception(QString("expr_node::[gs]et_name(\"...\") called on a node which does not support a name... (type: %1)").arg(static_cast<int>(static_cast<node_type_t>(f_type))));

        }
#endif
    }

    void verify_children(int idx, bool size_is_legal = false) const
    {
#ifdef DEBUG
        switch(f_type)
        {
        case node_type_t::NODE_TYPE_LOADING: // this happens while loading; the type is set afterward because of the recursion mechanism...
        case node_type_t::NODE_TYPE_OPERATION_LIST:
        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_NOT:
        case node_type_t::NODE_TYPE_OPERATION_BITWISE_NOT:
        case node_type_t::NODE_TYPE_OPERATION_NEGATE:
        case node_type_t::NODE_TYPE_OPERATION_FUNCTION:
        case node_type_t::NODE_TYPE_OPERATION_MULTIPLY:
        case node_type_t::NODE_TYPE_OPERATION_DIVIDE:
        case node_type_t::NODE_TYPE_OPERATION_MODULO:
        case node_type_t::NODE_TYPE_OPERATION_ADD:
        case node_type_t::NODE_TYPE_OPERATION_SUBTRACT:
        case node_type_t::NODE_TYPE_OPERATION_SHIFT_LEFT:
        case node_type_t::NODE_TYPE_OPERATION_SHIFT_RIGHT:
        case node_type_t::NODE_TYPE_OPERATION_LESS:
        case node_type_t::NODE_TYPE_OPERATION_LESS_OR_EQUAL:
        case node_type_t::NODE_TYPE_OPERATION_GREATER:
        case node_type_t::NODE_TYPE_OPERATION_GREATER_OR_EQUAL:
        case node_type_t::NODE_TYPE_OPERATION_MINIMUM:
        case node_type_t::NODE_TYPE_OPERATION_MAXIMUM:
        case node_type_t::NODE_TYPE_OPERATION_EQUAL:
        case node_type_t::NODE_TYPE_OPERATION_NOT_EQUAL:
        case node_type_t::NODE_TYPE_OPERATION_BITWISE_AND:
        case node_type_t::NODE_TYPE_OPERATION_BITWISE_XOR:
        case node_type_t::NODE_TYPE_OPERATION_BITWISE_OR:
        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_AND:
        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_XOR:
        case node_type_t::NODE_TYPE_OPERATION_LOGICAL_OR:
        case node_type_t::NODE_TYPE_OPERATION_CONDITIONAL:
        case node_type_t::NODE_TYPE_OPERATION_ASSIGNMENT:
            break;

        default:
            throw snap_logic_exception(QString("expr_node::add_child/children_size/get_child() called on a node which does not support children... (type: %1)")
                    .arg(static_cast<int>(static_cast<node_type_t>(f_type))));

        }
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
        if(idx >= 0)
        {
#pragma GCC diagnostic pop
            if(static_cast<uint32_t>(idx) >= static_cast<uint32_t>(f_children.size()))
            {
                if(static_cast<uint32_t>(idx) != static_cast<uint32_t>(f_children.size())
                || !size_is_legal)
                {
                    throw snap_logic_exception(QString("expr_node::get_child(%1) called with an out of bounds index (max: %2)")
                                            .arg(idx).arg(f_children.size()));
                }
            }
        }
    }

    void verify_unary(variable_t::variable_vector_t const& sub_results)
    {
#ifdef DEBUG
        if(sub_results.size() != 1)
        {
            throw snap_logic_exception(QString("expr_node::execute() found an unary operator (%1) with a number of results which is not 1")
                    .arg(static_cast<int>(static_cast<node_type_t>(f_type))).arg(sub_results.size()));
        }
#else
        NOTUSED(sub_results);
#endif
    }

    void verify_binary(variable_t::variable_vector_t const& sub_results)
    {
#ifdef DEBUG
        if(sub_results.size() != 2)
        {
            throw snap_logic_exception(QString("expr_node::execute() found a binary operator (%1) with %2 results, expected exactly 2")
                    .arg(static_cast<int>(static_cast<node_type_t>(f_type))).arg(sub_results.size()));
        }
#else
        NOTUSED(sub_results);
#endif
    }

    node_type_t             f_type = node_type_t::NODE_TYPE_VARIABLE;

    QString                 f_name;
    variable_t              f_variable;

    expr_node_vector_t      f_children;
};


functions_t::function_call_table_t const expr_node::internal_functions[] =
{
    { // read the content of a cell
        "cell",
        expr_node::call_cell
    },
    { // check whether a cell exists in a table and row
        "cell_exists",
        expr_node::call_cell_exists
    },
    { // concatenate a child to a path (add "foo" or "/foo" as required)
        "child",
        expr_node::call_child
    },
    //{ // converts a date to microseconds
    //    "date_to_us",
    //    expr_node::call_date_to_us
    //},
    { // cast to format
        "format",
        expr_node::call_format
    },
    { // cast to int16
        "int16",
        expr_node::call_int16
    },
    { // cast to int32
        "int32",
        expr_node::call_int32
    },
    { // cast to int64
        "int64",
        expr_node::call_int64
    },
    { // cast to int8
        "int8",
        expr_node::call_int8
    },
    { // check whether parameter is an integer
        "is_integer",
        expr_node::call_is_integer
    },
    { // retrieve the parent of a path (remove the last /foo name)
        "parent",
        expr_node::call_parent
    },
    { // replace parts of a string
        "preg_replace",
        expr_node::call_preg_replace
    },
    { // check whether a row exists in a table
        "row_exists",
        expr_node::call_row_exists
    },
    { // retrieve a segment
        "segment",
        expr_node::call_segment
    },
    { // convert the parameter to a string
        "string",
        expr_node::call_string
    },
    { // return length of a string
        "strlen",
        expr_node::call_strlen
    },
    { // return true if the string matches the regular expression
        "strmatch",
        expr_node::call_strmatch
    },
    { // retrieve part of a string
        "substr",
        expr_node::call_substr
    },
    { // check whether a named table exists
        "table_exists",
        expr_node::call_table_exists
    },
    { // convert string to lowercase
        "tolower",
        expr_node::call_tolower
    },
    { // convert string to uppercase
        "toupper",
        expr_node::call_toupper
    },
    { // cast to uint16
        "uint16",
        expr_node::call_uint16
    },
    { // cast to uint32
        "uint32",
        expr_node::call_uint32
    },
    { // cast to uint64
        "uint64",
        expr_node::call_uint64
    },
    { // cast to uint8
        "uint8",
        expr_node::call_uint8
    },
    {
        nullptr,
        nullptr
    }
};


char const *expr_node::type_names[static_cast<size_t>(node_type_t::NODE_TYPE_VARIABLE) + 1] =
{
    /* NODE_TYPE_UNKNOWN */ "Unknown",
    /* NODE_TYPE_LOADING */ "Loading",
    /* NODE_TYPE_OPERATION_LIST */ "Operator: ,",
    /* NODE_TYPE_OPERATION_LOGICAL_NOT */ "Operator: !",
    /* NODE_TYPE_OPERATION_BITWISE_NOT */ "Operator: ~",
    /* NODE_TYPE_OPERATION_NEGATE */ "Operator: - (negate)",
    /* NODE_TYPE_OPERATION_FUNCTION */ "Operator: function()",
    /* NODE_TYPE_OPERATION_MULTIPLY */ "Operator: *",
    /* NODE_TYPE_OPERATION_DIVIDE */ "Operator: /",
    /* NODE_TYPE_OPERATION_MODULO */ "Operator: %",
    /* NODE_TYPE_OPERATION_ADD */ "Operator: +",
    /* NODE_TYPE_OPERATION_SUBTRACT */ "Operator: - (subtract)",
    /* NODE_TYPE_OPERATION_SHIFT_LEFT */ "Operator: <<",
    /* NODE_TYPE_OPERATION_SHIFT_RIGHT */ "Operator: >>",
    /* NODE_TYPE_OPERATION_LESS */ "Operator: <",
    /* NODE_TYPE_OPERATION_LESS_OR_EQUAL */ "Operator: <=",
    /* NODE_TYPE_OPERATION_GREATER */ "Operator: >",
    /* NODE_TYPE_OPERATION_GREATER_OR_EQUAL */ "Operator: >=",
    /* NODE_TYPE_OPERATION_MINIMUM */ "Operator: <?",
    /* NODE_TYPE_OPERATION_MAXIMUM */ "Operator: >?",
    /* NODE_TYPE_OPERATION_EQUAL */ "Operator: ==",
    /* NODE_TYPE_OPERATION_NOT_EQUAL */ "Operator: !=",
    /* NODE_TYPE_OPERATION_BITWISE_AND */ "Operator: &",
    /* NODE_TYPE_OPERATION_BITWISE_XOR */ "Operator: ^",
    /* NODE_TYPE_OPERATION_BITWISE_OR */ "Operator: |",
    /* NODE_TYPE_OPERATION_LOGICAL_AND */ "Operator: &&",
    /* NODE_TYPE_OPERATION_LOGICAL_XOR */ "Operator: ^^",
    /* NODE_TYPE_OPERATION_LOGICAL_OR */ "Operator: ||",
    /* NODE_TYPE_OPERATION_CONDITIONAL */ "Operator: ?:",
    /* NODE_TYPE_OPERATION_ASSIGNMENT */ "Operator: :=",
    /* NODE_TYPE_OPERATION_VARIABLE */ "Operator: variable-name",
    /* NODE_TYPE_LITERAL_BOOLEAN */ "Boolean",
    /* NODE_TYPE_LITERAL_INTEGER */ "Integer",
    /* NODE_TYPE_LITERAL_FLOATING_POINT */ "Floating Point",
    /* NODE_TYPE_LITERAL_STRING */ "String",
    /* NODE_TYPE_VARIABLE */ "Variable"
};



/** \brief Merge qualified names in one single identifier.
 *
 * This function is used to transform qualified names in one single long
 * identifier. So
 *
 * \code
 * a :: b
 *   :: c
 * \endcode
 *
 * will result in "a::b::c" as one long identifier.
 *
 * \param[in] r  The rule calling this function.
 * \param[in,out] t  The token tree to be tweaked.
 */
void list_qualified_name(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<parser::token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    (*t)[0]->set_value((*n)[0]->get_value().toString() + "::" + (*t)[2]->get_value().toString());
}


void list_expr_binary_operation(QSharedPointer<token_node>& t, expr_node::node_type_t operation)
{
    QSharedPointer<token_node> n0(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    expr_node::expr_node_pointer_t l(qSharedPointerDynamicCast<expr_node, parser_user_data>(n0->get_user_data()));

    QSharedPointer<token_node> n2(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    expr_node::expr_node_pointer_t r(qSharedPointerDynamicCast<expr_node, parser_user_data>(n2->get_user_data()));

    QSharedPointer<expr_node> v(new expr_node(operation));
    v->add_child(l);
    v->add_child(r);
    t->set_user_data(v);
}

#define LIST_EXPR_BINARY_OPERATION(a, b) \
    void list_expr_##a(rule const& r, QSharedPointer<token_node>& t) \
    { \
        NOTUSED(r); \
        list_expr_binary_operation(t, expr_node::node_type_t::NODE_TYPE_OPERATION_##b); \
    }

LIST_EXPR_BINARY_OPERATION(multiplicative_multiply, MULTIPLY)
LIST_EXPR_BINARY_OPERATION(multiplicative_divide, DIVIDE)
LIST_EXPR_BINARY_OPERATION(multiplicative_modulo, MODULO)
LIST_EXPR_BINARY_OPERATION(additive_add, ADD)
LIST_EXPR_BINARY_OPERATION(additive_subtract, SUBTRACT)
LIST_EXPR_BINARY_OPERATION(shift_left, SHIFT_LEFT)
LIST_EXPR_BINARY_OPERATION(shift_right, SHIFT_RIGHT)
LIST_EXPR_BINARY_OPERATION(relational_less, LESS)
LIST_EXPR_BINARY_OPERATION(relational_less_or_equal, LESS_OR_EQUAL)
LIST_EXPR_BINARY_OPERATION(relational_greater, GREATER)
LIST_EXPR_BINARY_OPERATION(relational_greater_or_equal, GREATER_OR_EQUAL)
LIST_EXPR_BINARY_OPERATION(relational_minimum, MINIMUM)
LIST_EXPR_BINARY_OPERATION(relational_maximum, MAXIMUM)
LIST_EXPR_BINARY_OPERATION(equality_equal, EQUAL)
LIST_EXPR_BINARY_OPERATION(equality_not_equal, NOT_EQUAL)
LIST_EXPR_BINARY_OPERATION(bitwise_and, BITWISE_AND)
LIST_EXPR_BINARY_OPERATION(bitwise_xor, BITWISE_XOR)
LIST_EXPR_BINARY_OPERATION(bitwise_or, BITWISE_OR)
LIST_EXPR_BINARY_OPERATION(logical_and, LOGICAL_AND)
LIST_EXPR_BINARY_OPERATION(logical_xor, LOGICAL_XOR)
LIST_EXPR_BINARY_OPERATION(logical_or, LOGICAL_OR)



void list_expr_unary_operation(QSharedPointer<token_node>& t, expr_node::node_type_t operation)
{
    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    expr_node::expr_node_pointer_t i(qSharedPointerDynamicCast<expr_node, parser_user_data>(n->get_user_data()));

    expr_node::expr_node_pointer_t v(new expr_node(operation));
    v->add_child(i);
    t->set_user_data(v);
}

#define LIST_EXPR_UNARY_OPERATION(a, b) \
    void list_expr_##a(rule const& r, QSharedPointer<token_node>& t) \
    { \
        NOTUSED(r); \
        list_expr_unary_operation(t, expr_node::node_type_t::NODE_TYPE_OPERATION_##b); \
    }

LIST_EXPR_UNARY_OPERATION(logical_not, LOGICAL_NOT)
LIST_EXPR_UNARY_OPERATION(bitwise_not, BITWISE_NOT)
LIST_EXPR_UNARY_OPERATION(negate, NEGATE)


void list_expr_conditional(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n0(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    expr_node::expr_node_pointer_t c(qSharedPointerDynamicCast<expr_node, parser_user_data>(n0->get_user_data()));

    QSharedPointer<token_node> n2(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    expr_node::expr_node_pointer_t at(qSharedPointerDynamicCast<expr_node, parser_user_data>(n2->get_user_data()));

    QSharedPointer<token_node> n4(qSharedPointerDynamicCast<token_node, token>((*t)[4]));
    expr_node::expr_node_pointer_t af(qSharedPointerDynamicCast<expr_node, parser_user_data>(n4->get_user_data()));

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::node_type_t::NODE_TYPE_OPERATION_CONDITIONAL));
    v->add_child(c);
    v->add_child(at);
    v->add_child(af);
    t->set_user_data(v);
}


void list_expr_level_child(expr_node::expr_node_pointer_t n)
{
    int max_children(n->children_size());
    for(int i(0); i < max_children; ++i)
    {
        expr_node::expr_node_pointer_t child(n->get_child(i));
        if(child->get_type() == expr_node::node_type_t::NODE_TYPE_OPERATION_LIST)
        {
            n->remove_child(i);

            // if the child is a list, first reduce it
            list_expr_level_child(child);

            // now add its children to use at this curent location
            int const max_child_children(child->children_size());
            for(int j(0); j < max_child_children; ++j, ++i)
            {
                n->insert_child(i, child->get_child(j));
            }
            max_children = n->children_size();
            --i; // we're going to do ++i in the for()
        }
    }
}


void list_expr_list(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n0(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    expr_node::expr_node_pointer_t l(qSharedPointerDynamicCast<expr_node, parser_user_data>(n0->get_user_data()));

    QSharedPointer<token_node> n2(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    expr_node::expr_node_pointer_t i(qSharedPointerDynamicCast<expr_node, parser_user_data>(n2->get_user_data()));

    // This is very premature optimization and it is WRONG
    // DO NOT OPTIMIZE HERE -- instead we have to optimize when
    // expr_list is being reduced (in the unary and function expressions.)
    // see the list_expr_level_child() function
    //if(l->get_type() == expr_node::node_type_t::NODE_TYPE_OPERATION_LIST)
    //{
    //    // just add to the existing list
    //    l->add_child(i);
    //    t->set_user_data(l);
    //}
    //else
    {
        // not a list yet, create it
        expr_node::expr_node_pointer_t v(new expr_node(expr_node::node_type_t::NODE_TYPE_OPERATION_LIST));
        v->add_child(l);
        v->add_child(i);
        t->set_user_data(v);
    }
}


void list_expr_identity(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    expr_node::expr_node_pointer_t i(qSharedPointerDynamicCast<expr_node, parser_user_data>(n->get_user_data()));

    // we completely ignore the + and (...) they served there purpose already
    t->set_user_data(i);
}


void list_expr_function(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    // the function name is a string
    QSharedPointer<token_node> n0(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QString const func_name((*n0)[0]->get_value().toString());

    // at this point this node is an expr_list which only returns the
    // last item as a result, we want all the parameters when calling a
    // function so we remove the parent node below (see loop)
    QSharedPointer<token_node> n2(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    expr_node::expr_node_pointer_t l(qSharedPointerDynamicCast<expr_node, parser_user_data>(n2->get_user_data()));

    QSharedPointer<expr_node> v(new expr_node(expr_node::node_type_t::NODE_TYPE_OPERATION_FUNCTION));
    v->set_name(func_name);

    if(l->get_type() == expr_node::node_type_t::NODE_TYPE_OPERATION_LIST)
    {
        list_expr_level_child(l);
        int const max_children(l->children_size());
        for(int i(0); i < max_children; ++i)
        {
            v->add_child(l->get_child(i));
        }
    }
    else
    {
        v->add_child(l);
    }

    t->set_user_data(v);
}


void list_expr_function_no_param(rule const & r, QSharedPointer<token_node> & t)
{
    NOTUSED(r);

    // the function name is a string
    QSharedPointer<token_node> n0(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QString const func_name((*n0)[0]->get_value().toString());

    QSharedPointer<expr_node> v(new expr_node(expr_node::node_type_t::NODE_TYPE_OPERATION_FUNCTION));
    v->set_name(func_name);

    t->set_user_data(v);
}


void list_expr_true(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::node_type_t::NODE_TYPE_LITERAL_BOOLEAN));
    QtCassandra::QCassandraValue value;
    value.setBoolValue(true);
    variable_t variable;
    variable.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    v->set_variable(variable);
    t->set_user_data(v);
}


void list_expr_false(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::node_type_t::NODE_TYPE_LITERAL_BOOLEAN));
    QtCassandra::QCassandraValue value;
    value.setBoolValue(false);
    variable_t variable;
    variable.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    v->set_variable(variable);
    t->set_user_data(v);
}


void list_expr_string(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QString const str((*t)[0]->get_value().toString());

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::node_type_t::NODE_TYPE_LITERAL_STRING));
    QtCassandra::QCassandraValue value;
    value.setStringValue(str);
    variable_t variable;
    variable.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
    v->set_variable(variable);
    t->set_user_data(v);
}


void list_expr_integer(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    int64_t const integer((*t)[0]->get_value().toLongLong());

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::node_type_t::NODE_TYPE_LITERAL_INTEGER));
    QtCassandra::QCassandraValue value;
    value.setInt64Value(integer);
    variable_t variable;
    variable.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64, value);
    v->set_variable(variable);
    t->set_user_data(v);
}


void list_expr_float(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    double const floating_point((*t)[0]->get_value().toDouble());

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::node_type_t::NODE_TYPE_LITERAL_FLOATING_POINT));
    QtCassandra::QCassandraValue value;
    value.setDoubleValue(floating_point);
    variable_t variable;
    variable.set_value(variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE, value);
    v->set_variable(variable);
    t->set_user_data(v);
}


void list_expr_level_list(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n0(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    expr_node::expr_node_pointer_t n(qSharedPointerDynamicCast<expr_node, parser_user_data>(n0->get_user_data()));

    list_expr_level_child(n);

    // we completely ignore the + and (...) they served there purpose already
    t->set_user_data(n);
}


void list_expr_variable(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QString const name((*t)[0]->get_value().toString());

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::node_type_t::NODE_TYPE_OPERATION_VARIABLE));
    v->set_name(name);
    t->set_user_data(v);
}


void list_expr_assignment(rule const& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QString const name((*t)[0]->get_value().toString());

    QSharedPointer<token_node> n2(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    expr_node::expr_node_pointer_t i(qSharedPointerDynamicCast<expr_node, parser_user_data>(n2->get_user_data()));

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::node_type_t::NODE_TYPE_OPERATION_ASSIGNMENT));
    v->set_name(name);
    v->add_child(i);
    t->set_user_data(v);
}


void list_expr_copy_result(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    // we don't need the dynamic cast since we don't need to access the data
    t->set_user_data(n->get_user_data());
}



/** \brief Compile a list test to binary.
 *
 * This function transforms a "list test" to byte code (similar to
 * Java byte code if you wish, althgouh really our result is an XML
 * string.) A list test is used to test whether a page is part of a
 * list or not.
 *
 * The \p script parameter is the C-like expression to be transformed.
 *
 * If the script is invalid, the function returns false and the \p program
 * parameter is left untouched.
 *
 * The script is actually a C-like expression.
 *
 * The following defines what is supported in the script (note that this
 * simplified yacc does not show you the priority of the binary operator;
 * we use the C order to the letter):
 *
 * \code
 * // start the parser here
 * start: expr_list
 *
 * // expression
 * expr: '(' expr_list ')'
 *     | expr binary_operator expr
 *     | unary_operator expr
 *     | expr '?' expr ':' expr
 *     | qualified_name '(' expr_list ')'
 *     | TOKEN_ID_IDENTIFIER
 *     | TOKEN_ID_STRING
 *     | TOKEN_ID_INTEGER
 *     | TOKEN_ID_FLOAT
 *     | 'true'
 *     | 'false'
 *
 * qualified_name: TOKEN_ID_IDENTIFIER
 *                     | qualified_name '::' TOKEN_ID_IDENTIFIER
 *
 * expr_list: expr
 *          | expr_list ',' expr
 *
 * // operators
 * unary_operator: '!' | '~' | '+' | '-'
 *
 * binary_operator: '*' | '/' | '%'
 *                | '+' | '-'
 *                | '<<' | '>>'
 *                | '<' | '>' | '<=' | '>=' | '~=' | '<?' | '>?'
 *                | '==' | '!='
 *                | '&'
 *                | '^'
 *                | '|'
 *                | '&&'
 *                | '^^'
 *                | '||'
 *                | ':='
 * \endcode
 *
 * Functions we support:
 *
 * \li cell( \<table>, \<row>, \<cell> )
 *
 * Read the content of a cell named \<cell> from table \<table>
 * in row \<row>. The function always returns a binary piece of
 * content (i.e. the raw data from the cell). It can be converted using
 * a cast:
 *
 * \code
 * int32(cell("content", website_uri() + "path/to/data", "stats::counter"));
 * \endcode
 *
 * Note that a missing cell returns an empty buffer which represents zero or
 * an empty string, although you may use `cell_exists()` first to know
 * whether the cell exists or is just empty.
 *
 * \li cell_exists( \<table>, \<row>, \<cell> )
 *
 * Check whether the specified cell exists in the specified row of the
 * specified table.
 *
 * \li int16( \<any> )
 *
 * Transform \<any> in a signed integer of 16 bits.
 *
 * \li int32( \<any> )
 *
 * Transform \<any> in a signed integer of 32 bits.
 *
 * \li int64( \<any> )
 *
 * Transform \<any> in a signed integer of 64 bits.
 *
 * \li int8( \<any> )
 *
 * Transform \<any> in a signed integer of 8 bits (-128 to +127).
 *
 * \li parent( \<path> )
 *
 * Compute the parent path of \<path>. (i.e. remove the last name on the
 * path.) If the path ends with a slash, the slash is first removed, the
 * the last name.
 *
 * \li row_exists( \<table>, \<row> )
 *
 * Check whether \<row> exists in \<table>.
 *
 * \li string( \<any> )
 *
 * Return \<any> converted to a string.
 *
 * \li strlen( \<string> )
 *
 * Return the length of the \<string>.
 *
 * \li substr( \<string>, \<start>, \<length> )
 *
 * Return the portion of the \<string> starting at \<start> and with at
 * most \<length> characters.
 *
 * \li table_exists( \<table> )
 *
 * Check wether the named table exists, if so return true.
 *
 * \li uint16( \<any> )
 *
 * Transform the parameter to an integer of 16 bit, unsigned.
 *
 * \li uint32( \<any> )
 *
 * Transform the parameter to an integer of 32 bit, unsigned.
 *
 * \li uint64( \<any> )
 *
 * Transform the parameter to an integer of 64 bit, unsigned.
 *
 * \li uint8( \<any> )
 *
 * Transform the parameter to an integer of 8 bits, unsigned (from 0 to 255).
 *
 * \param[in] script  The script to be compiled to binary code bytes.
 *
 * \return A pointer to the program tree or nullptr.
 */
expr_node::expr_node_pointer_t compile_expression(QString const& script)
{
    // LEXER

    // lexer object
    parser::lexer lexer;
    lexer.set_input(script);
    parser::keyword keyword_true(lexer, "true");
    parser::keyword keyword_false(lexer, "false");

    // GRAMMAR
    parser::grammar g;

    // qualified_name
    choices qualified_name(&g, "qualified_name");
    qualified_name >>= TOKEN_ID_IDENTIFIER // keep identifier as is
                     | qualified_name >> "::" >> TOKEN_ID_IDENTIFIER
                                                         >= list_qualified_name
    ;

    // forward definitions
    choices expr(&g, "expr");

    // expr_list
    choices expr_list(&g, "expr_list");
    expr_list >>= expr
                                                    >= list_expr_copy_result
                | expr_list >> "," >> expr
                                                    >= list_expr_list
    ;

    // unary_expr
    choices unary_expr(&g, "unary_expr");
    unary_expr >>= "!" >> unary_expr
                                                    >= list_expr_logical_not
                 | "~" >> unary_expr
                                                    >= list_expr_bitwise_not
                 | "+" >> unary_expr
                                                    >= list_expr_identity
                 | "-" >> unary_expr
                                                    >= list_expr_negate
                 | "(" >> expr_list >> ")"
                                                    >= list_expr_level_list
                 | qualified_name >> "(" >> ")"
                                                    >= list_expr_function_no_param
                 | qualified_name >> "(" >> expr_list >> ")"
                                                    >= list_expr_function
                 | TOKEN_ID_IDENTIFIER
                                                    >= list_expr_variable
                 | keyword_true
                                                    >= list_expr_true
                 | keyword_false
                                                    >= list_expr_false
                 | TOKEN_ID_STRING
                                                    >= list_expr_string
                 | TOKEN_ID_INTEGER
                                                    >= list_expr_integer
                 | TOKEN_ID_FLOAT
                                                    >= list_expr_float
    ;

    // multiplicative_expr
    choices multiplicative_expr(&g, "multiplicative_expr");
    multiplicative_expr >>= unary_expr
                                                    >= list_expr_copy_result
                          | multiplicative_expr >> "*" >> unary_expr
                                                    >= list_expr_multiplicative_multiply
                          | multiplicative_expr >> "/" >> unary_expr
                                                    >= list_expr_multiplicative_divide
                          | multiplicative_expr >> "%" >> unary_expr
                                                    >= list_expr_multiplicative_modulo
    ;

    // additive_expr
    choices additive_expr(&g, "additive_expr");
    additive_expr >>= multiplicative_expr
                                                    >= list_expr_copy_result
                    | additive_expr >> "+" >> multiplicative_expr
                                                    >= list_expr_additive_add
                    | additive_expr >> "-" >> multiplicative_expr
                                                    >= list_expr_additive_subtract
    ;

    // shift_expr
    choices shift_expr(&g, "shift_expr");
    shift_expr >>= additive_expr
                                                    >= list_expr_copy_result
                 | shift_expr >> "<<" >> additive_expr
                                                    >= list_expr_shift_left
                 | shift_expr >> ">>" >> additive_expr
                                                    >= list_expr_shift_right
    ;

    // relational_expr
    choices relational_expr(&g, "relational_expr");
    relational_expr >>= shift_expr
                                                    >= list_expr_copy_result
                      | relational_expr >> "<" >> shift_expr
                                                    >= list_expr_relational_less
                      | relational_expr >> "<=" >> shift_expr
                                                    >= list_expr_relational_less_or_equal
                      | relational_expr >> ">" >> shift_expr
                                                    >= list_expr_relational_greater
                      | relational_expr >> ">=" >> shift_expr
                                                    >= list_expr_relational_greater_or_equal
                      | relational_expr >> "<?" >> shift_expr
                                                    >= list_expr_relational_minimum
                      | relational_expr >> ">?" >> shift_expr
                                                    >= list_expr_relational_maximum
    ;

    // equality_expr
    choices equality_expr(&g, "equality_expr");
    equality_expr >>= relational_expr
                                                    >= list_expr_copy_result
                    | equality_expr >> "==" >> relational_expr
                                                    >= list_expr_equality_equal
                    | equality_expr >> "!=" >> relational_expr
                                                    >= list_expr_equality_not_equal
    ;

    // bitwise_and_expr
    choices bitwise_and_expr(&g, "bitwise_and_expr");
    bitwise_and_expr >>= equality_expr
                                                    >= list_expr_copy_result
                       | bitwise_and_expr >> "&" >> equality_expr
                                                    >= list_expr_bitwise_and
    ;

    // bitwise_xor_expr
    choices bitwise_xor_expr(&g, "bitwise_xor_expr");
    bitwise_xor_expr >>= bitwise_and_expr
                                                    >= list_expr_copy_result
                       | bitwise_xor_expr >> "^" >> bitwise_and_expr
                                                    >= list_expr_bitwise_xor
    ;

    // bitwise_or_expr
    choices bitwise_or_expr(&g, "bitwise_or_expr");
    bitwise_or_expr >>= bitwise_xor_expr
                                                    >= list_expr_copy_result
                      | bitwise_or_expr >> "|" >> bitwise_xor_expr
                                                    >= list_expr_bitwise_or
    ;

    // logical_and_expr
    choices logical_and_expr(&g, "logical_and_expr");
    logical_and_expr >>= bitwise_or_expr
                                                    >= list_expr_copy_result
                       | logical_and_expr >> "&&" >> bitwise_or_expr
                                                    >= list_expr_logical_and
    ;

    // logical_xor_expr
    choices logical_xor_expr(&g, "logical_xor_expr");
    logical_xor_expr >>= logical_and_expr
                                                    >= list_expr_copy_result
                       | logical_xor_expr >> "^^" >> logical_and_expr
                                                    >= list_expr_logical_xor
    ;

    // logical_or_expr
    choices logical_or_expr(&g, "logical_or_expr");
    logical_or_expr >>= logical_xor_expr
                                                    >= list_expr_copy_result
                      | logical_or_expr >> "||" >> logical_xor_expr
                                                    >= list_expr_logical_or
    ;

    // conditional_expr
    // The C/C++ definition is somewhat different:
    // logical-OR-expression ? expression : conditional-expression
    choices conditional_expr(&g, "conditional_expr");
    conditional_expr >>= logical_or_expr
                                                    >= list_expr_copy_result
                       | conditional_expr >> "?" >> expr >> ":" >> logical_or_expr
                                                    >= list_expr_conditional
    ;

    // assignment
    // (this is NOT a C compatible assignment, hence I used ":=")
    choices assignment(&g, "assignment");
    assignment >>= conditional_expr
                                                    >= list_expr_copy_result
                 | TOKEN_ID_IDENTIFIER >> ":=" >> conditional_expr
                                                    >= list_expr_assignment
    ;

    // expr
    expr >>= assignment
                                                    >= list_expr_copy_result
    ;

//std::cerr << expr.to_string() << "\n";
//std::cerr << expr_list.to_string() << "\n";
//std::cerr << unary_expr.to_string() << "\n";

    if(!g.parse(lexer, expr))
    {
        SNAP_LOG_ERROR() << "error #" << static_cast<int>(lexer.get_error_code())
                  << " on line " << lexer.get_error_line()
                  << ": " << lexer.get_error_message();

        expr_node::expr_node_pointer_t null;
        return null;
    }

    QSharedPointer<token_node> r(g.get_result());
    return qSharedPointerDynamicCast<expr_node, parser_user_data>(r->get_user_data());
}



bool expr::compile(QString const& expression)
{
    f_program_tree = compile_expression(expression);
    return f_program_tree;
}


QByteArray expr::serialize() const
{
    QByteArray result;

    QBuffer archive(&result);
    archive.open(QIODevice::WriteOnly);
    QtSerialization::QWriter w(archive, "expr", expr_node::LIST_TEST_NODE_MAJOR_VERSION, expr_node::LIST_TEST_NODE_MINOR_VERSION);
    static_cast<expr_node *>(&*f_program_tree)->write(w);

    return result;
}


void expr::unserialize(QByteArray const& serialized_code)
{
    QByteArray non_const(serialized_code);
    QBuffer archive(&non_const);
    archive.open(QIODevice::ReadOnly);
    QtSerialization::QReader r(archive);
    f_program_tree = expr_node::load(r);
}


void expr::execute(variable_t& result, variable_t::variable_map_t& variables, functions_t& functions)
{
    if(!f_program_tree)
    {
        throw snap_expr_exception_unknown_function("cannot execute an empty program");
    }

    // an internal variable
    variable_t pi("pi");
    pi.set_value(pi_number());
    variables["pi"] = pi;

    static_cast<expr_node *>(&*f_program_tree)->execute(result, variables, functions);
}


void expr::set_cassandra_context(QtCassandra::QCassandraContext::pointer_t context)
{
    g_context = context;
}


} // namespace snap_expr
} // snap

// vim: ts=4 sw=4 et
