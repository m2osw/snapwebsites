// Snap Communicator -- classes to ease handling communication between processes
// Copyright (C) 2012-2018  Made to Order Software Corp.
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

#include "snapwebsites/snap_communicator.h"
#include "snapwebsites/log.h" // should be in the snap_communicator.h but can't really

namespace snap
{


/** \brief A template to create a list of messages to dispatch on receival.
 *
 * Whenever you receive messages, they can automatically get dispatched to
 * various functions using the dispatcher.
 *
 * You define a dispatcher_match array and then add a dispatcher to
 * your connection object.
 *
 * \code
 *      snap::dispatcher<my_connection>::dispatcher_match const my_messages[] =
 *      {
 *          {
 *              "HELP"
 *            , &my_connection::msg_help
 *            //, &dispatcher<my_connection>::dispatcher_match::one_to_one_match -- use default
 *          },
 *          {
 *              "STATUS"
 *            , &my_connection::msg_status
 *            //, &dispatcher<my_connection>::dispatcher_match::one_to_one_match -- use default
 *          },
 *          ... // other messages
 *
 *          // if you'd like you can end your list with a catch all which
 *          // generate the UNKNOWN message with the following (not required)
 *          // if you have that entry, your own process_message() function
 *          // will not get called
 *          {
 *              nullptr
 *            , &dispatcher<my_connection>::dispatcher_match::msg_reply_with_unknown
 *            , &dispatcher<my_connection>::dispatcher_match::always_match
 *          },
 *      };
 * \endcode
 *
 * In most cases you do not need to specify the matching function. It will
 * use the default which is a one to one match. So in the example above,
 * for "HELP", only a message with the command set to "HELP" will match.
 * When a match is found, the correcsponding function (msg_help() here)
 * gets called.
 *
 * Note that "functions" are actually offsets. You will get `this` defined
 * as expected when your function gets called. The one drawback is that
 * only a function of the connection you attach the dispatcher to can
 * be called from the dispatcher. This is because we want to have a
 * static table instead of a dynamic one created each time we start
 * a process (in a website server, many processes get started again and
 * again.)
 *
 * \note
 * This is documented here because it is a template and we cannot do
 * that in the .cpp (at least older versions of doxygen could not.)
 */
template<typename T>
class dispatcher
    : public dispatcher_base
{
public:
    /** \brief A smart pointer of the dispatcher.
     *
     * Although we expect the array of `dispatcher_match` to be
     * statically defined, the `dispatcher`, on the other hand,
     * is quite dynamic and needs to be allocated in a smart
     * pointer then added to your connection.
     */
    typedef std::shared_ptr<dispatcher> pointer_t;

    /** \brief This structure is used to define the list of supported messages.
     *
     * Whenever you create an array of messages, you use this structure.
     *
     * The structure takes a few parameters as follow:
     *
     * \li f_expr -- the "expression" to be matched to the command name
     *               for example "HELP".
     * \li f_execute -- the function offset to execute on a match.
     * \li f_match -- the function to check whether the expression is a match.
     *
     * The command name is called "f_expr" but some matching functions may
     * make use of the "f_expr" parameter as an expression such as a
     * regular expression. (such function will be added with time.)
     */
    struct dispatcher_match
    {
        /** \brief The execution function.
         *
         * This type defines the execution function. We give it the message
         * on a match. If the command name is not a match, it is ignored.
         */
        typedef void (T::*execute_func_t)(snap_communicator_message & msg);

        /** \brief The match function.
         *
         * This type defines the match function. We give it the message
         * which has the command name, although specialized matching
         * function could test other parameters from the message such
         * as the origination of the message.
         */
        typedef bool (*match_func_t)(QString const & expr, snap_communicator_message & msg);

        /** \brief The default matching function.
         *
         * This function checks the command one to one to the expression.
         * The word in the expression is compared as is to the command
         * name:
         *
         * \code
         *          return expr == msg.get_command();
         * \endcode
         *
         * We will add other matching functions with time
         * (start_with_match(), regex_match(), etc.)
         *
         * \note
         * It is permissible to use a match function to modify the
         * message in some way, however, it is not recommended.
         *
         * \param[in] expr  The expression to compare the command against.
         * \param[in] msg  The message to match against this expression.
         *
         * \return true if it is a match, false otherwise.
         */
        static bool one_to_one_match(QString const & expr, snap_communicator_message & msg)
        {
            return expr == msg.get_command();
        }

        /** \brief Always returns true.
         *
         * This function always returns true. This is practical to close
         * your list of messages and return a specific message. In most
         * cases this is used to reply with the UNKNOWN message.
         *
         * \param[in] expr  The expression to compare the command against.
         * \param[in] msg  The message to match against this expression.
         *
         * \return Always returns true.
         */
        static bool always_match(QString const & expr, snap_communicator_message & msg)
        {
            NOTUSED(expr);
            NOTUSED(msg);
            return true;
        }

        /** \brief The expression to compare against.
         *
         * The expression is most often going to be the exact command name
         * which will be matched with the one_to_one_match() function.
         *
         * For other match functions, this would be whatever type of
         * expression supported by those other functions.
         */
        char const *        f_expr;

        /** \brief The execute function.
         *
         * This is an offset in your connection class. We do not allow
         * std::bind() because we do not want the array of messages to be
         * dynamic (that way it is created at compile time and loaded as
         * ready/prepared data on load.)
         *
         * The functions called have `this` defined so you can access
         * your connection data and other functions. It requires the
         * `&` and the class name to define the pointer, like this:
         *
         * \code
         *      &MyClass::my_message_function
         * \endcode
         *
         * The execution is started by calling the run() function.
         */
        execute_func_t      f_execute;

        /** \brief The match function.
         *
         * The match function is used to know whether that command
         * dispatch function was found.
         *
         * By default this parameter is set to one_to_one_match().
         * This means the command has to be one to one equal to
         * the f_expr string.
         *
         * The matching is done in the match() function.
         */
        match_func_t        f_match = &dispatcher<T>::dispatcher_match::one_to_one_match;

        /** \brief Run the execution function if this is a match.
         *
         * First this function checks whether the command of the message
         * in \p msg matches this `dispatcher_match` expression. In
         * most cases the match function is going to be
         * one_on_one_match() which means it has to be exactly equal.
         *
         * If it is a match, this function runs your \p connection execution
         * function (i.e. the message gets dispatched) and then it returns
         * true.
         *
         * If the message is not a match, then the function returns false
         * and only the matching function was called. In this case the
         * \p connection does not get used.
         *
         * When this function returns true, you should not call the
         * process_message() function since that was already taken care
         * of. The process_message() function should only be called
         * if the message was not yet dispatched. When the list of
         * matches includes a catch all at the end, the process_message()
         * will never be called.
         *
         * \note
         * Note that we do not offer two functions, one to run the match
         * function and one to execute the match because you could otherwise
         * end up calling the execute function (dispatch) on any
         * `dispatcher_match` entry and we did not want that to happen.
         *
         * \param[in] connection  The connection attached to that
         *                        `dispatcher_match`.
         * \param[in] msg  The message that matched.
         *
         * \return true if the connection execute function was called.
         */
        bool execute(T * connection, snap::snap_communicator_message & msg) const
        {
            if(f_match(f_expr, msg))
            {
                (connection->*f_execute)(msg);
                return true;
            }

            return false;
        }
    };

private:
    /** \brief The connection pointer.
     *
     * This parameter is set by the constructor. It represents the
     * connection this dispatcher was added to (a form of parent of
     * this dispatcher object.)
     */
    T *                         f_connection = nullptr;

    /** \brief The array of possible matches.
     *
     * This is the array of your messages with the corresponding
     * match and execute functions. This is used to go through
     * the matches and execute (dispatch) as required.
     *
     * The number of items is calculated in the constructor
     * and saved in the f_matches_count parameter.
     */
    dispatcher_match const *    f_matches = nullptr;

    /** \brief Defines the number of matches in the f_matches array.
     *
     * Since we pass a static array (opposed to a vector because the
     * vector needs to be dynamically allocated, as far as I know),
     * we want to know the size of the array, which we get in the
     * constructor.
     *
     * The value here is already divided by the size of the
     * `dispatcher_match` structure size.
     */
    size_t                      f_matches_count = 0;

    /** \brief Tell whether messages should be traced or not.
     *
     * Because your service may accept and send many messages a full
     * trace on all of them can really be resource intensive. By default
     * the system will not trace anything. By setting this parameter to
     * true (call set_trace() for that) you request the SNAP_LOG_TRACE()
     * to run on each message received by this dispatcher. This is done
     * on entry so whether the message is processed by the dispatcher
     * or your own send_message() function, it will trace that message.
     */
    bool                        f_trace = false;

public:

    /** \brief Initialize the dispatcher with your connection and messages.
     *
     * This function takes a pointer to your connection and an array
     * of matches.
     *
     * Whenever a message is received by one of your connections, the
     * dispatch() function gets called which checks the message against
     * each entry in this array of \p matches.
     *
     * \param[in] connection  The connection for which this dispatcher is
     *                        created.
     * \param[in] matches  The array of dispatch keywords and functions.
     * \param[in] matches_size  The sizeof() of the matches array.
     */
    dispatcher<T>(T * connection, dispatcher_match const * matches, size_t matches_size)
        : f_connection(connection)
        , f_matches(matches)
        , f_matches_count(matches_size / sizeof(dispatcher_match))
    {
    }

    /** \brief The dispatch function.
     *
     * This is the function your message system will call whenever
     * the system receives a message.
     *
     * The function returns true if the message was dispatched.
     * When that happen, the process_message() function of the
     * connection should not be called.
     *
     * You may not include a message in the array of `dispatch_match`
     * if it is too complicated to match or too many variables are
     * necessary then you will probably want to use your
     * process_message().
     *
     * By adding a catch-all at the end of your list of matches, you
     * can easily have one function called for any message. By default
     * the dispatcher environment offers such a match function and
     * it also includes a function that sends the UNKNOWN message as
     * an immediate reply to a received message.
     *
     * \param[in] msg  The message to be dispatched.
     *
     * \return true if the message was dispatched, false otherwise.
     */
    virtual bool dispatch(snap::snap_communicator_message & msg) override
    {
        if(f_trace)
        {
            SNAP_LOG_TRACE("snap_communicator::dispatcher::dispatch(): handling message \"")
                          (msg.to_message())
                          ("\".");
        }

        for(size_t i(0); i < f_matches_count; ++i)
        {
            if(f_matches[i].execute(f_connection, msg))
            {
                return true;
            }
        }

        return false;
    }

    /** \brief Set whether the dispatcher should trace your messages or not.
     *
     * By default, the f_trace flag is set to false. You can change it to
     * true while debugging. You should remember to turn it back off once
     * you make an official version of your service to avoid the possibly
     * huge overhead of sending all those log messages. One way to do so
     * is to place the code within #ifdef/#endif as in:
     *
     * \code
     *     #ifdef _DEBUG
     *         my_dispatcher->set_trace();
     *     #endif
     * \endcode
     *
     * \param[in] trace  Set to true to get SNAP_LOG_TRACE() of each message.
     */
    void set_trace(bool trace = true)
    {
        f_trace = trace;
    }
};


} // namespace snap
// vim: ts=4 sw=4 et
