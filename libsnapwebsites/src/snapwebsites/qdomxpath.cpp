// Snap Websites Servers -- retrieve a list of nodes from a QDomDocument based on an XPath
// Copyright (C) 2013-2017  Made to Order Software Corp.
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
#include "snapwebsites/qdomxpath.h"

// snapwebsites lib
//
#include "snapwebsites/floats.h"
#include "snapwebsites/not_used.h"
#include "snapwebsites/qstring_stream.h"

// C++ lib
//
#include <cmath>
#include <iomanip>
#include <limits>

// last include
//
#include "snapwebsites/poison.h"



const char *                    QDomXPath::MAGIC = "XPTH";
const QDomXPath::instruction_t  QDomXPath::VERSION_MAJOR;
const QDomXPath::instruction_t  QDomXPath::VERSION_MINOR;



/** \brief Verification mode.
 *
 * If the QDOM_XPATH_VERIFICATION macro is set to zero (0) then no
 * verifications are used while executing a program. Assuming that we
 * ironed out all the bugs, this is as safe as with the verifications,
 * only faster since we can skip of all the checks.
 *
 * This flag is mainly for debug purposes to ensure that our tables
 * are properly defined as one make use of positioned pointers and
 * such need to be 100% correct for the execution environment to
 * work as expected.
 */
#define QDOM_XPATH_VERIFICATION     1


/** \file
 * \brief The DOM XPath Compiler transforms an XPath to bytecode.
 *
 * The bytecode is a very simple set of commands, each defined as one byte.
 * Only the PUSH and BRANCH instructions use additional bytes to allow for
 * immediate data to be used to get everything to work as expected.
 *
 * The idea of using a compiler allows us to avoid having to recompile the
 * XPath each time (very often) we use it. Overall, it is a lot of time
 * saved (if we were to run the XPath expression only once, it would be
 * a big waste, but if you run it 1 million times a day, that's quite a
 * saving.
 *
 * The process requires a lot of DOM calls and thus creating a C++ program,
 * instead of a simple bytecode would require hundreds of lines of code and
 * the result would probably not be any better than the bytecode because it
 * would not use much of the CPU code cache.
 *
 * The main process when executing an XPath is a loop over a set of nodes
 * related to what is currently the context node. Each node becomes the
 * context node in turn and gets checked against the following set of
 * predicates.
 *
 * The following represents one entry in an XPath:
 *
 * \code
 * // Note that although everything is optional, you should probably have
 * // at least one entry (i.e. "." or maybe "[@color='pink']") although
 * // the empty xpath is accepted but it returns an empty set of nodes...
 * [ '/' [ '/' ] ] [ <axis> '::' ] [ <prefix> ':' ] [ <tagname> ] ( '[' <predicate> ']' )*
 * \endcode
 *
 * Each entry is separated by a slash when there are several. If the XPath
 * starts with one or two slashes then the context node is set to the root.
 * Otherwise it is a relative path and the current node is chosen.
 *
 * By default the \<axis> is 'child::node()/' meaning that children of the
 * context node become the elements of the list we're working on. Note that
 * attributes of a node are somewhat considered as being children, but
 * they can only be accessed with the 'attribute' axis (which can be
 * abbreviated with '@'.) The double slash syntax is an equivalent to
 * the "descendant-or-self::node()/" axis.
 *
 * The axis defines the list of nodes and in theory all those nodes are
 * put in a list. However, in practice this is not wise since the list
 * can be quite big. However, many axis represent a large list of nodes
 * (possibly all the nodes in the document being worked on) and in that
 * case premature application of the tag name predicate would be incorrect.
 * For example, "/descendant::p" can be used to retrieve all the \<p> tag
 * of an HTML document. If we were to test the tag name early, then the
 * process would stop at the \<html> or \<body> tags and not find any
 * \<p> tag at all. The algorithms we use take that in account.
 *
 * Note that \<tagname> is mandatory in an entry, however, it can be set to
 * '*' to match all the nodes that the axis defines.
 *
 * The reason why we need to get all the nodes (at least in theory) is
 * because functions such as position(), last(), and count() require the
 * right information and that means a correct list (no possible optimization
 * there!) However, those functions cannot get applied before the predicates
 * written between square brackets ('[' and ']'). This means the gathering of
 * the list of nodes can make use of the axis, prefix, and tag name.
 *
 * Once we have a list of nodes, we can apply the predicates against that
 * list. This is done with a for() loop and an index (this is important to
 * get the right value for the position() command.) Since a predicate can
 * itself include an XPath, the process is repeated there to compute the
 * predicate. The current state is therefore saved and a new state created
 * to manage the predicate. Once the predicate returns, the result is a
 * Boolean. If true, that node is added to the result. If false, the node
 * is ignored.
 *
 * The axis is the function that determines what is to be read from the DOM.
 * The prefix and the tag name are the preliminary predicates that can be
 * applied as the DOM is being scanned.
 *
 * \code
 * setup state with input node(s) as the list of nodes in that state
 * for each union
 *   create a child state as a clone of the original input state
 *   for each entry
 *     if current state node-set is empty, exit loop
 *     mark entry as current
 *     for each node in the current state
 *       (as nodes are selected they become the current context node)
 *       create a child state
 *       run the current selection against the current state and save the nodes in the child state
 *       for each node in the child state
 *         for each predicate
 *           calculate the predicate
 *           if the predicate returned true
 *               or the predicate returned a number and position() == number
 *               or the predicate returned a non-empty list of nodes [TBD]
 *             save the node in the result
 *           endif
 *         loop
 *       loop
 *     loop
 *     add the child state result to the current state result
 *   loop
 *   add the child state result to the current state result
 * loop
 * \endcode
 *
 * The UnionExpr expression becomes the outter most loop. That loop may
 * be removed if no union actually exists (no | was used between two
 * paths.)
 *
 * Note that several of the "for each" loops are actually unfolded in
 * the program because each loop iteration has completely different things
 * to work on. Here are simplied examples of the UnionExpr and the
 * PathExpr.
 *
 * \code
 * // Main Program
 * setup original state
 * call UnionExpr 1
 * return
 *
 * // UnionExpr
 * create clone of original state, make the clone current
 * call PathExpr 1
 * add current state result to original state
 * call PathExpr 2
 * add current state result to original state
 * call PathExpr 3
 * add current state result to original state
 * ...
 * return
 *
 * // PathExpr 1
 * create an empty child state
 * apply Entry 1 computing a node-set
 * for each node in child state
 *   call Predicate 1.1
 * loop
 * if child state result is empty, return
 * replace child state node-set with result node-set
 * for each node in child state
 *   call Predicate 1.2
 * loop
 * if child state result is empty, return
 * replace child state node-set with result node-set
 * for each node in child state
 *   call Predicate 1.3
 * loop
 * if current state result is empty, return
 * replace child state node-set with result node-set
 * ...
 *
 * apply Entry 2 computing a node-set
 * for each node in child state
 *   call Predicate 2.1
 * loop
 * if child state result is empty, return
 * replace child state node-set with result node-set
 * for each node in child state
 *   call Predicate 2.2
 * loop
 * if child state result is empty, return
 * replace child state node-set with result node-set
 * for each node in child state
 *   call Predicate 2.3
 * loop
 * if current state result is empty, return
 * replace child state node-set with result node-set
 * ...
 *
 * apply Entry 3 computing a node-set
 * for each node in child state
 *   call Predicate 3.1
 * loop
 * if child state result is empty, return
 * replace child state node-set with result node-set
 * for each node in child state
 *   call Predicate 3.2
 * loop
 * if child state result is empty, return
 * replace child state node-set with result node-set
 * for each node in child state
 *   call Predicate 3.3
 * loop
 * if current state result is empty, return
 * replace child state node-set with result node-set
 * ...
 *
 * return
 *
 * \endcode
 *
 * \code
 * ...
 * create an empty child state + apply Entry 1 computing a node-set (i.e. axis)
 * if child node-set is empty, return
 * set current node position to 1
 * label repeat:
 * call predicate
 * increment position
 * if position <= size of current node-set, goto repeat
 * if child state result is empty, return
 * ...
 * \endcode
 *
 * So the selection of the nodes to fill the child state is a "simple"
 * instruction that implements the axis, that instruction loops through
 * the nodes from the DOM and add them to a list if they match the top
 * predicates: the prefix and the tag name.
 *
 * The calculation of the predicate, however, is the equivalent of a
 * recursive call to the whole process with the current node as the
 * context node. The return value of the predicate function is used
 * to know whether the node considered as the context node is kept
 * in the list. (Note that we cannot remove the node from the list
 * until all the nodes in that one list were processed of functions
 * such as count() and last() would fail misarably. Instead we use a
 * result list and replace the current list with that result list
 * before processing the next predicate.) Also, in the compiled
 * form, a predicate is a function since we need to be able to call the
 * same predicate over and over again with each child node.
 *
 * The resulting XPath program is very similar to what you'd get in
 * a forth language program. We use a stack to compute the current
 * expression and we have the equivalent of a stack for states (when
 * we create a child state, we simply make use of the next state in
 * a vector of states, adding a new state if the last one was already
 * in use.)
 *
 * The following are the instructions used to run our programs. Note
 * that in memory those instructions are bytes.
 *
 * The different types supported:
 *
 * \li boolean
 * \li integer
 * \li double
 * \li string
 * \li node-set
 *
 * \code
 * // stack instructions
 * push_true
 * push_false
 * push_zero
 * push_byte (1 byte)
 * push_short (2 bytes)
 * push_long (4 bytes)
 * push_longlong (8 bytes)
 * push_double_zero
 * push_double
 * push_empty_string
 * push_any (string "*")
 * push_string
 * push_empty_set
 * swap # (# is 0 to 9 meaning: apply to 'top of stack + #')
 * pop #
 * dup #
 *
 * // change Program Counter (pop 1 integer for offset)
 * call
 * return
 * return_if_true (pop 1 boolean item)
 * return_if_false (pop 1 boolean item)
 * jump
 * jump_if_true (pop 1 boolean item)
 * jump_if_false (pop 1 boolean item)
 * jump_if_zero (pop 1 number item)
 *
 * // state management
 * clone_state
 * new_state
 * get_position
 * set_position
 * get_node_set
 * set_node_set
 * get_result
 * set_result
 *
 * // applying entry, full (pop axis, pop node type, pop prefix, pop local part)
 * // popped axis: ancestor, ancestor-or-self, attribute, child,
 * //              descendant, descendant-or-self, following,
 * //              following, following-sibling, namespace, parent,
 * //              preceding, preceding-sibling, self
 * // popped node type: node, comment, processing-instruction, text
 * axis
 *
 * // variable handling
 * get_variable -- pop name, get variable, return value
 * set_variable -- pop name, pop value, set variable
 *
 * // arithmetic, logic, relational, ...
 * // pop one or two objects, apply operation, push result on the stack
 * add
 * subtract
 * increment
 * decrement
 * multiply
 * divide
 * modulo
 * negate
 * or
 * and
 * xor
 * not
 * equal
 * not_equal
 * less_than
 * less_or_equal
 * greater_than
 * greater_or_equal
 * node_set_size
 * append_node_set
 * interset_node_set
 * except_node_set
 * \endcode
 *
 * Example of the "loop" of the UnionExpr:
 *
 * \code
 * ; we assume that the class initializes an initial state
 * label start_program:
 * ; repeat union code from here
 * {insert path_expr_123 here--this does not need to be a sub-function}
 * ; WARNING: the following 3 instructions do not apply if the UnionExpr
 * ;          does not include a 'union' or '|' operator; this is very
 * ;          important since append_node_set does not accept but node sets
 * get_result
 * append_node_set
 * set_result
 * ; repeat for each entry
 * get_result
 * return
 * \endcode
 *
 * Example of the loop of a PathExpr:
 *
 * \code
 * label path_expr_123: ; some PathExpr
 * new_state
 * -- repeat PathExpr code from here
 * push "page"
 * push_any
 * push "node"
 * push "child"
 * axis
 * get_node_set
 * node_set_size
 * jump_if_zero exit
 * push_integer 1 ; these two lines probably represent the default position?
 * set_position
 * -- repeat Predicate code from here
 * label next:
 * call predicate ; some predicate function...
 * get_position
 * increment
 * dup
 * set_position
 * get_size
 * less_or_equal
 * jump_if_true next
 * get_result
 * node_set_size
 * jump_if_zero exit
 * get_result
 * set_node_set
 * push_empty_set
 * set_result
 * ; repeat with the each predicate
 * ; then repeat with the next path entry
 * label exit:
 * get_node_set
 * delete_state
 * return
 * \endcode
 */


/** \typedef QDomXPath::instruction_t
 * \brief Instructions composed the program once the XPath is compiled.
 *
 * An XPath is composed of many elements. In order to make it easy to
 * process an XPath against XML data, we compile the XPath to a byte
 * code language, which is defined as a vector of bytes representing
 * instructions, sizes, or immediate data.
 *
 * This works along an execution environment structure that keeps track
 * of the data (list of nodes and current status in general.) The status
 * uses a stack to handle expressions as with the forth language.
 *
 * We use a current state as well as the stack. The current state is
 * always accessible (i.e. it is like having access to a variable.)
 */


/** \brief Implementation of the DOM based XPath.
 *
 * This class is used to implement the internals of the XPath based
 * on a DOM. The class is an xpath 1.0 parser which makes use of a DOM
 * as its source of content. This is different from the standard XPath
 * implementation offered by Qt which only works on an XML stream.
 *
 * This implementation is specifically adapted for Snap! since Snap!
 * works with DOMs.
 */
class QDomXPath::QDomXPathImpl
{
public:


/** \brief The character type used by this class.
 *
 * The character type in case it were to change in the future to a UCS-4
 * character.
 */
typedef ushort  char_t;

/** \brief End of input indicator.
 *
 * While reading the input (f_in) characters are returned. Once the last
 * character is reached, the END_OF_PATH value is returned instead.
 *
 * Note that this -1 (or 0xFFFF) is not a valid XML character, it will
 * not detect END_OF_PATH at the wrong time.
 */
static const char_t END_OF_PATH = static_cast<char_t>(-1);

/** \brief Structure that holds the token information.
 *
 * This structure is used when parsing a token. By default it is
 * marked as undefined. The token can be tested with the bool
 * operator (i.e. if(token) ...) to know whether it is defined
 * (true) or undefined (false).
 */
struct token_t
{
    /** \brief List of tokens.
     *
     * This list of token is very large since The XML Path
     * defines a rather large number of function and other
     * names to be used to query an XML document node.
     */
    enum class tok_t
    {
        TOK_UNDEFINED,
        TOK_INVALID,

        TOK_OPEN_PARENTHESIS,
        TOK_CLOSE_PARENTHESIS,
        TOK_OPEN_SQUARE_BRACKET,
        TOK_CLOSE_SQUARE_BRACKET,
        TOK_DOT,
        TOK_DOUBLE_DOT,
        TOK_AT,
        TOK_COMMA,
        TOK_COLON,
        TOK_DOUBLE_COLON,
        TOK_SLASH,
        TOK_DOUBLE_SLASH,
        TOK_PIPE,
        TOK_PLUS,
        TOK_MINUS,
        TOK_EQUAL,
        TOK_NOT_EQUAL,
        TOK_LESS_THAN,
        TOK_LESS_OR_EQUAL,
        TOK_GREATER_THAN,
        TOK_GREATER_OR_EQUAL,
        TOK_ASTERISK,
        TOK_DOLLAR,
        TOK_STRING,
        TOK_INTEGER,
        TOK_REAL,
        TOK_OPERATOR_AND,
        TOK_OPERATOR_OR,
        TOK_OPERATOR_MOD,
        TOK_OPERATOR_DIV,
        TOK_NODE_TYPE_COMMENT,
        TOK_NODE_TYPE_TEXT,
        TOK_NODE_TYPE_PROCESSING_INSTRUCTION,
        TOK_NODE_TYPE_NODE,
        TOK_AXIS_NAME_ANCESTOR,
        TOK_AXIS_NAME_ANCESTOR_OR_SELF,
        TOK_AXIS_NAME_ATTRIBUTE,
        TOK_AXIS_NAME_CHILD,
        TOK_AXIS_NAME_DESCENDANT,
        TOK_AXIS_NAME_DESCENDANT_OR_SELF,
        TOK_AXIS_NAME_FOLLOWING,
        TOK_AXIS_NAME_FOLLOWING_SIBLING,
        TOK_AXIS_NAME_NAMESPACE,
        TOK_AXIS_NAME_PARENT,
        TOK_AXIS_NAME_PRECEDING,
        TOK_AXIS_NAME_PRECEDING_SIBLING,
        TOK_AXIS_NAME_SELF,
        TOK_PREFIX,
        TOK_NCNAME
    };

    /** \brief Initialize the token object.
     *
     * This function initializes the token object to its defaults
     * which is an undefined token.
     */
    token_t()
        : f_token(tok_t::TOK_UNDEFINED)
        //, f_string("") -- auto-init
        , f_integer(0)
        , f_real(0.0)
    {
    }

    /** \brief Test whether the token is defined.
     *
     * This function checks whether the token is defined. If defined,
     * it returns true.
     *
     * \return true if the token is not TOK_UNDEFINED.
     */
    operator bool ()
    {
        return f_token != tok_t::TOK_UNDEFINED;
    }

    /** \brief Test whether the token is undefined.
     *
     * This function checks whether the token is undefined. If not defined,
     * it returns true.
     *
     * \return true if the token is TOK_UNDEFINED.
     */
    bool operator ! ()
    {
        return f_token == tok_t::TOK_UNDEFINED;
    }

    /** \brief Make the token undefined.
     *
     * This function marks the token as being undefined.
     */
    void reset()
    {
        f_token = tok_t::TOK_UNDEFINED;
    }

    tok_t           f_token;
    QString         f_string;
    int64_t         f_integer;
    double          f_real;
};

/** \brief An array of tokens.
 *
 * The typedef defines an array of token which is generally used to
 * represent the stack.
 */
typedef QVector<token_t> token_vector_t;


/** \brief A map of label offsets.
 *
 * This map is used to define offset names and their offset in the program.
 */
typedef QMap<QString, uint32_t> label_offsets_t;


/** \brief Jump to Label structure.
 *
 * Whenever a jump to a future label is added, we need to "fix" the push
 * integer at a later time. This is done by saving the label in this way
 * and later by searching for it to fix it.
 */
typedef QVector<int> jump_to_label_vector_t;


/** \brief An array of future labels.
 *
 * The list of future labels is saved in a vector. Whenever you mark
 * the label, the future labels are corrected and removed from the
 * list. Since the calls are "recursive", this table can grow pretty
 * big and maybe we'll use a map at a later time.
 */
typedef QMap<QString, jump_to_label_vector_t> future_labels_t;


/** \brief An array of labels.
 *
 * This list of labels is used within the parsing of a location path.
 * Labels are created before each axis and "released" when the end of
 * the location path is reached.
 *
 * We do not need more than the offset at the time the label is created
 * since it is created first and jumped to later.
 */
typedef QVector<int> labels_t;


/** \brief Current context while running.
 *
 * While running the program, the context defines the current status of
 * the process. It includes the current set of nodes, the context node
 * (i.e. f_nodes[f_position]) and the set of nodes in the result being
 * computed right now.
 *
 * By default the position is set to -1 which means "not set"
 * and it cannot be used (if accessed [as in INST_GET_POSITION],
 * you get an error.)
 */
struct context_t
{
    context_t()
        : f_position(-1)
        //, f_nodes() -- auto-init
        //, f_result() -- auto-init
    {
    }

    int32_t                     f_position;
    QDomXPath::node_vector_t    f_nodes;
    QDomXPath::node_vector_t    f_result;
};


/** \brief An array of contexts.
 *
 * Whenever a new context is created (i.e. when a new entry in a list of
 * entries starts execution) then it is added at the back of this array.
 *
 * The program as a whole has one stack of states.
 */
typedef QVector<context_t> context_vector_t;


/** \brief A sub-class of the variant_t definition.
 *
 * The atomic values are defined in a separate class so that way we can
 * create sets of atomic values without being bothered by sub-sets which
 * are not supported by XPath 2.0 anyway (i.e. ((1, 2), (3, 4)) becomes
 * (1, 2, 3, 4) which is one set.)
 */
class atomic_value_t
{
public:
    /** \brief Atomic types.
     *
     * The atomic types are used internally to determine the type of
     * variant the data is. The type is defined in the atomic_value_t
     * since it is necessary here. It could be defined outside too
     * although this way it defines it in a namespace like environment.
     */
    enum class type_t
    {
        ATOMIC_TYPE_UNDEFINED,

        ATOMIC_TYPE_NULL,
        ATOMIC_TYPE_END_OF_ARGUMENTS,
        ATOMIC_TYPE_BOOLEAN,
        ATOMIC_TYPE_INTEGER,
        //ATOMIC_TYPE_DECIMAL,
        ATOMIC_TYPE_SINGLE,
        ATOMIC_TYPE_DOUBLE,
        ATOMIC_TYPE_STRING,

        // we include the non-atomic types too
        ATOMIC_TYPE_SET,
        ATOMIC_TYPE_NODE_SET
        //ATOMIC_TYPE_CONTEXT
    };

    /** \brief Initialize the atomic value.
     *
     * This function sets the type of the atomic value to NULL which means
     * that it is undefined. Trying to get a value when an atomic value
     * is NULL generates an error by default although if the cast is set
     * to true.
     */
    atomic_value_t()
        : f_type(type_t::ATOMIC_TYPE_NULL)
        //, f_boolean(false) -- not necessary -- not actually used for now
        //, f_integer(0) -- not necessary
        //, f_decimal(0) -- not necessary -- not actually used for now
        //, f_single(0.0f) -- not necessary
        //, f_doulbe(0.0) -- not necessary
        //, f_string("") -- auto-init
    {
    }

    /** \brief Copy a value in another.
     *
     * Because some parameters may not be defined, the copy operator is
     * overloaded to only copy what is necessary and avoid errors.
     *
     * \param[in] rhs  The right hand side to copy in this value.
     */
    atomic_value_t(atomic_value_t const& rhs)
        : f_type(rhs.f_type)
        //, f_boolean(false) -- handled below if necessary -- not actually used for now
        //, f_integer(0) -- handled below if necessary
        //, f_decimal(0) -- handled below if necessary -- not actually used for now
        //, f_single(0.0f) -- handled below if necessary
        //, f_doulbe(0.0) -- handled below if necessary
        //, f_string("") -- handled below if necessary
    {
        switch(f_type)
        {
        case type_t::ATOMIC_TYPE_NULL:
        case type_t::ATOMIC_TYPE_END_OF_ARGUMENTS:
            // no data to copy
            break;

        case type_t::ATOMIC_TYPE_BOOLEAN:
            //f_boolean = rhs.f_boolean; // using integer instead for now
        case type_t::ATOMIC_TYPE_INTEGER:
            f_integer = rhs.f_integer;
            break;

        case type_t::ATOMIC_TYPE_SINGLE:
            f_single = rhs.f_single;
            break;

        case type_t::ATOMIC_TYPE_DOUBLE:
            f_double = rhs.f_double;
            break;

        case type_t::ATOMIC_TYPE_STRING:
            f_string = rhs.f_string;
            break;

        case type_t::ATOMIC_TYPE_SET:
        case type_t::ATOMIC_TYPE_NODE_SET:
            break;

        //case type_t::ATOMIC_TYPE_UNDEFINED:
        default:
            throw QDomXPathException_NotImplemented(QString("copying of type %1 is not implemented").arg(static_cast<int>(static_cast<type_t>(f_type))));

        }
    }

    /** \brief Copy a value in another.
     *
     * Because some parameters may not be defined, the copy operator is
     * overloaded to only copy what is necessary and avoid errors.
     *
     * \param[in] rhs  The right hand side to copy in this value.
     *
     * \return A reference to this object.
     */
    atomic_value_t& operator = (atomic_value_t const& rhs)
    {
        if(this != &rhs)
        {
            f_type = rhs.f_type;
            switch(f_type)
            {
            case type_t::ATOMIC_TYPE_NULL:
            case type_t::ATOMIC_TYPE_END_OF_ARGUMENTS:
                // no data to copy
                break;

            case type_t::ATOMIC_TYPE_BOOLEAN:
                //f_boolean = rhs.f_boolean; // using integer instead for now
            case type_t::ATOMIC_TYPE_INTEGER:
                f_integer = rhs.f_integer;
                break;

            case type_t::ATOMIC_TYPE_SINGLE:
                f_single = rhs.f_single;
                break;

            case type_t::ATOMIC_TYPE_DOUBLE:
                f_double = rhs.f_double;
                break;

            case type_t::ATOMIC_TYPE_STRING:
                f_string = rhs.f_string;
                break;

            case type_t::ATOMIC_TYPE_SET:
            case type_t::ATOMIC_TYPE_NODE_SET:
                break;

            //case type_t::ATOMIC_TYPE_UNDEFINED:
            default:
                throw QDomXPathException_NotImplemented(QString("copying of type %1 is not implemented").arg(static_cast<int>(static_cast<type_t>(f_type))));

            }
        }
        return *this;
    }

    /** \brief Get the type.
     *
     * Return the type of this atomic value. This determines the get<name>()
     * function that can be called to retrieve the value as is. Other
     * get<name>() functions can also be called, but only to cast the value
     * and in that case the cast parameter must be set to true or the
     * function throws a QDomXPathException_WrongType exception.
     *
     * To test whether an atomtic_value_t is NULL, use the getType()
     * and compare it with type_t::ATOMIC_TYPE_NULL.
     */
    type_t getType() const
    {
        return f_type;
    }

    /** \brief Set the value to NULL.
     *
     * This function sets the value to NULL. NULL is the default value
     * of an atomic value object.
     *
     * All other types are lost after this call.
     */
    void setValue()
    {
        f_type = type_t::ATOMIC_TYPE_NULL;
    }

    /** \brief Set the value to End of Arguments.
     *
     * This function sets the value to End of Arguments. This special value
     * is used to end the list of arguments to a function. Note that it
     * is always expected to be popped by the function being called and
     * not by an expression. This is why there is not even one convertion
     * function for this special type.
     */
    void setEndOfArguments()
    {
        f_type = type_t::ATOMIC_TYPE_END_OF_ARGUMENTS;
    }

    /** \brief Retrieve the value as a Boolean.
     *
     * The function retrieves the atomic value as a Boolean. If the value
     * is not a Boolean and the \p cast parameter is not set to true,
     * then the function generates an exception. Otherwise the function
     * attempts to convert the value to a Boolean.
     *
     * \exception QDomXPathException_WrongType
     * This exception is raised if the atomic type is not
     * type_t::ATOMIC_TYPE_BOOLEAN.
     *
     * \param[in] cast  Whether to cast the value if it is not a Boolean.
     *
     * \return The value as a Boolean.
     */
    bool getBooleanValue(bool cast = false) const
    {
        if(f_type != type_t::ATOMIC_TYPE_BOOLEAN)
        {
            if(!cast)
            {
                throw QDomXPathException_WrongType(QString("atomic type is %1, when a Boolean was requested").arg(static_cast<int>(static_cast<type_t>(f_type))));
            }
        }
        switch(f_type)
        {
        case type_t::ATOMIC_TYPE_NULL:
            return false;

        case type_t::ATOMIC_TYPE_BOOLEAN:
            //return f_boolean;
        case type_t::ATOMIC_TYPE_INTEGER:
            return f_integer != 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        case type_t::ATOMIC_TYPE_SINGLE:
            return 0.0f != f_single;

        case type_t::ATOMIC_TYPE_DOUBLE:
            return 0.0 != f_double;
#pragma GCC diagnostic pop

        case type_t::ATOMIC_TYPE_STRING:
            // TODO -- I think this is totally wrong; if I'm correct it
            //         should interpret the string as true or false and
            //         not whether the string is empty (will test later)
            return !f_string.isEmpty();

        default:
            // this should be done in the previous level
            throw QDomXPathException_NotImplemented(QString("type %1 to Boolean conversion is not implemented").arg(static_cast<int>(static_cast<type_t>(f_type))));

        }
    }

    /** \brief Set the value to the specified Boolean.
     *
     * This function sets the atomic value to the specified Boolean
     * and sets the type to type_t::ATOMIC_TYPE_BOOLEAN.
     *
     * All other types are lost after this call.
     *
     * \param[in] b  The new Boolean value for this atomtic type.
     */
    void setValue(const bool b)
    {
        f_type = type_t::ATOMIC_TYPE_BOOLEAN;
        //f_boolean = b;
        f_integer = b ? 1 : 0;
    }

    /** \brief Retrieve the value as an Integer.
     *
     * The function retrieves the atomic value as an Integer. If the value
     * is not an integer and the \p cast parameter is not set to true,
     * then the function generates an exception. Otherwise the function
     * attempts to convert the value to an integer.
     *
     * \exception QDomXPathException_WrongType
     * This exception is raised if the atomic type is not
     * type_t::ATOMIC_TYPE_INTEGER.
     *
     * \param[in] cast  Whether to cast the value if it is not an Integer.
     *
     * \return The value as an Integer.
     */
    int64_t getIntegerValue(bool cast = false) const
    {
        if(f_type != type_t::ATOMIC_TYPE_INTEGER)
        {
            if(!cast)
            {
                throw QDomXPathException_WrongType(QString("atomic type is %1, when an Integer was requested").arg(static_cast<int>(static_cast<type_t>(f_type))));
            }
        }
        switch(f_type)
        {
        case type_t::ATOMIC_TYPE_NULL:
            return 0;

        case type_t::ATOMIC_TYPE_BOOLEAN:
            return f_integer != 0 ? 1 : 0;

        case type_t::ATOMIC_TYPE_INTEGER:
            return f_integer;

        case type_t::ATOMIC_TYPE_SINGLE:
            return static_cast<int64_t>(std::floor(f_single));

        case type_t::ATOMIC_TYPE_DOUBLE:
            return static_cast<int64_t>(std::floor(f_double));

        case type_t::ATOMIC_TYPE_STRING:
            // TODO -- create a "correct" string to integer convertion
            return static_cast<int64_t>(atol(f_string.toUtf8().data()));

        default:
            // this should be done in the previous level
            throw QDomXPathException_NotImplemented(QString("type %1 to integer conversion is not implemented").arg(static_cast<int>(static_cast<type_t>(f_type))));

        }
    }

    /** \brief Set the value to the specified integer.
     *
     * This function sets the atomic value to the specified integer
     * and sets the type to type_t::ATOMIC_TYPE_INTEGER.
     *
     * All other types are lost after this call.
     *
     * \param[in] integer  The new integer value for this atomtic type.
     */
    void setValue(const int64_t integer)
    {
        f_type = type_t::ATOMIC_TYPE_INTEGER;
        f_integer = integer;
    }

    //void setValue(const QDecimal& decimal)
    //{
    //    f_type = type_t::ATOMIC_TYPE_DECIMAL;
    //    f_decimal = decimal;
    //}

    /** \brief Retrieve the value as a Single.
     *
     * The function retrieves the atomic value as a Single. If the value
     * is not a Single and the \p cast parameter is not set to true,
     * then the function generates an exception. Otherwise the function
     * attempts to convert the value to a Single.
     *
     * \exception QDomXPathException_WrongType
     * This exception is raised if the atomic type is not
     * type_t::ATOMIC_TYPE_SINGLE.
     *
     * \param[in] cast  Whether to cast the value if it is not a Single.
     *
     * \return The value as a Single.
     */
    float getSingleValue(bool cast = false) const
    {
        if(f_type != type_t::ATOMIC_TYPE_SINGLE)
        {
            if(!cast)
            {
                throw QDomXPathException_WrongType(QString("atomic type is %1, when a Single was requested").arg(static_cast<int>(static_cast<type_t>(f_type))));
            }
        }
        switch(f_type)
        {
        case type_t::ATOMIC_TYPE_NULL:
            return 0.0f;

        case type_t::ATOMIC_TYPE_BOOLEAN:
            return f_integer == 0 ? 0.0f : 1.0f;

        case type_t::ATOMIC_TYPE_INTEGER:
            return static_cast<float>(f_integer);

        case type_t::ATOMIC_TYPE_SINGLE:
            return f_single;

        case type_t::ATOMIC_TYPE_DOUBLE:
            return static_cast<float>(f_double);

        case type_t::ATOMIC_TYPE_STRING:
            return static_cast<float>(atof(f_string.toUtf8().data()));

        default:
            // this should be done in the previous level
            throw QDomXPathException_NotImplemented(QString("type %1 to single is not implemented").arg(static_cast<int>(static_cast<type_t>(f_type))));

        }
    }

    /** \brief Set the value to the specified Single.
     *
     * This function sets the atomic value to the specified Single
     * and sets the type to type_t::ATOMIC_TYPE_SINGLE.
     *
     * All other types are lost after this call.
     *
     * \param[in] real  The new Single value for this atomtic type.
     */
    void setValue(const float real)
    {
        f_type = type_t::ATOMIC_TYPE_SINGLE;
        f_single = real;
    }

