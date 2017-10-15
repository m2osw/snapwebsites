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

#include "snap-manager-initialize-website.h"
#include "snap-manager.h"
#include "get_child.h"

#include <QSettings>

#include <stdio.h>

snap_manager_initialize_website::snap_manager_initialize_website(QWidget *snap_parent)
    : QDialog(snap_parent)
    //, f_close_button() -- initialized below
    //, f_send_request_button() -- initialized below
    //, f_snap_server_host() -- initialized below
    //, f_snap_server_port() -- initialized below
    //, f_website_url() -- initialized below
    //, f_port() -- initialized below
    //, f_initialize_website() -- auto-init
    //, f_timer_id(0) -- auto-init
{
    setWindowModality(Qt::ApplicationModal);
    setupUi(this);

    QSettings settings( this );
    //restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
    //restoreState   ( settings.value( "state"   , saveState()    ).toByteArray() );
    //
    websiteURL->setText( settings.value( "initialization_url",  ""   ).toString() );
    port      ->setText( settings.value( "initialization_port", "80" ).toString() );

    // setup widgets
    f_close_button        = getChild<QPushButton> ( this,           "closeButton"       );
    f_send_request_button = getChild<QPushButton> ( this,           "sendRequestButton" );
    f_snap_server_host    = getChild<QLineEdit>   ( snap_parent,    "snapServerHost"    );
    f_snap_server_port    = getChild<QLineEdit>   ( snap_parent,    "snapServerPort"    );
    f_website_url         = getChild<QLineEdit>   ( parentWidget(), "websiteURL"        );
    f_port                = getChild<QLineEdit>   ( parentWidget(), "port"              );

    // Close
    connect( f_close_button.data(), &QPushButton::clicked, this, &snap_manager_initialize_website::close );

    // Send Request
    connect( f_send_request_button.data(), &QPushButton::clicked, this, &snap_manager_initialize_website::send_request );
}

snap_manager_initialize_website::~snap_manager_initialize_website()
{
}


void snap_manager_initialize_website::close()
{
    hide();

    QSettings settings( this );
    settings.setValue( "initialization_url",  websiteURL->text()  );
    settings.setValue( "initialization_port", port      ->text()  );
    //settings.setValue( "geometry",            saveGeometry()           );
}


void snap_manager_initialize_website::add_status(QString const & msg, bool const clear)
{
    QListWidget * status(getChild<QListWidget>(this, "statusInfo"));
    if(clear)
    {
        status->clear();
    }
    status->addItem(msg);
    status->scrollToBottom();
}


