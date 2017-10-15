// Snap Websites Server -- manage messages (record, display)
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

#include "messages.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <QtSerialization/QSerializationComposite.h>
#include <QtSerialization/QSerializationFieldString.h>
#include <QtSerialization/QSerializationFieldBasicTypes.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(messages, 1, 0)


namespace
{

int32_t        g_message_id = 0;

} // noname namespace



/** \brief Get a fixed messages plugin name.
 *
 * The messages plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_MESSAGES_MESSAGES:
        return "messages::messages";

    case name_t::SNAP_NAME_MESSAGES_WARNING_HEADER:
        return "Warning";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_MESSAGES_...");

    }
    NOTREACHED();
}


/** \brief Initialize a default message object.
 *
 * This function initializes a default message object which is required
 * by the QVector implementation.
 */
messages::message::message()
    : f_type(message_type_t::MESSAGE_TYPE_ERROR)
    , f_id(++g_message_id)
    //, f_title("") -- auto-init
    //, f_body("") -- auto-init
    //, f_widget_name("") -- auto-init
{
}


/** \brief Initialize a message object.
 *
 * This function initializes a message object with the specified
 * type, title, and body.
 *
 * See the get_type() function for more details about the message types.
 *
 * \param[in] t  The type of message.
 * \param[in] title  The "title" of the message, it cannot be empty.
 * \param[in] body  The body of the message, it may be empty.
 */
messages::message::message(message_type_t t, QString const & title, QString const & body)
    : f_type(t)
    , f_id(++g_message_id)
    , f_title(title)
    , f_body(body)
    //, f_widget_name("") -- auto-init
{
}


/** \brief Copy operator so we can add messages to a vector.
 *
 * This function is overloaded because the default copy operator
 * cannot copy the f_type field (i.e. the initialization of an
 * f_type is required.)
 *
 * \param[in] rhs  The message to copy.
 */
messages::message::message(message const & rhs)
    : f_type(rhs.f_type)
    , f_id(rhs.f_id)
    , f_title(rhs.f_title)
    , f_body(rhs.f_body)
    , f_widget_name(rhs.f_widget_name)
{
}


/** \brief Retrieve the message type.
 *
 * This function returns the type of this message. It is set to:
 * error, warning, informational, or debug as defined here:
 *
 * \li MESSAGE_TYPE_ERROR -- the message represents an error
 * \li MESSAGE_TYPE_WARNING -- the message represents a warning
 * \li MESSAGE_TYPE_INFO -- the message represents feedback (i.e. it worked!)
 * \li MESSAGE_TYPE_DEBUG -- the user is trying to debug some part of the Snap! software
 *
 * \return One of the message enumeration code.
 */
messages::message::message_type_t messages::message::get_type() const
{
    return f_type;
}


/** \brief Retrieve the message identifier.
 *
 * This function returns this message unique identifier. Note that
 * this identifier is unique per session. In other words, if you load
 * a new page with messages, the first message has again identifier 1.
 *
 * Some identifier are skipped because the implementation of QVector
 * creates empty messages.
 *
 * \return This message identifier.
 */
int messages::message::get_id() const
{
    return f_id;
}


/** \brief Retrieve the message title.
 *
 * This function retrieves the title of the message. In most cases
 * this is displayed in an HTML header such as an <h2>...</h2> tag.
 *
 * \return Return the title of the message.
 */
QString const & messages::message::get_title() const
{
    return f_title;
}


/** \brief Retrieve the message body.
 *
 * This function returns the body of the message which generally represents
 * the main contents of the message.
 *
 * \return The body of the message.
 */
QString const & messages::message::get_body() const
{
    return f_body;
}


/** \brief Retrieve the name of the widget associated with this message.
 *
 * This message may have been associated with a widget name using
 * the set_widget_name() function.
 *
 * If the message was not associated with a widget, then this function
 * returns an empty string.
 *
 * \return The name of the widget associated with this message or "".
 */
QString const & messages::message::get_widget_name() const
{
    return f_widget_name;
}


