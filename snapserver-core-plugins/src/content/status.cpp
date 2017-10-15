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
 * \brief The implementation of the content plugin status class.
 *
 * This file contains the status class implementation.
 */

#include "content.h"

#include <snapwebsites/log.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(content)









/** \typedef uint32_t path_info_t::status_t::status_type
 * \brief Basic status type to save the status in the database.
 *
 * This basic status is used by the content plugin to manage a page
 * availability. It is called "basic" because this feature does not
 * use the taxonomy to mark the page as being in a specific status
 * that the end user has control over.
 *
 * By default a page is in the "normal state"
 * (path_info_t::status_t::NORMAL). A normal page can be viewed as
 * fully available and will be shown to anyone with enough permissions
 * to access that page.
 *
 * A page can also be hidden from view (path_info_t::status_t::HIDDEN),
 * in which case the page is accessible by the administrators with enough
 * permissions to see hidden pages, but no one else who instead gets an
 * error (a "404 Page Not Found" error.)
 *
 * When the user changes the path to a page, the original path is marked
 * as path_info_t::status_t::MOVED. This allows us to automatically
 * generate a redirect since the destination is saved in the original
 * page.
 *
 * A deleted page (path_info_t::status_t::DELETED) is similar to a normal
 * page, only it is found in the trashcan and thus it cannot be edited or
 * viewed. It can only be "undeleted" (cloned back to its original
 * location or to a new location in the \em regular tree.)
 */

/** \var path_info_t::status_t::error_t::NO_ERROR
 * \brief The page is valid.
 *
 * When creating a new status object, we mark it as a "no error" object.
 * Generatlly, this represents a valid page if the state is not set
 * to path_info_t::status_t::state_t::UNKNOWN_STATE.
 *
 * In this state a status can be saved to the database. If not in this
 * state, trying to save the status will fail with an exception.
 */

/** \var path_info_t::status_t::error_t::UNSUPPORTED
 * \brief Read a status that this version does not understand.
 *
 * This value is returned by the get_status() function whenever a path
 * to a page returns a number that the current status implementation does
 * not understand.
 *
 * Unfortunately, such status cannot really be dealt with in any other way.
 */

/** \var path_info_t::status_t::error_t::UNDEFINED
 * \brief The state is not defined in the database.
 *
 * This value is returned by the get_status() function whenever a path
 * to a non-existant page is read.
 *
 * This is similar to saying this is a 404. There is no redirect or anything
 * else that will help in this circumstance.
 *
 * \note
 * If the content::primary_owner field exists but no content::status field,
 * then the error status is set to error_t::NO_ERROR and the state uses
 * state_t::CREATE.
 */

/** \var path_info_t::status_t::state_t::UNKNOWN_STATE
 * \brief The state was not yet defined.
 *
 * This value is used internally to indicate that the status was not yet
 * read from the database. It should never be saved in the database itself.
 *
 * This is used in the status_t class up until the status gets read from
 * the content table.
 */

/** \var path_info_t::status_t::state_t::CREATE
 * \brief We are in the process of creating a page.
 *
 * While creating a page, the page is marked with this state.
 *
 * Once the page is created, it is marked as path_info_t::status_t::NORMAL.
 */

/** \var path_info_t::status_t::state_t::NORMAL
 * \brief This page is valid. You can use it as is.
 *
 * This is the only status that makes a page 100% valid for anyone with
 * enough permissions to visit the page.
 */

/** \var path_info_t::status_t::state_t::HIDDEN
 * \brief The page is currently hidden.
 *
 * A hidden page is similar to a normal page, only it returns a 404 to
 * normal users.
 *
 * Only administrators with the correct permissions can see the page.
 */

/** \var path_info_t::status_t::state_t::MOVED
 * \brief This page was moved, users coming here shall be redirected.
 *
 * This page content is still intact from the time it was cloned and it
 * should not be used. Instead, since it is considered moved, it generates
 * a 301 (it could be made a 302?) so that way the users who had links to
 * the old path still get to the page.
 *
 * A moved page may get deleted at a later time.
 */

