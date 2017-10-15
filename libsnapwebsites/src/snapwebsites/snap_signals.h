// Snap Websites Servers -- signals
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

#include <boost/signals2.hpp>

/** \file
 * \brief Declarations necessary to setup signals in plugins.
 *
 * This header defines a macro one uses to add signals understood by a plugin.
 * The plugin also defines a set of macros one can use to connect to those
 * signals: SNAP_LISTEN and SNAP_LISTEN0.
 *
 * When a signal is called, the process is two or three steps:
 *
 * 1. call the plugin signal implementation (the \<name>_impl() function)
 *    if that function returns false, stop the process immediately
 *
 * 2. process the signal so any other plugin that registered to receive
 *    this signal is called; the process cannot stop early, all plugins
 *    functions will be called in an undertermined order
 *
 * 3. if it exists, call the signal done function (the \<name>_done()
 *    function of the plugin); this function can do some cleanup or emit
 *    another signal as required
 *
 * Example of signals created with these macros:
 *
 * \code
 * SNAP_SIGNAL(bootstrap, (), ())
 * SNAP_SIGNAL(execute, (const QString& url), (url));
 * \endcode
 *
 */



#define     SNAP_SIGNAL_PROCESS_MODE_NEITHER(name, parameters, variables)   \
    public: \
        void name parameters { \
            f_signal_##name variables; \
        }

#define     SNAP_SIGNAL_PROCESS_MODE_START(name, parameters, variables)   \
        bool name##_impl parameters; \
    public: \
        void name parameters { \
            if(name##_impl variables) \
            { \
                f_signal_##name variables; \
            } \
        }

#define     SNAP_SIGNAL_PROCESS_MODE_DONE(name, parameters, variables)   \
        void name##_done parameters; \
    public: \
        void name parameters { \
            f_signal_##name variables; \
            name##_done variables; \
        }

#define     SNAP_SIGNAL_PROCESS_MODE_START_AND_DONE(name, parameters, variables)   \
        bool name##_impl parameters; \
        void name##_done parameters; \
    public: \
        void name parameters { \
            if(name##_impl variables) \
            { \
                f_signal_##name variables; \
                name##_done variables; \
            } \
        }


/** \brief Define a named signal.
 *
 * This macro is used to quickly define a signal that other plugins can
 * listen to.
 *
 * The first macro parameter is the signal name. The macro creates:
 *
 * \li typedef ... signal_\<name>_t; -- the signal type
 * \li signal_listen_\<name>(signal_\<name>_t::slot_type const& slot);
 *     -- the function used to register a plugin as a listener
 * \li void \<name>(\<parameters>) -- the function used to trigger the signal
 *
 * This macro also expects a couple of functions named:
 *
 * \li \<name>_impl(\<parameters>), and
 * \li \<name>_done(\<parameters>)
 *
 * These functions are called as shown below:
 *
 * \li calls \<name>_impl(\<parameters>)
 * \li calls all the signal functions of the plugins that registered
 * \li calls \<name>_done(\<parameters>)
 *
 * Note that the \<name>_impl() function returns a Boolean value. If it
 * returns false, then the process stops immediately and no other
 * functions get called. If the mode says to not include a \<name>_impl()
 * function, then it is assumed that it always returns true.
 *
 * Because it is time consuming and adds more functions calls for nothing
 * the macro allows you to define whether the two extra functions are
 * defined using the mode parameter set to one of the following values:
 *
 * \li NEITHER -- none of the \<name>_impl and \<name>_done get called
 * \li START -- only the \<name>_impl is called
 * \li DONE -- only the \<name>_done function is called
 * \li START_AND_DONE -- both the functions get called
 *
 * \param[in] name  The name of the signal.
 * \param[in] parameters  A list of parameters written between parenthesis.
 * \param[in] variables  List the variable names as they appear in the
 *                       parameters, written between parenthesis.
 * \param[in] mode  The mode used to call the various functions.
 */
#define    SNAP_SIGNAL_WITH_MODE(name, parameters, variables, mode) \
    typedef boost::signals2::signal<void parameters> signal_##name##_t; \
    boost::signals2::connection signal_listen_##name(signal_##name##_t::slot_type const & slot) \
        { \
            return f_signal_##name.connect(slot); \
        } \
    private: \
        signal_##name##_t f_signal_##name; \
        SNAP_SIGNAL_PROCESS_MODE_##mode(name, parameters, variables)


/** \brief This macro defines a default mode set to 'impl'.
 *
 * The macro calls the SNAP_SIGNAL_WITH_MODE() macro with the mode
 * set to SNAP_SIGNAL_MODE_IMPL. Please check that macro definition
 * for additional information about the macro and what it does.
 *
 * \param[in] name  The name of the signal.
 * \param[in] parameters  A list of parameters written between parenthesis.
 * \param[in] variables  List the variable names as they appear in the
 *                       parameters, written between parenthesis.
 * \param[in] mode  The mode used to call the various functions.
 */
#define    SNAP_SIGNAL(name, parameters, variables) \
    SNAP_SIGNAL_WITH_MODE(name, parameters, variables, START)



// vim: ts=4 sw=4 et