/** \brief Set the name of the widget that generated this message.
 *
 * This message can be linked to a widget in a standard or an editor form.
 * This is particularly useful to display the message locally to the
 * widget (instead of all at the top or in a popup, but "disorganized"
 * to the end user point of view.)
 *
 * \param[in] widget_name  The name of the widget.
 */
void messages::message::set_widget_name(QString const & widget_name)
{
    f_widget_name = widget_name;
}


/** \brief Unserialize a message.
 *
 * This function unserializes a message that was serialized using
 * the serialize() function. This is considered an internal function as it
 * is called by the unserialize() function of the messages object.
 *
 * \param[in] r  The reader used to read the input data.
 *
 * \sa serialize()
 */
void messages::message::unserialize(QtSerialization::QReader & r)
{
    QtSerialization::QComposite comp;
    int32_t type(static_cast<int32_t>(static_cast<message_type_t>(message_type_t::MESSAGE_TYPE_ERROR)));
    QtSerialization::QFieldInt32 tag_type(comp, "type", type);
    int32_t id(0);
    QtSerialization::QFieldInt32 tag_id(comp, "id", id);
    QtSerialization::QFieldString tag_title(comp, "title", f_title);
    QtSerialization::QFieldString tag_body(comp, "body", f_body);
    r.read(comp);

    f_type = static_cast<message_type_t>(type);
    f_id = id;
}


/** \brief Read the contents one tag from the reader.
 *
 * This function reads the contents of the message tag. It handles
 * the message fields.
 *
 * \param[in] name  The name of the tag being read.
 * \param[in] r  The reader used to read the input data.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void messages::message::readTag(QString const & name, QtSerialization::QReader & r)
{
}
#pragma GCC diagnostic pop


/** \brief Serialize a message to a writer.
 *
 * This function serialize a message so it can be saved in the database
 * in the form of a string.
 *
 * \param[in,out] w  The writer where the data gets saved.
 *
 * \sa unserialize()
 */
void messages::message::serialize(QtSerialization::QWriter & w) const
{
    QtSerialization::QWriter::QTag tag(w, "message");
    QtSerialization::writeTag(w, "type", static_cast<int32_t>(static_cast<message_type_t>(f_type)));
    QtSerialization::writeTag(w, "id", static_cast<int32_t>(f_id));
    QtSerialization::writeTag(w, "title", f_title);
    QtSerialization::writeTag(w, "body", f_body);
}




/** \brief Initialize the messages plugin.
 *
 * This function is used to initialize the messages plugin object.
 */
messages::messages()
    //: f_snap(nullptr) -- auto-init
    //, f_messages() -- auto-init
    //, f_error_count(0) -- auto-init
    //, f_warning_count(0) -- auto-init
{
}


/** \brief Clean up the messages plugin.
 *
 * Ensure the messages object is clean before it is gone.
 */
messages::~messages()
{
}


/** \brief Get a pointer to the messages plugin.
 *
 * This function returns an instance pointer to the messages plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the messages plugin.
 */