    /** \brief Retrieve the value as a Double.
     *
     * The function retrieves the atomic value as a Double. If the value
     * is not a Double and the \p cast parameter is not set to true,
     * then the function generates an exception. Otherwise the function
     * attempts to convert the value to a Double.
     *
     * \exception QDomXPathException_WrongType
     * This exception is raised if the atomic type is not
     * type_t::ATOMIC_TYPE_DOUBLE.
     *
     * \param[in] cast  Whether to cast the value if it is not a Double.
     *
     * \return The value as a double.
     */
    double getDoubleValue(bool cast = false) const
    {
        if(f_type != type_t::ATOMIC_TYPE_DOUBLE)
        {
            if(!cast)
            {
                throw QDomXPathException_WrongType(QString("atomic type is %1, when a Double was requested").arg(static_cast<int>(static_cast<type_t>(f_type))));
            }
        }
        switch(f_type)
        {
        case type_t::ATOMIC_TYPE_NULL:
            return 0.0;

        case type_t::ATOMIC_TYPE_BOOLEAN:
            return f_integer == 0 ? 0.0 : 1.0;

        case type_t::ATOMIC_TYPE_INTEGER:
            return static_cast<double>(f_integer);

        case type_t::ATOMIC_TYPE_SINGLE:
            return static_cast<double>(f_single);

        case type_t::ATOMIC_TYPE_DOUBLE:
            return f_double;

        case type_t::ATOMIC_TYPE_STRING:
            // TODO -- properly convert the string (as per the XPath
            //         documentation) we whouls have an external function
            //         for the purpose so any convertion uses the same code
            return atof(f_string.toUtf8().data());

        default:
            // this should be done in the previous level
            throw QDomXPathException_NotImplemented(QString("type %1 to double conversion is not implemented").arg(static_cast<int>(static_cast<type_t>(f_type))));

        }
    }

    /** \brief Set the value to the specified double.
     *
     * This function sets the atomic value to the specified double
     * and sets the type to type_t::ATOMIC_TYPE_DOUBLE.
     *
     * All other types are lost after this call.
     *
     * \param[in] real  The new double value for this atomtic type.
     */
    void setValue(const double real)
    {
        f_type = type_t::ATOMIC_TYPE_DOUBLE;
        f_double = real;
    }

    /** \brief Retrieve the value as a String.
     *
     * The function retrieves the atomic value as a String. If the value
     * is not a String and the \p cast parameter is not set to true,
     * then the function generates an exception. Otherwise the function
     * attempts to convert the value to a String.
     *
     * \exception QDomXPathException_WrongType
     * This exception is raised if the atomic type is not
     * type_t::ATOMIC_TYPE_STRING.
     *
     * \param[in] cast  Whether to cast the value if it is not a String.
     *
     * \return The value as a String.
     */
    QString getStringValue(bool cast = false) const
    {
        if(f_type != type_t::ATOMIC_TYPE_STRING)
        {
            if(!cast)
            {
                throw QDomXPathException_WrongType(QString("atomic type is %1, when a String was requested").arg(static_cast<int>(static_cast<type_t>(f_type))));
            }
        }
        switch(f_type)
        {
        case type_t::ATOMIC_TYPE_NULL:
            return "null";

        case type_t::ATOMIC_TYPE_BOOLEAN:
            return f_integer != 0 ? "true" : "false";

        case type_t::ATOMIC_TYPE_INTEGER:
            return QString("%1").arg(f_integer);

        case type_t::ATOMIC_TYPE_SINGLE:
            return QString("%1").arg(f_single);

        case type_t::ATOMIC_TYPE_DOUBLE:
            return QString("%1").arg(f_double);

        case type_t::ATOMIC_TYPE_STRING:
            return f_string;

        default:
            // this should be done in the previous level
            throw QDomXPathException_NotImplemented(QString("type %1 to string conversion is not implemented").arg(static_cast<int>(static_cast<type_t>(f_type))));

        }
    }

    /** \brief Set the value to the specified string.
     *
     * This function sets the atomic value to the specified string
     * and sets the type to type_t::ATOMIC_TYPE_STRING.
     *
     * All other types are lost after this call.
     *
     * \param[in] str  The new string value for this atomtic type.
     */
    void setValue(const QString& str)
    {
        f_type = type_t::ATOMIC_TYPE_STRING;
        f_string = str;
    }

    /** \brief Set the value to the specified string.
     *
     * This function sets the atomic value to the specified string
     * and sets the type to type_t::ATOMIC_TYPE_STRING.
     *
     * All other types are lost after this call.
     *
     * \param[in] str  The new string value for this atomtic type.
     */
    void setValue(const char *str)
    {
        f_type = type_t::ATOMIC_TYPE_STRING;
        f_string = QString::fromUtf8(str);
    }


    /** \brief Set the value to the specified string.
     *
     * This function sets the atomic value to the specified string
     * and sets the type to type_t::ATOMIC_TYPE_STRING.
     *
     * All other types are lost after this call.
     *
     * \param[in] str  The new string value for this atomtic type.
     */
    void setValue(const wchar_t *str)
    {
        f_type = type_t::ATOMIC_TYPE_STRING;
        f_string = QString::fromWCharArray(str);
    }


    /** \brief Set the value to the specified string.
     *
     * This function sets the atomic value to the specified string
     * and sets the type to type_t::ATOMIC_TYPE_STRING.
     *
     * All other types are lost after this call.
     *
     * \param[in] str  The new string value for this atomtic type.
     */
    void setValue(const std::string& str)
    {
        f_type = type_t::ATOMIC_TYPE_STRING;
        f_string = QString::fromUtf8(str.c_str());
    }


protected:
    type_t                      f_type = type_t::ATOMIC_TYPE_UNDEFINED;

private:
    //bool                        f_boolean = false; -- save some space by using integer 0 or 1
    int64_t                     f_integer = 0;
    // definition? http://www.w3.org/2002/ws/databinding/patterns/6/09/PrecisionDecimal/
    //QDecimal                    f_decimal; // a 32:32 fix number (we need a fixed number class!)
    float                       f_single = 0.0f;
    double                      f_double = 0.0;
    QString                     f_string;
};


/** \brief Merge two types together to accelerate selections.
 *
 * This function is used to merge two types together so we can quickly
 * select two types through the use of a switch statement.
 *
 * \param[in] a  The left type.
 * \param[in] b  The right type.
 *
 * \return The two types merged in an int32_t.
 */
static constexpr int32_t merge_types(atomic_value_t::type_t a, atomic_value_t::type_t b) // we are inside a class, hence the 'static'
{
    return static_cast<int32_t>(a)
        | (static_cast<int32_t>(b) << 16);
}


/** \brief An array of atomic value.
 *
 * This typedef allows us to create arrays of atomic values.
 */
typedef QVector<atomic_value_t> atomic_vector_t;

/** \brief The variant structure is used at execution time.
 *
 * The variant_t represents a value generally on the stack. It can also
 * be viewed as the current set of nodes in a state and the current set
 * of nodes in the result although for those we directly use node_vector_t
 * because those are always sets of nodes.
 */
class variant_t : public atomic_value_t
{
public:
    // need to have an explicit constructor so we can have a copy
    // constructor as well...
    variant_t()
        //: atomic_value_t()
        //, f_atomic() -- auto-init
        //, f_set() -- auto-init
        //, f_node_set() -- auto-init
    {
    }

    variant_t(variant_t const& rhs)
        : atomic_value_t(rhs)
    {
        switch(f_type)
        {
        case type_t::ATOMIC_TYPE_NULL:
        case type_t::ATOMIC_TYPE_END_OF_ARGUMENTS:
        case type_t::ATOMIC_TYPE_BOOLEAN:
        case type_t::ATOMIC_TYPE_INTEGER:
        case type_t::ATOMIC_TYPE_SINGLE:
        case type_t::ATOMIC_TYPE_DOUBLE:
        case type_t::ATOMIC_TYPE_STRING:
            // already handled, avoid the default: ...
            break;

        case type_t::ATOMIC_TYPE_SET:
            f_set = rhs.f_set;
            break;

        case type_t::ATOMIC_TYPE_NODE_SET:
            f_node_set = rhs.f_node_set;
            break;

        default:
            // this should be done in the previous level
            // (i.e. this line should not be reachable)
            throw QDomXPathException_NotImplemented(QString("copying of type %1 is not implemented").arg(static_cast<int>(static_cast<type_t>(f_type))));

        }
    }

    /** \brief Copy a value in another.
     *
     * Because some parameters may not be defined, the copy operator is
     * overloaded to only copy what is necessary and avoid errors.
     *
     * \param[in] rhs  The right hand side to copy in this value.
     *
     * \return A reference to this object.
     */
    variant_t& operator = (variant_t const& rhs)
    {
        if(this != &rhs)
        {
            f_type = rhs.f_type;
            switch(f_type)
            {
            case type_t::ATOMIC_TYPE_NULL:
            case type_t::ATOMIC_TYPE_END_OF_ARGUMENTS:
            case type_t::ATOMIC_TYPE_BOOLEAN:
            case type_t::ATOMIC_TYPE_INTEGER:
            case type_t::ATOMIC_TYPE_SINGLE:
            case type_t::ATOMIC_TYPE_DOUBLE:
            case type_t::ATOMIC_TYPE_STRING:
                // ignore the result, we return *this below
                snap::NOTUSED(atomic_value_t::operator = (rhs));
                break;

            case type_t::ATOMIC_TYPE_SET:
                f_set = rhs.f_set;
                break;

            case type_t::ATOMIC_TYPE_NODE_SET:
                f_node_set = rhs.f_node_set;
                break;

            //case type_t::ATOMIC_TYPE_UNDEFINED:
            default:
                throw QDomXPathException_NotImplemented(QString("copying of type %1 is not implemented").arg(static_cast<int>(static_cast<type_t>(f_type))));

            }
        }
        return *this;
    }

    /** \brief Retrieve the Boolean value.
     *
     * This function retrieves the Boolean value from the variant. If
     * the \p cast parameter is true, then the sets are also checked
     * and return true if they are not empty.
     *
     * \param[in] cast  Whether to cast the value if not a Boolean.
     *
     * \return The Boolean value, or whatever value converted to a Boolean.
     */
    bool getBooleanValue(bool cast = false) const
    {
        if(cast)
        {
            switch(f_type)
            {
            case type_t::ATOMIC_TYPE_SET:
                // TODO -- do proper implementation
                return !f_set.isEmpty();

            case type_t::ATOMIC_TYPE_NODE_SET:
                // TODO -- do proper implementation
                return !f_node_set.isEmpty();

            default:
                break;

            }
        }
        return atomic_value_t::getBooleanValue(cast);
    }

    /** \brief Retrieve the Integer value.
     *
     * This function retrieves the Integer value from the variant. If
     * the \p cast parameter is true, then the sets are also checked
     * and return the first value if they are not empty.
     *
     * \todo
     * See how it should be handled in XPath 2.x because it is different
     * than in XPath 1.x.
     *
     * \param[in] cast  Whether to cast the value if not an Integer.
     *
     * \return The Integer value or whatever value converted to an Integer.
     */
    int64_t getIntegerValue(bool cast = false) const
    {
        if(cast)
        {
            switch(f_type)
            {
            case type_t::ATOMIC_TYPE_SET:
                // TODO -- do proper implementation
                return static_cast<int64_t>(!f_set.isEmpty());

            case type_t::ATOMIC_TYPE_NODE_SET:
                // TODO -- do proper implementation
                return static_cast<int64_t>(!f_node_set.isEmpty());

            default:
                break;

            }
        }
        return atomic_value_t::getIntegerValue(cast);
    }

    /** \brief Retrieve the single value.
     *
     * This function retrieves the single value from the variant. If
     * the \p cast parameter is true, then the sets are also checked
     * and return the first value if they are not empty.
     *
     * \todo
     * See how it should be handled in XPath 2.x because it is different
     * than in XPath 1.x.
     *
     * \param[in] cast  Whether to cast the value if not a Single.
     *
     * \return The Single value, or the converted value of another type.
     */
    float getSingleValue(bool cast = false) const
    {
        if(cast)
        {
            switch(f_type)
            {
            case type_t::ATOMIC_TYPE_SET:
                // TODO -- do proper implementation
                return static_cast<float>(!f_set.isEmpty());

            case type_t::ATOMIC_TYPE_NODE_SET:
                // TODO -- do proper implementation
                return static_cast<float>(!f_node_set.isEmpty());

            default:
                break;

            }
        }
        return atomic_value_t::getSingleValue(cast);
    }

    /** \brief Retrieve the double value.
     *
     * This function retrieves the double value from the variant. If
     * the \p cast parameter is true, then the sets are also checked
     * and return the first value if they are not empty.
     *
     * \todo
     * See how it should be handled in XPath 2.x because it is different
     * than in XPath 1.x.
     *
     * \param[in] cast  Whether to cast the value if not a Double.
     *
     * \return The Double value, or the converted value if not Double.
     */
    double getDoubleValue(bool cast = false) const
    {
        if(cast)
        {
            switch(f_type)
            {
            case type_t::ATOMIC_TYPE_SET:
                // TODO -- do proper implementation
                return static_cast<double>(!f_set.isEmpty());

            case type_t::ATOMIC_TYPE_NODE_SET:
            {
                QString str(getStringValue(true));
                // TODO -- do proper string to double implementation
                return atof(str.toUtf8().data());
            }

            default:
                break;

            }
        }
        return atomic_value_t::getDoubleValue(cast);
    }

    /** \brief Retrieve the string value.
     *
     * This function retrieves the string value from the variant. If
     * the \p cast parameter is true, then the sets are also checked.
     *
     * An atomic set is transformed in a list of atomic values written
     * between parenthesis and separated by commas.
     *
     * In case of a node-set, only the first node (in document order) is
     * transformed to a string with the DOM toString() function (maybe with
     * -1 as the indent parameter to avoid having spaces added by the DOM
     * output functions.) The content returned depends on the type of node:
     *
     * \li Document Node -- all the child text nodes.
     * \li Element -- all the child text nodes.
     * \li Namespace Node -- the URI property.
     * \li Attribute -- the attribute value.
     * \li Comment -- the comment value.
     * \li Processing Instruction -- the text between the \<? and ?\>
     * \li Text Node -- the text of the node
     *
     * \todo
     * See how it should be handled in XPath 2.x because it is different
     * than in XPath 1.x.
     *
     * Some information about casting a node-set to a string can be found in:
     *
     * http://saxon.sourceforge.net/saxon6.5.3/expressions.html
     *
     * \param[in] cast  Whether to cast the value if not a Double.
     *
     * \return The Double value, or the converted value if not Double.
     */
    QString getStringValue(bool cast = false) const
    {
        if(cast)
        {
            switch(f_type)
            {
            case type_t::ATOMIC_TYPE_SET:
                // TODO -- do proper implementation
                throw QDomXPathException_NotImplemented("cast(atomic set) as string is not implemented");

            case type_t::ATOMIC_TYPE_NODE_SET:
                if(f_node_set.isEmpty())
                {
                    // no nodes, return an empty string
                    return "";
                }
                return node_to_string(f_node_set[0]);

            default:
                break;

            }
        }
        return atomic_value_t::getStringValue(cast);
    }

    static QString node_to_string(const QDomNode& node)
    {
        switch(node.nodeType())
        {
        case QDomNode::ElementNode:
            // return all the text nodes from all the children
            return node.toElement().text();

        case QDomNode::AttributeNode:
            // return the corresponding value
            return node.toAttr().value();

        case QDomNode::TextNode:
            return node.toText().data();

        case QDomNode::CDATASectionNode:
            return node.toCDATASection().data();

        case QDomNode::ProcessingInstructionNode:
            return node.toProcessingInstruction().data();

        case QDomNode::CommentNode:
            return node.toComment().data();

        case QDomNode::DocumentNode:
            {
                QDomDocument document(node.toDocument());
                QDomElement element(document.documentElement());
                if(element.isNull())
                {
                    return "";
                }
                return element.text();
            }

        case QDomNode::CharacterDataNode:
            return node.toCharacterData().data();

        //case QDomNode::BaseNode:
        //case QDomNode::DocumentTypeNode:
        //case QDomNode::DocumentFragmentNode:
        //case QDomNode::EntityReferenceNode:
        //case QDomNode::EntityNode:
        //case QDomNode::NotationNode:
        default:
            throw QDomXPathException_NotImplemented(QString("cast(node) as string for this node type (%1) is not implemented")
                                                            .arg(static_cast<int>(node.nodeType())));

        }
        /*NOTREACHED*/
    }

    /** \brief Retrieve the set value.
     *
     * This function retrieves the set value from the variant. If
     * the \p cast parameter is true, then any other value is returned
     * as a set composed of that value (in case of NULL, an empty set.)
     *
     * \param[in] cast  Whether to cast the value if not a Set.
     *
     * \return A set of atomic values.
     */
    atomic_vector_t getSetValue(bool cast = false) const
    {
        if(f_type != type_t::ATOMIC_TYPE_SET)
        {
            if(!cast)
            {
                throw QDomXPathException_WrongType(QString("atomic type is %1, when a Set was requested").arg(static_cast<int>(static_cast<type_t>(f_type))));
            }
        }
        atomic_vector_t result;
        switch(f_type)
        {
        case type_t::ATOMIC_TYPE_NULL:
            return result; // empty set

        case type_t::ATOMIC_TYPE_BOOLEAN:
        case type_t::ATOMIC_TYPE_INTEGER:
        case type_t::ATOMIC_TYPE_SINGLE:
        case type_t::ATOMIC_TYPE_DOUBLE:
        case type_t::ATOMIC_TYPE_STRING:
            result.push_back(*this);
            return result;

        case type_t::ATOMIC_TYPE_SET:
            return f_set;

        case type_t::ATOMIC_TYPE_NODE_SET: // TODO -- is that an error or should we return something like the string() of each node
        default:
            throw QDomXPathException_NotImplemented(QString("type %1 to set conversion is not implemented").arg(static_cast<int>(static_cast<type_t>(f_type))));

        }
    }

    /** \brief Set the variant to a set of atomic values.
     *
     * In XPath version 2.0 sets of atomic values were added to the XPath
     * specification. For example, a set of all multiple of 5 between 0
     * and 100 inclusive are defined as:
     *
     * \code
     * (0 to 100)[. mod 5 = 0]
     * \endcode
     *
     * \param[in] set  The set to become the variant atomic set.
     */
    void setValue(atomic_vector_t const& set)
    {
        f_type = type_t::ATOMIC_TYPE_SET;
        f_set = set;
    }


    /** \brief Retrieve the node set value.
     *
     * This function retrieves the node set value from the variant.
     *
     * If the variant is not a node set then an error is generated
     * (there is no way really to convert an atomic type to a set
     * of nodes.)
     *
     * \param[in] cast  Ignored.
     *
     * \return The node set.
     */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    QDomXPath::node_vector_t& getNodeSetValue(bool cast = false) const
    {
        if(f_type != type_t::ATOMIC_TYPE_NODE_SET)
        {
            throw QDomXPathException_WrongType(QString("atomic type is %1, when a Node Set was requested").arg(static_cast<int>(static_cast<type_t>(f_type))));
        }
        return const_cast<QDomXPath::node_vector_t&>(f_node_set);
    }
#pragma GCC diagnostic pop


    /** \brief Set the variant to a node set.
     *
     * This function sets the variant to the specified \p node_set.
     *
     * The input value is copied, although remember that in a DOM all
     * the object we have access to are references and changing the
     * DOM will affect the nodes wherever they are.
     *
     * \param[in] node_set  The node set to save in this variant.
     */
    void setValue(const QDomXPath::node_vector_t& node_set)
    {
        f_type = type_t::ATOMIC_TYPE_NODE_SET;
        f_node_set = node_set;
    }


    /** \brief Retrieve the context value.
     *
     * This function retrieves the context value from the variant.
     *
     * If the variant is not a context then an error is generated.
     * Contexts should only be used in very specific places which do
     * not call for casting. If an error occurs then there is an
     * internal error.
     *
     * \param[in] cast  Ignored.
     *
     * \return The context.
     */
    //context_t& getContextValue(bool cast = false) const
    //{
    //    if(f_type != type_t::ATOMIC_TYPE_CONTEXT)
    //    {
    //        throw QDomXPathException_WrongType(QString("atomic type is %1, when a Context was requested").arg(static_cast<int>(f_type)));
    //    }
    //    return const_cast<context_t&>(f_context);
    //}


    /** \brief Set the variant to a context.
     *
     * This function sets the variant to the specified \p context.
     *
     * The input value is copied, although remember that in a DOM all
     * the object we have access to are references and changing the
     * DOM will affect the nodes wherever they are.
     *
     * \param[in] context  The context to save in this variant.
     */
    //void setValue(const context_t& context)
    //{
    //    f_type = type_t::ATOMIC_TYPE_CONTEXT;
    //    f_context = context;
    //}

private:
    atomic_vector_t             f_set;      // set of atomic values
    QDomXPath::node_vector_t    f_node_set; // set of nodes -- use the node vector in the context to save space
    //context_t                   f_context;  // the current context
};


/** \brief An array of variants.
 *
 * The array of variants is especially useful as a function stack.
 */
typedef QVector<variant_t>  variant_vector_t;


/** \brief Current function being run.
 *
 * While running we may call functions. For that purpose we need to have
 * a function class because the current state of the current function
 * cannot be touched while a child function is running. Similarly, the
 * stack of the child should not affect the stack of the caller. This is
 * the easiest way to protect those stacks and have an easy way to
 * apply the INST_RETURN instruction.
 */
struct function_t
{
    typedef QMap<QString, variant_t> variables_t;

    function_t()
        : f_pc(0)
        //, f_stack() -- auto-init
        //, f_variables() -- auto-init
    {
    }

    uint32_t                f_pc;
    variant_vector_t        f_stack;
    context_vector_t        f_contexts;
    variables_t             f_variables;
};


/** \brief An array of function status.
 *
 * This array holds a set of function_t structures, each representing a
 * function environment. When a function calls another, a new function_t
 * is created at the back of the stack of function environment.
 *
 * The program as a whole has one stack of functions.
 *
 * The first function (i.e. front) represents the main program and it
 * is not removed with a return, instead the program ends on a return.
 */
typedef QVector<function_t> function_vector_t;


/** \brief List of internal functions.
 *
 * This enumeration defines a list of internal functions which are called
 * using the INST_CALL basic instruction.
 *
 * Note that some internal functions are transformed at compile time to
 * work just like a simple push or some other basic instruction.
 */
enum class internal_func_t
{
    FUNC_UNKNOWN,
    FUNC_AVG,
    FUNC_MIN,
    FUNC_MAX,
    FUNC_SUM
};


/** \brief All the instructions are parameter less functions.
 *
 * Instructions have full access to the QDomXPathImpl class so it is
 * parameter less and they return void.
 *
 * If an internal error is detected, then the corresponding internal
 * error exception is thrown.
 *
 * Otherwise instructions do not really generate errors at this point.
 * In version 2.0 of XPath we are expected to generate errors if a
 * parameter type is incorrect (i.e. you pass a string to a function
 * that expects an integer). That will be added later.
 */
typedef void (QDomXPathImpl::*instruction_function_t)();

/** \brief The array of instructions.
 *
 * This array definition is used to declare all the instructions.
 * Because of the way the compiler functions, the array itself is
 * initialized outside of the class (search on g_instructions to
 * find it.)
 *
 * Since instructions are defined on one byte, all the entries (256)
 * are defined. Instructions that are undefined call the default
 * inst_undefined_instruction() function which throws the
 * QDomXPathException_UndefinedInstructionError exception.
 */
static instruction_function_t const g_instructions[256];

/** \brief Function to disassemble instructions.
 *
 * Each one of these functions print the given instruction that looks like
 * assembly language code. The function returns the size of the instruction.
 * It is passed the PC just after the instruction.
 */
typedef uint32_t (QDomXPathImpl::*disassembly_function_t)(uint32_t pc);

/** \brief The array of disassembling functions.
 *
 * This array definition is used to print out the different instructions
 * either as they are executed or as they are compiled.
 */
static disassembly_function_t const g_disassemble_instructions[256];


static const QDomXPath::instruction_t      INST_END                        = 0x00;
static const QDomXPath::instruction_t      INST_CALL                       = 0x01;
static const QDomXPath::instruction_t      INST_SMALL_FUNCTION             = 0x02;
static const QDomXPath::instruction_t      INST_LARGE_FUNCTION             = 0x03;
static const QDomXPath::instruction_t      INST_JUMP                       = 0x04;
static const QDomXPath::instruction_t      INST_JUMP_IF_TRUE               = 0x05;
static const QDomXPath::instruction_t      INST_JUMP_IF_FALSE              = 0x06;
static const QDomXPath::instruction_t      INST_JUMP_IF_ZERO               = 0x07;
static const QDomXPath::instruction_t      INST_RETURN                     = 0x08;

static const QDomXPath::instruction_t      INST_GET_VARIABLE               = 0x10;
static const QDomXPath::instruction_t      INST_SET_VARIABLE               = 0x11;

static const QDomXPath::instruction_t      INST_POP1                       = 0x20;
static const QDomXPath::instruction_t      INST_POP2                       = 0x21;
static const QDomXPath::instruction_t      INST_POP3                       = 0x22;
static const QDomXPath::instruction_t      INST_POP4                       = 0x23;
static const QDomXPath::instruction_t      INST_POP5                       = 0x24;

static const QDomXPath::instruction_t      INST_DUPLICATE1                 = 0x2A;
static const QDomXPath::instruction_t      INST_DUPLICATE2                 = 0x2B;
static const QDomXPath::instruction_t      INST_DUPLICATE3                 = 0x2C;
static const QDomXPath::instruction_t      INST_DUPLICATE4                 = 0x2D;
static const QDomXPath::instruction_t      INST_DUPLICATE5                 = 0x2E;

static const QDomXPath::instruction_t      INST_SWAP1                      = 0x30;
static const QDomXPath::instruction_t      INST_SWAP2                      = 0x31;
static const QDomXPath::instruction_t      INST_SWAP3                      = 0x32;
static const QDomXPath::instruction_t      INST_SWAP4                      = 0x33;
static const QDomXPath::instruction_t      INST_SWAP5                      = 0x34;
static const QDomXPath::instruction_t      INST_SWAP2_3                    = 0x35;

static const QDomXPath::instruction_t      INST_PUSH_ANY_STRING            = 0x40;
static const QDomXPath::instruction_t      INST_PUSH_BYTE                  = 0x41;
static const QDomXPath::instruction_t      INST_PUSH_DOUBLE                = 0x42;
static const QDomXPath::instruction_t      INST_PUSH_DOUBLE_ZERO           = 0x43;
static const QDomXPath::instruction_t      INST_PUSH_EMPTY_NODE_SET        = 0x44;
static const QDomXPath::instruction_t      INST_PUSH_EMPTY_SET             = 0x45;
static const QDomXPath::instruction_t      INST_PUSH_EMPTY_STRING          = 0x46;
static const QDomXPath::instruction_t      INST_PUSH_END_OF_ARGUMENTS      = 0x47;
static const QDomXPath::instruction_t      INST_PUSH_FALSE                 = 0x48;
static const QDomXPath::instruction_t      INST_PUSH_LARGE_STRING          = 0x49;
static const QDomXPath::instruction_t      INST_PUSH_LONG                  = 0x4A;
static const QDomXPath::instruction_t      INST_PUSH_LONGLONG              = 0x4B;
static const QDomXPath::instruction_t      INST_PUSH_MEDIUM_STRING         = 0x4C;
static const QDomXPath::instruction_t      INST_PUSH_NEGATIVE_BYTE         = 0x4D;
static const QDomXPath::instruction_t      INST_PUSH_NEGATIVE_SHORT        = 0x4E;
static const QDomXPath::instruction_t      INST_PUSH_NEGATIVE_LONG         = 0x4F;
static const QDomXPath::instruction_t      INST_PUSH_SHORT                 = 0x50;
static const QDomXPath::instruction_t      INST_PUSH_SMALL_STRING          = 0x51;
static const QDomXPath::instruction_t      INST_PUSH_TRUE                  = 0x52;
static const QDomXPath::instruction_t      INST_PUSH_ZERO                  = 0x53;

static const QDomXPath::instruction_t      INST_ADD                        = 0x60;
static const QDomXPath::instruction_t      INST_AND                        = 0x61;
static const QDomXPath::instruction_t      INST_CEILING                    = 0x62;
static const QDomXPath::instruction_t      INST_DECREMENT                  = 0x63;
static const QDomXPath::instruction_t      INST_DIVIDE                     = 0x64;
static const QDomXPath::instruction_t      INST_EQUAL                      = 0x65;
static const QDomXPath::instruction_t      INST_FLOOR                      = 0x66;
static const QDomXPath::instruction_t      INST_GREATER_OR_EQUAL           = 0x67;
static const QDomXPath::instruction_t      INST_GREATER_THAN               = 0x68;
static const QDomXPath::instruction_t      INST_IDIVIDE                    = 0x69;
static const QDomXPath::instruction_t      INST_INCREMENT                  = 0x6A;
static const QDomXPath::instruction_t      INST_LESS_OR_EQUAL              = 0x6B;
static const QDomXPath::instruction_t      INST_LESS_THAN                  = 0x6C;
static const QDomXPath::instruction_t      INST_MODULO                     = 0x6D;
static const QDomXPath::instruction_t      INST_MULTIPLY                   = 0x6E;
static const QDomXPath::instruction_t      INST_NEGATE                     = 0x6F;
static const QDomXPath::instruction_t      INST_NOT                        = 0x70;
static const QDomXPath::instruction_t      INST_NOT_EQUAL                  = 0x71;
static const QDomXPath::instruction_t      INST_OR                         = 0x72;
static const QDomXPath::instruction_t      INST_ROUND                      = 0x73;
static const QDomXPath::instruction_t      INST_STRING_LENGTH              = 0x74;
static const QDomXPath::instruction_t      INST_SUBTRACT                   = 0x75;

static const QDomXPath::instruction_t      INST_AXIS                       = 0x80;
static const QDomXPath::instruction_t      INST_ROOT                       = 0x81;
static const QDomXPath::instruction_t      INST_GET_NODE_SET               = 0x82;
static const QDomXPath::instruction_t      INST_SET_NODE_SET               = 0x83;
static const QDomXPath::instruction_t      INST_GET_RESULT                 = 0x84;
static const QDomXPath::instruction_t      INST_SET_RESULT                 = 0x85;
static const QDomXPath::instruction_t      INST_GET_POSITION               = 0x86;
static const QDomXPath::instruction_t      INST_SET_POSITION               = 0x87;
static const QDomXPath::instruction_t      INST_NODE_SET_SIZE              = 0x88;
static const QDomXPath::instruction_t      INST_MERGE_SETS                 = 0x89;
static const QDomXPath::instruction_t      INST_PREDICATE                  = 0x8A;
static const QDomXPath::instruction_t      INST_CREATE_NODE_CONTEXT        = 0x8B;
static const QDomXPath::instruction_t      INST_GET_CONTEXT_NODE           = 0x8C;
static const QDomXPath::instruction_t      INST_NEXT_CONTEXT_NODE          = 0x8D;
static const QDomXPath::instruction_t      INST_POP_CONTEXT                = 0x8E;


enum class axis_t
{
    AXIS_ANCESTOR,
    AXIS_ANCESTOR_OR_SELF,
    AXIS_ATTRIBUTE,
    AXIS_CHILD,
    AXIS_DESCENDANT,
    AXIS_DESCENDANT_OR_SELF,
    AXIS_FOLLOWING,
    AXIS_FOLLOWING_SIBLING,
    AXIS_NAMESPACE,
    AXIS_PARENT,
    AXIS_PRECEDING,
    AXIS_PRECEDING_SIBLING,
    AXIS_SELF
};

enum class node_type_t
{
    // XPath 1.0
    NODE_TYPE_COMMENT,
    NODE_TYPE_NODE,
    NODE_TYPE_PROCESSING_INSTRUCTION,
    NODE_TYPE_TEXT,