/** \var path_info_t::status_t::state_t::DELETED
 * \brief This page was deleted (moved to the trash).
 *
 * A page that gets moved to the trashcan is marked as deleted since we
 * cannot redirect someone (other than an administrator with enough
 * permissions) to the trashcan.
 *
 * Someone with enough permission can restore a deleted page.
 *
 * A page marked as deleted is eventually removed from the database by
 * the content backend. Pages in the trashcan are also eventually deleted
 * from the database. That depends on the trashcan policy settings.
 */

/** \var path_info_t::status_t::f_error
 * \brief The current error of this status object.
 *
 * The error of this status. By default this parameter is set to
 * path_info_t::status_t::NO_ERROR.
 *
 * When a status is erroneous, the is_error() function returns true and
 * the status cannot be saved in the database.
 *
 * The state and working state of the status are ignored if
 * the status is in error (is_error() returns true.)
 *
 * There is one special case the transition function accepts a
 * path_info_t::status_t::UNDEFINED status as a valid input to
 * transit to a path_info_t::status_t::CREATE and
 * path_info_t::status_t::CREATING status. However, the erroneous
 * status itself is otherwise still considered to be in error.
 */

/** \var path_info_t::status_t::f_state
 * \brief The current state of the status.
 *
 * The state of this status. By default this parameter is set to
 * path_info_t::status_t::UNKNOWN_STATE. You may check whether the
 * state is unknown using the is_unknown() function.
 *
 * \warning
 * The working state is ignored if is_error() is true.
 */



/** \brief Initialize the status with the default status values.
 *
 * The default constructor of the status class defines a status object
 * with default values.
 *
 * The default values are:
 *
 * \li path_info_t::status_t::NO_ERROR for error
 * \li path_info_t::status_t::UNKNOWN_STATE for state
 *
 * The default values can then be changed using the set_...() functions
 * of the class.
 *
 * You may also set the status using the set_status() function in case
 * you get a 'current_status' (see other constructor) after you created
 * a status object.
 *
 * \sa set_status()
 */
path_info_t::status_t::status_t()
    //: f_error(error_t::NO_ERROR)
    //, f_state(state_t::UNKNOWN_STATE)
{
}


/** \brief Initialize the status with the specified current_status value.
 *
 * The constructor and get_status() make use of an integer to
 * save in the database but they do not declare the exact format
 * of that integer (i.e. the format is internal, hermetic.)
 *
 * The input parameter can only be defined from the get_status() of
 * another status. If you are not reading a new status, you must make
 * use of the constructor without a status specified.
 *
 * \param[in] current_status  The current status to save in this instance.
 *
 * \sa set_status()
 */
path_info_t::status_t::status_t(status_type current_status)
    //: f_error(error_t::NO_ERROR)
    //, f_state(state_t::UNKNOWN_STATE)
{
    set_status(current_status);
}


/** \brief Set the current status from the specified \p current_status value.
 *
 * This function accepts a \p current_status value which gets saved in the
 * corresponding f_state and f_working variable members.
 *
 * How the status is encoded in the \p current_status value is none of your
 * business. It is encoded by the get_status() function and decoded using
 * this set_status() function. That value can directly be saved in the
 * database.
 *
 * \note
 * The constructor accepting a \p current_status parameter calls this
 * set_status() function to save its input value.
 *
 * \note
 * The error value is set to error_t::NO_ERROR assuming the input status
 * is considered valid. Otherwise the error is set to error_t::UNSUPPORTED.
 *
 * \param[in] current_status  The current status to save in this instance.
 */
void path_info_t::status_t::set_status(status_type current_status)
{
    // set some defaults so that way we have "proper" defaults on errors
    f_state = state_t::UNKNOWN_STATE;

    state_t const state(static_cast<state_t>(static_cast<int>(current_status) & 255));
    switch(state)
    {
    case state_t::NORMAL:
    case state_t::HIDDEN:
    case state_t::MOVED:
    case state_t::DELETED:
        break;

    //case state_t::UNKNOWN_STATE:    // this state is never saved
    //case state_t::CREATE:           // this state is never saved
    default:
        // any other status is not understood by this version of snap
        f_error = error_t::UNSUPPORTED;
        return;

    }

    f_error = error_t::NO_ERROR;
    f_state = state;
}


