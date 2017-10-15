// Snap Manager -- snap database manager Initialize Website dialog
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

#include "ui_snap-manager-initialize-websitebox.h"

#include <snapwebsites/snap_initialize_website.h>

#include <QPointer>

class snap_manager_initialize_website : public QDialog, public Ui_initializeWebsiteBox
{
    Q_OBJECT

public:
                    snap_manager_initialize_website(QWidget *parent);
    virtual         ~snap_manager_initialize_website();

    void            add_status(QString const& msg, bool const clear = false);

protected:
    virtual void    timerEvent(QTimerEvent *event);

private slots:
    void            close();
    void            send_request();

private:
    void            enableAll(bool enable);

    QPointer<QPushButton>                       f_close_button;
    QPointer<QPushButton>                       f_send_request_button;
    QPointer<QLineEdit>                         f_snap_server_host;
    QPointer<QLineEdit>                         f_snap_server_port;
    QPointer<QLineEdit>                         f_website_url;
    QPointer<QLineEdit>                         f_port;
    snap::snap_initialize_website::pointer_t    f_initialize_website;
    int32_t                                     f_timer_id = 0;
};


// vim: ts=4 sw=4 et