    // XPath 2.0 (not supported yet)
    NODE_TYPE_DOCUMENT_NODE,
    NODE_TYPE_ELEMENT,
    NODE_TYPE_SCHEMA_ELEMENT,
    NODE_TYPE_ATTRIBUTE,
    NODE_TYPE_SCHEMA_ATTRIBUTE
};


/** \brief Initialize the class.
 *
 * This function initializes the class. Once the constructor returns
 * the object parse() function can be called in order to get the
 * XPath transformed to tokens and ready to be applied against nodes.
 *
 * The function also initializes the program header as defined in the
 * QDomXPath::getProgram().
 *
 * \param[in] owner  The QDomXPath, the owner or parent of this QDomXPathImpl
 * \param[in] xpath  The XPath to be compiled; may be "" when used only to execute a program
 */
QDomXPathImpl(QDomXPath *owner, const QString& xpath)
    : f_owner(owner)
    //f_show_commands() -- auto-init
    , f_xpath(xpath)
    , f_start(f_xpath.data())
    , f_in(f_start)
    //, f_unget_token() -- auto-init
    //, f_last_token() -- auto-init
    //, f_label_counter(0) -- auto-init
    //, f_labels() -- auto-init
    //, f_future_labels() -- auto-init
    //, f_end_label("") -- auto-init
    //, f_predicate_variable("") -- auto-init
    //, f_result() -- auto-init -- not currently used
    //, f_program_start_offset(...) -- see below
    //, f_program() -- see below
    //, f_functions() -- auto-init
{
    f_program.push_back(QDomXPath::MAGIC[0]);
    f_program.push_back(QDomXPath::MAGIC[1]);
    f_program.push_back(QDomXPath::MAGIC[2]);
    f_program.push_back(QDomXPath::MAGIC[3]);
    f_program.push_back(VERSION_MAJOR);
    f_program.push_back(VERSION_MINOR);
    std::string str(xpath.toUtf8().data());
    size_t size(str.length());
    if(size > 65535)
    {
        size = 65535;
    }
    f_program.push_back(static_cast<instruction_t>(size >> 8));
    f_program.push_back(static_cast<instruction_t>(size));
    for(size_t i(0); i < size; ++i)
    {
        f_program.push_back(str[i]);
    }
    f_program_start_offset = f_program.size();
}



/** \brief While executing, read a byte.
 *
 * There are two sets of instructions that get data from the program area:
 *
 * \li The push instructions that converts the data to a variant value
 *     and push that on the stack.
 *
 * \li The program execution environment which reads one byte instruction
 *     and executes the corresponding function.
 *
 * \return The following 1 byte.
 */
int get_next_program_byte()
{
    if(f_functions.back().f_pc >= static_cast<uint32_t>(f_program.size()))
    {
        throw QDomXPathException_InternalError("trying to read more bytes from f_program than available");
    }
    int result(f_program[f_functions.back().f_pc]);
    ++f_functions.back().f_pc;
    return result;
}


/** \brief Verify that the stack is not empty.
 *
 * This function checks the stack, if empty, it throws an internal error
 * as this should never happend.
 *
 * By default the type of the variant at the top of the stack is not checked.
 * By setting the \p type parameter to something else than
 * type_t::ATOMIC_TYPE_UNDEFINED, the function enforces that type for the object
 * at the top of the stack.
 *
 * \param[in] type  The type of the variant on the top of the stack.
 *                  Ignore if set to type_t::ATOMIC_TYPE_UNDEFINED.
 */
void stack_not_empty(atomic_value_t::type_t type = atomic_value_t::type_t::ATOMIC_TYPE_UNDEFINED)
{
    if(f_functions.back().f_stack.empty())
    {
        throw QDomXPathException_InternalError("cannot pop anything from an empty stack");
    }
    if(type != atomic_value_t::type_t::ATOMIC_TYPE_UNDEFINED
    && f_functions.back().f_stack.back().getType() != type)
    {
        throw QDomXPathException_WrongType(QString("the current type at the top of the stack is not of the right type (expected %1, it is %2)")
                                            .arg(static_cast<int>(type))
                                            .arg(static_cast<int>(f_functions.back().f_stack.back().getType())));
    }
}


/** \brief Check that the stack of contexts is not empty.
 *
 * The stack of contexts is created by the INST_CREATE_NODE_CONTEXT
 * instruction. It should never be used if empty so we have this
 * function to check the validity in one place.
 */
void contexts_not_empty()
{
    if(f_functions.back().f_contexts.empty())
    {
        throw QDomXPathException_InternalError("cannot pop anything from an empty stack of contexts");
    }
}


/** \brief Pop one entry from the stack.
 *
 * This function checks the stack, if empty, it throws an internal error
 * as this should never happend.
 *
 * \return A variant with the data from the top of the stack.
 */
variant_t pop_variant_data()
{
    stack_not_empty();

#if 0
    printf("Stack Trace:\n");
    for(int i(0); i < f_functions.back().f_stack.size(); ++i)
    {
        variant_t v(f_functions.back().f_stack[i]);
        switch(v.getType())
        {
        case atomic_value_t::type_t::ATOMIC_TYPE_UNDEFINED:
            printf(" + Undefined\n");
            break;

        case atomic_value_t::type_t::ATOMIC_TYPE_NULL:
            printf(" + NULL\n");
            break;

        case atomic_value_t::type_t::ATOMIC_TYPE_END_OF_ARGUMENTS:
            printf(" + End of Arguments\n");
            break;

        case atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN:
            printf(" + Boolean (%s)\n", (v.getBooleanValue() ? "true" : "false"));
            break;

        case atomic_value_t::type_t::ATOMIC_TYPE_INTEGER:
            printf(" + Integer (%ld)\n", v.getIntegerValue());
            break;

        //case atomic_value_t::type_t::ATOMIC_TYPE_DECIMAL:
        case atomic_value_t::type_t::ATOMIC_TYPE_SINGLE:
            printf(" + Single (%g)\n", v.getSingleValue());
            break;

        case atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE:
            printf(" + Double (%g)\n", v.getDoubleValue());
            break;

        case atomic_value_t::type_t::ATOMIC_TYPE_STRING:
            printf(" + String (\"%s\")\n", v.getStringValue().toUtf8().data());
            break;

        case atomic_value_t::type_t::ATOMIC_TYPE_SET:
            printf(" + Atomic Set\n");
            break;

        case atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET:
            printf(" + Node Set\n");
            break;

        }
    }
#endif

    variant_t v(f_functions.back().f_stack.back());
    f_functions.back().f_stack.pop_back();

    return v;
}


/** \brief For all undefined instructions.
 *
 * This function is used by all the undefined instructions so we can always
 * run something (i.e. instead of using NULL) and avoid testing the pointers.
 *
 * This function always raises the QDomXPathException_UndefinedInstructionError
 * exception.
 */
void inst_undefined_instruction()
{
    QDomXPath::instruction_t inst(f_program[f_functions.back().f_pc - 1]);
    throw QDomXPathException_UndefinedInstructionError(QString("instruction %1 is not defined (pc = %2)")
            .arg(static_cast<int>(inst)).arg(f_functions.back().f_pc - 1));
}


/** \brief The End instruction.
 *
 * This function raises an exception because it should never be executed.
 * Instead the function running the execution loop is expected to catch this
 * special case and return. However, just in case it were to be executed,
 * we have a function that throws.
 */
void inst_end()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_END)
    {
        throw QDomXPathException_InternalError("INST_END not at the right location in the table of instructions");
    }
#endif
    throw QDomXPathException_InternalError("the End instruction is not expected to be executed");
}


/** \brief The Call instruction.
 *
 * This function creates a new function_t with its PC set to the function
 * being called as found on the stack (the last PUSH is the PC of the
 * function being called.)
 *
 * The arguments are also popped from the input stack, up until the
 * End of Argument special value is found. Then this function returns
 * which means we now are running in this sub-function.
 */
void inst_call()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_CALL)
    {
        throw QDomXPathException_InternalError("INST_CALL not at the right location in the table of instructions");
    }
#endif
    // get the internal function number (FUNC_...)
    variant_t function_number(pop_variant_data());
    if(function_number.atomic_value_t::getType() != atomic_value_t::type_t::ATOMIC_TYPE_INTEGER)
    {
        throw QDomXPathException_InternalError("INST_CALL expects the first element on the stack to be of type INTEGER");
    }

    // save arguments in variables named "a1", "a2", "a3"...
    // these can be accessed with "$a1", "$a2", "$a3"...
    // --the number of arguments is saved in $argc (argument count)--
    variant_vector_t arguments;
    for(int a(1);; ++a)
    {
        variant_t arg(pop_variant_data());
        if(arg.atomic_value_t::getType() == atomic_value_t::type_t::ATOMIC_TYPE_END_OF_ARGUMENTS)
        {
            // found the end of the argument list
            break;
        }
        arguments.push_back(arg);
    }

    // now select the function to run
    switch(static_cast<internal_func_t>(function_number.getIntegerValue()))
    {
    case internal_func_t::FUNC_AVG:
        func_avg(arguments);
        break;

    case internal_func_t::FUNC_MAX:
        func_max(arguments);
        break;

    case internal_func_t::FUNC_MIN:
        func_min(arguments);
        break;

    case internal_func_t::FUNC_SUM:
        func_sum(arguments);
        break;

    default:
        throw QDomXPathException_NotImplemented(QString("function %1 is not yet implemented").arg(function_number.getIntegerValue()));

    }
}


void func_default_to_context_node(variant_vector_t& arguments)
{
    if(arguments.size() == 0)
    {
        // retrieve the context node if no parameters were specified
        context_vector_t::reference context(f_functions.back().f_contexts.back());
        if(context.f_position == -1)
        {
            throw QDomXPathException_EmptyContext("the sum() function cannot be used without a context node and no parameters");
        }
        QDomXPath::node_vector_t context_node;
        context_node.push_back(context.f_nodes[context.f_position]);
        variant_t value;
        value.setValue(context_node);
        arguments.push_back(value);
    }
}

void func_calculate_sum_or_average(variant_vector_t& arguments, bool sum_only)
{
    func_default_to_context_node(arguments);
    // TODO: support sums of only decimals and singles
    bool integer(true);
    int64_t isum(0);
    int count(0);
    double dsum(0.0);
    const int imax(arguments.size());
    for(int i(0); i < imax; ++i)
    {
        variant_t arg(arguments[i]);
        switch(arg.getType())
        {
        case atomic_value_t::type_t::ATOMIC_TYPE_INTEGER:
            if(integer)
            {
                isum += arg.getIntegerValue();
                dsum = static_cast<double>(isum);
                ++count;
                break;
            }
        //case atomic_type_t::ATOMIC_TYPE_DECIMAL:
        case atomic_value_t::type_t::ATOMIC_TYPE_SINGLE:
        case atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE:
            integer = false;
            dsum += arg.getDoubleValue(true);
            ++count;
            break;

        case atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET:
            {
                integer = false;
                QDomXPath::node_vector_t node_set(arg.getNodeSetValue());
                const int jmax(node_set.size());
                for(int j(0); j < jmax; ++j)
                {
                    QString str(variant_t::node_to_string(node_set[j]));
                    // TODO verify that the string is a valid number
                    // TODO if the number is decimal, avoid forcing double
                    // (in XPath 2.0 we can use the best possible type as
                    // defined here: http://www.w3.org/TR/xpath20/#promotion )
                    dsum += atof(str.toUtf8().data());
                    ++count;
                }
            }
            break;

        default:
            throw QDomXPathException_WrongType("the sum/avg() functions cannot be used with types other than numbers and node-set");

        }
    }

    variant_t return_value;
    if(integer && sum_only)
    {
        return_value.atomic_value_t::setValue(isum);
    }
    else
    {
        if(!sum_only && count > 0)
        {
            // compute the average
            dsum /= static_cast<double>(count);
        }
        return_value.atomic_value_t::setValue(dsum);
    }
    f_functions.back().f_stack.push_back(return_value);
}


void func_sum(variant_vector_t& arguments)
{
    func_calculate_sum_or_average(arguments, true);
}


void func_avg(variant_vector_t& arguments)
{
    func_calculate_sum_or_average(arguments, false);
}


void func_calculate_min_or_max(variant_vector_t& arguments, bool min)
{
    func_default_to_context_node(arguments);
    // TODO: support min/max of only decimals and singles
    // TODO: support min/max on strings (XPath 2.0 supports such + collation)
    bool integer(true);
    bool first(true);
    int64_t iresult(0);
    double dresult(0.0);
    const int imax(arguments.size());
    for(int i(0); i < imax; ++i)
    {
        variant_t arg(arguments[i]);
        switch(arg.getType())
        {
        case atomic_value_t::type_t::ATOMIC_TYPE_INTEGER:
            if(integer)
            {
                int64_t v(arg.getIntegerValue());
                if(v > iresult ^ min || first)
                {
                    iresult = v;
                    first = false;
                }
                dresult = static_cast<double>(iresult);
                break;
            }
        //case atomic_type_t::ATOMIC_TYPE_DECIMAL:
        case atomic_value_t::type_t::ATOMIC_TYPE_SINGLE:
        case atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE:
            integer = false;
            {
                double v(arg.getDoubleValue(true));
                if(v > dresult ^ min || first)
                {
                    dresult = v;
                    first = false;
                }
            }
            break;

        case atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET:
            {
                integer = false;
                QDomXPath::node_vector_t node_set(arg.getNodeSetValue());
                const int jmax(node_set.size());
                for(int j(0); j < jmax; ++j)
                {
                    QString str(variant_t::node_to_string(node_set[j]));
                    // TODO verify that the string is a valid number
                    // TODO if the number is decimal, avoid forcing double
                    // (in XPath 2.0 we can use the best possible type as
                    // defined here: http://www.w3.org/TR/xpath20/#promotion )
                    double v(atof(str.toUtf8().data()));
                    if(v > dresult ^ min || first)
                    {
                        dresult = v;
                        first = false;
                    }
                }
            }
            break;

        default:
            throw QDomXPathException_WrongType("the min/max() functions cannot be used with types other than numbers and node-set");

        }
    }

    variant_t return_value;
    if(integer)
    {
        return_value.atomic_value_t::setValue(iresult);
    }
    else
    {
        return_value.atomic_value_t::setValue(dresult);
    }
    f_functions.back().f_stack.push_back(return_value);
}


void func_max(variant_vector_t& arguments)
{
    func_calculate_min_or_max(arguments, false);
}


void func_min(variant_vector_t& arguments)
{
    func_calculate_min_or_max(arguments, true);
}


/** \brief Return from a function.
 *
 * This instruction allows the instruction stream to return to the caller.
 *
 * The last variant that got pushed on the stack is returned to the caller.
 */
void inst_return()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_RETURN)
    {
        throw QDomXPathException_InternalError("INST_RETURN not at the right location in the table of instructions");
    }
    if(f_functions.size() <= 1)
    {
        throw QDomXPathException_InternalError("INST_RETURN cannot be called with an empty stack of functions");
    }
#endif
    // copy the return value from the function stack
    // to the caller's stack
    variant_t return_value(pop_variant_data());

    // now return to the old function
    f_functions.pop_back();

    f_functions.back().f_stack.push_back(return_value);
}


/** \brief Get the contents of a variable.
 *
 * This instruction retrieves the contents of a variable. First the function
 * checks the current function. If the current function does not define such
 * a variable, then the user bound variables are checked.
 *
 * If no variable with that name is defined, then an error is raised.
 */
void inst_get_variable()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_GET_VARIABLE)
    {
        throw QDomXPathException_InternalError("INST_GET_VARIABLE not at the right location in the table of instructions");
    }
#endif
    // retrieve variable name
    variant_t variable(pop_variant_data());
    QString variable_name(variable.getStringValue());

    if(f_functions.back().f_variables.contains(variable_name))
    {
        f_functions.back().f_stack.push_back(f_functions.back().f_variables[variable_name]);
    }
    else
    {
        variant_t value;
        value.atomic_value_t::setValue(f_owner->getVariable(variable_name));
        f_functions.back().f_stack.push_back(value);
    }
}



/** \brief Set the contents of a variable.
 *
 * This instruction sets the contents of a variable. The variable is set in
 * the current function only. There is currently no function to change a
 * global variable.
 */
void inst_set_variable()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SET_VARIABLE)
    {
        throw QDomXPathException_InternalError("INST_SET_VARIABLE not at the right location in the table of instructions");
    }
#endif
    // retrieve variable name
    variant_t variable(pop_variant_data());
    QString variable_name(variable.getStringValue());

    // retrieve the value for the variable
    variant_t value(pop_variant_data());

    f_functions.back().f_variables[variable_name] = value;
}


/** \brief Found a function.
 *
 * Functions are just inline code that gets skipped when bumped into.
 * Functions are not named, they just have an offset so we do not need
 * to do anything special about them. At a later time, we may have to
 * handle user functions, but I have the impression that those would
 * be quite different that this.
 *
 * The size of the small function is 16 bits.
 */
void inst_small_function()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SMALL_FUNCTION)
    {
        throw QDomXPathException_InternalError("INST_PUSH_END_OF_ARGUMENTS not at the right location in the table of instructions");
    }
#endif
    int size((get_next_program_byte() << 8) | get_next_program_byte());
    f_functions.back().f_pc += size;
}


/** \brief Found a function.
 *
 * Functions are just inline code that gets skipped when bumped into.
 * Functions are not named, they just have an offset so we do not need
 * to do anything special about them. At a later time, we may have to
 * handle user functions, but I have the impression that those would
 * be quite different that this.
 *
 * The size of the large function is 32 bits.
 */
void inst_large_function()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_LARGE_FUNCTION)
    {
        throw QDomXPathException_InternalError("INST_LARGE_FUNCTION not at the right location in the table of instructions");
    }
#endif
    int size((get_next_program_byte() << 24)
           | (get_next_program_byte() << 16)
           | (get_next_program_byte() << 8)
           | get_next_program_byte());
    f_functions.back().f_pc += size;
}


/** \brief Jump to a new location.
 *
 * Jump to a new location as found on the stack.
 */
void inst_jump()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_JUMP)
    {
        throw QDomXPathException_InternalError("INST_JUMP not at the right location in the table of instructions");
    }
#endif
    variant_t pc(pop_variant_data());
    f_functions.back().f_pc = static_cast<uint32_t>(pc.getIntegerValue());
}


/** \brief Jump to a new location if true.
 *
 * This function pops a new location and then a Boolean value. If the
 * Boolean value is false, the new location is ignored. If the Boolean
 * value is true, then PC becomes that new location.
 */
void inst_jump_if_true()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_JUMP_IF_TRUE)
    {
        throw QDomXPathException_InternalError("INST_JUMP_IF_TRUE not at the right location in the table of instructions");
    }
#endif
    variant_t pc(pop_variant_data());
    variant_t boolean(pop_variant_data());
    if(boolean.getBooleanValue())
    {
        f_functions.back().f_pc = static_cast<uint32_t>(pc.getIntegerValue());
    }
}


/** \brief Jump to a new location if false.
 *
 * This function pops a new location and then a Boolean value. If the
 * Boolean value is true, the new location is ignored. If the Boolean
 * value is false, then PC becomes that new location.
 */
void inst_jump_if_false()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_JUMP_IF_FALSE)
    {
        throw QDomXPathException_InternalError("INST_JUMP_IF_FALSE not at the right location in the table of instructions");
    }
#endif
    variant_t pc(pop_variant_data());
    variant_t boolean(pop_variant_data());
    if(!boolean.getBooleanValue())
    {
        f_functions.back().f_pc = static_cast<uint32_t>(pc.getIntegerValue());
    }
}


/** \brief Jump to a new location if zero.
 *
 * This function pops a new location and then a value. If the
 * value is not zero, the new location is ignored. If the
 * value is zero, then PC becomes that new location.
 */
void inst_jump_if_zero()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_JUMP_IF_ZERO)
    {
        throw QDomXPathException_InternalError("INST_JUMP_IF_ZERO not at the right location in the table of instructions");
    }
#endif
    variant_t pc(pop_variant_data());
    variant_t object(pop_variant_data());
    if(object.getIntegerValue(true) == 0)
    {
        f_functions.back().f_pc = static_cast<uint32_t>(pc.getIntegerValue());
    }
}


/** \brief Pop one object from the stack.
 *
 * This instruction pops one object from the top of the stack.
 *
 * \code
 * before   after
 * o1       o2
 * o2       o3
 * o3       ...
 * ...
 * \endcode
 */
void inst_pop1()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_POP1)
    {
        throw QDomXPathException_InternalError("INST_POP1 not at the right location in the table of instructions");
    }
#endif
    if(f_functions.back().f_stack.size() < 1)
    {
        throw QDomXPathException_EmptyStack("cannot pop anything from an empty stack");
    }
    f_functions.back().f_stack.pop_back();
}


/** \brief Pop two objects from the stack.
 *
 * This instruction pops the second object counting from the top stack of
 * the stack.
 *
 * \code
 * before   after
 * o1       o1
 * o2       o3
 * o3       ...
 * ...
 * \endcode
 */
void inst_pop2()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_POP2)
    {
        throw QDomXPathException_InternalError("INST_POP2 not at the right location in the table of instructions");
    }
#endif
    if(f_functions.back().f_stack.size() < 2)
    {
        throw QDomXPathException_EmptyStack("cannot pop the second object from the stack if the stack is not at least two objects");
    }
    f_functions.back().f_stack.remove(f_functions.back().f_stack.size() - 2);
}


/** \brief Pop three objects from the stack.
 *
 * This instruction pops the thrid object counting from the top stack of
 * the stack.
 *
 * \code
 * before   after
 * o1       o1
 * o2       o2
 * o3       o4
 * o4       ...
 * ...
 * \endcode
 */
void inst_pop3()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_POP3)
    {
        throw QDomXPathException_InternalError("INST_POP3 not at the right location in the table of instructions");
    }
#endif
    if(f_functions.back().f_stack.size() < 3)
    {
        throw QDomXPathException_EmptyStack("cannot pop the third object from the stack if the stack is not at least three objects");
    }
    f_functions.back().f_stack.remove(f_functions.back().f_stack.size() - 3);
}


/** \brief Pop four objects from the stack.
 *
 * This instruction pops the fourth object counting from the top stack of
 * the stack.
 *
 * \code
 * before   after
 * o1       o1
 * o2       o2
 * o3       o3
 * o4       o5
 * o5       ...
 * ...
 * \endcode
 */
void inst_pop4()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_POP4)
    {
        throw QDomXPathException_InternalError("INST_POP4 not at the right location in the table of instructions");
    }
#endif
    if(f_functions.back().f_stack.size() < 4)
    {
        throw QDomXPathException_EmptyStack("cannot pop the forth object from the stack if the stack is not at least four objects");
    }
    f_functions.back().f_stack.remove(f_functions.back().f_stack.size() - 4);
}


/** \brief Pop five objects from the stack.
 *
 * This instruction pops the fifth object counting from the top stack of
 * the stack.
 *
 * \code
 * before   after
 * o1       o1
 * o2       o2
 * o3       o3
 * o4       o4
 * o5       o6
 * o6       ...
 * ...
 * \endcode
 */
void inst_pop5()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_POP5)
    {
        throw QDomXPathException_InternalError("INST_POP5 not at the right location in the table of instructions");
    }
#endif
    if(f_functions.back().f_stack.size() < 5)
    {
        throw QDomXPathException_EmptyStack("cannot pop the fifth object from the stack if the stack is not at least five objects");
    }
    f_functions.back().f_stack.remove(f_functions.back().f_stack.size() - 5);
}


/** \brief Duplicate the last object on the stack.
 *
 * This instruction pops the last object on the stack and then push
 * it back twice on the stack.
 */
void inst_duplicate1()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_DUPLICATE1)
    {
        throw QDomXPathException_InternalError("INST_DUPLICATE1 not at the right location in the table of instructions");
    }
#endif
    if(f_functions.back().f_stack.size() == 0)
    {
        throw QDomXPathException_EmptyStack("duplicate cannot be used with an empty stack");
    }
    variant_t value(f_functions.back().f_stack.back());
    f_functions.back().f_stack.push_back(value);
}


/** \brief Duplicate the second to last object on the stack.
 *
 * This instruction gets a copy of the second to last object on the stack
 * and push a copy on the stack.
 */
void inst_duplicate2()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_DUPLICATE2)
    {
        throw QDomXPathException_InternalError("INST_DUPLICATE2 not at the right location in the table of instructions");
    }
#endif
    int size(f_functions.back().f_stack.size());
    if(size <= 1)
    {
        throw QDomXPathException_EmptyStack("duplicate(2) cannot be used with a stack of less than 2 items");
    }
    variant_t value(f_functions.back().f_stack[size - 2]);
    f_functions.back().f_stack.push_back(value);
}


/** \brief Duplicate the third to last object on the stack.
 *
 * This instruction gets a copy of the third to last object on the stack
 * and push a copy on the stack.
 */
void inst_duplicate3()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_DUPLICATE3)
    {
        throw QDomXPathException_InternalError("INST_DUPLICATE3 not at the right location in the table of instructions");
    }
#endif
    int size(f_functions.back().f_stack.size());
    if(size <= 2)
    {
        throw QDomXPathException_EmptyStack("duplicate(3) cannot be used with a stack of less than 3 items");
    }
    variant_t value(f_functions.back().f_stack[size - 3]);
    f_functions.back().f_stack.push_back(value);
}


/** \brief Duplicate the forth to last object on the stack.
 *
 * This instruction gets a copy of the forth to last object on the stack
 * and push a copy on the stack.
 */
void inst_duplicate4()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_DUPLICATE4)
    {
        throw QDomXPathException_InternalError("INST_DUPLICATE4 not at the right location in the table of instructions");
    }
#endif
    int size(f_functions.back().f_stack.size());
    if(size <= 3)
    {
        throw QDomXPathException_EmptyStack("duplicate(4) cannot be used with a stack of less than 4 items");
    }
    variant_t value(f_functions.back().f_stack[size - 4]);
    f_functions.back().f_stack.push_back(value);
}


/** \brief Duplicate the fifth to last object on the stack.
 *
 * This instruction gets a copy of the fifth to last object on the stack
 * and push a copy on the stack.
 */
void inst_duplicate5()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_DUPLICATE5)
    {
        throw QDomXPathException_InternalError("INST_DUPLICATE5 not at the right location in the table of instructions");
    }
#endif
    int size(f_functions.back().f_stack.size());
    if(size <= 4)
    {
        throw QDomXPathException_EmptyStack("duplicate(5) cannot be used with a stack of less than 5 items");
    }
    variant_t value(f_functions.back().f_stack[size - 5]);
    f_functions.back().f_stack.push_back(value);
}


/** \brief Swap the last two objects on the stack.
 *
 * This instruction pops the last two objects and push them back in the other
 * order so the last value on the stack is the one before last and vice versa.
 *
 * \code
 * Source        Destination
 * stack a       stack b
 * stack b       stack a
 * stack c       stack c
 * stack d       stack d
 * ...           ...
 * \endcode
 */
void inst_swap1()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SWAP1)
    {
        throw QDomXPathException_InternalError("INST_SWAP1 not at the right location in the table of instructions");
    }
#endif
    int size(f_functions.back().f_stack.size());
    if(size < 2)
    {
        throw QDomXPathException_EmptyStack("swap(1) cannot be used with a stack of less than 2 items");
    }
    std::swap(f_functions.back().f_stack[size - 2], f_functions.back().f_stack[size - 1]);
    //variant_t a(f_functions.back().f_stack[size - 2]);
    //variant_t b(f_functions.back().f_stack[size - 1]);
    //f_functions.back().f_stack[size - 2] = b;
    //f_functions.back().f_stack[size - 1] = a;
}


/** \brief Swap the third to last object with the last object on the stack.
 *
 * This instruction swaps the third to last object with the last object on
 * the stack.
 *
 * \code
 * Source        Destination
 * stack a       stack c
 * stack b       stack b
 * stack c       stack a
 * stack d       stack d
 * ...           ...
 * \endcode
 */
void inst_swap2()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SWAP2)
    {
        throw QDomXPathException_InternalError("INST_SWAP2 not at the right location in the table of instructions");
    }
#endif
    int size(f_functions.back().f_stack.size());
    if(size < 3)
    {
        throw QDomXPathException_EmptyStack("swap(3) cannot be used with a stack of less than 3 items");
    }
    std::swap(f_functions.back().f_stack[size - 3], f_functions.back().f_stack[size - 1]);
}


/** \brief Swap the forth to last object with the last object on the stack.
 *
 * This instruction swaps the forth to last object with the last object on
 * the stack.
 *
 * \code
 * Source        Destination
 * stack a       stack d
 * stack b       stack b
 * stack c       stack c
 * stack d       stack a
 * stack e       stack e
 * ...           ...
 * \endcode
 */
void inst_swap3()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SWAP3)
    {
        throw QDomXPathException_InternalError("INST_SWAP3 not at the right location in the table of instructions");
    }
#endif
    int size(f_functions.back().f_stack.size());
    if(size < 4)
    {
        throw QDomXPathException_EmptyStack("swap(4) cannot be used with a stack of less than 4 items");
    }
    std::swap(f_functions.back().f_stack[size - 4], f_functions.back().f_stack[size - 1]);
}


/** \brief Swap the fifth to last object with the last object on the stack.
 *
 * This instruction swaps the fifth to last object with the last object on
 * the stack.
 *
 * \code
 * Source        Destination
 * stack a       stack e
 * stack b       stack b
 * stack c       stack c
 * stack d       stack d
 * stack e       stack a
 * stack f       stack e
 * ...           ...
 * \endcode
 */
void inst_swap4()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SWAP4)
    {
        throw QDomXPathException_InternalError("INST_SWAP4 not at the right location in the table of instructions");
    }
#endif
    int size(f_functions.back().f_stack.size());
    if(size < 5)
    {
        throw QDomXPathException_EmptyStack("swap(5) cannot be used with a stack of less than 5 items");
    }
    std::swap(f_functions.back().f_stack[size - 5], f_functions.back().f_stack[size - 1]);
}


/** \brief Swap the sixth to last object with the last object on the stack.
 *
 * This instruction swaps the sixth to last object with the last object on
 * the stack.
 *
 * \code
 * Source        Destination
 * stack a       stack f
 * stack b       stack b
 * stack c       stack c
 * stack d       stack d
 * stack e       stack e
 * stack f       stack a
 * stack g       stack g
 * ...           ...
 * \endcode
 */
void inst_swap5()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SWAP5)
    {
        throw QDomXPathException_InternalError("INST_SWAP5 not at the right location in the table of instructions");
    }
#endif
    int size(f_functions.back().f_stack.size());
    if(size < 6)
    {
        throw QDomXPathException_EmptyStack("swap(6) cannot be used with a stack of less than 6 items");
    }
    std::swap(f_functions.back().f_stack[size - 6], f_functions.back().f_stack[size - 1]);
}


/** \brief Swap the second object with the thrid object.
 *
 * This instruction swaps the second and third objects.
 *
 * \code
 * Source        Destination
 * stack a       stack a
 * stack b       stack c
 * stack c       stack b
 * stack d       stack d
 * ...           ...
 * \endcode
 */
void inst_swap2_3()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SWAP2_3)
    {
        throw QDomXPathException_InternalError("INST_SWAP2_3 not at the right location in the table of instructions");
    }
#endif
    const int size(f_functions.back().f_stack.size());
    if(size < 3)
    {
        throw QDomXPathException_EmptyStack("swap(2, 3) cannot be used with a stack of less than 3 items");
    }
    std::swap(f_functions.back().f_stack[size - 3], f_functions.back().f_stack[size - 2]);
}


/** \brief Push the special value: End of Arguments.
 *
 * This function pushes the End of Arguments special mark on the stack.
 * This special value is used to end the list of arguments. It is as fast
 * as having a counter and know how many arguments are defined to call
 * a function.
 */
