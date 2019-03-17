//===============================================================================
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
// 
// https://snapwebsites.org/
// contact@m2osw.com
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
//===============================================================================
#pragma once

#include <QtGui>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include "ui_SettingsDialog.h"
class SettingsDialog
    : public QDialog
    , Ui::SettingsDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog( QWidget *p = nullptr, const bool first_time = false );
    virtual ~SettingsDialog() override;

    static bool tryConnection( QWidget* p = nullptr );

protected:
    void closeEvent( QCloseEvent * e );

private slots:
    void on_f_buttonBox_accepted();
    void on_f_buttonBox_rejected();
    void on_f_hostnameEdit_textEdited(const QString &arg1);
    void on_f_portEdit_valueChanged(int arg1);
    void on_f_useSslCB_toggled(bool checked);
    void on_f_promptCB_toggled(bool checked);
    void on_f_contextEdit_textChanged(const QString &arg1);

private:
    QVariant    f_server = QVariant();
    QVariant    f_port = QVariant();
    QVariant    f_useSSL = QVariant();
    QVariant    f_promptBeforeSave = QVariant();
    QVariant    f_contextName = QVariant();
};
#pragma GCC diagnostic pop


// vim: ts=4 sw=4 et
