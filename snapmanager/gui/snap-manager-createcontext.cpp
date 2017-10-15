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

#include "snap-manager-createcontext.h"
#include "get_child.h"

#include <QSettings>

#include <stdio.h>

using namespace casswrapper;

snap_manager_createcontext::snap_manager_createcontext
        ( QWidget * snap_parent
        )
    : QDialog(snap_parent)
    //, f_query()               -- initialized below
    //, f_close_button()        -- initialized below
    //, f_send_request_button() -- initialized below
    //, f_snap_server_host()    -- initialized below
    //, f_snap_server_port()    -- initialized below
    //, f_website_url()         -- initialized below
    //, f_port()                -- initialized below
    //, f_initialize_website()  -- auto-init
    //, f_timer_id(0)           -- auto-init
{
    setWindowModality(Qt::ApplicationModal);
    setupUi(this);

    QSettings settings( this );
    //restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
    //restoreState   ( settings.value( "state"   , saveState()    ).toByteArray() );
    //
    replicationFactor->setText( settings.value( "createcontext_replicationfactor", "3"      ).toString() );
    strategy         ->setCurrentIndex( settings.value( "createcontext_strategy",  0        ).toInt()    );
    dataCenters      ->setText( settings.value( "createcontext_datacenter",        "DC1"    ).toString() );

    // setup widgets
    f_cancel_button        = getChild<QPushButton> (this, "cancelButton");
    f_createcontext_button = getChild<QPushButton> (this, "createContextButton");
    f_replication_factor   = getChild<QLineEdit>   (this, "replicationFactor");
    f_strategy             = getChild<QComboBox>   (this, "strategy");
    f_data_centers         = getChild<QTextEdit>   (this, "dataCenters");

    // Close
    connect( f_cancel_button.data(), &QPushButton::clicked, this, &snap_manager_createcontext::cancel );

    // Send Request
    connect( f_createcontext_button.data(), &QPushButton::clicked, this, &snap_manager_createcontext::createcontext );
}


snap_manager_createcontext::~snap_manager_createcontext()
{
}


void snap_manager_createcontext::close()
{
    hide();

    QSettings settings( this );
    settings.setValue( "createcontext_replicationfactor", replicationFactor->text()         );
    settings.setValue( "createcontext_strategy",          strategy         ->currentIndex() );
    settings.setValue( "createcontext_datacenter",        dataCenters      ->toPlainText()  );
    //settings.setValue( "geometry",            saveGeometry()           );
}


void snap_manager_createcontext::cancel()
{
    close();

    emit disconnectRequested();
#if 0
    // allow user to try again
    snap_manager * sm(dynamic_cast<snap_manager *>(parent()));
    if(sm)
    {
        sm->cassandraDisconnectButton_clicked();
    }
#endif
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
void snap_manager_createcontext::createcontext()
{
    int const s(strategy->currentIndex());

    snap::snap_string_list data_centers;
    {
        snap::snap_string_list const names(dataCenters->toPlainText().split('\n'));
        int const max_names(names.size());
        for(int idx(0); idx < max_names; ++idx)
        {
            // remove all spaces in each name
            QString const name(names[idx].simplified());
            if(!name.isEmpty())
            {
                data_centers << name;
            }
        }
        if(data_centers.isEmpty() && s != 0)
        {
            QMessageBox msg(
                    QMessageBox::Information,
                    "Invalid List of Data Centers",
                    "When using a strategy other than Simple the list of Data Centers cannot be empty.",
                    QMessageBox::Ok,
                    this);
            msg.exec();
            dataCenters->setFocus();
            return;
        }
    }

#if 0
    snap_manager * sm(dynamic_cast<snap_manager *>(parent()));

    if(sm)
    {
        // our parent has all the necessary info about cassandra, so it
        // implements the actual function to create the context...
        //
        sm->create_context(replicationFactor->text().toInt(), s, data_centers, host_name);

        close();

        // allow user to try again
        sm->context_is_valid();
    }
#else
    emit createContext( replicationFactor->text().toInt(), s, data_centers );
    close();
#endif
}
#pragma GCC diagnostic pop



// vim: ts=4 sw=4 et