void inst_push_end_of_arguments()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_END_OF_ARGUMENTS)
    {
        throw QDomXPathException_InternalError("INST_PUSH_END_OF_ARGUMENTS not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setEndOfArguments();
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push the empty node set.
 *
 * This function pushes an empty set of nodes on the stack.
 */
void inst_push_empty_node_set()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_EMPTY_NODE_SET)
    {
        throw QDomXPathException_InternalError("INST_PUSH_EMPTY_NODE_SET not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    QDomXPath::node_vector_t empty;
    value.setValue(empty);
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push the empty atomic set.
 *
 * This function pushes an empty set of atomic values on the stack.
 */
void inst_push_empty_set()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_EMPTY_SET)
    {
        throw QDomXPathException_InternalError("INST_PUSH_EMPTY_SET not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    atomic_vector_t empty;
    value.setValue(empty);
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push the "" string.
 *
 * This function pushes the "*" string on the stack.
 */
void inst_push_empty_string()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_EMPTY_STRING)
    {
        throw QDomXPathException_InternalError("INST_PUSH_EMPTY_STRING not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue("");
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push the "*" string.
 *
 * This function pushes the "*" string on the stack.
 */
void inst_push_any_string()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_ANY_STRING)
    {
        throw QDomXPathException_InternalError("INST_PUSH_ANY_STRING not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue("*");
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push a small string.
 *
 * This function pushes the string as defined by the push instruction.
 * This string is limited to 0 to 255 characters.
 */
void inst_push_small_string()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_SMALL_STRING)
    {
        throw QDomXPathException_InternalError("INST_PUSH_SMALL_STRING not at the right location in the table of instructions");
    }
#endif
    int length(static_cast<int>(get_next_program_byte()));
    variant_t value;
    value.atomic_value_t::setValue(QString::fromUtf8(reinterpret_cast<char *>(f_program.data() + f_functions.back().f_pc), length));
    f_functions.back().f_pc += length;
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push a medium string.
 *
 * This function pushes the string as defined by the push instruction.
 * This string is limited to 1 to 65535 characters (although strings
 * of 1 to 255 characters are considered Small Strings and this
 * PUSH would not apply to them.)
 */
void inst_push_medium_string()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_MEDIUM_STRING)
    {
        throw QDomXPathException_InternalError("INST_PUSH_MEDIUM_STRING not at the right location in the table of instructions");
    }
#endif
    int64_t length((static_cast<int>(get_next_program_byte()) << 8)
                  | static_cast<int>(get_next_program_byte()));
    variant_t value;
    value.atomic_value_t::setValue(QString::fromUtf8(reinterpret_cast<char *>(f_program.data() + f_functions.back().f_pc), static_cast<int>(length)));
    f_functions.back().f_pc += static_cast<uint32_t>(length);
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push a large string.
 *
 * This function pushes the string as defined by the push instruction.
 * This string is limited to 1 to 4 billion characters (although strings
 * of 1 to 255 characters are considered Small Strings, also strings of
 * 256 to 65535 characters are considered Medium Strings and this
 * PUSH would not apply to either one of those cases.)
 *
 * Note that if you have a string this big... You may have a problem in
 * that XPath.
 */
void inst_push_large_string()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_LARGE_STRING)
    {
        throw QDomXPathException_InternalError("INST_PUSH_LARGE_STRING not at the right location in the table of instructions");
    }
#endif
    int64_t length((static_cast<int>(get_next_program_byte()) << 24)
                 | (static_cast<int>(get_next_program_byte()) << 16)
                 | (static_cast<int>(get_next_program_byte()) << 8)
                 |  static_cast<int>(get_next_program_byte()));
    variant_t value;
    value.atomic_value_t::setValue(QString::fromUtf8(reinterpret_cast<char *>(f_program.data() + f_functions.back().f_pc), static_cast<int>(length)));
    f_functions.back().f_pc += static_cast<uint32_t>(length);
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push the zero integer on the stack.
 *
 * This function pushes the value zero on the stack.
 */
void inst_push_zero()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_ZERO)
    {
        throw QDomXPathException_InternalError("INST_PUSH_ZERO not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    int64_t zero(0);
    value.atomic_value_t::setValue(zero);
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push the Boolean true.
 *
 * This function pushes the Boolean true on the stack.
 */
void inst_push_true()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_TRUE)
    {
        throw QDomXPathException_InternalError("INST_PUSH_TRUE not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue(true);
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push the Boolean false.
 *
 * This function pushes the Boolean false on the stack.
 */
void inst_push_false()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_FALSE)
    {
        throw QDomXPathException_InternalError("INST_PUSH_FALSE not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue(false);
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push an integer on the stack from one byte.
 *
 * This function reads one byte from the program and pushes it on the stack.
 * The byte is taken as an unsigned integer (0 to 255).
 */
void inst_push_byte()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_BYTE)
    {
        throw QDomXPathException_InternalError("INST_PUSH_BYTE not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue(static_cast<int64_t>(get_next_program_byte()));
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push an integer on the stack from one negative byte.
 *
 * This function reads one byte from the program and pushes it on the stack.
 * The byte is taken as a negative integer (-256 to -1).
 */
void inst_push_negative_byte()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_NEGATIVE_BYTE)
    {
        throw QDomXPathException_InternalError("INST_PUSH_NEGATIVE_BYTE not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue(static_cast<int64_t>(get_next_program_byte() | 0xFFFFFFFFFFFFFF00LL));
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push an integer on the stack from two bytes.
 *
 * This function reads two bytes from the program and pushes them on the stack
 * as a short.
 *
 * The bytes are viewed as an unsigned integer (0 to 65535).
 */
void inst_push_short()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_SHORT)
    {
        throw QDomXPathException_InternalError("INST_PUSH_SHORT not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue((static_cast<int64_t>(get_next_program_byte()) << 8)
                                  | static_cast<int64_t>(get_next_program_byte()));
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push an integer on the stack from two negative bytes.
 *
 * This function reads two bytes from the program and pushes them on the stack
 * after making all the other bits 1's since the bytes are taken as a
 * negative integer (-65536 to -1).
 */
void inst_push_negative_short()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_NEGATIVE_SHORT)
    {
        throw QDomXPathException_InternalError("INST_PUSH_NEGATIVE_SHORT not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue((static_cast<int64_t>(get_next_program_byte() << 8)
                                  | static_cast<int64_t>(get_next_program_byte())
                                  | static_cast<int64_t>(0xFFFFFFFFFFFF0000LL)));
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push an integer on the stack from four bytes.
 *
 * This function reads four bytes from the program and pushes them on the stack
 * as a long.
 *
 * The bytes are viewed as an unsigned integer (0 to 0xFFFFFFFF).
 */
void inst_push_long()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_LONG)
    {
        throw QDomXPathException_InternalError("INST_PUSH_LONG not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue((static_cast<int64_t>(get_next_program_byte()) << 24)
                                 | (static_cast<int64_t>(get_next_program_byte()) << 16)
                                 | (static_cast<int64_t>(get_next_program_byte()) << 8)
                                 |  static_cast<int64_t>(get_next_program_byte()));
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push an integer on the stack from four negative bytes.
 *
 * This function reads four bytes from the program and pushes them on the stack
 * after making all the other bits 1's since the bytes are taken as a
 * negative integer (-0x100000000 to -1).
 */
void inst_push_negative_long()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_NEGATIVE_LONG)
    {
        throw QDomXPathException_InternalError("INST_PUSH_NEGATIVE_LONG not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue((static_cast<int64_t>(get_next_program_byte()) << 24)
                                 | (static_cast<int64_t>(get_next_program_byte()) << 16)
                                 | (static_cast<int64_t>(get_next_program_byte()) << 8)
                                 |  static_cast<int64_t>(get_next_program_byte())
                                 |  static_cast<int64_t>(0xFFFFFFFF00000000LL));
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push an integer on the stack from eight bytes.
 *
 * This function reads eight bytes from the program and pushes them on the
 * stack as a long long.
 */
void inst_push_longlong()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_LONGLONG)
    {
        throw QDomXPathException_InternalError("INST_PUSH_LONGLONG not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    value.atomic_value_t::setValue((static_cast<int64_t>(get_next_program_byte()) << 56)
                                 | (static_cast<int64_t>(get_next_program_byte()) << 48)
                                 | (static_cast<int64_t>(get_next_program_byte()) << 40)
                                 | (static_cast<int64_t>(get_next_program_byte()) << 32)
                                 | (static_cast<int64_t>(get_next_program_byte()) << 24)
                                 | (static_cast<int64_t>(get_next_program_byte()) << 16)
                                 | (static_cast<int64_t>(get_next_program_byte()) << 8)
                                 |  static_cast<int64_t>(get_next_program_byte()));
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push a Double on the stack from eight bytes.
 *
 * This function reads eight bytes from the program and pushes them on the
 * stack as a double.
 */
void inst_push_double()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_DOUBLE)
    {
        throw QDomXPathException_InternalError("INST_PUSH_DOUBLE not at the right location in the table of instructions");
    }
#endif
    union
    {
        uint64_t    f_int;
        double      f_dbl;
    } convert;
    convert.f_int = (static_cast<int64_t>(get_next_program_byte()) << 56)
                  | (static_cast<int64_t>(get_next_program_byte()) << 48)
                  | (static_cast<int64_t>(get_next_program_byte()) << 40)
                  | (static_cast<int64_t>(get_next_program_byte()) << 32)
                  | (static_cast<int64_t>(get_next_program_byte()) << 24)
                  | (static_cast<int64_t>(get_next_program_byte()) << 16)
                  | (static_cast<int64_t>(get_next_program_byte()) << 8)
                  |  static_cast<int64_t>(get_next_program_byte());
    variant_t value;
    value.atomic_value_t::setValue(convert.f_dbl);
    f_functions.back().f_stack.push_back(value);
}

/** \brief Push a Double zero on the stack.
 *
 * This function pushes a double on the stack. The double is set to 0.0.
 */
void inst_push_double_zero()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PUSH_DOUBLE_ZERO)
    {
        throw QDomXPathException_InternalError("INST_PUSH_DOUBLE_ZERO not at the right location in the table of instructions");
    }
#endif
    variant_t value;
    double zero(0.0);
    value.atomic_value_t::setValue(zero);
    f_functions.back().f_stack.push_back(value);
}


/** \brief Add one to the number at the top of the stack.
 *
 * This function pops one integer, adds one to it, and push it back.
 * Note that one is added even to single and double numbers, and not
 * epsilon. This means really large numbers will not change.
 *
 * Only integers, decimal, single, and doubles can be added between
 * each others.
 */
void inst_increment()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_INCREMENT)
    {
        throw QDomXPathException_InternalError("INST_INCREMENT not at the right location in the table of instructions");
    }
#endif
    variant_t value(pop_variant_data());
    switch(value.getType())
    {
    case atomic_value_t::type_t::ATOMIC_TYPE_INTEGER:
        value.atomic_value_t::setValue(value.getIntegerValue() + 1);
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_SINGLE:
        value.atomic_value_t::setValue(value.getSingleValue(true) + 1.0f);
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE:
        value.atomic_value_t::setValue(value.getDoubleValue(true) + 1.0);
        break;

    default:
        throw QDomXPathException_WrongType("the '++' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(value);
}


/** \brief Subtract one to the number at the top of the stack.
 *
 * This function pops one integer, subtracts one to it, and push it back.
 * Note that one is subtracted even to single and double numbers, and not
 * epsilon. This means really large numbers will not change.
 *
 * Only integers, decimal, single, and doubles can be added between
 * each others.
 */
void inst_decrement()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_DECREMENT)
    {
        throw QDomXPathException_InternalError("INST_DECREMENT not at the right location in the table of instructions");
    }
#endif
    variant_t value(pop_variant_data());
    switch(value.getType())
    {
    case atomic_value_t::type_t::ATOMIC_TYPE_INTEGER:
        value.atomic_value_t::setValue(value.getIntegerValue() + 1);
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_SINGLE:
        value.atomic_value_t::setValue(value.getSingleValue(true) + 1);
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE:
        value.atomic_value_t::setValue(value.getDoubleValue(true) + 1);
        break;

    default:
        throw QDomXPathException_WrongType("the '--' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(value);
}


/** \brief Return the length of a string in characters.
 *
 * This function pops a string from the stack, computes its length in
 * characters, and pushes that length back on the stack.
 */
void inst_string_length()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_STRING_LENGTH)
    {
        throw QDomXPathException_InternalError("INST_STRING_LENGTH not at the right location in the table of instructions");
    }
#endif
    variant_t value(pop_variant_data());
    if(value.getType() != atomic_value_t::type_t::ATOMIC_TYPE_STRING)
    {
        throw QDomXPathException_WrongType("the string-length() function only accepts strings");
    }
    value.atomic_value_t::setValue(static_cast<int64_t>(value.getStringValue().length()));

    f_functions.back().f_stack.push_back(value);
}


/** \brief Compute the ceiling of a number.
 *
 * This function pops one number. If the number is an integer, then it pushes
 * it back as is. If the number if a Decimal, a Single, or a Double, it
 * checks the decimal digits. If all are zeroes, then the same value is
 * pushed back on the stack. Otherwise it resets all the decimal digits
 * adds one and then push the result back on the stack as the same type.
 *
 * \note
 * This function does not yet handle NaN numbers.
 *
 * \note
 * The value is NOT transformed to an integer. The cast needs to be used
 * for that purpose.
 */
void inst_ceiling()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_CEILING)
    {
        throw QDomXPathException_InternalError("INST_CEILING not at the right location in the table of instructions");
    }
#endif
    variant_t value(pop_variant_data());
    switch(value.getType())
    {
    case atomic_value_t::type_t::ATOMIC_TYPE_INTEGER:
        // nothing happens
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_SINGLE:
        value.atomic_value_t::setValue(std::ceil(value.getSingleValue()));
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE:
        value.atomic_value_t::setValue(std::ceil(value.getDoubleValue()));
        break;

    default:
        throw QDomXPathException_WrongType("the ceiling() function can only be applied against numbers");

    }

    f_functions.back().f_stack.push_back(value);
}


/** \brief Compute the floor of a number.
 *
 * This function pops one number. If the number is an integer, then it pushes
 * it back as is. If the number if a Decimal, a Single, or a Double, it
 * sets all of its decimal digits to zero and pushes that back on the
 * same as the same type.
 *
 * \note
 * This function does not yet handle NaN numbers.
 *
 * \note
 * The value is NOT transformed to an integer. The cast needs to be used
 * for that purpose.
 */
void inst_floor()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_FLOOR)
    {
        throw QDomXPathException_InternalError("INST_FLOOR not at the right location in the table of instructions");
    }
#endif
    variant_t value(pop_variant_data());
    switch(value.getType())
    {
    case atomic_value_t::type_t::ATOMIC_TYPE_INTEGER:
        // nothing happens
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_SINGLE:
        value.atomic_value_t::setValue(std::floor(value.getSingleValue()));
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE:
        value.atomic_value_t::setValue(std::floor(value.getDoubleValue()));
        break;

    default:
        throw QDomXPathException_WrongType("the floor() function can only be applied against numbers");

    }

    f_functions.back().f_stack.push_back(value);
}


/** \brief Compute the rounded value of a number.
 *
 * This function pops one number. If the number is an integer, then it pushes
 * it back as is. If the number if a Decimal, a Single, or a Double, then
 * the function adds 0.5 and then it computes the floor() (the floor
 * sets all the decimal digits of the result to zeroes.)
 *
 * The result is pushed back on the stack as the same type as the input.
 *
 * \note
 * Numbers such as -0.8 are expected to return -0.0 which this function
 * does not do.
 *
 * \note
 * This function does not yet handle NaN numbers.
 *
 * \note
 * The value is NOT transformed to an integer. The cast needs to be used
 * for that purpose.
 */
void inst_round()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_ROUND)
    {
        throw QDomXPathException_InternalError("INST_ROUND not at the right location in the table of instructions");
    }
#endif
    variant_t value(pop_variant_data());
    switch(value.getType())
    {
    case atomic_value_t::type_t::ATOMIC_TYPE_INTEGER:
        // nothing happens
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_SINGLE:
        value.atomic_value_t::setValue(std::floor(value.getSingleValue() + 0.5f));
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE:
        value.atomic_value_t::setValue(std::floor(value.getDoubleValue() + 0.5));
        break;

    default:
        throw QDomXPathException_WrongType("the round() function can only be applied against numbers");

    }

    f_functions.back().f_stack.push_back(value);
}


/** \brief Add two numbers together.
 *
 * This function adds two values that it first pops from the stack.
 * The add differs slightly depending on the type of each value.
 *
 * Only integers, decimal, single, and doubles can be added between
 * each others.
 */
void inst_add()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_ADD)
    {
        throw QDomXPathException_InternalError("INST_ADD not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.getIntegerValue() + rhs.getIntegerValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(lhs.getSingleValue(true) + rhs.getSingleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(lhs.getDoubleValue(true) + rhs.getDoubleValue(true));
        break;

    default:
        throw QDomXPathException_WrongType("the '+' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Subtract two numbers from each others.
 *
 * This function subtract two values that it first pops from the stack.
 * The subtract differs slightly depending on the type of each value.
 *
 * Only integers, decimal, single, and doubles can be subtracted from
 * each others.
 */
void inst_subtract()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SUBTRACT)
    {
        throw QDomXPathException_InternalError("INST_SUBTRACT not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.getIntegerValue() - rhs.getIntegerValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(lhs.getSingleValue(true) - rhs.getSingleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(lhs.getDoubleValue(true) - rhs.getDoubleValue(true));
        break;

    default:
        throw QDomXPathException_WrongType("the '-' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Negate a number.
 *
 * This function pops a number negates it and push it back on the stack.
 *
 * Only integers, decimal, single, and doubles can be negated from
 * each others.
 */
void inst_negate()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_NEGATE)
    {
        throw QDomXPathException_InternalError("INST_NEGATE not at the right location in the table of instructions");
    }
#endif
    variant_t value(pop_variant_data());
    variant_t result;
    switch(value.getType())
    {
    case atomic_value_t::type_t::ATOMIC_TYPE_INTEGER:
        result.atomic_value_t::setValue(-value.getIntegerValue());
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_SINGLE:
        result.atomic_value_t::setValue(-value.getSingleValue());
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE:
        result.atomic_value_t::setValue(-value.getDoubleValue());
        break;

    default:
        throw QDomXPathException_WrongType("the '-' operator cannot be used with this value type");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Multiply two numbers from each others.
 *
 * This function multiplies two values that it first pops from the stack.
 * The multiply differs slightly depending on the type of each value.
 *
 * Only integers, decimal, single, and doubles can be subtracted from
 * each others.
 */
void inst_multiply()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_MULTIPLY)
    {
        throw QDomXPathException_InternalError("INST_MULTIPLY not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.getIntegerValue() * rhs.getIntegerValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(lhs.getSingleValue(true) * rhs.getSingleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(lhs.getDoubleValue(true) * rhs.getDoubleValue(true));
        break;

    default:
        throw QDomXPathException_WrongType("the '*' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Divide two numbers from each others.
 *
 * This function divides two values that it first pops from the stack.
 * The divide differs slightly depending on the type of each value.
 * Note that dividing two integers generates a double as the result.
 *
 * Only integers, decimal, single, and doubles can be subtracted from
 * each others.
 */
void inst_divide()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_DIVIDE)
    {
        throw QDomXPathException_InternalError("INST_DIVIDE not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.getDoubleValue(true) / rhs.getDoubleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(lhs.getSingleValue(true) / rhs.getSingleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(lhs.getDoubleValue(true) / rhs.getDoubleValue(true));
        break;

    default:
        throw QDomXPathException_WrongType("the 'div' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Divide two numbers as integers.
 *
 * This function divides two values that it first pops from the stack.
 * The divide transforms the left and right hand side numbers to integers
 * before performing the division and the result is always an integer.
 *
 * Only integers, decimal, single, and doubles can be subtracted from
 * each others.
 */
void inst_idivide()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_IDIVIDE)
    {
        throw QDomXPathException_InternalError("INST_IDIVIDE not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    int64_t const right_value(rhs.getIntegerValue(true));
    if(right_value == 0)
    {
        throw QDomXPathException_DivisionByZero("the 'idiv' operator cannot be used with the left and right hand side types");
    }
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(lhs.getIntegerValue(true) / right_value);
        break;

    default:
        throw QDomXPathException_WrongType("the 'idiv' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Compute the modulo of two numbers from each others.
 *
 * This function computes the modulo of two number that it first pops
 * from the stack. The modulo differs slightly depending on the type of
 * each value.
 *
 * Only integers, decimal, single, and doubles can be subtracted from
 * each others.
 */
void inst_modulo()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_MODULO)
    {
        throw QDomXPathException_InternalError("INST_MODULO not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.getIntegerValue() % rhs.getIntegerValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(std::fmod(lhs.getSingleValue(true), rhs.getSingleValue(true)));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(std::fmod(lhs.getDoubleValue(true), rhs.getDoubleValue(true)));
        break;

    default:
        throw QDomXPathException_WrongType("the 'mod' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Inverse a Boolean.
 *
 * This function pops a Boolean value and returns the opposite Boolean
 * (so true becomes false and vice versa.)
 *
 * Only Boolean are accepted by this operator.
 */
void inst_not()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_NOT)
    {
        throw QDomXPathException_InternalError("INST_NOT not at the right location in the table of instructions");
    }
#endif
    variant_t boolean(pop_variant_data());
    if(boolean.getType() != atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN)
    {
        throw QDomXPathException_WrongType("the Not operator can only be applied against a Boolean value");
    }
    boolean.atomic_value_t::setValue(!boolean.atomic_value_t::getBooleanValue());

    f_functions.back().f_stack.push_back(boolean);
}


/** \brief Compute the multiplication of two Booleans.
 *
 * This function pops two Boolean values and returns the AND operation
 * (i.e. Boolean multiplication) so if both sides are true, it pushes
 * true back on the stack. In all other cases it pushes false.
 *
 * Only Boolean are accepted by this operator.
 */
void inst_and()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_AND)
    {
        throw QDomXPathException_InternalError("INST_AND not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    if(lhs.getType() != atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN
    || rhs.getType() != atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN)
    {
        throw QDomXPathException_WrongType("the And operator can only be applied against Boolean values");
    }
    variant_t result;
    result.atomic_value_t::setValue(lhs.atomic_value_t::getBooleanValue() && rhs.atomic_value_t::getBooleanValue());

    f_functions.back().f_stack.push_back(result);
}


/** \brief Compute the addition of two Booleans.
 *
 * This function pops two Boolean values and returns the OR operation
 * (i.e. Boolean addition) so if either sides is true, it pushes
 * true back on the stack. In all other cases it pushes false.
 *
 * Only Boolean are accepted by this operator.
 */
void inst_or()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_OR)
    {
        throw QDomXPathException_InternalError("INST_OR not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    if(lhs.getType() != atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN
    || rhs.getType() != atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN)
    {
        throw QDomXPathException_WrongType("the Or operator can only be applied against Boolean values");
    }
    variant_t result;
    result.atomic_value_t::setValue(lhs.atomic_value_t::getBooleanValue() || rhs.atomic_value_t::getBooleanValue());

    f_functions.back().f_stack.push_back(result);
}


/** \brief Check whether two values are equal.
 *
 * This function compares the two top-most values from the stack.
 * The result is pushed back on the stack.
 *
 * The comparison process depends on the type of data being compared.
 */
void inst_equal()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_EQUAL)
    {
        throw QDomXPathException_InternalError("INST_EQUAL not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
        result.atomic_value_t::setValue(lhs.atomic_value_t::getBooleanValue() == rhs.atomic_value_t::getBooleanValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.atomic_value_t::getIntegerValue() == rhs.atomic_value_t::getIntegerValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(snap::compare_floats(lhs.atomic_value_t::getSingleValue(true), rhs.atomic_value_t::getSingleValue(true)));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(snap::compare_floats(lhs.atomic_value_t::getDoubleValue(true), rhs.atomic_value_t::getDoubleValue(true)));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING,   atomic_value_t::type_t::ATOMIC_TYPE_STRING):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET, atomic_value_t::type_t::ATOMIC_TYPE_STRING):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING,   atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET, atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET):
        try
        {
            result.atomic_value_t::setValue(lhs.getStringValue(true) == rhs.getStringValue(true));
        }
        catch(QDomXPathException_NotImplemented const&)
        {
            result.atomic_value_t::setValue(false);
        }
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL, atomic_value_t::type_t::ATOMIC_TYPE_NULL):
        result.atomic_value_t::setValue(true);
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL,    atomic_value_t::type_t::ATOMIC_TYPE_STRING):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL,    atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL,    atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL,    atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL,    atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING,  atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING,  atomic_value_t::type_t::ATOMIC_TYPE_NULL):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_NULL):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_NULL):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_NULL):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_NULL):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_STRING):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(false);
        break;

    default:
        throw QDomXPathException_WrongType("the '=' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Check whether two values are not equal.
 *
 * This function compares the two top-most values from the stack.
 * The result is pushed back on the stack.
 *
 * Note that not-equal is not the same as not(equal). Some data may be at
 * the same time equal and not equal or neither.
 */
void inst_not_equal()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_NOT_EQUAL)
    {
        throw QDomXPathException_InternalError("INST_NOT_EQUAL not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
        result.atomic_value_t::setValue(lhs.getBooleanValue() != rhs.getBooleanValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.getIntegerValue() != rhs.getIntegerValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(snap::compare_floats(lhs.getSingleValue(true), rhs.getSingleValue(true)));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(snap::compare_floats(lhs.getDoubleValue(true), rhs.getDoubleValue(true)));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING, atomic_value_t::type_t::ATOMIC_TYPE_STRING):
        result.atomic_value_t::setValue(lhs.getStringValue() != rhs.getStringValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL, atomic_value_t::type_t::ATOMIC_TYPE_NULL):
        result.atomic_value_t::setValue(false);
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL,    atomic_value_t::type_t::ATOMIC_TYPE_STRING):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL,    atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL,    atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL,    atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL,    atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING , atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING,  atomic_value_t::type_t::ATOMIC_TYPE_NULL):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_NULL):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_NULL):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_NULL):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_NULL):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_STRING):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(true);
        break;

    default:
        throw QDomXPathException_WrongType("the '!=' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Check whether two values are ordered.
 *
 * This function compares the two top-most values on the stack.
 * The result is pushed back on the stack.
 */
void inst_less_than()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_LESS_THAN)
    {
        throw QDomXPathException_InternalError("INST_LESS_THAN not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
        result.atomic_value_t::setValue(lhs.getBooleanValue() < rhs.getBooleanValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.getIntegerValue() < rhs.getIntegerValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(lhs.getSingleValue(true) < rhs.getSingleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(lhs.getDoubleValue(true) < rhs.getDoubleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING, atomic_value_t::type_t::ATOMIC_TYPE_STRING):
        result.atomic_value_t::setValue(lhs.getStringValue() < rhs.getStringValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL, atomic_value_t::type_t::ATOMIC_TYPE_NULL):
        result.atomic_value_t::setValue(false);
        break;

    default:
        throw QDomXPathException_WrongType("the '<' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Check whether two values are ordered.
 *
 * This function compares the two top-most values on the stack.
 * The result is pushed back on the stack.
 */
void inst_less_or_equal()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_LESS_OR_EQUAL)
    {
        throw QDomXPathException_InternalError("INST_LESS_OR_EQUAL not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
        result.atomic_value_t::setValue(lhs.getBooleanValue() <= rhs.getBooleanValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.getIntegerValue() <= rhs.getIntegerValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(lhs.getSingleValue(true) <= rhs.getSingleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(lhs.getDoubleValue(true) <= rhs.getDoubleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING, atomic_value_t::type_t::ATOMIC_TYPE_STRING):
        result.atomic_value_t::setValue(lhs.getStringValue() <= rhs.getStringValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL, atomic_value_t::type_t::ATOMIC_TYPE_NULL):
        result.atomic_value_t::setValue(true);
        break;

    default:
        throw QDomXPathException_WrongType("the '<=' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Check whether two values are ordered.
 *
 * This function compares the two top-most values on the stack.
 * The result is pushed back on the stack.
 */
void inst_greater_than()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_GREATER_THAN)
    {
        throw QDomXPathException_InternalError("INST_GREATER_THAN not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
        result.atomic_value_t::setValue(lhs.getBooleanValue() > rhs.getBooleanValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.getIntegerValue() > rhs.getIntegerValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(lhs.getSingleValue(true) > rhs.getSingleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(lhs.getDoubleValue(true) > rhs.getDoubleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING, atomic_value_t::type_t::ATOMIC_TYPE_STRING):
        result.atomic_value_t::setValue(lhs.getStringValue() > rhs.getStringValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL, atomic_value_t::type_t::ATOMIC_TYPE_NULL):
        result.atomic_value_t::setValue(false);
        break;

    default:
        throw QDomXPathException_WrongType("the '>' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Check whether two values are ordered.
 *
 * This function compares the two top-most values on the stack.
 * The result is pushed back on the stack.
 */
void inst_greater_or_equal()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_GREATER_OR_EQUAL)
    {
        throw QDomXPathException_InternalError("INST_GREATER_OR_EQUAL not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    variant_t result;
    switch(merge_types(lhs.getType(), rhs.getType()))
    {
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN, atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN):
        result.atomic_value_t::setValue(lhs.getBooleanValue() >= rhs.getBooleanValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
        result.atomic_value_t::setValue(lhs.getIntegerValue() >= rhs.getIntegerValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
        result.atomic_value_t::setValue(lhs.getSingleValue(true) >= rhs.getSingleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_INTEGER):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_INTEGER, atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_SINGLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_SINGLE):
    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE,  atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE):
        result.atomic_value_t::setValue(lhs.getDoubleValue(true) >= rhs.getDoubleValue(true));
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_STRING, atomic_value_t::type_t::ATOMIC_TYPE_STRING):
        result.atomic_value_t::setValue(lhs.getStringValue() >= rhs.getStringValue());
        break;

    case merge_types(atomic_value_t::type_t::ATOMIC_TYPE_NULL, atomic_value_t::type_t::ATOMIC_TYPE_NULL):
        result.atomic_value_t::setValue(true);
        break;

    default:
        throw QDomXPathException_WrongType("the '>=' operator cannot be used with the left and right hand side types");

    }
    f_functions.back().f_stack.push_back(result);
}


/** \brief Get node-set size.
 *
 * This function pushes the size of the node-set on the stack.
 */
void inst_node_set_size()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_NODE_SET_SIZE)
    {
        throw QDomXPathException_InternalError("INST_NODE_SET_SIZE not at the right location in the table of instructions");
    }
#endif
    variant_t value(pop_variant_data());
    QDomXPath::node_vector_t node_set(value.getNodeSetValue());
    variant_t result;
    result.atomic_value_t::setValue(static_cast<int64_t>(node_set.size()));
    f_functions.back().f_stack.push_back(result);
}


/** \brief Append two node-sets or atomic sets together.
 *
 * This function pops two sets from the stack, append one to the other,
 * and pushes the result back on the stack.
 */
void inst_merge_sets()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_MERGE_SETS)
    {
        throw QDomXPathException_InternalError("INST_MERGE_SETS not at the right location in the table of instructions");
    }
#endif
    variant_t rhs(pop_variant_data());
    variant_t lhs(pop_variant_data());
    if(rhs.getType() == atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET
    && lhs.getType() == atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET)
    {
        QDomXPath::node_vector_t l(lhs.getNodeSetValue());
        QDomXPath::node_vector_t r(rhs.getNodeSetValue());
        const int imax(r.size());
        for(int i(0); i < imax; ++i)
        {
            l.push_back(r[i]);
        }
        variant_t result;
        result.setValue(l);
        f_functions.back().f_stack.push_back(result);
    }
    else if(rhs.getType() == atomic_value_t::type_t::ATOMIC_TYPE_SET
         && lhs.getType() == atomic_value_t::type_t::ATOMIC_TYPE_SET)
    {
        atomic_vector_t l(lhs.getSetValue());
        atomic_vector_t r(rhs.getSetValue());
        const int imax(r.size());
        for(int i(0); i < imax; ++i)
        {
            l.push_back(r[i]);
        }
        variant_t result;
        result.setValue(l);
        f_functions.back().f_stack.push_back(result);
    }
    else
    {
        throw QDomXPathException_WrongType("the 'union' operator cannot be used with anything else than node sets at this point");
    }
}


/** \brief Retrieve the current position.
 *
 * This function pushes the current position of the context node
 * on the stack.
 */
void inst_get_position()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_GET_POSITION)
    {
        throw QDomXPathException_InternalError("INST_GET_POSITION not at the right location in the table of instructions");
    }
#endif
    contexts_not_empty();
    variant_t result;
    int64_t position(static_cast<int64_t>(f_functions.back().f_contexts.back().f_position));
    result.atomic_value_t::setValue(position + 1);
    f_functions.back().f_stack.push_back(result);
}


/** \brief Set the position.
 *
 * This internal function is used to set the current position. Internally
 * we use those to go through the whole list of nodes in a node-set.
 */
void inst_set_position()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SET_POSITION)
    {
        throw QDomXPathException_InternalError("INST_SET_POSITION not at the right location in the table of instructions");
    }
#endif
    variant_t position(pop_variant_data());
    if(position.getType() != atomic_value_t::type_t::ATOMIC_TYPE_INTEGER)
    {
        throw QDomXPathException_WrongType("the 'set_position' operator cannot be used with anything else than an integer as its first operand");
    }
    contexts_not_empty();
    int p(static_cast<int>(position.getIntegerValue()));
    if(p < 1 || p > f_functions.back().f_contexts.back().f_nodes.size())
    {
        throw QDomXPathException_OutOfRange("the new position in 'set_position' is out of range");
    }
    // within XPath position is 1 based
    f_functions.back().f_contexts.back().f_position = p - 1;
}


/** \brief Get the node-set on the stack.
 *
 * This function retrieves the current node-set from this state.
 */
void inst_get_node_set()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_GET_NODE_SET)
    {
        throw QDomXPathException_InternalError("INST_GET_NODE_SET not at the right location in the table of instructions");
    }
#endif
    contexts_not_empty();
    variant_t result;
    result.setValue(f_functions.back().f_contexts.back().f_nodes);
    f_functions.back().f_stack.push_back(result);
}


/** \brief Set the node-set from the stack.
 *
 * This function saves the stack node-set as the new node-set of this state.
 */
void inst_set_node_set()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SET_NODE_SET)
    {
        throw QDomXPathException_InternalError("INST_SET_NODE_SET not at the right location in the table of instructions");
    }
#endif
    variant_t node_set(pop_variant_data());
    if(node_set.getType() != atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET)
    {
        throw QDomXPathException_WrongType("the 'set_node_set' operator cannot be used with anything else than a node-set");
    }
    contexts_not_empty();
    f_functions.back().f_contexts.back().f_nodes = node_set.getNodeSetValue();
}


/** \brief Get the result on the stack.
 *
 * This function retrieves the current result on this state.
 */
void inst_get_result()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_GET_RESULT)
    {
        throw QDomXPathException_InternalError("INST_GET_RESULT not at the right location in the table of instructions");
    }
#endif
    contexts_not_empty();
    variant_t result;
    result.setValue(f_functions.back().f_contexts.back().f_result);
    f_functions.back().f_stack.push_back(result);
}


/** \brief Set the result on the stack.
 *
 * This function saves the stack node-set as the new result of this state.
 */
void inst_set_result()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_SET_RESULT)
    {
        throw QDomXPathException_InternalError("INST_SET_RESULT not at the right location in the table of instructions");
    }
#endif
    variant_t result(pop_variant_data());
    if(result.getType() != atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET)
    {
        throw QDomXPathException_WrongType("the 'set_result' operator cannot be used with anything else than a node-set");
    }
    contexts_not_empty();
    f_functions.back().f_contexts.back().f_result = result.getNodeSetValue();
}


/** \brief Replace the node-set with the root node.
 *
 * This function clears the existing node-set and replace it with one
 * node: the root node.
 */
void inst_root()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_ROOT)
    {
        throw QDomXPathException_InternalError("INST_ROOT not at the right location in the table of instructions");
    }