/** \brief Retrieve the current value of the status of this object.
 *
 * This function returns the encoded status so one can save it in a
 * database, or some other place. The returned value is an integer.
 *
 * Internally, the value is handled as an error or a state.
 * The encoder does not know how to handle errors in this function,
 * so if an error is detected, it actually throws an exception.
 * It is expected that your code will first check whether is_error()
 * returns true. If so, then you cannot call this function.
 *
 * Note that if the state is still set to state_t::UNKNOWN_STATE,
 * then the function also raises an exception. This is because we
 * cannot allow saving that kind of a status in the database.
 *
 * \exception snap_logic_exception
 * This exception is raised if this function gets called when the
 * status is currently representing an error. This is done that
 * way because there is really no reasons to allow for saving
 * an error in the database.
 *
 * \return The current status encrypted for storage.
 */
path_info_t::status_t::status_type path_info_t::status_t::get_status() const
{
    // errors have priority and you cannot convert an error to a status_type
    if(f_error != error_t::NO_ERROR)
    {
        throw snap_logic_exception(QString("attempting to convert a status to status_type when it represents an error (%1).")
                                                .arg(static_cast<int>(f_error)));
    }

    // of the 4 x 5 = 20 possibilities, we only allow 14 of them
    switch(f_state)
    {
    case state_t::NORMAL:
    case state_t::HIDDEN:
    case state_t::MOVED:
    case state_t::DELETED:
        break;

    //case state_t::UNKNOWN_STATE:    // this state is never saved
    //case state_t::CREATE:           // this sstate is never saved
    default:
        throw snap_logic_exception(QString("attempting to convert status with state %1 which is not allowed")
                                                .arg(static_cast<int>(static_cast<state_t>(f_state))));

    }

    // if no error, then the resulting value is equal to 'f_state'
    return static_cast<status_type>(static_cast<state_t>(f_state));
}


/** \brief Verify status transition validity.
 * 
 * Verify that going from the current status (this) to the \p destination
 * status is acceptable.
 *
 * \param[in] destination  The new state.
 *
 * \return true if the transition is acceptable, false otherwise.
 */
bool path_info_t::status_t::valid_transition(status_t destination) const
{
    struct subfunc
    {
        /** \brief Convert states s1 and s2 in one integer value.
         *
         * This function combines two states together.
         */
        static constexpr int status_combo(state_t s1, state_t s2)
        {
            return static_cast<unsigned char>(static_cast<int>(s1))
                 | static_cast<unsigned char>(static_cast<int>(s2)) * 0x100;
        }
    };

    // TBD: is this function really necessary now that we removed the
    //      'working' state?
    //
    if(is_error())
    {
        return f_error == error_t::UNDEFINED
            && destination.f_state == state_t::CREATE;
    }

    switch(subfunc::status_combo(f_state, destination.f_state))
    {
    case subfunc::status_combo(state_t::NORMAL,     state_t::NORMAL):
    case subfunc::status_combo(state_t::NORMAL,     state_t::HIDDEN):
    case subfunc::status_combo(state_t::NORMAL,     state_t::MOVED):
    case subfunc::status_combo(state_t::NORMAL,     state_t::DELETED):

    case subfunc::status_combo(state_t::HIDDEN,     state_t::HIDDEN):
    case subfunc::status_combo(state_t::HIDDEN,     state_t::NORMAL):
    case subfunc::status_combo(state_t::HIDDEN,     state_t::DELETED):

    case subfunc::status_combo(state_t::MOVED,      state_t::MOVED):
    case subfunc::status_combo(state_t::MOVED,      state_t::NORMAL):
    case subfunc::status_combo(state_t::MOVED,      state_t::HIDDEN):

    case subfunc::status_combo(state_t::DELETED,    state_t::DELETED):
    case subfunc::status_combo(state_t::DELETED,    state_t::NORMAL): // in case of a "re-use that page"

    // see error handling prior to this switch
    //case subfunc::status_combo(state_t::UNKNOWN_STATE,  state_t::CREATE):

    case subfunc::status_combo(state_t::CREATE,     state_t::CREATE):
    case subfunc::status_combo(state_t::CREATE,     state_t::NORMAL):
    case subfunc::status_combo(state_t::CREATE,     state_t::HIDDEN):

        // this is a valid combo
        return true;

    default:
        // we do not like this combo at this time
        return false;

    }
}