messages * messages::instance()
{
    return g_plugin_messages_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString messages::settings_path() const
{
    return "/admin/settings/info";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString messages::icon() const
{
    return "/images/snap/messages-logo-64x64.png";
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString messages::description() const
{
    return "The messages plugin is used by many other plugins to manage"
        " debug, information, warning, and error messages in the Snap! system.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString messages::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run yet.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t messages::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize the messages.
 *
 * This function terminates the initialization of the messages plugin
 * by registering for different events it supports.
 *
 * \param[in] snap  The child handling this request.
 */
void messages::bootstrap(snap_child * snap)
{
    f_snap = snap;
}


/** \brief Set an HTTP error on this page.
 *
 * This function is used to display an HTTP error message to the end user when
 * something's went wrong while processing a request.
 *
 * The HTTP error code is saved in the Status header. If another function calls
 * the set_header() with "Status" as the field name then it can overwrite this
 * error code. (However, no one should set the Status header with a 200 code.)
 * Only one code can be returned to the user, so only the last one is kept
 * (as it overwrites the previous one.) The error code must be a 400 or a
 * 500 code (any 4xx or 5xx, but no 1xx, 2xx, or 3xx.)
 *
 * This function returns and is expected to be used when an error is detected
 * and needs to be displayed to the end user on the current page. The function
 * can be called any number of times as it cumulates all types of messages.
 *
 * This function should only be used if the error can be represented by an
 * HTTP error code (i.e. a 403 or a 501). Other errors should be passed to
 * the message system using the set_error() function instead.
 *
 * \param[in] err_code  The error code such as 501 or 503.
 * \param[in] err_name  The name of the error such as "Service Not Available".
 * \param[in] err_description  HTML message about the problem.
 * \param[in] err_details  Server side text message with details that are logged only.
 * \param[in] err_security  Whether this message is considered a security related message.
 *
 * \sa set_error()
 * \sa set_warning()
 * \sa set_info()
 * \sa set_debug()
 */
messages::message & messages::set_http_error(snap_child::http_code_t err_code, QString err_name, const QString& err_description, const QString& err_details, bool err_security)
{
    ++f_error_count;

    // the error code must be valid (i.e. an actual HTTP error!)
    if(static_cast<int>(err_code) < 400 || static_cast<int>(err_code) > 599)
    {
        throw snap_logic_exception("the set_http_error() function was called with an invalid error code number");
    }

    // define a default error name if undefined
    snap_child::define_http_name(err_code, err_name);

    // log the error
    logging::log_security_t const sec(err_security ? logging::log_security_t::LOG_SECURITY_SECURE : logging::log_security_t::LOG_SECURITY_NONE);
    SNAP_LOG_FATAL(sec)(err_details)(" (")(err_name)(": ")(err_description)(")");

    // Status Header
    // i.e. "Status: 503 Service Unavailable"
    QString status(QString("%1 %2").arg(static_cast<int>(err_code)).arg(err_name));
    f_snap->set_header("Status", status);

    message msg(message::message_type_t::MESSAGE_TYPE_ERROR, QString("%1 %2").arg(static_cast<int>(err_code)).arg(err_name), err_description);
    f_messages.push_back(msg);
    return f_messages.last();
}


/** \brief Set an error on this page.
 *
 * This function is used to display an error message to the end user when
 * something's went wrong but the error is not an HTTP error
 * (i.e. an error message for data sent via POST doesn't validate.)
 *
 * This function returns and is expected to be used when something is detected
 * that requires the end user to make changes to a form. The function can be
 * called any number of times as it cumulates any number of messages.
 *
 * \param[in] err_name  The name of the error such as "Value Too Small".
 * \param[in] err_description  HTML message about the problem.
 * \param[in] err_details  Server side text message with details that are logged only.
 * \param[in] err_security  Whether this message is considered a security related message.
 *
 * \sa set_http_error()
 * \sa set_warning()
 * \sa set_info()
 * \sa set_debug()
 */
messages::message & messages::set_error(QString err_name, QString const & err_description, QString const & err_details, bool err_security)
{
    ++f_error_count;

    if(err_name.isEmpty())
    {
        throw snap_logic_exception("The err_name parameter of the messages::set_error() function cannot be empty.");
    }

    // log the error
    logging::log_security_t sec(err_security ? logging::log_security_t::LOG_SECURITY_SECURE : logging::log_security_t::LOG_SECURITY_NONE);
    SNAP_LOG_ERROR(sec)(err_details)(" (")(err_name)(": ")(err_description)(")");

    message msg(message::message_type_t::MESSAGE_TYPE_ERROR, err_name, err_description);
    f_messages.push_back(msg);
    return f_messages.last();
}


/** \brief Set a warning on this page.
 *
 * This function is used to display a warning message to the end user when
 * something's went wrong but not too wrong while processing a request.
 * (i.e. this is a recoverable error message.)
 *
 * This function returns and is expected to be used when something is detected
 * that needs the attention of the end user. The function can be called any
 * number of times as it cumulates any number of messages.
 *
 * \param[in] warning_name  The name of the error such as "Service Not Available".
 * \param[in] warning_description  HTML message about the problem.
 * \param[in] warning_details  Server side text message with details that are logged only.
 *
 * \sa set_http_error()
 * \sa set_error()
 * \sa set_info()
 * \sa set_debug()
 */
messages::message & messages::set_warning(QString warning_name, QString const & warning_description, QString const & warning_details)
{
    ++f_warning_count;

    if(warning_name.isEmpty())
    {
        throw snap_logic_exception("The warning_name parameter of the messages::set_warning() function cannot be empty.");
    }

    // log the warning
    SNAP_LOG_WARNING(warning_details)(" (")(warning_name)(": ")(warning_description)(")");

    message msg(message::message_type_t::MESSAGE_TYPE_WARNING, warning_name, warning_description);
    f_messages.push_back(msg);
    return f_messages.last();
}


/** \brief Set an informational message on this page.
 *
 * This function is used to display a message to the end user when
 * something succeeded (i.e. the user saved content.)
 *
 * This function returns and is expected to be used when the user does not
 * otherwise get any feedback that his action was successful.
 *
 * The function can be used any number of times since it cumulates all the
 * messages as required.
 *
 * \param[in] info_name  The name of the error such as "Service Not Available".
 * \param[in] info_description  HTML message about the problem.
 *
 * \sa set_http_error()
 * \sa set_error()
 * \sa set_warning()
 * \sa set_debug()
 */
messages::message & messages::set_info(QString info_name, QString const & info_description)
{
    if(info_name.isEmpty())
    {
        throw snap_logic_exception("The info_name parameter of the messages::set_info() function cannot be empty.");
    }

    SNAP_LOG_INFO("(")(info_name)(": ")(info_description)(")");

    message msg(message::message_type_t::MESSAGE_TYPE_INFO, info_name, info_description);
    f_messages.push_back(msg);
    return f_messages.last();
}


/** \brief Set a debug message on this page.
 *
 * This function is used to display a debug message to the end user while
 * attempting to debug something in the Snap! C++ server.
 *
 * This function returns and is expected to be used when the user requested
 * to receive debug information for your plugin.
 *
 * The function can be used any number of times since it cumulates all the
 * messages as required.
 *
 * \param[in] debug_name  The name (title) of the debug message.
 * \param[in] debug_description  HTML description about the problem.
 *
 * \sa set_http_error()
 * \sa set_error()
 * \sa set_warning()
 * \sa set_info()
 */
messages::message & messages::set_debug(QString debug_name, QString const & debug_description)
{
    if(debug_name.isEmpty())
    {
        throw snap_logic_exception("The debug_name parameter of the messages::set_debug() function cannot be empty.");
    }

    SNAP_LOG_DEBUG("(")(debug_name)(": ")(debug_description)(")");

    message msg(message::message_type_t::MESSAGE_TYPE_DEBUG, debug_name, debug_description);
    f_messages.push_back(msg);
    return f_messages.last();
}


/** \brief Return the total number of messages.
 *
 * This function returns the total number of messages currently defined
 * in the messages plugin.
 *
 * When no messages were generated, the system should not save anything.
 *
 * \return The error counter.
 */
int messages::get_message_count() const
{
    return f_messages.count();
}


/** \brief Return the number of times errors were generated.
 *
 * This function returns the counter that gets increased any time the
 * set_error() or set_http_error() functions are called.
 *
 * When no errors were generated this function returns zero (0).
 *
 * \return The error counter.
 */
int messages::get_error_count() const
{
    return f_error_count;
}


/** \brief Return the number of times warnings were generated.
 *
 * This function returns the counter that gets incremented each
 * time the set_warning() function is called.
 *
 * When no warnings were generated this function returns zero (0).
 *
 * \return The warning counter.
 */
int messages::get_warning_count() const
{
    return f_warning_count;
}


/** \brief Clear the list of messages.
 *
 * This function clears the list of messages. In general this is called only
 * after the messages were either saved with serialize() or sent to the
 * user in HTML form.
 */
void messages::clear_messages()
{
    f_messages.clear();
}


/** \brief Retrieve the last message and title.
 *
 * This function can be used to retrieve the last message that
 * was added to the message vector.
 *
 * \exception snap_logic_exception
 * The snap_logic_exception is raised if the function is called with an
 * invalid index. The get_message_count() function can be used to know
 * the upper limit (the lower limit is always zero).
 *
 * \param[in] idx  The index of the message to retrieve.
 *
 * \return The message is returned.
 */
messages::message const & messages::get_message(int idx) const
{
    if(idx < 0 || idx >= f_messages.size())
    {
        throw snap_logic_exception(QString("get_message() cannot be called with an index (%1) our of bounds (0..%2).").arg(idx).arg(f_messages.size()));
    }
    return f_messages[idx];
}


/** \brief Retrieve the last message and title.
 *
 * This function can be used to retrieve the last message that
 * was added to the message vector.
 *
 * \exception snap_logic_exception
 * The snap_logic_exception is raised if the function is called when no
 * messages were previously added. To know whether some messages were
 * added, one can use the get_message_count() function and check that
 * it does not return zero.
 *
 * \return The message is returned.
 */
messages::message const & messages::get_last_message() const
{
    if(f_messages.isEmpty())
    {
        throw snap_logic_exception("get_last_message() cannot be called if no messages were added to the messages plugin.");
    }
    return f_messages.last();
}


/** \brief Unserialize a set of messages.
 *
 * This function unserializes a message that was serialized using
 * the serialize() function.
 *
 * \param[in] data  The data to unserialize.
 *
 * \sa serialize()
 */
void messages::unserialize(QString const & data)
{
    // QBuffer takes a non-const QByteArray so we have to create a copy
    QByteArray non_const_data(data.toUtf8().data());
    QBuffer in(&non_const_data);
    in.open(QIODevice::ReadOnly);
    QtSerialization::QReader reader(in);
    QtSerialization::QComposite comp;
    QtSerialization::QFieldTag messages_tag(comp, "messages", this);
    reader.read(comp);
}


/** \brief Read the contents of one tag from the reader.
 *
 * This function reads the contents of one message tag. It calls
 * the attachment unserialize() as required whenever an attachment
 * is found in the stream.
 *
 * \param[in] name  The name of the tag being read.
 * \param[in] r  The reader used to read the input data.
 */
void messages::readTag(QString const & name, QtSerialization::QReader & r)
{
    if(name == "messages")
    {
        QtSerialization::QComposite comp;
        int32_t error_count(0);
        QtSerialization::QFieldInt32 error_count_tag(comp, "error_count", error_count);
        int32_t warning_count(0);
        QtSerialization::QFieldInt32 warning_count_tag(comp, "warning_count", warning_count);
        QtSerialization::QFieldTag tag_attachment(comp, "message", this);
        r.read(comp);
        f_error_count = error_count;
        f_warning_count = warning_count;
    }
    else if(name == "message")
    {
        message msg;
        msg.unserialize(r);
        f_messages.push_back(msg);
    }
}


/** \brief Serialize a list of messages to a writer.
 *
 * This function serializes the current list of messages so it can be
 * saved in the database in the form of a string.
 *
 * You can clear the list of messages so that way it does not get
 * saved in the session.
 *
 * \sa unserialize()
 */
QString messages::serialize() const
{
    QByteArray result;
    QBuffer archive(&result);
    archive.open(QIODevice::WriteOnly);
    {
        QtSerialization::QWriter w(archive, "messages", MESSAGES_MAJOR_VERSION, MESSAGES_MINOR_VERSION);
        QtSerialization::QWriter::QTag tag(w, "messages");
        QtSerialization::writeTag(w, "error_count", f_error_count);
        QtSerialization::writeTag(w, "warning_count", f_warning_count);
        const int max_msg(f_messages.count());
        for(int i(0); i < max_msg; ++i)
        {
            f_messages[i].serialize(w);
        }
        // end the writer so everything gets saved in the buffer (result)
    }

    return QString::fromUtf8(result.data());
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