#endif
    // if empty then it stays that way
    stack_not_empty(atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET);
    QDomXPath::node_vector_t& node_set(f_functions.back().f_stack.back().getNodeSetValue());
    if(!node_set.isEmpty())
    {
        QDomNode root(node_set[0].ownerDocument());
        if(root.isElement())
        {
            // this happens when the node we start with is an attribute
            root = root.ownerDocument();
        }
        node_set.clear();
        if(!root.isNull())
        {
            // ignore null nodes and keep our list empty of them
            node_set.push_back(root);
        }
    }
}


/** \brief Check the current result as the predicate result.
 *
 * This function checks the last result on the stack as the result of
 * a predicate. If true, the current node is added to the result set.
 */
void inst_predicate()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_PREDICATE)
    {
        throw QDomXPathException_InternalError("INST_PREDICATE not at the right location in the table of instructions");
    }
#endif
    bool result(false);
    variant_t predicate_result(pop_variant_data());
    switch(predicate_result.getType())
    {
    case atomic_value_t::type_t::ATOMIC_TYPE_BOOLEAN:
    case atomic_value_t::type_t::ATOMIC_TYPE_STRING:
        // true or false
        result = predicate_result.getBooleanValue(true);
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_INTEGER:
    case atomic_value_t::type_t::ATOMIC_TYPE_SINGLE:
    case atomic_value_t::type_t::ATOMIC_TYPE_DOUBLE:
        contexts_not_empty();
        result = predicate_result.getIntegerValue(true) == f_functions.back().f_contexts.back().f_position + 1;
        break;

    case atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET:
        {
            // for a node set, it is considered true only if not empty
            QDomXPath::node_vector_t r(predicate_result.getNodeSetValue());
            result = r.size() != 0;
        }
        break;

    default:
        // anything else always returns false
        //throw QDomXPathException_WrongType(QString("the 'predicate' operator cannot be used with anything else than a node-set (got node of type %1)").arg(static_cast<int>(predicate_result.getType())));
        break;

    }

    context_vector_t::reference context(f_functions.back().f_contexts.back());
    if(context.f_position != -1
    && result)
    {
        context.f_result.push_back(context.f_nodes[context.f_position]);
    }

    // if +1 to position is too large, just return false
    const bool has_another_position(context.f_position + 1 < context.f_nodes.size());
    if(has_another_position)
    {
        ++context.f_position;
    }

    variant_t value;
    value.atomic_value_t::setValue(has_another_position);
    f_functions.back().f_stack.push_back(value);
}


void inst_create_node_context()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_CREATE_NODE_CONTEXT)
    {
        throw QDomXPathException_InternalError("INST_CREATE_NODE_CONTEXT not at the right location in the table of instructions");
    }
#endif
    variant_t node_set(pop_variant_data());
    if(node_set.getType() != atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET)
    {
        throw QDomXPathException_WrongType("a node set is required to create a node context");
    }
    context_t context;
    context.f_nodes = node_set.getNodeSetValue();
    context.f_position = context.f_nodes.empty() ? -1 : 0;
    f_functions.back().f_contexts.push_back(context);
}


void inst_next_context_node()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_NEXT_CONTEXT_NODE)
    {
        throw QDomXPathException_InternalError("INST_NEXT_CONTEXT_NODE not at the right location in the table of instructions");
    }
#endif
    contexts_not_empty();

    variant_t expr_result(pop_variant_data());
    if(expr_result.getType() != atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET)
    {
        throw QDomXPathException_WrongType("the 'next_context_node' operation expected the input to be a node-set");
    }

    context_vector_t::reference context(f_functions.back().f_contexts.back());

    QDomXPath::node_vector_t r(expr_result.getNodeSetValue());
    QDomXPath::node_vector_t& context_result(context.f_result);
    const int imax(r.size());
    for(int i(0); i < imax; ++i)
    {
        context_result.push_back(r[i]);
    }

    // if +1 to position is too large, just return false
    const bool has_another_position(context.f_position + 1 < context.f_nodes.size());
    if(has_another_position)
    {
        ++context.f_position;
    }

    variant_t value;
    value.atomic_value_t::setValue(has_another_position);
    f_functions.back().f_stack.push_back(value);
}


void inst_pop_context()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_POP_CONTEXT)
    {
        throw QDomXPathException_InternalError("INST_POP_CONTEXT not at the right location in the table of instructions");
    }
#endif
    contexts_not_empty();
    f_functions.back().f_contexts.pop_back();
}


void inst_get_context_node()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_GET_CONTEXT_NODE)
    {
        throw QDomXPathException_InternalError("INST_GET_CONTEXT_NODE not at the right location in the table of instructions");
    }
#endif
    contexts_not_empty();
    const context_t context(f_functions.back().f_contexts.back());
    QDomXPath::node_vector_t node_set;
    if(context.f_position != -1)
    {
        node_set.push_back(context.f_nodes[context.f_position]);
    }
    variant_t result;
    result.setValue(node_set);
    f_functions.back().f_stack.push_back(result);
}


/** \brief Compute an axis.
 *
 * This function computes a list of nodes as specified by the axis
 * parameters.
 *
 * In some cases this can result in a very large number of nodes getting
 * pushed around!
 */
void inst_axis()
{
#if QDOM_XPATH_VERIFICATION
    // verify instruction location
    if(f_program[f_functions.back().f_pc - 1] != INST_AXIS)
    {
        throw QDomXPathException_InternalError("INST_AXIS not at the right location in the table of instructions");
    }
#endif
    // the first parameter is the axis (enum axis_t)
    variant_t axis_variant(pop_variant_data());
    axis_t axis(static_cast<axis_t>(axis_variant.getIntegerValue()));

    // the next parameter is the prefix or the node type parameter
    // (in case of a processing-instruction node)
    variant_t prefix_or_processing_language(pop_variant_data());
    QString prefix;
    QString processing_language;

    // the next parameter is the local_part (i.e. path entry)
    // or the node type enumeration (enum node_type_t)
    variant_t local_part_or_node_type(pop_variant_data());
    QString local_part;
    node_type_t node_type(node_type_t::NODE_TYPE_ELEMENT);

    // the last parameter is the context node which we cannot pop because
    // it is required to run through all the nodes, we only make use of the
    // context node which is the one we're interested in
    variant_t context_node_variant(pop_variant_data());
    if(context_node_variant.getType() != atomic_value_t::type_t::ATOMIC_TYPE_NODE_SET)
    {
        throw QDomXPathException_WrongType("the 4th axis parameters must be a node-set");
    }
    //stack_not_empty(atomic_value_t::type_t::ATOMIC_TYPE_CONTEXT);
    //context_t& context(f_functions.back().f_stack.back().getContextValue());
    //variant_t context_variant(pop_variant_data());
    //context_t& context(context_variant.getContextValue());

    if(local_part_or_node_type.getType() == atomic_value_t::type_t::ATOMIC_TYPE_INTEGER)
    {
        // we have a node type, thus if not empty the
        // prefix_or_processing_language variable is the
        // language name to match
        if(axis == axis_t::AXIS_ATTRIBUTE || axis == axis_t::AXIS_NAMESPACE)
        {
            // Note: XPath 2.0 has an 'attribute' NodeType (KindTest) which
            //       means attribute axis would be fine here
            throw QDomXPathException_WrongType("attribute and namespace axis are not compatible with a node type");
        }
        node_type = static_cast<node_type_t>(local_part_or_node_type.getIntegerValue());
        processing_language = prefix_or_processing_language.getStringValue();
    }
    else
    {
        // we got a prefix + local_name
        prefix = prefix_or_processing_language.getStringValue();
        local_part = local_part_or_node_type.getStringValue();
        if(local_part == "*")
        {
            local_part = "";
        }

        if(axis == axis_t::AXIS_ATTRIBUTE)
        {
            // This is an XPath 2.0 feature
            node_type = node_type_t::NODE_TYPE_ATTRIBUTE;
        }
    }
    const bool any_prefix(prefix == "*");

    // node_type_t::NODE_TYPE_COMMENT
    // node_type_t::NODE_TYPE_NODE
    // node_type_t::NODE_TYPE_PROCESSING_INSTRUCTION
    // node_type_t::NODE_TYPE_TEXT
    QDomNode::NodeType dom_node_type( QDomNode::ElementNode );
    switch(node_type)
    {
    case node_type_t::NODE_TYPE_COMMENT:
        dom_node_type = QDomNode::CommentNode;
        break;

    case node_type_t::NODE_TYPE_NODE: // any node? (BaseNode)
    case node_type_t::NODE_TYPE_ELEMENT:
        dom_node_type = QDomNode::ElementNode;
        break;

    case node_type_t::NODE_TYPE_PROCESSING_INSTRUCTION:
        dom_node_type = QDomNode::ProcessingInstructionNode;
        break;

    case node_type_t::NODE_TYPE_TEXT:
        dom_node_type = QDomNode::TextNode;
        // should include QDomNode::CDATASectionNode
        break;

    case node_type_t::NODE_TYPE_DOCUMENT_NODE:
        dom_node_type = QDomNode::DocumentNode;
        break;

    case node_type_t::NODE_TYPE_SCHEMA_ELEMENT:
        //dom_node_type = QDomNode::??;
        throw QDomXPathException_NotImplemented("the schema_element node type is not yet implemented");
        break;

    case node_type_t::NODE_TYPE_ATTRIBUTE:
        dom_node_type = QDomNode::AttributeNode;
        break;

    case node_type_t::NODE_TYPE_SCHEMA_ATTRIBUTE:
        //dom_node_type = QDomNode::??;
        throw QDomXPathException_NotImplemented("the schema_attribute node type is not yet implemented");
        break;

    }

    QDomXPath::node_vector_t result;
    QDomNode context_node;
    if(context_node_variant.getNodeSetValue().size() == 1) // TBD -- can it even be more than one?
    {
        context_node = context_node_variant.getNodeSetValue()[0];
    }

    // axis only applies on element nodes, right?
    if(context_node.isElement()
    || context_node.isDocument())
    {
        switch(axis)
        {
        case axis_t::AXIS_SELF:
            switch(node_type)
            {
            case node_type_t::NODE_TYPE_NODE:
            case node_type_t::NODE_TYPE_ELEMENT:
                // as far as I know the type node is considered to be
                // the same as elements (at least in XPath 1.0)
                // the local_part & prefix must match for us to keep the node
                if(!context_node.isNull()
                && (local_part.isEmpty() || local_part == context_node.toElement().tagName())
                && (any_prefix || prefix == context_node.prefix()))
                {
                    result.push_back(context_node);
                }
                break;

            default:
                throw QDomXPathException_NotImplemented(QString("this axis (%1) does not support this node type (%2)").arg(static_cast<int>(axis)).arg(static_cast<int>(node_type)));

            }
            break;

        case axis_t::AXIS_PARENT:
            switch(node_type)
            {
            case node_type_t::NODE_TYPE_NODE:
            case node_type_t::NODE_TYPE_ELEMENT:
                {
                    // as far as I know the type node is considered to be
                    // the same as elements (at least in XPath 1.0)
                    QDomNode node(context_node.parentNode());
                    // the local_part & prefix must match for us to keep the node
                    if(!node.isNull()
                    && (local_part.isEmpty() || local_part == node.toElement().tagName())
                    && (any_prefix || prefix == node.prefix()))
                    {
                        result.push_back(node);
                    }
                }
                break;

            default:
                throw QDomXPathException_NotImplemented(QString("this axis (%1) does not support this node type (%2)").arg(static_cast<int>(axis)).arg(static_cast<int>(node_type)));

            }
            break;

        case axis_t::AXIS_ATTRIBUTE:
axis_attribute:
            if(local_part.isEmpty())
            {
                QDomNamedNodeMap attributes(context_node.attributes());
                const int imax(attributes.size());
                for(int i(0); i < imax; ++i)
                {
                    QDomNode attr(attributes.item(i));
                    if(any_prefix || prefix == attr.prefix())
                    {
                        result.push_back(attr);
                    }
                }
            }
            else
            {
                // no need to go through the whole list slowly if
                // we can just query the map at once
                QDomNode attr(context_node.attributes().namedItem(local_part));
                if(!attr.isNull()
                && (any_prefix || prefix == attr.prefix()))
                {
                    result.push_back(attr);
                }
            }
            break;

        case axis_t::AXIS_ANCESTOR:
        case axis_t::AXIS_ANCESTOR_OR_SELF:
            switch(node_type)
            {
            case node_type_t::NODE_TYPE_NODE:
            case node_type_t::NODE_TYPE_ELEMENT:
                {
                    // as far as I know the type node is considered to be
                    // the same as elements (at least in XPath 1.0)
                    QDomNode node(context_node);
                    if(axis == axis_t::AXIS_ANCESTOR)
                    {
                        node = node.parentNode();
                    }
                    while(!node.isNull())
                    {
                        // the local_part & prefix must match for us to keep the node
                        if((local_part.isEmpty() || local_part == node.toElement().tagName())
                        && (any_prefix || prefix == node.prefix()))
                        {
                            result.push_back(node);
                        }
                        node = node.parentNode();
                    }
                }
                break;

            default:
                throw QDomXPathException_NotImplemented(QString("this axis (%1) does not support this node type (%2)").arg(static_cast<int>(axis)).arg(static_cast<int>(node_type)));

            }
            break;

        case axis_t::AXIS_CHILD:
            switch(node_type)
            {
            case node_type_t::NODE_TYPE_NODE:
            case node_type_t::NODE_TYPE_ELEMENT:
                {
                    // as far as I know the type node is considered to be
                    // the same as elements (at least in XPath 1.0)
                    //
                    // TBD: here we may want to add support for XML files
                    //      that were loaded without setting the
                    //      namespaceProcessing parameter set to true
                    //      (i.e. thus tag names are "blah:foo")
                    QDomNode node(context_node.firstChildElement(local_part));
                    while(!node.isNull())
                    {
                        // the prefix must also match
                        if(any_prefix || prefix == node.prefix())
                        {
                            result.push_back(node);
                        }
                        node = node.nextSiblingElement(local_part);
                    }
                }
                break;

            case node_type_t::NODE_TYPE_ATTRIBUTE:
                goto axis_attribute;

            case node_type_t::NODE_TYPE_COMMENT:
            case node_type_t::NODE_TYPE_TEXT:
                {
                    QDomNode node(context_node.firstChildElement(local_part));
                    while(!node.isNull())
                    {
                        // the prefix must also match
                        if(dom_node_type == node.nodeType()
                        && (any_prefix || prefix == node.prefix()))
                        {
                            result.push_back(node);
                        }
                        node = node.nextSiblingElement(local_part);
                    }
                }
                break;

            case node_type_t::NODE_TYPE_PROCESSING_INSTRUCTION:
                {
                    QDomNode node(context_node.firstChildElement(local_part));
                    while(!node.isNull())
                    {
                        // the prefix must also match
                        if(QDomNode::ProcessingInstructionNode == node.nodeType()
                        && node.isProcessingInstruction()
                        && prefix == node.prefix())
                        {
                            if(!processing_language.isEmpty())
                            {
                                // further test the processing language
                                QDomProcessingInstruction pi(node.toProcessingInstruction());
                                if(pi.target() == processing_language)
                                {
                                    result.push_back(node);
                                }
                            }
                            else
                            {
                                result.push_back(node);
                            }
                        }
                        node = node.nextSiblingElement(local_part);
                    }
                }
                break;

            default:
                throw QDomXPathException_NotImplemented(QString("this axis (%1) does not support this node type (%2)").arg(static_cast<int>(axis)).arg(static_cast<int>(node_type)));

            }
            break;

        case axis_t::AXIS_DESCENDANT:
        case axis_t::AXIS_DESCENDANT_OR_SELF:
            switch(node_type)
            {
            case node_type_t::NODE_TYPE_NODE:
            case node_type_t::NODE_TYPE_ELEMENT:
                {
                    // as far as I know the type node is considered to be
                    // the same as elements (at least in XPath 1.0)
                    QDomNode node(context_node);
                    if(axis == axis_t::AXIS_DESCENDANT_OR_SELF
                    && (local_part.isEmpty() || local_part == context_node.toElement().tagName())
                    && (any_prefix || prefix == context_node.prefix()))
                    {
                        result.push_back(context_node);
                    }
                    while(!node.isNull())
                    {
                        QDomNode next(node.firstChild());
                        if(next.isNull())
                        {
                            next = node;
                            while(!next.isNull()) // this should never happend since we should bump in context_node first
                            {
                                if(next == context_node)
                                {
                                    // break 2;
                                    goto axis_descendant_done;
                                }
                                QDomNode parent(next.parentNode());
                                next = next.nextSibling();
                                if(!next.isNull())
                                {
                                    break;
                                }
                                next = parent;
                            }
                        }
                        // the local_part & prefix must match for us to keep the node
                        node = next;
                        if((local_part.isEmpty() || local_part == node.toElement().tagName())
                        && (any_prefix || prefix == node.prefix()))
                        {
                            // push so nodes stay in document order
                            result.push_back(node);
                        }
                    }
axis_descendant_done:;
                }
                break;

            default:
                throw QDomXPathException_NotImplemented(QString("this axis (%1) does not support this node type (%2)").arg(static_cast<int>(axis)).arg(static_cast<int>(node_type)));

            }
            break;

        case axis_t::AXIS_NAMESPACE:
            /*
             * I found the following documentation about namespace nodes on
             * an Oracle website:
             *
             * http://docs.oracle.com/cd/B19306_01/appdev.102/b14259/appcxpa.htm
             *
             * It clearly explains what the so called "Namespace Nodes" are
             * and at this time we simply do not support them at all for
             * several reasons:
             *
             * \li We do not need them.
             * \li XPath 2.0 does not support them (it is clearly marked
             *     as deprecated).
             * \li Namespace Nodes are not actual nodes in the DOM. We'd
             *     have to simulate those special nodes and that's a lot
             *     of special cases for something nearly no one uses.
             *
             * Oracle documentation:
             *
             * Namespace Nodes
             *
             * Each element has an associated set of namespace nodes, one for
             * each distinct namespace prefix that is in scope for the element
             * (including the xml prefix, which is implicitly declared by the
             * XML Namespaces Recommendation) and one for the default namespace
             * if one is in scope for the element. The element is the parent of
             * each of these namespace nodes; however, a namespace node is not
             * a child of its parent element.
             *
             * Elements never share namespace nodes: if one element node is not
             * the same node as another element node, then none of the
             * namespace nodes of the one element node will be the same node as
             * the namespace nodes of another element node. This means that an
             * element will have a namespace node:
             *
             * For every attribute on the element whose name starts with
             * xmlns:;
             *
             * For every attribute on an ancestor element whose name starts
             * xmlns: unless the element itself or a nearer ancestor
             * re-declares the prefix;
             *
             * For an xmlns attribute, if the element or some ancestor has an
             * xmlns attribute, and the value of the xmlns attribute for the
             * nearest such element is nonempty
             *
             * \note
             * An attribute xmlns="" undeclares the default namespace.
             *
             * A namespace node has an expanded-name: the local part is the
             * namespace prefix (this is empty if the namespace node is for the
             * default namespace); the namespace URI is always NULL.
             *
             * The string-value of a namespace node is the namespace URI that
             * is being bound to the namespace prefix; if it is relative, then
             * it must be resolved just like a namespace URI in an
             * expanded-name.
             */
            throw QDomXPathException_NotImplemented("the namespace axis is not implemented");

        case axis_t::AXIS_FOLLOWING:
        case axis_t::AXIS_FOLLOWING_SIBLING:
            switch(node_type)
            {
            case node_type_t::NODE_TYPE_NODE:
            case node_type_t::NODE_TYPE_ELEMENT:
                {
                    // as far as I know the type node is considered to be
                    // the same as elements (at least in XPath 1.0)
                    QDomNode node(context_node.nextSibling());
                    while(!node.isNull())
                    {
                        // the local_part & prefix must match for us to keep the node
                        if((local_part.isEmpty() || local_part == node.toElement().tagName())
                        && (any_prefix || prefix == node.prefix()))
                        {
                            // push so nodes stay in document order
                            result.push_back(node);
                        }
                        node = node.nextSibling();
                    }
                    if(axis == axis_t::AXIS_FOLLOWING)
                    {
                        QDomNode next(context_node.parentNode());
                        while(!next.isNull())
                        {
                            QDomNode parent(next.parentNode());
                            next = next.nextSibling();
                            if(!next.isNull())
                            {
                                break;
                            }
                            next = parent;
                        }
                        while(!next.isNull())
                        {
                            // the local_part & prefix must match for us to keep the node
                            node = next;
                            if((local_part.isEmpty() || local_part == node.toElement().tagName())
                            && (any_prefix || prefix == node.prefix()))
                            {
                                // push so nodes stay in document order
                                result.push_back(node);
                            }
                            next = node.firstChild();
                            if(next.isNull())
                            {
                                next = node;
                                while(!next.isNull())
                                {
                                    QDomNode parent(next.parentNode());
                                    next = next.nextSibling();
                                    if(!next.isNull())
                                    {
                                        break;
                                    }
                                    next = parent;
                                }
                            }
                        }
                    }
                }
                break;

            default:
                throw QDomXPathException_NotImplemented(QString("this axis (%1) does not support this node type (%2)").arg(static_cast<int>(axis)).arg(static_cast<int>(node_type)));

            }
            break;

        case axis_t::AXIS_PRECEDING:
        case axis_t::AXIS_PRECEDING_SIBLING:
            // WARNING: 'preceding' never include the ancestors
            switch(node_type)
            {
            case node_type_t::NODE_TYPE_NODE:
            case node_type_t::NODE_TYPE_ELEMENT:
                {
                    // as far as I know the type node is considered to be
                    // the same as elements (at least in XPath 1.0)
                    QDomNode node(context_node.previousSibling());
                    while(!node.isNull())
                    {
                        // the local_part & prefix must match for us to keep the node
                        if((local_part.isEmpty() || local_part == node.toElement().tagName())
                        && (any_prefix || prefix == node.prefix()))
                        {
                            result.push_back(node);
                        }
                        node = node.previousSibling();
                    }
                    if(axis == axis_t::AXIS_PRECEDING)
                    {
                        QDomNode previous(context_node.parentNode());
                        while(!previous.isNull())
                        {
                            QDomNode parent(previous.parentNode());
                            previous = previous.previousSibling();
                            if(!previous.isNull())
                            {
                                break;
                            }
                            previous = parent;
                        }
                        while(!previous.isNull())
                        {
                            // the local_part & prefix must match for us to keep the node
                            node = previous;
                            if((local_part.isEmpty() || local_part == node.toElement().tagName())
                            && (any_prefix || prefix == node.prefix()))
                            {
                                result.push_back(node);
                            }
                            previous = node.lastChild();
                            if(previous.isNull())
                            {
                                do
                                {
                                    QDomNode parent(node.parentNode());
                                    previous = node.previousSibling();
                                    if(!previous.isNull())
                                    {
                                        break;
                                    }
                                    node = parent;
                                }
                                while(!previous.isNull());
                            }
                        }
                    }
                }
                break;

            default:
                throw QDomXPathException_NotImplemented(QString("this axis (%1) does not support this node type (%2)").arg(static_cast<int>(axis)).arg(static_cast<int>(node_type)));

            }
            break;

        }
    }

    variant_t node_set;
    node_set.setValue(result);
    f_functions.back().f_stack.push_back(node_set);
}







/** \brief Get the next character.
 *
 * This function returns the next character found in the input string.
 * If the character is invalid, the function throws an exception.
 *
 * Note that the function returns characters encoded in UTF-16, even
 * though XML expects UCS-4 characters. The main reason is because the
 * QString implementation returns those characters in this way. This
 * works because none of the characters with code values larger than
 * 0xFFFF are tested within this parser. All of those are viewed as
 * standard 'Char' and thus they can as well be defined as 0xD800 to
 * 0xDFFF byte codes.
 *
 * \exception QDomXPathException_InvalidCharacter
 * This exception is raised in the even an input character is not a valid
 * XML character (i.e. Ctrl-A is not acceptable in the input.)
 *
 * \return The next character in the form of an encoded UTF-16 character.
 */
char_t getc()
{
    char_t c(f_in->unicode());
    if(c == '\0')
    {
        return END_OF_PATH;
    }
    // Char ::= #x9
    //        | #xA
    //        | #xD
    //        | [#x20-#xD7FF]
    //        | [#xE000-#xFFFD]
    //        | [#x10000-#x10FFFF]
    // The Qt QChar is a UTF-16 character which means that
    // characters larger then 0xFFFF are defined with codes
    // between 0xD800 and 0xDFFF. These are therefore included
    // although we could check that the characters are correct
    // we do not because we do not have to test for specific
    // characters with codes that large.)
    if(c != 0x09
    && c != 0x0A
    && c != 0x0D
    && (c < 0x20 || c > 0xFFFD))
    {
        throw QDomXPathException_InvalidCharacter(QString("invalid XML character 0x%1").arg(static_cast<int>(c), 4, 16, QChar('0')));
    }
    ++f_in;
    return c;
}

/** \brief Restore the input character pointer position.
 *
 * This function can be called to restore the character pointer position
 * to a previous position. It can be called as many times as the getc()
 * function was called. However, note that you cannot specify which
 * character is being ungotten. It will always be the character that
 * you got at that time with getc().
 *
 * Note that the function takes a character as input, if that character is
 * END_OF_PATH, then ungetc() does nothing. This can be a problem if multiple
 * ungetc() need to be used. In that case, just use a character such as ' '.
 *
 * \exception QDomXPathException_TooManyUnget
 * As a protection, the function checks that the unget() function is called
 * more times than the get() function was called. If so, we have a bug.
 *
 * \param[in] c  The character to be ungotten.
 */
void ungetc(char_t c)
{
    if(c == END_OF_PATH)
    {
        return;
    }
    if(f_in <= f_start)
    {
        throw QDomXPathException_TooManyUnget("ungetc() called too many times, the algorithm is spurious");
    }
    --f_in;
}

/** \brief Get the next token.
 *
 * This function reads one XML XPath token from the input. This can be a
 * single character ('(' or '@') or a whole complex token such as a string
 * or a real number.
 *
 * The function returns true if the token was successfully read. The token
 * is found in the f_last_token variable. If necessary, the token can be
 * copied for further processing at a later time.
 *
 * \exception QDomXPathException_InvalidCharacter
 * This exception is raised if an invalid combination of characters is found.
 * For example, the exclamation mark ('!') character must be followed by
 * an equal sign ('=') to be valid.
 *
 * \exception QDomXPathException_InvalidString
 * This exception is raised whenever a string ends without a quote (i.e. we
 * reached the end of the input buffer before the end of the string.)
 *
 * \return true if a token was read, false if no more tokens are available.
 */
