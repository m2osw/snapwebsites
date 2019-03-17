// Snap Manager -- snap database manager encode/decode dialog
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

#include <QPointer>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include "ui_snap-manager-decode-utf8-dialog.h"
class snap_manager_decode_utf8
    : public QDialog
    , public Ui_decodeUtf8SnapManager
{
    Q_OBJECT

public:
                        snap_manager_decode_utf8(QWidget * parent);
    virtual             ~snap_manager_decode_utf8();

private slots:
    void                on_clear_clicked();
    void                on_decode_clicked();
    void                on_encode_clicked();
    void                on_close_clicked();

private:
    QPointer<QTextEdit> f_data = QPointer<QTextEdit>();
};
#pragma GCC diagnostic pop


// vim: ts=4 sw=4 et
