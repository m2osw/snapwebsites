// Snap Manager -- snap database manager Initialize Website dialog
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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


#include <snapwebsites/snap_string_list.h>

#include <casswrapper/session.h>
#include <casswrapper/query.h>

#include <QPointer>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include "ui_snap-manager-createcontextbox.h"
class snap_manager_createcontext
    : public QDialog
    , public Ui_createContextBox
{
    Q_OBJECT

public:
                    snap_manager_createcontext( QWidget *prnt );
    virtual         ~snap_manager_createcontext();

    void            add_status(QString const& msg, bool const clear = false);

signals:
    void            disconnectRequested();
    void            createContext( int replication_factor, int strategy, snap::snap_string_list const & data_centers );

private slots:
    void            cancel();
    void            createcontext();

private:
    void            close();
    //void            enableAll(bool enable);

    QPointer<QPushButton>                       f_createcontext_button = QPointer<QPushButton>();
    QPointer<QPushButton>                       f_cancel_button = QPointer<QPushButton>();
    QPointer<QLineEdit>                         f_replication_factor = QPointer<QLineEdit>();
    QPointer<QComboBox>                         f_strategy = QPointer<QComboBox>();
    QPointer<QTextEdit>                         f_data_centers = QPointer<QTextEdit>();
    QPointer<QLineEdit>                         f_snap_server_name = QPointer<QLineEdit>();
};
#pragma GCC diagnostic pop


// vim: ts=4 sw=4 et