bool get_token()
{
    // ExprToken ::= '(' | ')'
    //             | '[' | ']'
    //             | '.'
    //             | '..'
    //             | '@'
    //             | ','
    //             | '::'
    //             | NameTest
    //             | NodeType
    //             | Operator
    //             | FunctionName
    //             | AxisName
    //             | Literal
    //             | Number
    //             | VariableReference
    //
    // Number ::= Digits ('.' Digits?)?
    //          | '.' Digits
    //
    // Digits ::= [0-9]+
    //
    // Operator ::= OperatorName
    //            | MultiplyOperator
    //            | '/'
    //            | '//'
    //            | '|'
    //            | '+'
    //            | '-'
    //            | '='
    //            | '!='
    //            | '<'
    //            | '<='
    //            | '>'
    //            | '>='
    //
    // MultiplyOperator ::= '*'
    //
    // Literal ::= '"' [^"]* '"'
    //           | "'" [^']* "'"
    //
    // NameTest ::= '*'
    //            | NCName ':' '*'
    //            | QName
    //
    // NCName ::= Name - (Char* ':' Char*)
    //
    // NameStartChar ::= ':'
    //                 | [A-Z]
    //                 | '_'
    //                 | [a-z]
    //                 | [#xC0-#xD6]
    //                 | [#xD8-#xF6]
    //                 | [#xF8-#x2FF]
    //                 | [#x370-#x37D]
    //                 | [#x37F-#x1FFF]
    //                 | [#x200C-#x200D]
    //                 | [#x2070-#x218F]
    //                 | [#x2C00-#x2FEF]
    //                 | [#x3001-#xD7FF]
    //                 | [#xF900-#xFDCF]
    //                 | [#xFDF0-#xFFFD]
    //                 | [#x10000-#xEFFFF]
    //
    // NameChar ::= NameStartChar
    //            | '-'
    //            | '.'
    //            | [0-9]
    //            | #xB7
    //            | [#x0300-#x036F]
    //            | [#x203F-#x2040]
    //
    // Name ::= NameStartChar (NameChar)*
    //
    // OperatorName ::= 'and'
    //                | 'or'
    //                | 'mod'
    //                | 'div'
    //
    // NodeType ::= 'comment'
    //            | 'text'
    //            | 'processing-instruction'
    //            | 'node'
    //
    // FunctionName ::= QName - NodeType
    //
    // AxisName ::= 'ancestor'
    //            | 'ancestor-or-self'
    //            | 'attribute'
    //            | 'child'
    //            | 'descendant'
    //            | 'descendant-or-self'
    //            | 'following'
    //            | 'following-sibling'
    //            | 'namespace'
    //            | 'parent'
    //            | 'preceding'
    //            | 'preceding-sibling'
    //            | 'self'
    //
    // VariableReference ::= '$' QName
    //
    // QName ::= PrefixedName
    //         | UnprefixedName
    //
    // PrefixedName ::= Prefix ':' LocalPart
    //
    // UnprefixedName ::= LocalPart
    //
    // Prefix ::= NCName
    //
    // LocalPart ::= NCName
    //


    // if we got an ungotten token, return it
    if(f_unget_token)
    {
        f_last_token = f_unget_token;
        f_unget_token.reset();
        return f_last_token;
    }
    else
    {
        f_last_token.f_string = "";
        char_t c(getc());
        // ignore spaces between tokens
        while(c == 0x20 || c == 0x09 || c == 0x0D || c == 0x0A)
        {
            c = getc();
        }
        if(c == END_OF_PATH)
        {
            f_last_token.reset();
            return f_last_token;
        }
        f_last_token.f_string += QChar(c);
        switch(c)
        {
        case '(':
            f_last_token.f_token = token_t::tok_t::TOK_OPEN_PARENTHESIS;
            break;

        case ')':
            f_last_token.f_token = token_t::tok_t::TOK_CLOSE_PARENTHESIS;
            break;

        case '[':
            f_last_token.f_token = token_t::tok_t::TOK_OPEN_SQUARE_BRACKET;
            break;

        case ']':
            f_last_token.f_token = token_t::tok_t::TOK_CLOSE_SQUARE_BRACKET;
            break;

        case '@':
            f_last_token.f_token = token_t::tok_t::TOK_AT;
            break;

        case ',':
            f_last_token.f_token = token_t::tok_t::TOK_COMMA;
            break;

        case ':':
            c = getc();
            if(c == ':')
            {
                f_last_token.f_token = token_t::tok_t::TOK_DOUBLE_COLON;
                f_last_token.f_string += QChar(c);
            }
            else
            {
                ungetc(c);
                f_last_token.f_token = token_t::tok_t::TOK_COLON;
            }
            break;

        case '/':
            c = getc();
            if(c == '/')
            {
                f_last_token.f_token = token_t::tok_t::TOK_DOUBLE_SLASH;
                f_last_token.f_string += QChar(c);
            }
            else
            {
                ungetc(c);
                f_last_token.f_token = token_t::tok_t::TOK_SLASH;
            }
            break;

        case '|':
            f_last_token.f_token = token_t::tok_t::TOK_PIPE;
            break;

        case '$':
            f_last_token.f_token = token_t::tok_t::TOK_DOLLAR;
            break;

        case '+':
            f_last_token.f_token = token_t::tok_t::TOK_PLUS;
            break;

        case '-':
            f_last_token.f_token = token_t::tok_t::TOK_MINUS;
            break;

        case '=':
            f_last_token.f_token = token_t::tok_t::TOK_EQUAL;
            break;

        case '!':
            c = getc();
            if(c == '=')
            {
                f_last_token.f_token = token_t::tok_t::TOK_NOT_EQUAL;
                f_last_token.f_string += QChar(c);
            }
            else
            {
                throw QDomXPathException_InvalidCharacter("found a stand alone '!' character which is not supported at that location");
            }
            break;

        case '<':
            c = getc();
            if(c == '=')
            {
                f_last_token.f_token = token_t::tok_t::TOK_LESS_OR_EQUAL;
                f_last_token.f_string += QChar(c);
            }
            else
            {
                ungetc(c);
                f_last_token.f_token = token_t::tok_t::TOK_LESS_THAN;
            }
            break;

        case '>':
            c = getc();
            if(c == '=')
            {
                f_last_token.f_token = token_t::tok_t::TOK_GREATER_OR_EQUAL;
                f_last_token.f_string += QChar(c);
            }
            else
            {
                ungetc(c);
                f_last_token.f_token = token_t::tok_t::TOK_GREATER_THAN;
            }
            break;

        case '*':
            // '*' can represent a NameTest or the Multiply operator
            // (this is context dependent)
            f_last_token.f_token = token_t::tok_t::TOK_ASTERISK;
            break;

        case '\'':
        case '"':
            f_last_token.f_token = token_t::tok_t::TOK_STRING;
            f_last_token.f_string = ""; // remove the quote
            {
                char_t quote(c);
                for(;;)
                {
                    c = getc();
                    if(c == END_OF_PATH)
                    {
                        throw QDomXPathException_InvalidString("a string that was not properly closed");
                    }
                    if(c == quote)
                    {
                        // in XPath 2.0 we can double quotes to insert
                        // a quote in the string
                        c = getc();
                        if(c != quote)
                        {
                            ungetc(c);
                            break;
                        }
                    }
                    f_last_token.f_string += QChar(c);
                }
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
            f_last_token.f_token = token_t::tok_t::TOK_INTEGER;
            f_last_token.f_integer = c - '0';
            for(;;)
            {
                c = getc();
                if(c < '0' || c > '9')
                {
                    break;
                }
                f_last_token.f_string += QChar(c);
                f_last_token.f_integer = f_last_token.f_integer * 10 + c - '0';
            }
            if(c != '.')
            {
                ungetc(c);
                break;
            }
            f_last_token.f_string += QChar(c);
            f_last_token.f_real = static_cast<double>(f_last_token.f_integer);
        case '.':
            c = getc();
            if(f_last_token.f_string == ".")
            {
                if(c == '.')
                {
                    f_last_token.f_token = token_t::tok_t::TOK_DOUBLE_DOT;
                    f_last_token.f_string += QChar(c);
                    break;
                }
                else if(c < '0' || c > '9')
                {
                    ungetc(c);
                    f_last_token.f_token = token_t::tok_t::TOK_DOT;
                    break;
                }
                f_last_token.f_string = "0.";
            }
            f_last_token.f_token = token_t::tok_t::TOK_REAL;
            { // "protect" frac
                double frac(1.0);
                for(;;)
                {
                    if(c < '0' || c > '9')
                    {
                        break;
                    }
                    f_last_token.f_string += QChar(c);
                    frac /= 10.0;
                    f_last_token.f_real += (c - '0') * frac;
                    c = getc();
                }
            }
            ungetc(c);
            if(f_last_token.f_string.right(1) == ".")
            {
                f_last_token.f_string += '0';
            }
            // we prepend and append zeroes to the f_string for clarity
            // yet, the function really returns f_last_token.f_real
            // the f_last_token.f_integer is the floor()
            break;

        default:
            if((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= 0x00C0 && c <= 0x00D6)
            || (c >= 0x00D8 && c <= 0x00F6)
            || (c >= 0x00F8 && c <= 0x02FF)
            || (c >= 0x0370 && c <= 0x037D)
            || (c >= 0x037F && c <= 0x1FFF)
            || (c >= 0x200C && c <= 0x200D)
            || (c >= 0x2070 && c <= 0x218F)
            || (c >= 0x2C00 && c <= 0x2FEF)
            || (c >= 0x3001 && c <= 0xDFFF) // includes 0x10000 to 0xEFFFF
            || (c >= 0xF900 && c <= 0xFDCF)
            || (c >= 0xFDF0 && c <= 0xFFFD)
            || c == '_')
            {
                for(;;)
                {
                    c = getc();
                    if(c == END_OF_PATH)
                    {
                        break;
                    }
                    if((c < 'a'    || c > 'z')
                    && (c < 'A'    || c > 'Z')
                    && (c < '0'    || c > '9')
                    && (c < 0x00C0 || c > 0x00D6)
                    && (c < 0x00D8 || c > 0x00F6)
                    && (c < 0x00F8 || c > 0x02FF)
                    && (c < 0x0300 || c > 0x037D)
                    && (c < 0x037F || c > 0x1FFF)
                    && (c < 0x200C || c > 0x200D)
                    && (c < 0x203F || c > 0x2040)
                    && (c < 0x2070 || c > 0x218F)
                    && (c < 0x2C00 || c > 0x2FEF)
                    && (c < 0x3001 || c > 0xDFFF) // includes 0x10000 to 0xEFFFF
                    && (c < 0xF900 || c > 0xFDCF)
                    && (c < 0xFDF0 || c > 0xFFFD)
                    && c != '_' && c != '.' && c != '-' && c != 0xB7)
                    {
                        ungetc(c);
                        break;
                    }
                    f_last_token.f_string += QChar(c);
                }
                // at this point we return an NCNAME
                // (NC means No Colon)
                // what the name represents changes depending on context
                f_last_token.f_token = token_t::tok_t::TOK_NCNAME;
            }
            else
            {
                // this won't match anything and thus return and error
                f_last_token.f_token = token_t::tok_t::TOK_INVALID;
            }
            break;

        }
    }

    return f_last_token;
}

#if 0
/** \brief Check whether an NCNAME represents an operator.
 *
 * This function transforms a TOK_NCNAME token in one of the named operators:
 *
 * \li TOK_OPERATOR_AND
 * \li TOK_OPERATOR_OR
 * \li TOK_OPERATOR_DIV
 * \li TOK_OPERATOR_MOD
 *
 * \note
 * At this point the function is not being used because it is not really
 * practical/useful.
 *
 * \return true if the f_last_token represents an operator.
 */
bool token_is_operator()
{
    switch(f_last_token.f_token)
    {
    case token_t::tok_t::TOK_NCNAME:
        if(f_last_token.f_string == "and")
        {
            f_last_token.f_token = token_t::tok_t::TOK_OPERATOR_AND;
        }
        else if(f_last_token.f_string == "or")
        {
            f_last_token.f_token = token_t::tok_t::TOK_OPERATOR_OR;
        }
        else if(f_last_token.f_string == "div")
        {
            f_last_token.f_token = token_t::tok_t::TOK_OPERATOR_DIV;
        }
        else if(f_last_token.f_string == "mod")
        {
            f_last_token.f_token = token_t::tok_t::TOK_OPERATOR_MOD;
        }
        else
        {
            return false;
        }
        /*FALLTHROUGH*/
    case token_t::tok_t::TOK_OPERATOR_AND:
    case token_t::tok_t::TOK_OPERATOR_OR:
    case token_t::tok_t::TOK_OPERATOR_MOD:
    case token_t::tok_t::TOK_OPERATOR_DIV:
        return true;

    default:
        return false;

    }
}
#endif


/** \brief Check whether an NCNAME represents a node type.
 *
 * This function transforms a TOK_NCNAME token in one of the node types:
 *
 * \li TOK_NODE_TYPE_COMMENT
 * \li TOK_NODE_TYPE_TEXT
 * \li TOK_NODE_TYPE_PROCESSING_INSTRUCTION
 * \li TOK_NODE_TYPE_NODE
 *
 * \return true if the f_last_token represents a node type.
 */
bool token_is_node_type()
{
    switch(f_last_token.f_token)
    {
    case token_t::tok_t::TOK_NCNAME:
        if(f_last_token.f_string == "comment")
        {
            f_last_token.f_token = token_t::tok_t::TOK_NODE_TYPE_COMMENT;
        }
        else if(f_last_token.f_string == "text")
        {
            f_last_token.f_token = token_t::tok_t::TOK_NODE_TYPE_TEXT;
        }
        else if(f_last_token.f_string == "processing-instruction")
        {
            f_last_token.f_token = token_t::tok_t::TOK_NODE_TYPE_PROCESSING_INSTRUCTION;
        }
        else if(f_last_token.f_string == "node")
        {
            f_last_token.f_token = token_t::tok_t::TOK_NODE_TYPE_NODE;
        }
        else
        {
            return false;
        }
        /*FALLTHROUGH*/
    case token_t::tok_t::TOK_NODE_TYPE_COMMENT:
    case token_t::tok_t::TOK_NODE_TYPE_TEXT:
    case token_t::tok_t::TOK_NODE_TYPE_PROCESSING_INSTRUCTION:
    case token_t::tok_t::TOK_NODE_TYPE_NODE:
        return true;

    default:
        return false;

    }
}

/** \brief Check whether an NCNAME represents an axis.
 *
 * This function transforms a TOK_NCNAME token in one of the axis types:
 *
 * \li TOK_AXIS_NAME_ANCESTOR
 * \li TOK_AXIS_NAME_ANCESTOR_OR_SELF
 * \li TOK_AXIS_NAME_ATTRIBUTE
 * \li TOK_AXIS_NAME_CHILD
 * \li TOK_AXIS_NAME_DESCENDANT
 * \li TOK_AXIS_NAME_DESCENDANT_OR_SELF
 * \li TOK_AXIS_NAME_FOLLOWING
 * \li TOK_AXIS_NAME_FOLLOWING_SIBLING
 * \li TOK_AXIS_NAME_NAMESPACE
 * \li TOK_AXIS_NAME_PARENT
 * \li TOK_AXIS_NAME_PRECEDING
 * \li TOK_AXIS_NAME_PRECEDING_SIBLING
 * \li TOK_AXIS_NAME_SELF
 *
 */
bool token_is_axis_name()
{
    switch(f_last_token.f_token)
    {
    case token_t::tok_t::TOK_NCNAME:
        // TODO: add one more level to test the first letter really fast
        if(f_last_token.f_string == "ancestor")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_ANCESTOR;
        }
        else if(f_last_token.f_string == "ancestor-or-self")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_ANCESTOR_OR_SELF;
        }
        else if(f_last_token.f_string == "attribute")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_ATTRIBUTE;
        }
        else if(f_last_token.f_string == "child")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_CHILD;
        }
        else if(f_last_token.f_string == "descendant")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_DESCENDANT;
        }
        else if(f_last_token.f_string == "descendant-or-self")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_DESCENDANT_OR_SELF;
        }
        else if(f_last_token.f_string == "following")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_FOLLOWING;
        }
        else if(f_last_token.f_string == "following-sibling")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_FOLLOWING_SIBLING;
        }
        else if(f_last_token.f_string == "namespace")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_NAMESPACE;
        }
        else if(f_last_token.f_string == "parent")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_PARENT;
        }
        else if(f_last_token.f_string == "preceding")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_PRECEDING;
        }
        else if(f_last_token.f_string == "preceding-sibling")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_PRECEDING_SIBLING;
        }
        else if(f_last_token.f_string == "self")
        {
            f_last_token.f_token = token_t::tok_t::TOK_AXIS_NAME_SELF;
        }
        /*FALLTHROUGH*/
    case token_t::tok_t::TOK_AXIS_NAME_ANCESTOR:
    case token_t::tok_t::TOK_AXIS_NAME_ANCESTOR_OR_SELF:
    case token_t::tok_t::TOK_AXIS_NAME_ATTRIBUTE:
    case token_t::tok_t::TOK_AXIS_NAME_CHILD:
    case token_t::tok_t::TOK_AXIS_NAME_DESCENDANT:
    case token_t::tok_t::TOK_AXIS_NAME_DESCENDANT_OR_SELF:
    case token_t::tok_t::TOK_AXIS_NAME_FOLLOWING:
    case token_t::tok_t::TOK_AXIS_NAME_FOLLOWING_SIBLING:
    case token_t::tok_t::TOK_AXIS_NAME_NAMESPACE:
    case token_t::tok_t::TOK_AXIS_NAME_PARENT:
    case token_t::tok_t::TOK_AXIS_NAME_PRECEDING:
    case token_t::tok_t::TOK_AXIS_NAME_PRECEDING_SIBLING:
    case token_t::tok_t::TOK_AXIS_NAME_SELF:
        return true;

    default:
        return false;

    }
}

void add_to_program(QDomXPath::instruction_t inst)
{
    f_program.push_back(inst);
}

void append_instruction(QDomXPath::instruction_t inst)
{
    add_to_program(inst);

    if(f_show_commands)
    {
        disassemble_instruction(f_program.size() - 1);
    }
}

void append_push_string(const QString& string)
{
    int offset(f_program.size());

    // TODO: make sure that an NCNAME can be pushed as a string instead
    //       of an NCNAME
    if(string.isEmpty())
    {
        // push_empty_string
        add_to_program(INST_PUSH_EMPTY_STRING);
    }
    else if(string == "*")
    {
        // push_any
        add_to_program(INST_PUSH_ANY_STRING);
    }
    else
    {
        // push_string
        std::string str(string.toUtf8().data());
        const size_t imax(str.length());
        if(imax < 256)
        {
            add_to_program(INST_PUSH_SMALL_STRING);
            add_to_program(static_cast<QDomXPath::instruction_t>(imax));
            // TODO: use memcpy
            for(size_t i(0); i < imax; ++i)
            {
                add_to_program(str[i]);
            }
        }
        else if(imax < 65536)
        {
            add_to_program(INST_PUSH_MEDIUM_STRING);
            add_to_program(static_cast<QDomXPath::instruction_t>(imax >> 8));
            add_to_program(static_cast<QDomXPath::instruction_t>(imax));
            // TODO: use memcpy
            for(size_t i(0); i < imax; ++i)
            {
                add_to_program(str[i]);
            }
        }
        else
        {
            add_to_program(INST_PUSH_LARGE_STRING);
            add_to_program(static_cast<QDomXPath::instruction_t>(imax >> 24));
            add_to_program(static_cast<QDomXPath::instruction_t>(imax >> 16));
            add_to_program(static_cast<QDomXPath::instruction_t>(imax >> 8));
            add_to_program(static_cast<QDomXPath::instruction_t>(imax));
            // TODO: use memcpy
            for(size_t i(0); i < imax; ++i)
            {
                add_to_program(str[i]);
            }
        }
    }

    if(f_show_commands)
    {
        disassemble_instruction(offset);
    }
}


void append_push_boolean(const bool boolean)
{
    // note: I used a terciary here, but that prevents the compiler from
    //       optimizing the INST_PUSH_TRUE/FALSE and the link fails!?
    if(boolean)
    {
        append_instruction(INST_PUSH_TRUE);
    }
    else
    {
        append_instruction(INST_PUSH_FALSE);
    }
}


void append_push_integer(const int64_t integer)
{
    int offset(f_program.size());

    if(integer == 0)
    {
        add_to_program(INST_PUSH_ZERO);
    }
    else if(integer >= 0 && integer < 256)
    {
        add_to_program(INST_PUSH_BYTE);
        add_to_program(static_cast<QDomXPath::instruction_t>(integer));
    }
    else if(integer >= -256 && integer < 0)
    {
        add_to_program(INST_PUSH_NEGATIVE_BYTE);
        add_to_program(static_cast<QDomXPath::instruction_t>(integer));
    }
    else if(integer >= 0 && integer < 65536)
    {
        add_to_program(INST_PUSH_SHORT);
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 8));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer));
    }
    else if(integer >= -65536 && integer < 0)
    {
        add_to_program(INST_PUSH_NEGATIVE_SHORT);
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 8));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer));
    }
    else if(integer >= 0 && integer < 0x100000000LL)
    {
        add_to_program(INST_PUSH_LONG);
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 24));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 16));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 8));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer));
    }
    else if(integer >= -0x100000000LL && integer < 0)
    {
        add_to_program(INST_PUSH_NEGATIVE_LONG);
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 24));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 16));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 8));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer));
    }
    else
    {
        add_to_program(INST_PUSH_LONGLONG);
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 56));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 48));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 40));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 32));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 24));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 16));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer >> 8));
        add_to_program(static_cast<QDomXPath::instruction_t>(integer));
    }

    if(f_show_commands)
    {
        disassemble_instruction(offset);
    }
}

void append_push_integer(node_type_t const type)
{
    append_push_integer(static_cast<int64_t>(type));
}

void append_push_integer(axis_t const type)
{
    append_push_integer(static_cast<int64_t>(type));
}

void append_push_integer(internal_func_t const type)
{
    append_push_integer(static_cast<int64_t>(type));
}

void append_push_double(double const real)
{
    int offset(f_program.size());

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    if(real == 0.0)
#pragma GCC diagnostic pop
    {
        add_to_program(INST_PUSH_DOUBLE_ZERO);
    }
    else
    {
        add_to_program(INST_PUSH_DOUBLE);
        union
        {
            uint64_t    f_int;
            double      f_dbl;
        } convert;
        convert.f_dbl = real;
        add_to_program(static_cast<QDomXPath::instruction_t>(convert.f_int >> 56));
        add_to_program(static_cast<QDomXPath::instruction_t>(convert.f_int >> 48));
        add_to_program(static_cast<QDomXPath::instruction_t>(convert.f_int >> 40));
        add_to_program(static_cast<QDomXPath::instruction_t>(convert.f_int >> 32));
        add_to_program(static_cast<QDomXPath::instruction_t>(convert.f_int >> 24));
        add_to_program(static_cast<QDomXPath::instruction_t>(convert.f_int >> 16));
        add_to_program(static_cast<QDomXPath::instruction_t>(convert.f_int >> 8));
        add_to_program(static_cast<QDomXPath::instruction_t>(convert.f_int));
    }

    if(f_show_commands)
    {
        disassemble_instruction(offset);
    }
}

void append_push_token(const token_t& token)
{
    switch(token.f_token)
    {
    case token_t::tok_t::TOK_ASTERISK:
        append_push_string("*");
        break;

    case token_t::tok_t::TOK_STRING:
    case token_t::tok_t::TOK_PREFIX: // this is like a string
    case token_t::tok_t::TOK_NCNAME: // this can be a lie (in case of variable names, it can include a colon)
        append_push_string(token.f_string);
        break;

    case token_t::tok_t::TOK_INTEGER:
        append_push_integer(token.f_integer);
        break;

    case token_t::tok_t::TOK_REAL:
        append_push_double(token.f_real);
        break;

    default:
        throw QDomXPathException_InternalError(QString("unexpected token type (%1/%2) in an append_push_token() call").arg(static_cast<int>(token.f_token)).arg(token.f_string));

    }
}

void append_function(const QDomXPath::program_t& function)
{
    const size_t size(function.size());
    if(size < 65536)
    {
        append_instruction(INST_SMALL_FUNCTION);
        append_instruction(static_cast<QDomXPath::instruction_t>(size >> 8));
        append_instruction(static_cast<QDomXPath::instruction_t>(size));
    }
    else
    {
        append_instruction(INST_LARGE_FUNCTION);
        append_instruction(static_cast<QDomXPath::instruction_t>(size >> 24));
        append_instruction(static_cast<QDomXPath::instruction_t>(size >> 16));
        append_instruction(static_cast<QDomXPath::instruction_t>(size >> 8));
        append_instruction(static_cast<QDomXPath::instruction_t>(size));
    }
    f_program += function;
}


void append_axis(const token_t& axis, const token_t& prefix, const token_t& local_part)
{
    // if the prefix is marked as "undefined" then we have a NodeType
    // instead of a name
    if(prefix.f_token == token_t::tok_t::TOK_UNDEFINED)
    {
        // Axis '::' NodeType '(' ')'
        switch(local_part.f_token)
        {
        case token_t::tok_t::TOK_NODE_TYPE_COMMENT:                 append_push_integer(node_type_t::NODE_TYPE_COMMENT);                 break;
        case token_t::tok_t::TOK_NODE_TYPE_NODE:                    append_push_integer(node_type_t::NODE_TYPE_NODE);                    break;
        case token_t::tok_t::TOK_NODE_TYPE_PROCESSING_INSTRUCTION:  append_push_integer(node_type_t::NODE_TYPE_PROCESSING_INSTRUCTION);  break;
        case token_t::tok_t::TOK_NODE_TYPE_TEXT:                    append_push_integer(node_type_t::NODE_TYPE_TEXT);                    break;

        default:
            throw QDomXPathException_InvalidError("invalid node type");

        }

        // if 'processing-instruction' then we could have a string here
        // if empty, then no name was specified to the NodeType '(' ')'
        append_push_string(axis.f_string);
    }
    else
    {
        // push the name, it may be "*:*" if the node type is specified
        append_push_token(local_part);
        append_push_token(prefix);
    }

    // TODO: node type
 
    switch(axis.f_token)
    {
    case token_t::tok_t::TOK_AXIS_NAME_ANCESTOR:           append_push_integer(axis_t::AXIS_ANCESTOR);             break;
    case token_t::tok_t::TOK_AXIS_NAME_ANCESTOR_OR_SELF:   append_push_integer(axis_t::AXIS_ANCESTOR_OR_SELF);     break;
    case token_t::tok_t::TOK_AXIS_NAME_ATTRIBUTE:          append_push_integer(axis_t::AXIS_ATTRIBUTE);            break;
    case token_t::tok_t::TOK_AXIS_NAME_CHILD:              append_push_integer(axis_t::AXIS_CHILD);                break;
    case token_t::tok_t::TOK_AXIS_NAME_DESCENDANT:         append_push_integer(axis_t::AXIS_DESCENDANT);           break;
    case token_t::tok_t::TOK_AXIS_NAME_DESCENDANT_OR_SELF: append_push_integer(axis_t::AXIS_DESCENDANT_OR_SELF);   break;
    case token_t::tok_t::TOK_AXIS_NAME_FOLLOWING:          append_push_integer(axis_t::AXIS_FOLLOWING);            break;
    case token_t::tok_t::TOK_AXIS_NAME_FOLLOWING_SIBLING:  append_push_integer(axis_t::AXIS_FOLLOWING_SIBLING);    break;
    case token_t::tok_t::TOK_AXIS_NAME_NAMESPACE:          append_push_integer(axis_t::AXIS_NAMESPACE);            break;
    case token_t::tok_t::TOK_AXIS_NAME_PARENT:             append_push_integer(axis_t::AXIS_PARENT);               break;
    case token_t::tok_t::TOK_AXIS_NAME_PRECEDING:          append_push_integer(axis_t::AXIS_PRECEDING);            break;
    case token_t::tok_t::TOK_AXIS_NAME_PRECEDING_SIBLING:  append_push_integer(axis_t::AXIS_PRECEDING_SIBLING);    break;
    case token_t::tok_t::TOK_AXIS_NAME_SELF:               append_push_integer(axis_t::AXIS_SELF);                 break;

    default:
        throw QDomXPathException_InvalidError("invalid axis type");

    }

    append_instruction(INST_AXIS);
}


void append_push_for_jump(const QString& label)
{
    if(label.isEmpty())
    {
        throw QDomXPathException_InternalError("pushing for a future label with an empty string is not supported");
    }
    if(f_show_commands)
    {
        std::cout << "=== push for jump (" << f_program.size() << ")\n";
    }
    f_future_labels[label].push_back(f_program.size());
    append_push_integer(0x1111); // reserve 2 bytes for pc
}


void mark_with_label(const QString& label)
{
    int offset(f_program.size());
    f_labels[label] = offset;

    if(f_future_labels.contains(label))
    {
        future_labels_t::iterator future(f_future_labels.find(label));
        const int imax((*future).size());
        for(int i(0); i < imax; ++i)
        {
            int pc((*future)[i]);
            f_program[pc + 1] = static_cast<instruction_t>(offset >> 8);
            f_program[pc + 2] = static_cast<instruction_t>(offset);

            if(f_show_commands)
            {
                std::cout << "# Fix offset -- ";
                disassemble_instruction(pc);
            }
        }
        f_future_labels.erase(future);
    }
}



void unary_expr()
{
    int negate(0);
    for(;;)
    {
        if(f_last_token.f_token == token_t::tok_t::TOK_MINUS)
        {
            negate ^= 1;
        }
        else if(f_last_token.f_token != token_t::tok_t::TOK_PLUS) // XPath 2.0 allows unary '+'
        {
            break;
        }
        get_token();
    }
    union_expr();
    if(negate)
    {
        append_instruction(INST_NEGATE);
    }
}

void multiplicative_expr()
{
    unary_expr();
    for(;;)
    {
        instruction_t inst(INST_END);
        switch(f_last_token.f_token)
        {
        case token_t::tok_t::TOK_ASTERISK:
            inst = INST_MULTIPLY;
            break;

        case token_t::tok_t::TOK_NCNAME:
            if(f_last_token.f_string == "div")
            {
                inst = INST_DIVIDE;
            }
            else if(f_last_token.f_string == "idiv")
            {
                inst = INST_IDIVIDE;
            }
            else if(f_last_token.f_string == "mod")
            {
                inst = INST_MODULO;
            }
            else
            {
                return;
            }
            break;

        default:
            return;

        }
        get_token();
        unary_expr();
        append_instruction(inst);
    }
}

void additive_expr()
{
    multiplicative_expr();
    for(;;)
    {
        instruction_t inst(INST_END);
        switch(f_last_token.f_token)
        {
        case token_t::tok_t::TOK_PLUS:
            inst = INST_ADD;
            break;

        case token_t::tok_t::TOK_MINUS:
            inst = INST_SUBTRACT;
            break;

        default:
            return;

        }
        get_token();
        multiplicative_expr();
        append_instruction(inst);
    }
}

void relational_expr()
{
    additive_expr();
    for(;;)
    {
        instruction_t inst(INST_END);
        switch(f_last_token.f_token)
        {
        case token_t::tok_t::TOK_LESS_THAN:
            inst = INST_LESS_THAN;
            break;

        case token_t::tok_t::TOK_LESS_OR_EQUAL:
            inst = INST_LESS_OR_EQUAL;
            break;

        case token_t::tok_t::TOK_GREATER_THAN:
            inst = INST_GREATER_THAN;
            break;

        case token_t::tok_t::TOK_GREATER_OR_EQUAL:
            inst = INST_GREATER_OR_EQUAL;
            break;

        default:
            return;

        }
        get_token();
        additive_expr();
        append_instruction(inst);
    }
}

void equality_expr()
{
    relational_expr();
    for(;;)
    {
        instruction_t inst(INST_END);
        switch(f_last_token.f_token)
        {
        case token_t::tok_t::TOK_EQUAL:
            inst = INST_EQUAL;
            break;

        case token_t::tok_t::TOK_NOT_EQUAL:
            inst = INST_NOT_EQUAL;
            break;

        default:
            return;

        }
        get_token();
        relational_expr();
        append_instruction(inst);
    }
}

void and_expr()
{
    equality_expr();
    while(f_last_token.f_token == token_t::tok_t::TOK_NCNAME && f_last_token.f_string == "and")
    {
        get_token();
        equality_expr();
        append_instruction(INST_AND);
    }
}

/** \brief The OrExpr
 *
 * The OrExpr is a one to one equivalent to the Expr.
 *
 * It appears between parenthesis from within a location path.
 */
void or_expr()
{
    and_expr();
    while(f_last_token.f_token == token_t::tok_t::TOK_NCNAME && f_last_token.f_string == "or")
    {
        get_token();
        and_expr();
        append_instruction(INST_OR);
    }
}


/** \brief Parse a function call parameters.
 *
 * This function handles internal function calls such as fn:position().
 *
 * If the prefix is not defined, we use 'fn' as the default. I'm not
 * totally sure that this is correct since some internal functions
 * are defined with the xs prefix. (the op:... functions are operators
 * only so it wouldn't apply as a default anyway.)
 *
 * Note that for speed a certain number of internal functions are fully
 * parsed here and transformed into code directly instead of an INST_CALL
 * instruction.
 *
 * \param[in] prefix_token  The prefix of the function, if "*" or "", then "fn" is used
 * \param[in] local_part  The name of the function (i.e. "position", "last", ...)
 */
void function_call(token_t prefix_token, token_t local_part)
{
    // the last token read was the '(', now we read the arguments which
    // each are full expressions:
    //      Argument ::= Expr
    //      Expr ::= OrExpr

    // skip the '('
    get_token();

    // make sure we have the default namespace if the user did not specify it
    if(prefix_token.f_string == "*" || prefix_token.f_string.isEmpty())
    {
        // the default namespace is "fn"
        prefix_token.f_string = "fn";
    }

    // set of internal functions that we can transform in one or a very few
    // basic instructions
    if(prefix_token.f_string == "fn")
    {
        switch(local_part.f_string[0].unicode())
        {
        case 'c':
            if(local_part.f_string == "ceiling")
            {
                if(f_last_token.f_token == token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected one parameter for the ceiling() function");
                }
                or_expr();
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected exactly one parameter for the ceiling() function");
                }
                append_instruction(INST_CEILING);
                // skip the ')'
                get_token();
                return;
            }
            if(local_part.f_string == "count")
            {
                if(f_last_token.f_token == token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected one parameter for the count() function");
                }
                or_expr();
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected exactly one parameter for the count() function");
                }
                append_instruction(INST_NODE_SET_SIZE);
                // skip the ')'
                get_token();
                return;
            }
            break;

        case 'e':
            if(local_part.f_string == "empty"
            || local_part.f_string == "exists")
            {
                if(f_last_token.f_token == token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected one parameter for the empty() function");
                }
                or_expr();
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected exactly one parameter for the empty() function");
                }
                append_instruction(INST_NODE_SET_SIZE);
                append_push_integer(0);
                append_instruction(INST_EQUAL);
                // skip the ')'
                get_token();
                return;
            }
            break;

        case 'f':
            if(local_part.f_string == "false")
            {
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected ')' immediately for the false() function does not accept parameters");
                }
                append_push_boolean(false);
                // skip the ')'
                get_token();
                return;
            }
            if(local_part.f_string == "floor")
            {
                if(f_last_token.f_token == token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected one parameter for the floor() function");
                }
                or_expr();
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected exactly one parameter for the floor() function");
                }
                append_instruction(INST_FLOOR);
                // skip the ')'
                get_token();
                return;
            }
            break;

        case 'l':
            if(local_part.f_string == "last")
            {
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected ')' immediately for the last() function does not accept parameters");
                }
                append_instruction(INST_GET_NODE_SET);
                append_instruction(INST_NODE_SET_SIZE);
                // skip the ')'
                get_token();
                return;
            }
            break;

        case 'n':
            if(local_part.f_string == "not")
            {
                if(f_last_token.f_token == token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected one parameter for the not() function");
                }
                or_expr();
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected exactly one parameter for the not() function");
                }
                append_instruction(INST_NOT);
                // skip the ')'
                get_token();
                return;
            }
            break;

        case 'p':
            if(local_part.f_string == "position")
            {
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected ')' immediately for the position() function does not accept parameters");
                }
                append_instruction(INST_GET_POSITION);
                // skip the ')'
                get_token();
                return;
            }
            break;

        case 'r':
            if(local_part.f_string == "round")
            {
                if(f_last_token.f_token == token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected one parameter for the round() function");
                }
                or_expr();
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected exactly one parameter for the round() function");
                }
                append_instruction(INST_ROUND);
                // skip the ')'
                get_token();
                return;
            }
            break;

        case 's':
            if(local_part.f_string == "string-length")
            {
                // TODO add support for '.' instead of an argument
                if(f_last_token.f_token == token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected one parameter for the string-length() function");
                }
                or_expr();
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected exactly one parameter for the string-length() function");
                }
                append_instruction(INST_STRING_LENGTH);
                // skip the ')'
                get_token();
                return;
            }
            break;

        case 't':
            if(local_part.f_string == "true")
            {
                if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("expected ')' immediately for the true() function does not accept parameters");
                }
                append_push_boolean(true);
                // skip the ')'
                get_token();
                return;
            }
            break;

        }
    }

    int argc(0);
    append_instruction(INST_PUSH_END_OF_ARGUMENTS);
    if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
    {
        ++argc;
        or_expr();
        while(f_last_token.f_token == token_t::tok_t::TOK_COMMA)
        {
            ++argc;
            get_token();
            or_expr();
        }
        if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
        {
            throw QDomXPathException_SyntaxError("expected ')' or ',' in the list of argument to a function call");
        }
    }
    // skip the ')'
    get_token();

    // by default we expect no parameters
    int min_argc(0);
    int max_argc(0);
    internal_func_t f(internal_func_t::FUNC_UNKNOWN);

    switch(prefix_token.f_string[0].unicode())
    {
    case 'f':
        if(prefix_token.f_string == "fn")
        {
            switch(local_part.f_string[0].unicode())
            {
            case 'a':
                if(local_part.f_string == "avg")
                {
                    f = internal_func_t::FUNC_AVG;
                    max_argc = -1; // any number of parameters
                }
                break;

            case 'm':
                if(local_part.f_string == "max")
                {
                    f = internal_func_t::FUNC_MAX;
                    max_argc = -1; // any number of parameters
                }
                else if(local_part.f_string == "min")
                {
                    f = internal_func_t::FUNC_MIN;
                    max_argc = -1; // any number of parameters
                }
                break;

            case 's':
                if(local_part.f_string == "sum")
                {
                    f = internal_func_t::FUNC_SUM;
                    max_argc = -1; // any number of parameters
                }
                break;

            }
        }
        break;

    default:
        break;

    }

    // we do not yet support user defined functions
    if(f == internal_func_t::FUNC_UNKNOWN)
    {
        throw QDomXPathException_UnknownFunctionError(QString("'%1' is not a known function (we may not yet support it...)")
                                                    .arg(prefix_token.f_string + ":" + local_part.f_string));
    }
    if(argc < min_argc)
    {
        throw QDomXPathException_UnknownFunctionError(QString("'%1' expects at least %2 arguments, but got %3 instead")
                                                    .arg(prefix_token.f_string + ":" + local_part.f_string)
                                                    .arg(min_argc).arg(argc));
    }
    if(max_argc != -1 && argc > max_argc)
    {
        throw QDomXPathException_UnknownFunctionError(QString("'%1' expects at most %2 arguments, it got %3 instead")
                                                    .arg(prefix_token.f_string + ":" + local_part.f_string)
                                                    .arg(max_argc).arg(argc));
    }

    append_push_integer(f);
    append_instruction(INST_CALL);
}


