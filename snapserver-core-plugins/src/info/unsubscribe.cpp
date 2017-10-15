// Snap Websites Server -- manage sendmail (record, display)
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

#include "info.h"

#include "../sendmail/sendmail.h"
#include "../sessions/sessions.h"
#include "../users/users.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(info)


bool info::unsubscribe_on_path_execute(content::path_info_t & ipath)
{
    QString const cpath(ipath.get_cpath());
    if(!cpath.startsWith(QString("%1/").arg(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_PATH))))
    {
        return false;
    }

    content::path_info_t unsubscribe_ipath;
    unsubscribe_ipath.set_path("unsubscribe");
    snap_string_list const segments(cpath.split("/"));
    unsubscribe_ipath.set_parameter("identifier", segments[1]);
    f_snap->output(layout::layout::instance()->apply_layout(unsubscribe_ipath, this));
    return true;
}


void info::init_unsubscribe_editor_widgets(content::path_info_t & ipath, QString const & field_id, QDomElement & widget)
{
    if(field_id == "email")
    {
        // if we have an identifier parameter in the ipath then we want to
        // transform that to an email address and put it in this field
        QString const identifier(ipath.get_parameter("identifier"));
        if(!identifier.isEmpty())
        {
            sessions::sessions::session_info session_info;
            sessions::sessions::instance()->load_session(identifier, session_info, false);
            if(session_info.get_session_type() == sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID)
            {
                QString const & object_path(session_info.get_object_path());
                int const pos(object_path.lastIndexOf("/"));
                if(pos > 0)
                {
                    QString const to(object_path.mid(pos + 1));
                    if(!to.isEmpty())
                    {
                        QDomDocument doc(widget.ownerDocument());
                        QDomElement value(snap_dom::create_element(widget, "value"));
                        QDomText text(doc.createTextNode(to));
                        value.appendChild(text);
                    }
                }
            }
            // else -- TBD should we redirect the user to just /unsubscribe ?
        }
    }
}


void info::unsubscribe_on_finish_editor_form_processing(content::path_info_t & ipath)
{
    QString const cpath(ipath.get_cpath());
    bool const has_session(cpath.startsWith("unsubscribe/"));
    if(cpath != "unsubscribe" && !has_session)
    {
        return;
    }

    // user wants to unsubscribe from this Snap! installation
    //
    // . black list
    //
    //   save the email in the top user definition (in the "users"
    //   table)
    //
    // . orange list
    //
    //   if the user has an account in that specific website,
    //   then black list him on that website only; otherwise do
    //   like the black list (see above)
    //
    // TBD: should we check the email address "validity" when
    //      found in a session (i.e. unsubscribe/...)

    users::users * users_plugin(users::users::instance());

    int64_t const start_date(f_snap->get_start_date());

    // always save blacklist in the user parameter
    //
    QString const user_email(f_snap->postenv(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_FIELD_EMAIL)));
    users::users::user_info_t user_info(users_plugin->get_user_info_by_email(user_email));
    QString level(f_snap->postenv(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_FIELD_LEVEL)));
    //
    if(level == sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_LEVEL_BLACKLIST)
    || level == sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_LEVEL_ANGRYLIST))
    {
        user_info.save_user_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_SELECTION), level);
        user_info.save_user_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_ON), start_date);
    }
    else if(level == sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_LEVEL_ORANGELIST)
         || level == sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_LEVEL_PURPLELIST))
    {
        // The user may not exist in this website so we cannot hope to
        // set that up there; so instead we use a "special" key
        //    sendmail::unsubscribe_selection::<site-key>
        //
        if(level == sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_LEVEL_ORANGELIST))
        {
            level = sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_LEVEL_BLACKLIST);
        }
        else
        {
            level = sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_LEVEL_ANGRYLIST);
        }
        user_info.save_user_parameter(QString("%1::%2").arg(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_SELECTION)).arg(f_snap->get_site_key()), level);
        user_info.save_user_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_ON), start_date);
    }

    if(has_session)
    {
        // used session, "delete it" (mark it used up)
        // (the 3rd parameter on the load_session() does the work for us)
        //
        QString const session_id(cpath.mid(12));
        sessions::sessions::session_info session_info;
        sessions::sessions::instance()->load_session(session_id, session_info, true);
    }
}


SNAP_PLUGIN_END()
// vim: ts=4 sw=4 et