/** \brief Set the error number in this status.
 *
 * Change the current status in an erroneous status. By default an object
 * is considered to not have any errors.
 *
 * The current state status do not get modified.
 *
 * \param[in] error  The new error to save in this status.
 *
 * \sa reset_state()
 */
void path_info_t::status_t::set_error(error_t error)
{
    f_error = error;
}


/** \brief Retrieve the current error.
 *
 * This function returns the current error of an ipath status. If this
 * status represents an error, you may also call the is_error() function
 * which returns true for any error except error_t::NO_ERROR.
 *
 * \return The current error.
 */
path_info_t::status_t::error_t path_info_t::status_t::get_error() const
{
    return f_error;
}


/** \brief Check whether the path represents an error.
 *
 * If a path represents an error (which means the set_error() was called
 * with a value other than error_t::NO_ERROR) then this function returns
 * true, otherwise it returns false.
 *
 * The error error_t::UNDEFINED actually means that the page does not
 * exist at all. If the "content::primary_owner" field exists, then
 * the status is set to error_t::NO_ERROR and the status to
 * status_t::CREATE.
 *
 * \return true if the error in this status is not error_t::NO_ERROR.
 */
bool path_info_t::status_t::is_error() const
{
    return f_error != error_t::NO_ERROR;
}


/** \brief Reset this status with the specified values.
 *
 * This function can be used to reset the status to the specified
 * state. It also resets the current error status.
 *
 * This function is a shortcut from doing:
 *
 * \code
 *      status.set_error(error_t::NO_ERROR);
 *      status.set_state(state);
 * \endcode
 *
 * \param[in] state  The new state.
 */
void path_info_t::status_t::reset_state(state_t state)
{
    f_error = error_t::NO_ERROR;
    f_state = state;
}


/** \brief Change the current state of this status.
 *
 * This function can be used to save a new state in this status object.
 *
 * \note
 * This function does NOT affect the error state. This means that if the
 * status object has an error state other than error_t::NO_ERROR, it is
 * still considered to be erroneous, whatever the new state.
 *
 * \param[in] state  The new state of this status.
 */
void path_info_t::status_t::set_state(state_t state)
{
    f_state = state;
}


/** \brief Retrieve the current state.
 *
 * This function returns the current state of this status. The state is
 * set to unknown (path_info_t::status_t::state_t::UNKNOWN_STATE) by
 * default if no current_status is passed to the constructor.
 *
 * \return The current state.
 */
path_info_t::status_t::state_t path_info_t::status_t::get_state() const
{
    return f_state;
}


/** \brief Check whether the current state is unknown.
 *
 * When creating a new state object, the state is set to unknown by
 * default. It remains that way until you change it with set_state()
 * or reset_state().
 *
 * This function can be used to know whether the state is still set
 * to unknown.
 *
 * Note that is important because you cannot save an unknown state in
 * the database. The get_status() function will raise an exception
 * if that is attempted.
 *
 * \return true if the state is considered unknown (page does not exist.)
 */
bool path_info_t::status_t::is_unknown() const
{
    return f_state == state_t::UNKNOWN_STATE;
}


/** \brief Convert \p state to a string.
 *
 * This function converts the specified \p state number to a string.
 *
 * The state is expected to be a value returned by the get_state()
 * function.
 *
 * \exception content_exception_content_invalid_state
 * The state must be one of the defined state_t enumerations. Anything
 * else and this function raises this exception.
 *
 * \param[in] state  The state to convert to a string.
 *
 * \return The string representing the \p state.
 */