void predicate()
{
    QString save_predicate_variable(f_predicate_variable);
    ++f_label_counter;
    f_predicate_variable = QString("$%1").arg(f_label_counter);

    append_instruction(INST_CREATE_NODE_CONTEXT);

    // we just had an axis, now we have n predicates following
    // only the problem is that each predicate has to be applied
    // to each node as a context node of the current state
    do
    {
        // skip the '['
        get_token();

        // 'next_node' label
        const int next_node(f_program.size());

        append_instruction(INST_GET_CONTEXT_NODE);
        append_push_string(f_predicate_variable);
        append_instruction(INST_SET_VARIABLE);

        or_expr();
        if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_SQUARE_BRACKET)
        {
            throw QDomXPathException_SyntaxError("missing ']' to close a Predicate");
        }

        append_instruction(INST_PREDICATE); // "apply" predicate
        //append_instruction(INST_NEXT_CONTEXT_NODE);
        append_push_integer(next_node);
        append_instruction(INST_JUMP_IF_TRUE); // next_node
        append_instruction(INST_GET_RESULT);
        append_instruction(INST_SET_NODE_SET);
        append_instruction(INST_PUSH_EMPTY_NODE_SET);
        append_instruction(INST_SET_RESULT);

        get_token();
    }
    while(f_last_token.f_token == token_t::tok_t::TOK_OPEN_SQUARE_BRACKET);

    append_instruction(INST_GET_NODE_SET);
    append_instruction(INST_POP_CONTEXT); // get rid of the context

    f_predicate_variable = save_predicate_variable;
}

void location_path()
{
    labels_t labels;

    QString predicate_variable(f_predicate_variable);
    bool function_call_valid(f_last_token.f_token == token_t::tok_t::TOK_NCNAME);
    bool first_round(true);
    for(;;)
    {
        bool double_slash(false);
        switch(f_last_token.f_token)
        {
        case token_t::tok_t::TOK_DOUBLE_SLASH:
            double_slash = true;
        case token_t::tok_t::TOK_SLASH:
            if(first_round)
            {
                // this is absolute, start from the root

                // replace the current node-set into a node context
                // that way we can properly loop through the list of
                // nodes against this axis
                //
                // TODO: look into optimizating: return only one root
                //       per document...
                if(!predicate_variable.isEmpty())
                {
                    append_push_string(predicate_variable);
                    append_instruction(INST_GET_VARIABLE);
                    predicate_variable.clear();
                }
                append_instruction(INST_CREATE_NODE_CONTEXT);
                labels.push_back(f_program.size());
                append_instruction(INST_GET_CONTEXT_NODE);

                append_instruction(INST_ROOT);
            }
            get_token();
            break;

        default:
            if(!first_round)
            {
                // anything else is not part of the location path
                // if we are in a relative path (this allows "/")
                goto finished;
            }
            break;

        }
        bool accept_predicate(true);
        first_round = false;
        token_t save_token(f_last_token);
        token_t axis_token;
        axis_token.f_token = double_slash ? token_t::tok_t::TOK_AXIS_NAME_DESCENDANT : token_t::tok_t::TOK_AXIS_NAME_CHILD;
        axis_token.f_string = double_slash ? "descendant" : "child";
        token_t prefix_token;
        prefix_token.f_token = token_t::tok_t::TOK_PREFIX;
        prefix_token.f_string = "*";
        switch(f_last_token.f_token)
        {
        case token_t::tok_t::TOK_DOT:
            // self
            save_token.f_token = token_t::tok_t::TOK_NCNAME;
            save_token.f_string = "*";
            axis_token.f_token = token_t::tok_t::TOK_AXIS_NAME_SELF;
            axis_token.f_string = "self";
            accept_predicate = false;
            goto axis_apply;

        case token_t::tok_t::TOK_DOUBLE_DOT:
            // parent
            save_token.f_token = token_t::tok_t::TOK_NCNAME;
            save_token.f_string = "*";
            axis_token.f_token = token_t::tok_t::TOK_AXIS_NAME_PARENT;
            axis_token.f_string = "parent";
            accept_predicate = false;
            goto axis_apply;

        case token_t::tok_t::TOK_ASTERISK:
            // '*'  -- a name test by itself
            // this is actually the default!
            get_token();
            goto axis_apply;

        case token_t::tok_t::TOK_AT:
            // The '@' is the AbbreviatedAxisSpecifier
            //     '@' NoteTest Predicate*
            //     'attribute' :: NodeTest Predicate*
            axis_token.f_token = token_t::tok_t::TOK_AXIS_NAME_ATTRIBUTE;
            axis_token.f_string = "attribute";
            goto axis_name_attribute;

        case token_t::tok_t::TOK_NCNAME:
            get_token();
            if(f_last_token.f_token == token_t::tok_t::TOK_DOUBLE_COLON)
            {
                function_call_valid = false;
                // the NCNAME before '::' is an AxisName
                // AxisName '::' NoteTest Predicate*
                f_last_token = save_token;
                if(!token_is_axis_name())
                {
                    throw QDomXPathException_SyntaxError(QString("a double colon (::) must be preceded by a valid axis name, \"%1\" was not recognized as such").arg(f_last_token.f_string));
                }
                axis_token = f_last_token;
axis_name_attribute:
                get_token(); // NodeTest
                save_token = f_last_token;
                if(f_last_token.f_token == token_t::tok_t::TOK_ASTERISK)
                {
                    // no specific prefix or local part
                    get_token();
                    goto axis_apply;
                }
                if(f_last_token.f_token != token_t::tok_t::TOK_NCNAME)
                {
                    throw QDomXPathException_SyntaxError("a double colon (::) must be followed by an NCName or '*'");
                }
                get_token();
            }
            if(f_last_token.f_token == token_t::tok_t::TOK_COLON)
            {
                // namespace ':' NCName
                // namespace ':' '*'
                prefix_token = save_token;
                get_token();
                save_token = f_last_token;
                get_token();
                switch(save_token.f_token)
                {
                case token_t::tok_t::TOK_ASTERISK:
                    break;

                case token_t::tok_t::TOK_NCNAME:
                    if(function_call_valid && f_last_token.f_token == token_t::tok_t::TOK_OPEN_PARENTHESIS)
                    {
                        // A function name is a QName which means it can include a prefix
                        //      Prefix ':' LocalPart '(' ... ')'
                        function_call(prefix_token, save_token);
                        goto finished;
                    }
                    break;

                default:
                    throw QDomXPathException_SyntaxError("expected an NCName or '*' after a prefix");

                }
            }
            else if(f_last_token.f_token == token_t::tok_t::TOK_OPEN_PARENTHESIS)
            {
                // in this case we have:
                //    NodeType '(' [...] ')'
                //    FunctionCall '(' ... ')'
                f_last_token = save_token;
                if(!token_is_node_type())
                {
                    // Does a function call apply here?
                    //      LocalPart '(' ... ')'
                    if(function_call_valid)
                    {
                        // the default prefix token is "*" meaning no prefix
                        function_call(prefix_token, save_token);
                        goto finished;
                    }
                    // we got a function call here!
                    throw QDomXPathException_SyntaxError("a path followed by parenthesis must be a NodeType");
                }
                // save the NodeType token
                save_token.f_token = f_last_token.f_token;
                get_token();
                if(f_last_token.f_token == token_t::tok_t::TOK_STRING)
                {
                    if(axis_token.f_token != token_t::tok_t::TOK_NODE_TYPE_PROCESSING_INSTRUCTION)
                    {
                        throw QDomXPathException_InvalidError("only a processing-instruction NodeType can be given a Literal");
                    }
                    // a special case where the TOK_NODE_TYPE_PROCESSING_INSTRUCTION
                    axis_token.f_string = f_last_token.f_string;
                    get_token();
                }
                else
                {
                    axis_token.f_string = "";
                }
                if(f_last_token.f_token == token_t::tok_t::TOK_CLOSE_PARENTHESIS)
                {
                    throw QDomXPathException_SyntaxError("missing ')' after the NodeType definition");
                }
                prefix_token.f_token = token_t::tok_t::TOK_UNDEFINED;
            }

axis_apply:
            {
                // replace the current node-set into a node context
                // that way we can properly loop through the list of
                // nodes against this axis
                if(!predicate_variable.isEmpty())
                {
                    append_push_string(predicate_variable);
                    append_instruction(INST_GET_VARIABLE);
                    predicate_variable.clear();
                }
                append_instruction(INST_CREATE_NODE_CONTEXT);
                labels.push_back(f_program.size());
                append_instruction(INST_GET_CONTEXT_NODE);

                // axis_token (axis::)
                // prefix_token (prefix:)
                // save_token (path or '*' or NodeType)
                append_axis(axis_token, prefix_token, save_token);

                if(accept_predicate && f_last_token.f_token == token_t::tok_t::TOK_OPEN_SQUARE_BRACKET)
                {
                    // process all predicates following this step
                    predicate();
                }
            }
            break;

        default:
            if(!double_slash)
            {
                // in case we have a double slash, the relative path is optional
                //break 2;
                goto finished;
            }
            throw QDomXPathException_SyntaxError("expected a relative path, none found");

        }
        // if we loop function calls are not possible anymore
        function_call_valid = false;
    }

finished:
    for(int i(labels.size() - 1); i >= 0; --i)
    {
        // repeat with the next node of that context
        append_instruction(INST_NEXT_CONTEXT_NODE);
        append_push_integer(labels[i]);
        append_instruction(INST_JUMP_IF_TRUE);

        // return the result (node-set)
        append_instruction(INST_GET_RESULT);
        append_instruction(INST_POP_CONTEXT); // get rid of the node context
    }
}


void variable_reference()
{
    // we arrive here with the '$' as the token
    get_token();
    if(f_last_token.f_token != token_t::tok_t::TOK_NCNAME)
    {
        throw QDomXPathException_SyntaxError("expected a variable name after the '$' sign");
    }
    token_t prefix(f_last_token);
    get_token();
    if(f_last_token.f_token == token_t::tok_t::TOK_COLON)
    {
        get_token();
        if(f_last_token.f_token != token_t::tok_t::TOK_NCNAME)
        {
            throw QDomXPathException_SyntaxError(QString("expected a variable name after the prefix '%1:' sign").arg(prefix.f_string));
        }
        // concatenate the prefix within the name; not too sure why we'd
        // have a prefix in the first place and we'd not likely to use
        // variables at this point
        prefix.f_string += ":" + f_last_token.f_string;
    }
    append_push_token(prefix);
    append_instruction(INST_GET_VARIABLE);
}


void path_expr()
{
    switch(f_last_token.f_token)
    {
    case token_t::tok_t::TOK_OPEN_PARENTHESIS:
        // we directly call OrExpr since:
        //     Expr ::= OrExpr
        or_expr();
        if(f_last_token.f_token != token_t::tok_t::TOK_CLOSE_PARENTHESIS)
        {
            throw QDomXPathException_SyntaxError("expected a ')'");
        }
        break;

    case token_t::tok_t::TOK_INTEGER: // PathExpr => FilterExpr => PrimaryExpr
    case token_t::tok_t::TOK_REAL: // PathExpr => FilterExpr => PrimaryExpr
    case token_t::tok_t::TOK_STRING: // PathExpr => FilterExpr => PrimaryExpr
        append_push_token(f_last_token);
        get_token();
        break;

    case token_t::tok_t::TOK_DOLLAR:
        variable_reference();
        break;

    case token_t::tok_t::TOK_SLASH: // PathExpr => LocationPath => AbsolutePath
    case token_t::tok_t::TOK_DOUBLE_SLASH: // PathExpr => LocationPath => AbsolutePath
    case token_t::tok_t::TOK_NCNAME: // PathExpr => PrimaryExpr => FunctionCall
    case token_t::tok_t::TOK_AT: // PathExpr => LocationPath => RelativePath => Step => AxisSpecifier => AbbreviatedAxisSpecifier
    case token_t::tok_t::TOK_DOT:
    case token_t::tok_t::TOK_DOUBLE_DOT:
        location_path();
        break;

    case token_t::tok_t::TOK_PIPE:
        // the '|' character is for the caller
        return;

    default:
        throw QDomXPathException_SyntaxError(QString("unexpected token \"%1\"").arg(f_last_token.f_string));

    }
}


/** \brief The UnionExpr is where it starts in XPath version 1.0
 *
 * At this point the UnionExpr is as defined in version 1.0 of XPath. In
 * version 2.0 it changed and is a proper primary expression.
 *
 * The UnionExpr is quite peculiar at this point since if the path_expr
 * does not return with a TOK_PIPE, then the union does not happen and
 * the returned value has to be returned as is.
 */
void union_expr()
{
    // in case there is a pipe we'll need to have a duplicate of
    // the input...
    //append_instruction(INST_DUPLICATE1);
    //++f_label_counter;
    //QString variable(QString("$%1").arg(f_label_counter));
    //append_push_string(variable);
    //append_instruction(INST_SET_VARIABLE);

    path_expr();

    while(f_last_token.f_token == token_t::tok_t::TOK_PIPE)
    {
        // skip the pipe
        get_token();

        //append_push_string(variable);
        //append_instruction(INST_GET_VARIABLE);

        path_expr();
        append_instruction(INST_MERGE_SETS);
    }
}


void parse(bool show_commands)
{
    f_show_commands = show_commands;

    if(!get_token())
    {
        throw QDomXPathException_SyntaxError("calling parse() immediately generated an error");
    }
    //f_program.clear();
    f_label_counter = 0;

    // save the input (we expect a node-set) in a variable
    ++f_label_counter;
    f_predicate_variable = QString("$%1").arg(f_label_counter);
    append_push_string(f_predicate_variable);
    append_instruction(INST_SET_VARIABLE);

    union_expr();

    // terminate the program with an INST_END instruction
    append_instruction(INST_END);

    if(!f_future_labels.empty())
    {
        throw QDomXPathException_SyntaxError("some future labels never got defined");
    }
}


QDomXPath::node_vector_t apply(QDomXPath::node_vector_t& nodes)
{
    // reset the states and functions
    f_functions.clear();

    // setup the execution environment (program)
    function_t function;
    function.f_pc = f_program_start_offset;

    // set the node set on the stack
    variant_t node_set;
    node_set.setValue(nodes);
    function.f_stack.push_back(node_set);
    // one context to hold the result
    context_t context;
    function.f_contexts.push_back(context);

    f_functions.push_back(function);

    // execute the program
    for(;;)
    {
        uint32_t const pc(f_functions.back().f_pc);
        if(f_show_commands)
        {
            disassemble_instruction(pc);
        }

        instruction_t const instruction(f_program[pc]);
        if(instruction == INST_END)
        {
            // found the end of the program, return
            break;
        }
        ++f_functions.back().f_pc;
        (this->*g_instructions[instruction])();
    }

    // we must have just one function on exit
    if(f_functions.back().f_contexts.size() != 1)
    {
        throw QDomXPathException_InvalidError("function stack does not include just one item when existing program");
    }

    // the first context we push must still be here, but only that one
    if(f_functions.back().f_contexts.size() != 1)
    {
        throw QDomXPathException_InvalidError("context stack does not include just one item when existing program");
    }

    // retrieve the result
    if(f_functions.back().f_stack.size() != 1)
    {
        throw QDomXPathException_InvalidError("stack does not include just one item when existing program");
    }
    QDomXPath::node_vector_t result(f_functions.back().f_stack.back().getNodeSetValue());

    //context = f_functions.back().f_contexts.back();
    //QDomXPath::node_vector_t result(context.f_result);

    // reset the states and functions
    // (this won't happen on errors which is why we also do it on entry
    f_functions.clear();

    return result;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

uint32_t disassemble_undefined_instruction(uint32_t pc)
{
    std::cout << "***undefined instruction*** (" << f_program[pc - 1] << ")\n";
    return 1;
}

uint32_t disassemble_end(uint32_t pc)
{
    std::cout << "end\n";
    return 1;
}

uint32_t disassemble_call(uint32_t pc)
{
    std::cout << "call\n";
    return 1;
}

uint32_t disassemble_small_function(uint32_t pc)
{
    uint32_t size((f_program[pc + 0] << 8)
                 | f_program[pc + 1]);
    std::cout << "function (" << size << " bytes)\n";
    // we do not skip the function here, we only print the header
    // and the disassemble loop will print the contents
    // TODO what's missing is a way to mark the end of the function!
    return 3;
}

uint32_t disassemble_large_function(uint32_t pc)
{
    uint32_t size((f_program[pc + 0] << 24)
                | (f_program[pc + 1] << 16)
                | (f_program[pc + 2] << 8)
                |  f_program[pc + 3]);
    std::cout << "function (" << size << " bytes)\n";
    // we do not skip the function here, we only print the header
    // and the disassemble loop will print the contents
    // TODO what's missing is a way to mark the end of the function!
    return 5;
}

uint32_t disassemble_jump(uint32_t pc)
{
    std::cout << "jump\n";
    return 1;
}

uint32_t disassemble_jump_if_true(uint32_t pc)
{
    std::cout << "jump_if_true\n";
    return 1;
}

uint32_t disassemble_jump_if_false(uint32_t pc)
{
    std::cout << "jump_if_false\n";
    return 1;
}

uint32_t disassemble_jump_if_zero(uint32_t pc)
{
    std::cout << "jump_if_zero\n";
    return 1;
}

uint32_t disassemble_return(uint32_t pc)
{
    std::cout << "return\n";
    return 1;
}

uint32_t disassemble_get_variable(uint32_t pc)
{
    std::cout << "get_variable\n";
    return 1;
}

uint32_t disassemble_set_variable(uint32_t pc)
{
    std::cout << "set_variable\n";
    return 1;
}

uint32_t disassemble_pop1(uint32_t pc)
{
    std::cout << "pop 1\n";
    return 1;
}

uint32_t disassemble_pop2(uint32_t pc)
{
    std::cout << "pop 2\n";
    return 1;
}

uint32_t disassemble_pop3(uint32_t pc)
{
    std::cout << "pop 3\n";
    return 1;
}

uint32_t disassemble_pop4(uint32_t pc)
{
    std::cout << "pop 4\n";
    return 1;
}

uint32_t disassemble_pop5(uint32_t pc)
{
    std::cout << "pop 5\n";
    return 1;
}

uint32_t disassemble_duplicate1(uint32_t pc)
{
    std::cout << "duplicate 1\n";
    return 1;
}

uint32_t disassemble_duplicate2(uint32_t pc)
{
    std::cout << "duplicate 2\n";
    return 1;
}

uint32_t disassemble_duplicate3(uint32_t pc)
{
    std::cout << "duplicate 3\n";
    return 1;
}

uint32_t disassemble_duplicate4(uint32_t pc)
{
    std::cout << "duplicate 4\n";
    return 1;
}

uint32_t disassemble_duplicate5(uint32_t pc)
{
    std::cout << "duplicate 5\n";
    return 1;
}

uint32_t disassemble_swap1(uint32_t pc)
{
    std::cout << "swap 1, 2\n";
    return 1;
}

uint32_t disassemble_swap2(uint32_t pc)
{
    std::cout << "swap 1, 3\n";
    return 1;
}

uint32_t disassemble_swap3(uint32_t pc)
{
    std::cout << "swap 1, 4\n";
    return 1;
}

uint32_t disassemble_swap4(uint32_t pc)
{
    std::cout << "swap 1, 5\n";
    return 1;
}

uint32_t disassemble_swap5(uint32_t pc)
{
    std::cout << "swap 1, 6\n";
    return 1;
}

uint32_t disassemble_swap2_3(uint32_t pc)
{
    std::cout << "swap 2, 3\n";
    return 1;
}

uint32_t disassemble_push_any_string(uint32_t pc)
{
    std::cout << "push_string \"*\"\n";
    return 1;
}

uint32_t disassemble_push_byte(uint32_t pc)
{
    int64_t value(static_cast<int64_t>(f_program[pc]));
    std::cout << "push_integer " << value << std::endl;
    return 2;
}

uint32_t disassemble_push_double(uint32_t pc)
{
    union
    {
        uint64_t    f_int;
        double      f_dbl;
    } convert;
    convert.f_int = (static_cast<int64_t>(f_program[pc + 0]) << 56)
                  | (static_cast<int64_t>(f_program[pc + 1]) << 48)
                  | (static_cast<int64_t>(f_program[pc + 2]) << 40)
                  | (static_cast<int64_t>(f_program[pc + 3]) << 32)
                  | (static_cast<int64_t>(f_program[pc + 4]) << 24)
                  | (static_cast<int64_t>(f_program[pc + 5]) << 16)
                  | (static_cast<int64_t>(f_program[pc + 6]) <<  8)
                  |  static_cast<int64_t>(f_program[pc + 7]);
    std::cout << "push_double " << convert.f_dbl << std::endl;
    return 9;
}

uint32_t disassemble_push_double_zero(uint32_t pc)
{
    std::cout << "push_double_zero\n";
    return 1;
}

uint32_t disassemble_push_empty_node_set(uint32_t pc)
{
    std::cout << "push_empty_node_set\n";
    return 1;
}

uint32_t disassemble_push_empty_set(uint32_t pc)
{
    std::cout << "push_empty_set\n";
    return 1;
}

uint32_t disassemble_push_empty_string(uint32_t pc)
{
    std::cout << "push_string \"\"\n";
    return 1;
}

uint32_t disassemble_push_end_of_arguments(uint32_t pc)
{
    std::cout << "push_end_of_arguments\n";
    return 1;
}

uint32_t disassemble_push_false(uint32_t pc)
{
    std::cout << "push_false\n";
    return 1;
}

uint32_t disassemble_push_large_string(uint32_t pc)
{
    uint32_t size = (static_cast<uint32_t>(f_program[pc + 0]) << 24)
                  | (static_cast<uint32_t>(f_program[pc + 1]) << 16)
                  | (static_cast<uint32_t>(f_program[pc + 2]) <<  8)
                  |  static_cast<uint32_t>(f_program[pc + 3]);
    std::string str(reinterpret_cast<char *>(&f_program[pc + 4]), size);
    std::cout << "push_string \"" << str << "\"" << std::endl;
    return 5 + size;
}

uint32_t disassemble_push_long(uint32_t pc)
{
    int64_t value = (static_cast<int64_t>(f_program[pc + 0]) << 24)
                  | (static_cast<int64_t>(f_program[pc + 1]) << 16)
                  | (static_cast<int64_t>(f_program[pc + 2]) <<  8)
                  |  static_cast<int64_t>(f_program[pc + 3]);
    std::cout << "push_integer " << value << std::endl;
    return 5;
}

uint32_t disassemble_push_longlong(uint32_t pc)
{
    int64_t value = (static_cast<int64_t>(f_program[pc + 0]) << 56)
                  | (static_cast<int64_t>(f_program[pc + 1]) << 48)
                  | (static_cast<int64_t>(f_program[pc + 2]) << 40)
                  | (static_cast<int64_t>(f_program[pc + 3]) << 32)
                  | (static_cast<int64_t>(f_program[pc + 4]) << 24)
                  | (static_cast<int64_t>(f_program[pc + 5]) << 16)
                  | (static_cast<int64_t>(f_program[pc + 6]) <<  8)
                  |  static_cast<int64_t>(f_program[pc + 7]);
    std::cout << "push_integer " << value << std::endl;
    return 9;
}

uint32_t disassemble_push_medium_string(uint32_t pc)
{
    uint32_t size = (static_cast<uint32_t>(f_program[pc + 0]) << 8)
                  |  static_cast<uint32_t>(f_program[pc + 1]);
    std::string str(reinterpret_cast<char *>(&f_program[pc + 2]), size);
    std::cout << "push_string \"" << str << "\"\n";
    return 3 + size;
}

uint32_t disassemble_push_negative_byte(uint32_t pc)
{
    int64_t value(static_cast<int64_t>(f_program[pc]) | 0xFFFFFFFFFFFFFF00LL);
    std::cout << "push_integer " << value << std::endl;
    return 2;
}

uint32_t disassemble_push_negative_short(uint32_t pc)
{
    int64_t value((static_cast<int64_t>(f_program[pc]) << 8)
                 | static_cast<int64_t>(f_program[pc])
                 | 0xFFFFFFFFFFFF0000LL);
    std::cout << "push_integer " << value << std::endl;
    return 3;
}

uint32_t disassemble_push_negative_long(uint32_t pc)
{
    int64_t value((static_cast<int64_t>(f_program[pc]) << 24)
                | (static_cast<int64_t>(f_program[pc]) << 16)
                | (static_cast<int64_t>(f_program[pc]) << 8)
                |  static_cast<int64_t>(f_program[pc])
                |  0xFFFFFFFF00000000LL);
    std::cout << "push_integer " << value << std::endl;
    return 5;
}

uint32_t disassemble_push_short(uint32_t pc)
{
    int64_t value = (static_cast<int64_t>(f_program[pc + 0]) << 8)
                  |  static_cast<int64_t>(f_program[pc + 1]);
    std::cout << "push_integer " << value << std::endl;
    return 5;
}

uint32_t disassemble_push_small_string(uint32_t pc)
{
    uint32_t size = static_cast<uint32_t>(f_program[pc + 0]);
    std::string str(reinterpret_cast<char *>(&f_program[pc + 1]), size);
    std::cout << "push_string \"" << str << "\"\n";
    return 2 + size;
}

uint32_t disassemble_push_true(uint32_t pc)
{
    std::cout << "push_true\n";
    return 1;
}

uint32_t disassemble_push_zero(uint32_t pc)
{
    std::cout << "push_integer 0\n";
    return 1;
}

uint32_t disassemble_add(uint32_t pc)
{
    std::cout << "add\n";
    return 1;
}

uint32_t disassemble_and(uint32_t pc)
{
    std::cout << "and\n";
    return 1;
}

uint32_t disassemble_ceiling(uint32_t pc)
{
    std::cout << "ceiling\n";
    return 1;
}

uint32_t disassemble_decrement(uint32_t pc)
{
    std::cout << "decrement\n";
    return 1;
}

uint32_t disassemble_divide(uint32_t pc)
{
    std::cout << "divide\n";
    return 1;
}

uint32_t disassemble_equal(uint32_t pc)
{
    std::cout << "equal\n";
    return 1;
}

uint32_t disassemble_floor(uint32_t pc)
{
    std::cout << "floor\n";
    return 1;
}

uint32_t disassemble_greater_or_equal(uint32_t pc)
{
    std::cout << "greater_or_equal\n";
    return 1;
}

uint32_t disassemble_greater_than(uint32_t pc)
{
    std::cout << "greater_than\n";
    return 1;
}

uint32_t disassemble_idivide(uint32_t pc)
{
    std::cout << "idivide\n";
    return 1;
}

uint32_t disassemble_increment(uint32_t pc)
{
    std::cout << "increment\n";
    return 1;
}

uint32_t disassemble_less_or_equal(uint32_t pc)
{
    std::cout << "less_or_equal\n";
    return 1;
}

uint32_t disassemble_less_than(uint32_t pc)
{
    std::cout << "less_than\n";
    return 1;
}

uint32_t disassemble_modulo(uint32_t pc)
{
    std::cout << "modulo\n";
    return 1;
}

uint32_t disassemble_multiply(uint32_t pc)
{
    std::cout << "multiply\n";
    return 1;
}

uint32_t disassemble_negate(uint32_t pc)
{
    std::cout << "negate\n";
    return 1;
}

uint32_t disassemble_not(uint32_t pc)
{
    std::cout << "not\n";
    return 1;
}

uint32_t disassemble_not_equal(uint32_t pc)
{
    std::cout << "not_equal\n";
    return 1;
}

uint32_t disassemble_or(uint32_t pc)
{
    std::cout << "or\n";
    return 1;
}

uint32_t disassemble_round(uint32_t pc)
{
    std::cout << "round\n";
    return 1;
}

uint32_t disassemble_string_length(uint32_t pc)
{
    std::cout << "string_length\n";
    return 1;
}

uint32_t disassemble_subtract(uint32_t pc)
{
    std::cout << "subtract\n";
    return 1;
}

uint32_t disassemble_axis(uint32_t pc)
{
    std::cout << "axis\n";
    return 1;
}

uint32_t disassemble_root(uint32_t pc)
{
    std::cout << "root\n";
    return 1;
}

uint32_t disassemble_get_node_set(uint32_t pc)
{
    std::cout << "get_node_set\n";
    return 1;
}

uint32_t disassemble_set_node_set(uint32_t pc)
{
    std::cout << "set_node_set\n";
    return 1;
}

uint32_t disassemble_get_result(uint32_t pc)
{
    std::cout << "get_result\n";
    return 1;
}

uint32_t disassemble_set_result(uint32_t pc)
{
    std::cout << "set_result\n";
    return 1;
}

uint32_t disassemble_get_position(uint32_t pc)
{
    std::cout << "get_position\n";
    return 1;
}

uint32_t disassemble_set_position(uint32_t pc)
{
    std::cout << "set_position\n";
    return 1;
}

uint32_t disassemble_node_set_size(uint32_t pc)
{
    std::cout << "node_set_size\n";
    return 1;
}

uint32_t disassemble_merge_sets(uint32_t pc)
{
    std::cout << "merge_sets\n";
    return 1;
}

uint32_t disassemble_predicate(uint32_t pc)
{
    std::cout << "predicate\n";
    return 1;
}

uint32_t disassemble_create_node_context(uint32_t pc)
{
    std::cout << "create_node_context\n";
    return 1;
}

uint32_t disassemble_get_context_node(uint32_t pc)
{
    std::cout << "get_context_node\n";
    return 1;
}

uint32_t disassemble_next_context_node(uint32_t pc)
{
    std::cout << "next_context_node\n";
    return 1;
}

uint32_t disassemble_pop_context(uint32_t pc)
{
    std::cout << "pop_context\n";
    return 1;
}
#pragma GCC diagnostic pop



int disassemble_instruction(int const pc)
{
    // a small indentation
    std::ostream out(std::cout.rdbuf()); // copy so we don't mess up the manipulator in std::cout
    out << std::setw(6) << pc << "- ";
    instruction_t const inst(f_program[pc]);
    return (this->*g_disassemble_instructions[inst])(pc + 1);
}


void disassemble()
{
    int pc(f_program_start_offset);
    while(pc < f_program.size())
    {
        pc += disassemble_instruction(pc);
    }
}


QString setProgram(const QDomXPath::program_t& program, bool show_commands)
{
    if(program[0] != QDomXPath::MAGIC[0]
    || program[1] != QDomXPath::MAGIC[1]
    || program[2] != QDomXPath::MAGIC[2]
    || program[3] != QDomXPath::MAGIC[3])
    {
        throw QDomXPathException_InvalidMagic("this program does not start with the correct magic code");
    }
    if(program[4] != QDomXPath::VERSION_MAJOR
    || program[5] != QDomXPath::VERSION_MINOR)
    {
        // we need a better test as we have newer versions, at this time,
        // we do not need much more than what is here
        throw QDomXPathException_InvalidMagic("this program version is not compatible");
    }

    f_show_commands = show_commands;

    const int size((program[6] << 8) | program[7]);
    f_program_start_offset = size + 8;
    f_xpath = QString::fromUtf8(reinterpret_cast<const char *>(&program[8]), size);

    f_program = program;

    return f_xpath;
}


const QDomXPath::program_t& getProgram() const
{
    return f_program;
}


private:
    QDomXPath *                 f_owner;
    bool                        f_show_commands = false;

    // parser parameters
    QString                     f_xpath;
    const QChar *               f_start;
    const QChar *               f_in;
    token_t                     f_unget_token;
    token_t                     f_last_token;
    uint32_t                    f_label_counter = 0;
    label_offsets_t             f_labels;
    future_labels_t             f_future_labels;
    QString                     f_end_label;
    QString                     f_predicate_variable;

    // execution environment
    //QDomXPath::node_vector_t    f_result;
    int32_t                     f_program_start_offset = -1;
    QDomXPath::program_t        f_program;
    function_vector_t           f_functions;
};


