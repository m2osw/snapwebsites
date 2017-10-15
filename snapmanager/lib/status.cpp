//
// File:        status.cpp
// Object:      Manager the status object.
//
// Copyright:   Copyright (c) 2016-2017 Made to Order Software Corp.
//              All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// ourselves
//
#include "snapmanager/status.h"

// our lib
//
#include "snapmanager/manager.h"

// snapwebsites
//
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/string_replace.h>

// last entry
//
#include <snapwebsites/poison.h>


namespace snap_manager
{



status_t::status_t()
{
}


status_t::status_t(state_t s, QString const & plugin_name, QString const & field_name, QString const & value)
    : f_state(s)
    , f_plugin_name(plugin_name)
    , f_field_name(field_name)
    , f_value(value)
{
}


void status_t::clear()
{
    *this = status_t();
}


void status_t::set_state(state_t s)
{
    if(s == state_t::STATUS_STATE_UNDEFINED)
    {
        throw snapmanager_exception_invalid_parameters("libsnapmanager: status_t::set_state() cannot be set to UNDEFINED.");
    }

    f_state = s;
}


status_t::state_t status_t::get_state() const
{
    return f_state;
}


void status_t::set_plugin_name(QString const & name)
{
    f_plugin_name = name;
}


QString status_t::get_plugin_name() const
{
    return f_plugin_name;
}


void status_t::set_field_name(QString const & name)
{
    f_field_name = name;
}


QString status_t::get_field_name() const
{
    return f_field_name;
}


void status_t::set_value(QString const & value)
{
    f_value = value;
}


QString status_t::get_value() const
{
    return f_value;
}


/** \brief Encode all the status data in a string.
 *
 * This function encodes all the status data in a string which can then
 * be saved in a file. The from_string() can be used to convert such a
 * string back to a status.
 *
 * \return The encoded status.
 *
 * \sa from_string()
 */
QString status_t::to_string() const
{
    QString result;

    if(f_plugin_name.isEmpty()
    || f_field_name.isEmpty())
    {
        throw snapmanager_exception_invalid_parameters("libsnapmanager: to_string() expects f_plugin_name and f_name to both be defined.");
    }

    result += f_plugin_name;
    result += "::";
    result += f_field_name;

    switch(f_state)
    {
    case state_t::STATUS_STATE_UNDEFINED:
        // this should never occur since the set_state() prevents it
        throw snapmanager_exception_invalid_parameters("STATUS_STATE_UNDEFINED is for external program.");

    case state_t::STATUS_STATE_DEBUG:
        result += "[debug]";
        break;

    case state_t::STATUS_STATE_INFO:
        // This is the default so we do not have to save it
        //result += "[info]";
        break;

    case state_t::STATUS_STATE_MODIFIED:
        result += "[modified]";
        break;

    case state_t::STATUS_STATE_HIGHLIGHT:
        result += "[highlight]";
        break;

    case state_t::STATUS_STATE_WARNING:
        result += "[warning]";
        break;

    case state_t::STATUS_STATE_ERROR:
        result += "[error]";
        break;

    case state_t::STATUS_STATE_FATAL_ERROR:
        result += "[fatal error]";
        break;

    }

    result += "=";

    // values may include \r or \n and that's not compatible with the
    // reader, so we have to escape those
    //
    //QString value(f_value);

    std::string value(f_value.toUtf8().data());
    std::string const safe_value(snap::string_replace_many(
            value,
            {
                { "\\", "\\\\" },
                { "\n", "\\n" },
                { "\r", "\\r" }
            }));

    result += QString::fromUtf8(safe_value.c_str());

    return result;
}


/** \brief Parse a status string back to a status object.
 *
 * This function parses a line of status back to a status object.
 * The line should have been created with the to_string() function.
 *
 * \param[in] line  The line to parse to a status.
 *
 * \return true if the line could be parsed without error.
 */
bool status_t::from_string(QString const & line)
{
    clear();

    QChar const * s(line.data());

    // read plugin name
    for(; !s->isNull() && s->unicode() != ':'; ++s)
    {
        f_plugin_name += *s;
    }

    // make sure we have '::' between the plugin name and field name
    if(s->unicode() != ':'
    || (s + 1)->isNull()
    || (s + 1)->unicode() != ':')
    {
        // unexpected plugin name separator
        SNAP_LOG_ERROR("status_t::from_string(): invalid plugin name separator in \"")(line)("\"");
        return false;
    }

    // s += 2 to skip both ':' characters
    for(s += 2; !s->isNull() && s->unicode() != '[' && s->unicode() != '='; ++s)
    {
        f_field_name += *s;
    }

    // state specified?
    if(s->unicode() =='[')
    {
        QString state;
        for(++s; !s->isNull() && s->unicode() != ']'; ++s)
        {
            state += *s;
        }
        if(state == "debug")
        {
            f_state = state_t::STATUS_STATE_DEBUG;
        }
        else if(state == "info")
        {
            f_state = state_t::STATUS_STATE_INFO;
        }
        else if(state == "modified")
        {
            f_state = state_t::STATUS_STATE_MODIFIED;
        }
        else if(state == "highlight")
        {
            f_state = state_t::STATUS_STATE_HIGHLIGHT;
        }
        else if(state == "warning")
        {
            f_state = state_t::STATUS_STATE_WARNING;
        }
        else if(state == "error")
        {
            f_state = state_t::STATUS_STATE_ERROR;
        }
        else if(state == "fatal error")
        {
            f_state = state_t::STATUS_STATE_FATAL_ERROR;
        }
        else
        {
            // unknown state
            SNAP_LOG_ERROR("status_t::from_string(): unsupported state \"")(state)("\" in \"")(line)("\".");
            return false;
        }

        // skip the ']' so the next test works as expected
        ++s;
    }

    if(s->isNull()
    || s->unicode() != '=')
    {
        SNAP_LOG_ERROR("status_t::from_string(): '=' expected between name and value in \"")(line)("\".");
        return false;
    }

    ++s;

    // the rest of the string is the value
    //f_value = line.mid(s - line.data());

    // restore special characters
    //f_value.replace("\\\\", "\\").replace("\\n", "\n").replace("\\r", "\r");

    std::string value(line.mid(s - line.data()).toUtf8().data());
    std::string const unsafe_value(snap::string_replace_many(
            value,
            {
                { "\\\\", "\\" },
                { "\\n", "\n" },
                { "\\r", "\r" }
            }));

    f_value = QString::fromUtf8(unsafe_value.c_str());

    return true;
}


} // namespace snap_manager
// vim: ts=4 sw=4 et