std::string path_info_t::status_t::status_name_to_string(status_t::state_t const state)
{
    switch(state)
    {
    case status_t::state_t::UNKNOWN_STATE:
        return "unknown";

    case status_t::state_t::CREATE:
        return "create";

    case status_t::state_t::NORMAL:
        return "normal";

    case status_t::state_t::HIDDEN:
        return "hidden";

    case status_t::state_t::MOVED:
        return "moved";

    case status_t::state_t::DELETED:
        return "deleted";

    }

    throw content_exception_content_invalid_state("Unknown status.");
}


/** \brief Convert a string to a state.
 *
 * This function converts a string to a page state. If the string does
 * not represent a valid state, then the function returns UNKNOWN_STATE.
 *
 * The string must be all lowercase.
 *
 * \param[in] state  The \p state to convert to a number.
 *
 * \return The number representing the specified \p state string or UNKNOWN_STATE.
 */
path_info_t::status_t::state_t path_info_t::status_t::string_to_status_name(std::string const & state)
{
    // this is the default if nothing else matches
    //if(state == "unknown")
    //{
    //    return status_t::state_t::UNKNOWN_STATE;
    //}
    if(state == "create")
    {
        return status_t::state_t::CREATE;
    }
    if(state == "normal")
    {
        return status_t::state_t::NORMAL;
    }
    if(state == "hidden")
    {
        return status_t::state_t::HIDDEN;
    }
    if(state == "moved")
    {
        return status_t::state_t::MOVED;
    }
    if(state == "deleted")
    {
        return status_t::state_t::DELETED;
    }
    // TBD: should we understand "unknown" and throw here instead?
    return status_t::state_t::UNKNOWN_STATE;
}





///** \brief Handle the status of a page safely.
// *
// * This class saves the current status of a page and restores it when
// * the class gets destroyed with the hope that the page status will
// * always stay valid. We still have a "resetstate" action and call
// * that function from our backend whenever the backend runs.
// *
// * The object is actually used to change the status to the status
// * specified in \p now. You may set \p now to the current status
// * if you do not want to change it until you are done.
// *
// * The \p end parameter is what the status will be once the function
// * ends and this RAII object gets destroyed. This could be the
// * current status to restore the status after you are done with your
// * work.
// *
// * \param[in] ipath  The path of the page of which the path is to be safe.
// * \param[in] now  The status of the page when the constructor returns.
// * \param[in] end  The status of the page when the destructor returns.
// */
//path_info_t::raii_status_t::raii_status_t(path_info_t & ipath, status_t now, status_t end)
//    : f_ipath(ipath)
//    , f_end(end)
//{
//    status_t current(f_ipath.get_status());
//
//    // reset the error in case we are loading from a non-existant page
//    if(current.is_error())
//    {
//        if(current.get_error() != status_t::error_t::UNDEFINED)
//        {
//            // the page probably exists, but we still got an error
//            throw content_exception_content_invalid_state(QString("got error %1 when trying to change \"%2\" status.")
//                    .arg(static_cast<int>(current.get_error()))
//                    .arg(f_ipath.get_key()));
//        }
//        current.set_error(status_t::error_t::NO_ERROR);
//    }
//
//    // setup state if requested
//    if(now.get_state() != status_t::state_t::UNKNOWN_STATE)
//    {
//        current.set_state(now.get_state());
//    }
//
//    f_ipath.set_status(current);
//}
//
//
///** \brief This destructor attempts to restore the page status.
// *
// * This function is the counter part of the constructor. It ensures that
// * the state changes to what you want it to be when you release the
// * RAII object.
// */
//path_info_t::raii_status_t::~raii_status_t()
//{
//    status_t current(f_ipath.get_status());
//    if(f_end.get_state() != status_t::state_t::UNKNOWN_STATE)
//    {
//        // avoid exceptions in destructors
//        try
//        {
//            current.set_state(f_end.get_state());
//        }
//        catch(...)
//        {
//            SNAP_LOG_ERROR("caught exception in raii_status_t::~raii_status_t() -- set_state() failed");
//        }
//    }
//    try
//    {
//        f_ipath.set_status(current);
//    }
//    catch(...)
//    {
//        SNAP_LOG_ERROR("caught exception in raii_status_t::~raii_status_t() -- set_status() failed");
//    }
//}






SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