QDomXPath::QDomXPathImpl::instruction_function_t const QDomXPath::QDomXPathImpl::g_instructions[256] =
{
      /* 0x00 */ &QDomXPathImpl::inst_end
    , /* 0x01 */ &QDomXPathImpl::inst_call
    , /* 0x02 */ &QDomXPathImpl::inst_small_function
    , /* 0x03 */ &QDomXPathImpl::inst_large_function
    , /* 0x04 */ &QDomXPathImpl::inst_jump
    , /* 0x05 */ &QDomXPathImpl::inst_jump_if_true
    , /* 0x06 */ &QDomXPathImpl::inst_jump_if_false
    , /* 0x07 */ &QDomXPathImpl::inst_jump_if_zero
    , /* 0x08 */ &QDomXPathImpl::inst_return
    , /* 0x09 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x0A */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x0B */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x0C */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x0D */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x0E */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x0F */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0x10 */ &QDomXPathImpl::inst_get_variable
    , /* 0x11 */ &QDomXPathImpl::inst_set_variable
    , /* 0x12 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x13 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x14 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x15 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x16 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x17 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x18 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x19 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x1A */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x1B */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x1C */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x1D */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x1E */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x1F */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0x20 */ &QDomXPathImpl::inst_pop1
    , /* 0x21 */ &QDomXPathImpl::inst_pop2
    , /* 0x22 */ &QDomXPathImpl::inst_pop3
    , /* 0x23 */ &QDomXPathImpl::inst_pop4
    , /* 0x24 */ &QDomXPathImpl::inst_pop5
    , /* 0x25 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x26 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x27 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x28 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x29 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x2A */ &QDomXPathImpl::inst_duplicate1
    , /* 0x2B */ &QDomXPathImpl::inst_duplicate2
    , /* 0x2C */ &QDomXPathImpl::inst_duplicate3
    , /* 0x2D */ &QDomXPathImpl::inst_duplicate4
    , /* 0x2E */ &QDomXPathImpl::inst_duplicate5
    , /* 0x2F */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0x30 */ &QDomXPathImpl::inst_swap1
    , /* 0x31 */ &QDomXPathImpl::inst_swap2
    , /* 0x32 */ &QDomXPathImpl::inst_swap3
    , /* 0x33 */ &QDomXPathImpl::inst_swap4
    , /* 0x34 */ &QDomXPathImpl::inst_swap5
    , /* 0x35 */ &QDomXPathImpl::inst_swap2_3
    , /* 0x36 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x37 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x38 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x39 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x3A */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x3B */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x3C */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x3D */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x3E */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x3F */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0x40 */ &QDomXPathImpl::inst_push_any_string
    , /* 0x41 */ &QDomXPathImpl::inst_push_byte
    , /* 0x42 */ &QDomXPathImpl::inst_push_double
    , /* 0x43 */ &QDomXPathImpl::inst_push_double_zero
    , /* 0x44 */ &QDomXPathImpl::inst_push_empty_node_set
    , /* 0x45 */ &QDomXPathImpl::inst_push_empty_set
    , /* 0x46 */ &QDomXPathImpl::inst_push_empty_string
    , /* 0x47 */ &QDomXPathImpl::inst_push_end_of_arguments
    , /* 0x48 */ &QDomXPathImpl::inst_push_false
    , /* 0x49 */ &QDomXPathImpl::inst_push_large_string
    , /* 0x4A */ &QDomXPathImpl::inst_push_long
    , /* 0x4B */ &QDomXPathImpl::inst_push_longlong
    , /* 0x4C */ &QDomXPathImpl::inst_push_medium_string
    , /* 0x4D */ &QDomXPathImpl::inst_push_negative_byte
    , /* 0x4E */ &QDomXPathImpl::inst_push_negative_short
    , /* 0x4F */ &QDomXPathImpl::inst_push_negative_long

    , /* 0x50 */ &QDomXPathImpl::inst_push_short
    , /* 0x51 */ &QDomXPathImpl::inst_push_small_string
    , /* 0x52 */ &QDomXPathImpl::inst_push_true
    , /* 0x53 */ &QDomXPathImpl::inst_push_zero
    , /* 0x54 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x55 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x56 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x57 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x58 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x59 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x5A */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x5B */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x5C */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x5D */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x5E */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x5F */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0x60 */ &QDomXPathImpl::inst_add
    , /* 0x61 */ &QDomXPathImpl::inst_and
    , /* 0x62 */ &QDomXPathImpl::inst_ceiling
    , /* 0x63 */ &QDomXPathImpl::inst_decrement
    , /* 0x64 */ &QDomXPathImpl::inst_divide
    , /* 0x65 */ &QDomXPathImpl::inst_equal
    , /* 0x66 */ &QDomXPathImpl::inst_floor
    , /* 0x67 */ &QDomXPathImpl::inst_greater_or_equal
    , /* 0x68 */ &QDomXPathImpl::inst_greater_than
    , /* 0x69 */ &QDomXPathImpl::inst_idivide
    , /* 0x6A */ &QDomXPathImpl::inst_increment
    , /* 0x6B */ &QDomXPathImpl::inst_less_or_equal
    , /* 0x6C */ &QDomXPathImpl::inst_less_than
    , /* 0x6D */ &QDomXPathImpl::inst_modulo
    , /* 0x6E */ &QDomXPathImpl::inst_multiply
    , /* 0x6F */ &QDomXPathImpl::inst_negate

    , /* 0x70 */ &QDomXPathImpl::inst_not
    , /* 0x71 */ &QDomXPathImpl::inst_not_equal
    , /* 0x72 */ &QDomXPathImpl::inst_or
    , /* 0x73 */ &QDomXPathImpl::inst_round
    , /* 0x74 */ &QDomXPathImpl::inst_string_length
    , /* 0x75 */ &QDomXPathImpl::inst_subtract
    , /* 0x76 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x77 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x78 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x79 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x7A */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x7B */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x7C */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x7D */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x7E */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x7F */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0x80 */ &QDomXPathImpl::inst_axis
    , /* 0x81 */ &QDomXPathImpl::inst_root
    , /* 0x82 */ &QDomXPathImpl::inst_get_node_set
    , /* 0x83 */ &QDomXPathImpl::inst_set_node_set
    , /* 0x84 */ &QDomXPathImpl::inst_get_result
    , /* 0x85 */ &QDomXPathImpl::inst_set_result
    , /* 0x86 */ &QDomXPathImpl::inst_get_position
    , /* 0x87 */ &QDomXPathImpl::inst_set_position
    , /* 0x88 */ &QDomXPathImpl::inst_node_set_size
    , /* 0x89 */ &QDomXPathImpl::inst_merge_sets
    , /* 0x8A */ &QDomXPathImpl::inst_predicate
    , /* 0x8B */ &QDomXPathImpl::inst_create_node_context
    , /* 0x8C */ &QDomXPathImpl::inst_get_context_node
    , /* 0x8D */ &QDomXPathImpl::inst_next_context_node
    , /* 0x8E */ &QDomXPathImpl::inst_pop_context
    , /* 0x8F */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0x90 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x91 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x92 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x93 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x94 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x95 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x96 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x97 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x98 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x99 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x9A */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x9B */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x9C */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x9D */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x9E */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0x9F */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0xA0 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xA1 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xA2 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xA3 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xA4 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xA5 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xA6 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xA7 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xA8 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xA9 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xAA */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xAB */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xAC */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xAD */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xAE */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xAF */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0xB0 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xB1 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xB2 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xB3 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xB4 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xB5 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xB6 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xB7 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xB8 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xB9 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xBA */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xBB */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xBC */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xBD */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xBE */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xBF */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0xC0 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xC1 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xC2 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xC3 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xC4 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xC5 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xC6 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xC7 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xC8 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xC9 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xCA */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xCB */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xCC */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xCD */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xCE */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xCF */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0xD0 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xD1 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xD2 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xD3 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xD4 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xD5 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xD6 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xD7 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xD8 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xD9 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xDA */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xDB */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xDC */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xDD */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xDE */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xDF */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0xE0 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xE1 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xE2 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xE3 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xE4 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xE5 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xE6 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xE7 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xE8 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xE9 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xEA */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xEB */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xEC */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xED */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xEE */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xEF */ &QDomXPathImpl::inst_undefined_instruction

    , /* 0xF0 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xF1 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xF2 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xF3 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xF4 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xF5 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xF6 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xF7 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xF8 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xF9 */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xFA */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xFB */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xFC */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xFD */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xFE */ &QDomXPathImpl::inst_undefined_instruction
    , /* 0xFF */ &QDomXPathImpl::inst_undefined_instruction
};


QDomXPath::QDomXPathImpl::disassembly_function_t const QDomXPath::QDomXPathImpl::g_disassemble_instructions[256] =
{
      /* 0x00 */ &QDomXPathImpl::disassemble_end
    , /* 0x01 */ &QDomXPathImpl::disassemble_call
    , /* 0x02 */ &QDomXPathImpl::disassemble_small_function
    , /* 0x03 */ &QDomXPathImpl::disassemble_large_function
    , /* 0x04 */ &QDomXPathImpl::disassemble_jump
    , /* 0x05 */ &QDomXPathImpl::disassemble_jump_if_true
    , /* 0x06 */ &QDomXPathImpl::disassemble_jump_if_false
    , /* 0x07 */ &QDomXPathImpl::disassemble_jump_if_zero
    , /* 0x08 */ &QDomXPathImpl::disassemble_return
    , /* 0x09 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x0A */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x0B */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x0C */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x0D */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x0E */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x0F */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0x10 */ &QDomXPathImpl::disassemble_get_variable
    , /* 0x11 */ &QDomXPathImpl::disassemble_set_variable
    , /* 0x12 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x13 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x14 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x15 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x16 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x17 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x18 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x19 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x1A */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x1B */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x1C */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x1D */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x1E */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x1F */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0x20 */ &QDomXPathImpl::disassemble_pop1
    , /* 0x21 */ &QDomXPathImpl::disassemble_pop2
    , /* 0x22 */ &QDomXPathImpl::disassemble_pop3
    , /* 0x23 */ &QDomXPathImpl::disassemble_pop4
    , /* 0x24 */ &QDomXPathImpl::disassemble_pop5
    , /* 0x25 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x26 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x27 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x28 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x29 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x2A */ &QDomXPathImpl::disassemble_duplicate1
    , /* 0x2B */ &QDomXPathImpl::disassemble_duplicate2
    , /* 0x2C */ &QDomXPathImpl::disassemble_duplicate3
    , /* 0x2D */ &QDomXPathImpl::disassemble_duplicate4
    , /* 0x2E */ &QDomXPathImpl::disassemble_duplicate5
    , /* 0x2F */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0x30 */ &QDomXPathImpl::disassemble_swap1
    , /* 0x31 */ &QDomXPathImpl::disassemble_swap2
    , /* 0x32 */ &QDomXPathImpl::disassemble_swap3
    , /* 0x33 */ &QDomXPathImpl::disassemble_swap4
    , /* 0x34 */ &QDomXPathImpl::disassemble_swap5
    , /* 0x35 */ &QDomXPathImpl::disassemble_swap2_3
    , /* 0x36 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x37 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x38 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x39 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x3A */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x3B */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x3C */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x3D */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x3E */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x3F */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0x40 */ &QDomXPathImpl::disassemble_push_any_string
    , /* 0x41 */ &QDomXPathImpl::disassemble_push_byte
    , /* 0x42 */ &QDomXPathImpl::disassemble_push_double
    , /* 0x43 */ &QDomXPathImpl::disassemble_push_double_zero
    , /* 0x44 */ &QDomXPathImpl::disassemble_push_empty_node_set
    , /* 0x45 */ &QDomXPathImpl::disassemble_push_empty_set
    , /* 0x46 */ &QDomXPathImpl::disassemble_push_empty_string
    , /* 0x47 */ &QDomXPathImpl::disassemble_push_end_of_arguments
    , /* 0x48 */ &QDomXPathImpl::disassemble_push_false
    , /* 0x49 */ &QDomXPathImpl::disassemble_push_large_string
    , /* 0x4A */ &QDomXPathImpl::disassemble_push_long
    , /* 0x4B */ &QDomXPathImpl::disassemble_push_longlong
    , /* 0x4C */ &QDomXPathImpl::disassemble_push_medium_string
    , /* 0x4D */ &QDomXPathImpl::disassemble_push_negative_byte
    , /* 0x4E */ &QDomXPathImpl::disassemble_push_negative_short
    , /* 0x4F */ &QDomXPathImpl::disassemble_push_negative_long

    , /* 0x50 */ &QDomXPathImpl::disassemble_push_short
    , /* 0x51 */ &QDomXPathImpl::disassemble_push_small_string
    , /* 0x52 */ &QDomXPathImpl::disassemble_push_true
    , /* 0x53 */ &QDomXPathImpl::disassemble_push_zero
    , /* 0x54 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x55 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x56 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x57 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x58 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x59 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x5A */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x5B */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x5C */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x5D */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x5E */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x5F */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0x60 */ &QDomXPathImpl::disassemble_add
    , /* 0x61 */ &QDomXPathImpl::disassemble_and
    , /* 0x62 */ &QDomXPathImpl::disassemble_ceiling
    , /* 0x63 */ &QDomXPathImpl::disassemble_decrement
    , /* 0x64 */ &QDomXPathImpl::disassemble_divide
    , /* 0x65 */ &QDomXPathImpl::disassemble_equal
    , /* 0x66 */ &QDomXPathImpl::disassemble_floor
    , /* 0x67 */ &QDomXPathImpl::disassemble_greater_or_equal
    , /* 0x68 */ &QDomXPathImpl::disassemble_greater_than
    , /* 0x69 */ &QDomXPathImpl::disassemble_idivide
    , /* 0x6A */ &QDomXPathImpl::disassemble_increment
    , /* 0x6B */ &QDomXPathImpl::disassemble_less_or_equal
    , /* 0x6C */ &QDomXPathImpl::disassemble_less_than
    , /* 0x6D */ &QDomXPathImpl::disassemble_modulo
    , /* 0x6E */ &QDomXPathImpl::disassemble_multiply
    , /* 0x6F */ &QDomXPathImpl::disassemble_negate

    , /* 0x70 */ &QDomXPathImpl::disassemble_not
    , /* 0x71 */ &QDomXPathImpl::disassemble_not_equal
    , /* 0x72 */ &QDomXPathImpl::disassemble_or
    , /* 0x73 */ &QDomXPathImpl::disassemble_round
    , /* 0x74 */ &QDomXPathImpl::disassemble_string_length
    , /* 0x75 */ &QDomXPathImpl::disassemble_subtract
    , /* 0x76 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x77 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x78 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x79 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x7A */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x7B */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x7C */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x7D */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x7E */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x7F */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0x80 */ &QDomXPathImpl::disassemble_axis
    , /* 0x81 */ &QDomXPathImpl::disassemble_root
    , /* 0x82 */ &QDomXPathImpl::disassemble_get_node_set
    , /* 0x83 */ &QDomXPathImpl::disassemble_set_node_set
    , /* 0x84 */ &QDomXPathImpl::disassemble_get_result
    , /* 0x85 */ &QDomXPathImpl::disassemble_set_result
    , /* 0x86 */ &QDomXPathImpl::disassemble_get_position
    , /* 0x87 */ &QDomXPathImpl::disassemble_set_position
    , /* 0x88 */ &QDomXPathImpl::disassemble_node_set_size
    , /* 0x89 */ &QDomXPathImpl::disassemble_merge_sets
    , /* 0x8A */ &QDomXPathImpl::disassemble_predicate
    , /* 0x8B */ &QDomXPathImpl::disassemble_create_node_context
    , /* 0x8C */ &QDomXPathImpl::disassemble_get_context_node
    , /* 0x8D */ &QDomXPathImpl::disassemble_next_context_node
    , /* 0x8E */ &QDomXPathImpl::disassemble_pop_context
    , /* 0x8F */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0x90 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x91 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x92 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x93 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x94 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x95 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x96 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x97 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x98 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x99 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x9A */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x9B */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x9C */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x9D */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x9E */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0x9F */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0xA0 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xA1 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xA2 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xA3 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xA4 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xA5 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xA6 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xA7 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xA8 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xA9 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xAA */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xAB */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xAC */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xAD */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xAE */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xAF */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0xB0 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xB1 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xB2 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xB3 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xB4 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xB5 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xB6 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xB7 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xB8 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xB9 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xBA */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xBB */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xBC */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xBD */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xBE */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xBF */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0xC0 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xC1 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xC2 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xC3 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xC4 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xC5 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xC6 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xC7 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xC8 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xC9 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xCA */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xCB */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xCC */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xCD */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xCE */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xCF */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0xD0 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xD1 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xD2 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xD3 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xD4 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xD5 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xD6 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xD7 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xD8 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xD9 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xDA */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xDB */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xDC */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xDD */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xDE */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xDF */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0xE0 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xE1 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xE2 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xE3 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xE4 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xE5 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xE6 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xE7 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xE8 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xE9 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xEA */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xEB */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xEC */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xED */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xEE */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xEF */ &QDomXPathImpl::disassemble_undefined_instruction

    , /* 0xF0 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xF1 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xF2 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xF3 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xF4 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xF5 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xF6 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xF7 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xF8 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xF9 */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xFA */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xFB */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xFC */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xFD */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xFE */ &QDomXPathImpl::disassemble_undefined_instruction
    , /* 0xFF */ &QDomXPathImpl::disassemble_undefined_instruction
};


/** \class QDomXPath
 * \brief A private class used to handle XPath expressions.
 *
 * This class parses the XPath expression and is capable of executing it
 * against a QDomNode.
 *
 * The class is based on the XPath syntax as defined by the W3C consortium:
 *
 * http://www.w3.org/TR/xpath/#node-sets
 *
 * In a way, this is a rewrite of the QXmlQuery, except that this
 * implementation can be used against a QDomNode and thus I can avoid all
 * the problems with the QXmlQuery (i.e. having to convert the XML back to
 * text so it can be used with QXmlQuery without crashing.)
 *
 * As per point "2 Basics" XQuery is a case-sensitive language so we read
 * everything as is and test instructions in lowercase only as expected
 * by the language. There are a few exceptions when testing certain values
 * such as the contains of the lang attribute of the name of a processing
 * instruction language (i.e "php" matches the language name in
 * "&lt;?PHP ... ?&gt;").
 *
 * http://www.w3.org/TR/xquery/#id-basics
 *
 * \note
 * All the expressions are not supported.
 *
 * The following is 'UnionExpr' and corresponding children rules as
 * defined on the w3c website:
 *
 * \code
 * Expr ::= OrExpr
 *
 * PrimaryExpr ::= VariableReference
 *               | '(' Expr ')'
 *               | Literal
 *               | Number
 *               | FunctionCall
 *
 * FunctionCall ::= FunctionName '(' ( Argument ( ',' Argument )* )? ')'
 *
 * Argument ::= Expr
 *
 * OrExpr ::= AndExpr
 *          | OrExpr 'or' AndExpr
 *
 * AndExpr ::= EqualityExpr
 *          | AndExpr 'and' EqualityExpr
 *
 * EqualityExpr ::= RelationalExpr
 *          | EqualityExpr '=' RelationalExpr
 *          | EqualityExpr '!=' RelationalExpr
 *
 * RelationalExpr ::= AdditiveExpr
 *          | RelationalExpr '<' AdditiveExpr
 *          | RelationalExpr '>' AdditiveExpr
 *          | RelationalExpr '<=' AdditiveExpr
 *          | RelationalExpr '>=' AdditiveExpr
 *
 * AdditiveExpr ::= MultiplicativeExpr
 *          | AdditiveExpr '+' MultiplicativeExpr
 *          | AdditiveExpr '-' MultiplicativeExpr
 *
 * MultiplicativeExpr ::= UnaryExpr
 *          | MultiplicativeExpr MultiplicativeOperator UnaryExpr
 *          | MultiplicativeExpr 'div' UnaryExpr
 *          | MultiplicativeExpr 'mod' UnaryExpr
 *
 * UnaryExpr ::= UnionExpr
 *          | '-' UnaryExpr
 *
 * UnionExpr ::= PathExpr
 *             | UnionExpr '|' PathExpr
 *
 * PathExpr ::= LocationPath
 *            | FilterExpr
 *            | FilterExpr '/' RelativeLocationPath
 *            | FilterExpr '//' RelativeLocationPath
 *
 * FilterExpr ::= PrimaryExpr
 *              | FilterExpr Predicate
 *
 * LocationPath ::= RelativeLocationPath
 *                | AbsoluteLocationPath
 *
 * AbsoluteLocationPath ::= '/' RelativeLocationPath?
 *                        | AbbreviatedAbsoluteLocationPath
 *
 * AbbreviatedAbsoluteLocationPath ::= '//' RelativeLocationPath
 *
 * RelativeLocationPath ::= Step
 *                        | RelativeLocationPath '/' Step
 *                        | AbbreviatedRelativeLocationPath
 *
 * AbbreviatedRelativeLocationPath ::= RelativeLocationPath '//' Step
 *
 * Step ::= AxisSpecifier NodeTest Predicate*
 *        | AbbreviatedStep
 *
 * AxisSpecifier ::= AxisName '::'
 *                 | AbbreviatedAxisSpecifier
 *
 * AxisName ::= 'ancestor'
 *            | 'ancestor-or-self'
 *            | 'attribute'
 *            | 'child'
 *            | 'descendant'
 *            | 'descendant-or-self'
 *            | 'following'
 *            | 'following-sibling'
 *            | 'namespace'
 *            | 'parent'
 *            | 'preceding'
 *            | 'preceding-sibling'
 *            | 'self'
 *
 * NodeTest ::= NameTest
 *            | NodeType '(' ')'
 *            | 'processing-instruction' '(' Literal ')'
 *
 * Predicate ::= '[' PredicateExpr ']'
 *
 * PredicateExpr ::= Expr
 *
 * AbbreviatedStep ::= '.'
 *                   | '..'
 *
 * AbbreviatedAxisSpecifier ::= '@'?
 *
 * ExprToken ::= '(' | ')'
 *             | '[' | ']'
 *             | '.'
 *             | '..'
 *             | '@'
 *             | ','
 *             | '::'
 *             | NameTest
 *             | NodeType
 *             | Operator
 *             | FunctionName
 *             | AxisName
 *             | Literal
 *             | Number
 *             | VariableReference
 *
 * Literal ::= '"' [^"]* '"'
 *           | "'" [^']* "'"
 *
 * Number ::= Digits ('.' Digits?)?
 *          | '.' Digits
 *
 * Digits ::= [0-9]+
 *
 * Operator ::= OperatorName
 *            | MultiplyOperator
 *            | '/'
 *            | '//'
 *            | '|'
 *            | '+'
 *            | '-'
 *            | '='
 *            | '!='
 *            | '<'
 *            | '<='
 *            | '>'
 *            | '>='
 *
 * OperatorName ::= 'and'
 *                | 'or'
 *                | 'mod'
 *                | 'div'
 *
 * MultiplyOperator ::= '*'
 *
 * FunctionName ::= QName - NodeType
 *
 * VariableReference ::= '$' QName
 *
 * NameTest ::= '*'
 *            | NCName ':' '*'
 *            | QName
 *
 * NodeType ::= 'comment'
 *            | 'text'
 *            | 'processing-instruction'
 *            | 'node'
 *
 * ExprWhitespace ::= S
 *
 * NCName ::= Name - (Char* ':' Char*)
 *
 * S ::= (#x20 | #x9 | #xD | #xA)+
 *
 * Char ::= #x9
 *        | #xA
 *        | #xD
 *        | [#x20-#xD7FF]
 *        | [#xE000-#xFFFD]
 *        | [#x10000-#x10FFFF]
 *
 * NameStartChar ::= ':'
 *                 | [A-Z]
 *                 | '_'
 *                 | [a-z]
 *                 | [#xC0-#xD6]
 *                 | [#xD8-#xF6]
 *                 | [#xF8-#x2FF]
 *                 | [#x370-#x37D]
 *                 | [#x37F-#x1FFF]
 *                 | [#x200C-#x200D]
 *                 | [#x2070-#x218F]
 *                 | [#x2C00-#x2FEF]
 *                 | [#x3001-#xD7FF]
 *                 | [#xF900-#xFDCF]
 *                 | [#xFDF0-#xFFFD]
 *                 | [#x10000-#xEFFFF]
 *
 * NameChar ::= NameStartChar
 *            | '-'
 *            | '.'
 *            | [0-9]
 *            | #xB7
 *            | [#x0300-#x036F]
 *            | [#x203F-#x2040]
 *
 * Name ::= NameStartChar (NameChar)*
 *
 * Names ::= Name (#x20 Name)*
 *
 * Nmtoken ::= (NameChar)+
 *
 * Nmtokens ::= Nmtoken (#x20 Nmtoken)*
 *
 * QName ::= PrefixedName
 *         | UnprefixedName
 *
 * PrefixedName ::= Prefix ':' LocalPart
 *
 * UnprefixedName ::= LocalPart
 *
 * Prefix ::= NCName
 *
 * LocalPart ::= NCName
 * \endcode
 */



/** \brief Initialize the QDomXPath object.
 *
 * The constructor initializes the QDomXPath object. By default, the XPath
 * is viewed as "." and the internal parameter (f_impl) is set to NULL
 * until you call setXPath() or setProgram(). Once a program was defined,
 * it is possible to apply an XML file against the XPath by calling the
 * apply() functions.
 *
 * The array of variable is initialized, but it remains empty until you
 * bind some variables to it with bindVariable().
 *
 * \sa bindVariable()
 * \sa setXPath()
 * \sa setProgram()
 * \sa apply()
 */
QDomXPath::QDomXPath()
    //: f_xpath("") -- auto-init
    : f_impl(NULL)
    //, f_variables() -- auto-init
{
}


/** \brief Clean up the QDomXPath object.
 *
 * Since the f_impl variable member is defined in the qdomxpath.cpp, I
 * cannot have it as a smart pointer in the qdomxpath.h file. For this
 * reason, I manage it here.
 */
QDomXPath::~QDomXPath()
{
    delete f_impl;
}


/** \brief Set the XPath.
 *
 * This function sets the XPath of the QDomXPath object. By default, the
 * XPath is set to "." (i.e. return the current node.)
 *
 * If the XPath is considered invalid, then this function returns false
 * and the internal state is not changed. If considered valid, then the
 * new XPath takes effect and the function returns true.
 *
 * Note that if xpath is set to the empty string or ".", it is always
 * accepted and in both cases it represents the current node.
 *
 * \param[in] xpath  The new XPath to use in this QDomXPath.
 * \param[in] show_commands  Show the assembly while compiling.
 *
 * \return true if the \p xpath is valid, false otherwise.
 */
bool QDomXPath::setXPath(const QString& xpath, bool show_commands)
{
    if(xpath.isEmpty() || xpath == ".")
    {
        f_xpath = "";
        delete f_impl;
        f_impl = NULL;
        return true;
    }

    QDomXPath::QDomXPathImpl *impl(new QDomXPath::QDomXPathImpl(this, xpath));
    try
    {
        impl->parse(show_commands);
    }
    catch(...)
    {
        delete impl;
        throw;
    }

    // by contract this changes only at the very end of the function
    f_xpath = xpath;
    delete f_impl;
    f_impl = impl;

    return true;
}


/** \brief Get the current xpath.
 *
 * This function returns the current XPath. If it was never set, then the
 * function returns ".". Note that if the setXPath() function returns false,
 * then the XPath doesn't get changed and thus this function returns the
 * previous XPath.
 *
 * \return The XPath string, or "." if not yet defined.
 */
QString QDomXPath::getXPath() const
{
    if(f_xpath.isEmpty())
    {
        return ".";
    }
    return f_xpath;
}


/** \brief Apply the XPath against the specified node.
 *
 * This function applies (queries) the XPath that was previously set with
 * the setXPath() function agains the input \p node parameter.
 *
 * The function returns a vector of nodes because it is not possible to
 * add parameters to a QDomNodeList without being within the implementation
 * (i.e. there is no function to add any node to the list.) This may be
 * because a list of nodes is dynamic, it includes a way to remove the
 * node from the list in the event the node gets deleted (just an assumption
 * of course.)
 *
 * \todo
 * We want to implement an apply() function that can return processed
 * data so we could get a string or an integer instead of a list of
 * nodes.
 *
 * \note
 * If no program was loaded, this function returns its input as is.
 *
 * \note
 * XPath does not modifies its input document.
 *
 * \param[in] node  The node to query.
 *
 * \return A list of nodes (maybe empty.)
 */
QDomXPath::node_vector_t QDomXPath::apply(QDomNode node) const
{
    node_vector_t nodes;
    nodes.push_back(node);
    if(f_impl)
    {
        return f_impl->apply(nodes);
    }
    // no f_impl means "." which is just this node
    return nodes;
}


/** \brief Apply the XPath against the specified list of nodes.
 *
 * This function applies (queries) the XPath that was previously set with
 * one of the setXPath() or setProgram() functions against the set of
 * input nodes.
 *
 * \todo
 * We want to implement an apply() function that can return processed
 * data so we could get a string or an integer instead of a list of
 * nodes.
 *
 * \note
 * The different nodes in the node vector do not all need to be from the
 * same document.
 *
 * \note
 * If no program was loaded, this function returns its input as is.
 *
 * \note
 * XPath does not modifies its input document.
 *
 * \param[in] nodes  The list of nodes to query.
 *
 * \return A list of nodes (maybe empty.)
 */
QDomXPath::node_vector_t QDomXPath::apply(node_vector_t nodes) const
{
    if(f_impl)
    {
        return f_impl->apply(nodes);
    }
    // no f_impl means "." which is just these nodes
    return nodes;
}


/** \brief Disassemble the program.
 *
 * This function prints out the disassembled program in your stdout.
 *
 * The disassembled program shows the pointer counter (position inside the
 * program) the instruction, and for PUSH instruction, the data getting
 * pushed (i.e. a number or a string.)
 *
 * This function is used by the -d option of the cxpath compiler.
 */
void QDomXPath::disassemble() const
{
    if(f_impl)
    {
        f_impl->disassemble();
    }
    else
    {
        throw QDomXPathException_InternalError("error: no program to disassemble");
    }
}


/** \brief Bind a variable to this DOM XPath.
 *
 * This function binds the variable named \p name to this XPath.
 *
 * Within the script, variable can be accessed using the $\<name> syntax.
 *
 * \note
 * The bind function does not (yet) verify that the variable name is
 * valid. It has to be a valid QName to work with the XPath. A QName
 * is defined as an optional prefix, colon, and local-part:
 *
 * \code
 * [ <prefix> ':' ] <local-part>
 * \endcode
 *
 * \param[in] name  The name of the variable.
 * \param[in] value  The value of the variable.
 */
void QDomXPath::bindVariable(const QString& name, const QString& value)
{
    f_variables[name] = value;
}


/** \brief Check whether a variable is defined.
 *
 * This function checks whether a variable is set. If so, the function
 * returns true.
 *
 * Note that it is important for you to call this function if you'd like
 * to get a variable contents and not throw if the variable does not
 * exist:
 *
 * \code
 * if(xpath->hasVariable(name))
 * {
 *   value = xpath->getVariable(name);
 * }
 * else
 * {
 *   value = "*"; // some default value
 * }
 * \endcode
 *
 * \param[in] name  The name of the variable to check.
 *
 * \return true if the variable is defined, false otherwise.
 */
bool QDomXPath::hasVariable(const QString& name)
{
    return f_variables.contains(name);
}


/** \brief Retrieve the variable.
 *
 * This function is used to retrieve the value of a variable bound with
 * the XPath.
 *
 * It is used internally with the $\<QName\> syntax. Note that if a function
 * is called, then that function's variables are checked first and will
 * shadow the main variables.
 *
 * \exception QDomXPathException_UndefinedVariable
 * If the variable does not exist then the function raises this exception.
 * To avoid the exception, use the hasVariable() predicate first.
 *
 * \param[in] name  The name of the variable to retrieve.
 *
 * \return A copy of the variable contents as a string.
 */
QString QDomXPath::getVariable(const QString& name)
{
    if(!f_variables.contains(name))
    {
        throw QDomXPathException_UndefinedVariable(QString("variable \"%1\" is not defined").arg(name));
    }
    return f_variables[name];
}


/** \brief Set the program.
 *
 * When you store a previously compiled program somewhere (you can retrieve
 * a compiled program with the getProgram() function), you can later reload
 * it in your QDomXPath object with the setProgram() function.
 *
 * This is quite useful to compile many XPaths, save them in a file or
 * your Qt resources, and later load them and pass them to this function
 * for instant processing.
 *
 * Note that small XPaths may get compiled faster than the load from a file.
 * It will be up to you to test what seems to be the fastest. Very large
 * and complex XPaths are likely to benefit from a pre-compilation.
 *
 * \param[in] program  The program to save in this QDomXPath object.
 * \param[in] show_commands  Whether the commands are shown in stdout while executing the program.
 */
void QDomXPath::setProgram(const QDomXPath::program_t& program, bool show_commands)
{
    if(f_impl == NULL)
    {
        // the original XPath is not known (although it could be saved
        // in the program and restored?)
        f_impl = new QDomXPath::QDomXPathImpl(this, "");
    }
    f_xpath = f_impl->setProgram(program, show_commands);
}



/** \brief Retrieve the compiled program.
 *
 * The program can be retrieved after calling the setXPath() function.
 * The program must be considered to be an array of bytes once outside
 * of the QDomXPath environment.
 *
 * For the Unix 'file' tool trying to determine the type of a file, it
 * can checks the first few bytes as:
 *
 * \li Byte 0 -- 'X'
 * \li Byte 1 -- 'P'
 * \li Byte 2 -- 'T'
 * \li Byte 3 -- 'H'
 * \li Byte 4 -- The major version, 0x01 or more (unsigned)
 * \li Byte 5 -- The minor version, 0x00 or more (unsigned)
 * \li Byte 6 -- higher size
 * \li Byte 7 -- lower size
 *
 * The first 8 bytes are then followed by the original XPath so one can
 * retrieve it, just in case. Bytes 6 and 7 represent the size of that
 * XPath. If the XPath is more than 64Kb then only the first 65535 bytes
 * are saved in the file.
 *
 * Note that it is possible to set byte 6 and 7 to zero and not save
 * the XPath (this can be useful to save space.)
 *
 * \return A constant reference to the program.
 */
const QDomXPath::program_t& QDomXPath::getProgram() const
{
    if(f_impl)
    {
        return f_impl->getProgram();
    }
    throw QDomXPathException_InternalError("error: no program to retrieve");
}



// vim: ts=4 sw=4 et
