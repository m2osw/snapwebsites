//===============================================================================
// Copyright (c) 2011-2017 by Made to Order Software Corporation
// 
// http://snapwebsites.org/
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

#include <casswrapper/schema.h>
#include <casswrapper/session.h>
#include <algorithm>

#include <QtWidgets>

#include "SettingsDialog.h"

using namespace casswrapper;

namespace
{
    const char* SERVER_ID       = "cassandra_host"       ;
    const char* PORT_ID         = "cassandra_port"       ;
    const char* USESSL_ID       = "use_ssl"              ;
    const char* PROMPT_ID       = "prompt_before_commit" ;
    const char* CONTEXT_ID      = "snap_keyspace"        ;
    const char* GEOMETRY_ID     = "settings_geometry"    ;
    const char* SERVER_DEFAULT  = "127.0.0.1"            ;
    const int   PORT_DEFAULT    = 9042                   ;
    const bool  USESSL_DEFAULT  = true                   ; // Connect to Cassandra via SSL
    const bool  PROMPT_DEFAULT  = true                   ; // Prompt before saving to database
    const char* CONTEXT_DEFAULT = "snap_websites"        ;
}


SettingsDialog::SettingsDialog( QWidget *p, const bool first_time )
    : QDialog(p)
{
    setupUi( this );

    QSettings settings( this );
    restoreGeometry( settings.value( GEOMETRY_ID, saveGeometry() ).toByteArray() );
    f_server           = settings.value( SERVER_ID,  SERVER_DEFAULT  );
    f_port             = settings.value( PORT_ID,    PORT_DEFAULT    );
    f_useSSL           = settings.value( USESSL_ID,  USESSL_DEFAULT  );
    f_promptBeforeSave = settings.value( PROMPT_ID,  PROMPT_DEFAULT  );
    f_contextName      = settings.value( CONTEXT_ID, CONTEXT_DEFAULT );
    //
    f_hostnameEdit ->setText    ( f_server.toString()         );
    f_portEdit     ->setValue   ( f_port.toInt()              );
    f_useSslCB     ->setChecked ( f_useSSL.toBool()           );
    f_promptCB     ->setChecked ( f_promptBeforeSave.toBool() );
    f_contextEdit  ->setText    ( f_contextName.toString()    );
    //
    f_buttonBox    ->button( QDialogButtonBox::Ok )->setEnabled( first_time );
}


SettingsDialog::~SettingsDialog()
{
    QSettings settings( this );
    settings.setValue( GEOMETRY_ID, saveGeometry() );
}


bool SettingsDialog::tryConnection( QWidget* p )
{
    try
    {
        QSettings settings;
        const QString server  = settings.value( SERVER_ID,  SERVER_DEFAULT  ).toString();
        const int     port    = settings.value( PORT_ID,    PORT_DEFAULT    ).toInt();
        const QString context = settings.value( CONTEXT_ID, CONTEXT_DEFAULT ).toString();

        Session::pointer_t session( Session::create() );
        session->connect( server, port, settings.value( USESSL_ID, USESSL_DEFAULT ).toBool() );

        schema::SessionMeta::pointer_t meta( schema::SessionMeta::create( session ) );
        meta->loadSchema();
        auto keyspaces( meta->getKeyspaces() );
        if( keyspaces.find(context) == keyspaces.end() )
        {
            throw std::runtime_error( tr("Context '%1' does not exist!").arg(context).toUtf8().data() );
        }
    }
    catch( const std::exception& ex )
    {
        qDebug()
            << "Caught exception in SettingsDialog::tryConnection()! "
            << ex.what();
        QMessageBox::critical( p, tr("Cassview Connection Error"), tr("Cannot connect to cassandra server! what=[%1]").arg(ex.what()) );
        return false;
    }

    return true;
}

void SettingsDialog::on_f_buttonBox_accepted()
{
    QSettings settings( this );

    QString const prev_server  = settings.value( SERVER_ID,  SERVER_DEFAULT  ).toString();
    int     const prev_port    = settings.value( PORT_ID,    PORT_DEFAULT    ).toInt();
    bool    const prev_ssl     = settings.value( USESSL_ID,  USESSL_DEFAULT  ).toBool();
    QString const prev_context = settings.value( CONTEXT_ID, CONTEXT_DEFAULT ).toString();
    //
    settings.setValue( SERVER_ID,  f_server      );
    settings.setValue( PORT_ID,    f_port        );
    settings.setValue( USESSL_ID,  f_useSSL      );
    settings.setValue( CONTEXT_ID, f_contextName );

    if( !tryConnection( this ) )
    {
        // Put back the old values and return, causing the dialog to stay open.
        settings.setValue( SERVER_ID,  prev_server  );
        settings.setValue( PORT_ID,    prev_port    );
        settings.setValue( USESSL_ID,  prev_ssl     );
        settings.setValue( CONTEXT_ID, prev_context );
        return;
    }

    // Accept the settings and exit
    //
    settings.setValue( PROMPT_ID,  f_promptBeforeSave );

    accept();
}

void SettingsDialog::on_f_buttonBox_rejected()
{
    reject();
}


void SettingsDialog::closeEvent( QCloseEvent * e )
{
    // Closing the dialog by the "x" constitutes "reject."
    //
    e->accept();

    reject();
}


void SettingsDialog::on_f_hostnameEdit_textEdited(const QString &arg1)
{
    f_server = arg1;
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
}


void SettingsDialog::on_f_portEdit_valueChanged(int arg1)
{
    f_port = arg1;
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
}

void SettingsDialog::on_f_useSslCB_toggled(bool checked)
{
    f_useSSL = checked;
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
}

void SettingsDialog::on_f_promptCB_toggled(bool checked)
{
    f_promptBeforeSave = checked;
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
}

void SettingsDialog::on_f_contextEdit_textChanged(const QString &arg1)
{
    f_contextName = arg1;
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
}

// vim: ts=4 sw=4 et
