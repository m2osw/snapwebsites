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

#include "ui_snap-manager-createcontextbox.h"

#include <snapwebsites/snap_string_list.h>

#include <casswrapper/session.h>
#include <casswrapper/query.h>

#include <QPointer>

class snap_manager_createcontext : public QDialog, public Ui_createContextBox
{
    Q_OBJECT

public:
                    snap_manager_createcontext( QWidget *prnt );
    virtual         ~snap_manager_createcontext();

    void            add_status(QString const& msg, bool const clear = false);

signals:
    void			disconnectRequested();
    void		    createContext( int replication_factor, int strategy, snap::snap_string_list const & data_centers );

private slots:
    void            cancel();
    void            createcontext();

private:
    void            close();
    //void            enableAll(bool enable);

    QPointer<QPushButton>                       f_createcontext_button;
    QPointer<QPushButton>                       f_cancel_button;
    QPointer<QLineEdit>                         f_replication_factor;
    QPointer<QComboBox>                         f_strategy;
    QPointer<QTextEdit>                         f_data_centers;
    QPointer<QLineEdit>                         f_snap_server_name;
};


// vim: ts=4 sw=4 et