void snap_manager_initialize_website::send_request()
{
    // send info to the console too
    QListWidget * console = getChild<QListWidget>(parentWidget(), "snapServerConsole");
    console->clear();
    QString const version("snap::server version: " + QString(snap::server::version()));
    console->addItem(version);
    add_status(version, true);

    // get snap server host/port from the parent window
    QString snap_host(f_snap_server_host->text());
    if(snap_host.isEmpty())
    {
        snap_host = "localhost";
    }
    QString const server_host("Snap Server Host: " + snap_host);
    console->addItem(server_host);
    add_status(server_host);

    int snap_port(4004);
    if(!f_snap_server_port->text().isEmpty())
    {
        bool ok;
        snap_port = f_snap_server_port->text().toInt(&ok);
        if(!ok)
        {
            console->addItem("Invalid Port.");
            add_status("Invalid Port.");
            QMessageBox msg(QMessageBox::Critical, "Invalid Port", "The Port is not a valid integer. Please close this window and fix the port to connect to the Snap! server.", QMessageBox::Ok, this);
            msg.exec();
            return;
        }
        if(snap_port < 1 | snap_port > 65535)
        {
            console->addItem("Invalid Port (out of range).");
            add_status("Invalid Port (out of range).");
            QMessageBox msg(QMessageBox::Critical, "Invalid Port", "The Port is out of range. Please close this window and fix the port to connect to the Snap! server.", QMessageBox::Ok, this);
            msg.exec();
            return;
        }
    }
    QString const server_port("Snap Server Port: " + QString::number(snap_port));
    console->addItem(server_port);
    add_status(server_port);

    // get URI and port for query
    QString const url(f_website_url->text());
    if(url.isEmpty())
    {
        console->addItem("Missing URI.");
        add_status("Missing URI.");
        QMessageBox msg(QMessageBox::Critical, "Missing URI", "The URI is missing. Please enter a URI first and try again.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    QString const website_uri("Website URI: " + url);
    console->addItem(website_uri);
    add_status(website_uri);

    if(f_port->text().isEmpty())
    {
        console->addItem("Missing Port.");
        add_status("Missing Port.");
        QMessageBox msg(QMessageBox::Critical, "Missing Port", "The Port is missing. Please enter a Port first and try again.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    bool ok(false);
    int const site_port(f_port->text().toInt(&ok));
    if(!ok)
    {
        console->addItem("Invalid Port.");
        add_status("Invalid Port.");
        QMessageBox msg(QMessageBox::Critical, "Invalid Port", "The Port is not a valid integer. Please enter a valid Port number and try again.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    if(site_port < 1 | site_port > 65535)
    {
        console->addItem("Invalid Port (out of range).");
        add_status("Invalid Port (out of range).");
        QMessageBox msg(QMessageBox::Critical, "Invalid Port", "The Port is out of range. Please enter a valid Port number and try again.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    QString const apache_port("Apache Port: " + QString::number(site_port));
    console->addItem(apache_port);
    add_status(apache_port);

    // now send the request to the server
    // TODO: add support for SSL...
    QString const protocol(site_port == 443 ? "HTTPS" : "HTTP");
    f_initialize_website.reset(new snap::snap_initialize_website(snap_host, snap_port, false, url, site_port, "", protocol));

    // this starts a thread which sends the info to the backend
    // and wait on status messages from the backend.
    if(!f_initialize_website->start_process())
    {
        f_initialize_website.reset();

        console->addItem("Failed starting initialization process.");
        add_status("Failed starting initialization process.");
        QMessageBox msg(QMessageBox::Critical, "Failure", "Somehow the initialization process did not start.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    console->addItem("Processing Request...");
    add_status("Processing Request...");

    // disable the interface until the thread is done
    f_timer_id = startTimer(100); // 0.1 second interval

    enableAll(false);

std::cerr << "Started!\n";
}


void snap_manager_initialize_website::timerEvent(QTimerEvent *timer_event)
{
    if(timer_event->timerId() == f_timer_id)
    {
        if(f_initialize_website)
        {
            // loop over all the messages at once
            for(;;)
            {
                QString const msg(f_initialize_website->get_status());
                if(msg.isEmpty())
                {
                    if(f_initialize_website->is_done())
                    {
                        // thread all done
std::cerr << "Timer reset...\n";
                        f_initialize_website.reset();
                        killTimer(f_timer_id);
                        f_timer_id = 0;
                        enableAll(true);
                    }
                    break;
                }
std::cerr << "Timer add_status(\"" << msg << "\");...\n";
                add_status(msg);
            }
        }
        else
        {
            // this should never happen
            killTimer(f_timer_id);
            f_timer_id = 0;

            // send info to the console too
            QListWidget * console = getChild<QListWidget>(parentWidget(), "snapServerConsole");
            console->addItem("Spurious timer event.");
            QMessageBox msg(QMessageBox::Critical, "Invalid State", "We received a spurious timer event (the f_initialize_website pointer is NULL).", QMessageBox::Ok, this);
            msg.exec();
        }
    }
}


void snap_manager_initialize_website::enableAll(bool enable)
{
    f_close_button->setEnabled(enable);
    f_send_request_button->setEnabled(enable);
    f_website_url->setEnabled(enable);
    f_port->setEnabled(enable);
}


// vim: ts=4 sw=4 et
