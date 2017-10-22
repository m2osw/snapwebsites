// Snap Manager -- snap database manager to work on Cassandra's tables
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

#include "snap-manager.h"
#include "snap-manager-about.h"
#include "snap-manager-help.h"
#include "snap-manager-decode-utf8.h"
#include "get_child.h"

#include <casswrapper/schema.h>
#include <casswrapper/version.h>

#include <snapwebsites/dbutils.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/snap_uri.h>
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/tcp_client_server.h>

#include <libtld/tld.h>

#include <QHostAddress>
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QSettings>
#include <QTimer>

#include <stdio.h>

using namespace casswrapper;
using namespace casswrapper::schema;

namespace
{
    const int g_paging_size = 10;
}

snap_manager::snap_manager(QWidget *snap_parent)
    : QMainWindow(snap_parent)
    , f_current_domain_index(-1)
    , f_current_website_index(-1)
{
    setupUi(this);

    QSettings settings( this );
    restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
    restoreState   ( settings.value( "state"   , saveState()    ).toByteArray() );
    //
    cassandraHost->setText  ( settings.value( "cassandra_host", "localhost" ).toString() );
    cassandraPort->setText  ( settings.value( "cassandra_port", "9042"      ).toString() );
    useSSLCB->setChecked    ( settings.value( "use_ssl",        true        ).toBool()   );
    snapServerHost->setText ( settings.value( "snap_host",      "localhost" ).toString() );
    snapServerPort->setText ( settings.value( "snap_port",      "4004"      ).toString() );

    // Help
    QAction *a = getChild<QAction>(this, "actionSnap_Manager_Help");
    connect(a, &QAction::triggered, this, &snap_manager::help);

    // About
    a = getChild<QAction>(this, "actionAbout_Snap_Manager");
    connect(a, &QAction::triggered, this, &snap_manager::about);

    // Tools: Initialize a Website
    f_initialize_website = getChild<QAction>(this, "actionInitializeWebsite");
    connect(f_initialize_website.data(), &QAction::triggered, this, &snap_manager::initialize_website);

    // Tools: Decode UTF-8
    a = getChild<QAction>(this, "actionDecodeUTF8");
    connect(a, &QAction::triggered, this, &snap_manager::decode_utf8);

    f_tabs = getChild<QTabWidget>(this, "tabWidget");
    f_tabs->setTabEnabled(TAB_DOMAINS, false);
    f_tabs->setTabEnabled(TAB_WEBSITES, false);
    f_tabs->setTabEnabled(TAB_SITES, false);

    connect( f_tabs.data(), &QTabWidget::currentChanged, this, &snap_manager::onCurrentTabChanged );

    // Snap! Server Test and Statistics
    QPushButton *b = getChild<QPushButton>(this, "snapTest");
    connect(b, &QPushButton::clicked, this, &snap_manager::snapTest);
    b = getChild<QPushButton>(this, "snapStats");
    connect(b, &QPushButton::clicked, this, &snap_manager::snapStats);

    // Snap! Server Info
    QListWidget *console = getChild<QListWidget>(this, "snapServerConsole");
    console->addItem("snap::server version: " + QString(snap::server::version()));
    console->addItem("Not tested.");

    // Cassandra Info
    console = getChild<QListWidget>(this, "cassandraConsole");
    console->addItem("libcasswrapper version: " + QString(LIBCASSWRAPPER_LIBRARY_VERSION_STRING));
    console->addItem("Not connected.");

    // get domain friends that are going to be used here and there
    f_domain_filter = getChild<QPushButton>(this, "domainFilter");
    //connect(f_domain_filter, SIGNAL(itemClicked()), this, SLOT(on_domainFilter_clicked()));
    f_domain_filter_string = getChild<QLineEdit>(this, "domainFilterString");
    f_domain_list = getChild<QListView>(this, "domainList");
    f_domain_list->setModel( &f_domain_model );
    connect
        ( f_domain_list->selectionModel()
        , &QItemSelectionModel::currentChanged
        , this
        , &snap_manager::on_domainSelectionChanged
        );
    connect
        ( &f_domain_model
        , &TableModel::queryFinished
        , this
        , &snap_manager::onDomainsLoaded
        );
    f_domain_name = getChild<QLineEdit>(this, "domainName");
    f_domain_rules = getChild<QTextEdit>(this, "domainRules");
    f_domain_new = getChild<QPushButton>(this, "domainNew");
    f_domain_save = getChild<QPushButton>(this, "domainSave");
    f_domain_cancel = getChild<QPushButton>(this, "domainCancel");
    f_domain_delete = getChild<QPushButton>(this, "domainDelete");

    // get website friends that are going to be used here and there
    f_website_list = getChild<QListView>(this, "websiteList");
    f_website_list->setModel( &f_website_model );
    connect
        ( f_website_list->selectionModel()
        , &QItemSelectionModel::currentChanged
        , this
        , &snap_manager::on_websiteSelectionChanged
        );
    connect
        ( &f_website_model
        , &TableModel::queryFinished
        , this
        , &snap_manager::onWebsitesLoaded
        );
    f_website_name = getChild<QLineEdit>(this, "fullDomainName");
    f_website_rules = getChild<QTextEdit>(this, "websiteRules");
    f_website_new = getChild<QPushButton>(this, "websiteNew");
    f_website_save = getChild<QPushButton>(this, "websiteSave");
    f_website_cancel = getChild<QPushButton>(this, "websiteCancel");
    f_website_delete = getChild<QPushButton>(this, "websiteDelete");

    // get sites friends that are going to be used here and there
    //
    f_sites_filter = getChild<QPushButton>(this, "sitesFilter");
    f_sites_filter_string = getChild<QLineEdit>(this, "sitesFilterString");
    f_sites_list = getChild<QListView>(this, "sitesList");
    f_sites_list->setModel( &f_sites_table_model );
    connect( f_sites_list->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &snap_manager::onSitesListCurrentChanged );
    f_sites_name = getChild<QLineEdit>(this, "sitesDomainName");
    //
    f_sites_parameters = getChild<QTableView>(this, "sitesParameters");
    f_sites_parameters->setModel( &f_params_row_model );
    connect
        ( &f_params_row_model
        , &RowModel::dataChanged
        , this
        , &snap_manager::onSitesParamsDataChanged
        );
    connect( f_sites_parameters->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &snap_manager::onSitesParamsCurrentChanged );
    f_sites_parameter_name  = getChild<QLineEdit>(this, "sitesParameterName");
    f_sites_parameter_value = getChild<QLineEdit>(this, "sitesParameterValue");
    f_sites_parameter_type  = getChild<QComboBox>(this, "sitesParameterType");
    f_sites_new    = getChild<QPushButton>(this, "sitesNew");
    f_sites_save   = getChild<QPushButton>(this, "sitesSave");
    f_sites_delete = getChild<QPushButton>(this, "sitesDelete");
    f_sites_apply  = getChild<QPushButton>(this, "sitesApply");
    f_sites_revert = getChild<QPushButton>(this, "sitesRevert");
    connect( f_sites_new, &QPushButton::clicked,
            this, &snap_manager::onSitesNewClicked );
    connect( f_sites_save, &QPushButton::clicked,
            this, &snap_manager::onSitesSaveClicked );
    connect( f_sites_delete, &QPushButton::clicked,
            this, &snap_manager::onSitesDeleteClicked );
    connect( f_sites_apply, &QPushButton::clicked,
            this, &snap_manager::onSitesApplyClicked );
    connect( f_sites_revert, &QPushButton::clicked,
            this, &snap_manager::onSitesRevertClicked );

    f_sites_parameter_type->addItem("Null");
    f_sites_parameter_type->addItem("String"); // this is the default
    f_sites_parameter_type->addItem("Boolean");
    f_sites_parameter_type->addItem("Integer (8 bit)");
    f_sites_parameter_type->addItem("Integer (16 bit)");
    f_sites_parameter_type->addItem("Integer (32 bit)");
    f_sites_parameter_type->addItem("Integer (64 bit)");
    f_sites_parameter_type->addItem("Floating Point (32 bit)");
    f_sites_parameter_type->addItem("Floating Point (64 bit)");
    f_sites_parameter_type->setCurrentIndex(1);

    connect( qApp, &QApplication::aboutToQuit, this, &snap_manager::OnAboutToQuit );
}

snap_manager::~snap_manager()
{
}


void snap_manager::OnAboutToQuit()
{
    QSettings settings( this );
    settings.setValue( "cassandra_host", cassandraHost->text()  );
    settings.setValue( "cassandra_port", cassandraPort->text()  );
    settings.setValue( "use_ssl",        useSSLCB->isChecked()  );
    settings.setValue( "snap_host",      snapServerHost->text() );
    settings.setValue( "snap_port",      snapServerPort->text() );
    settings.setValue( "geometry",       saveGeometry()         );
    settings.setValue( "state",          saveState()            );
}


void snap_manager::about()
{
    if(f_about == nullptr)
    {
        f_about = new snap_manager_about(this);
    }
    f_about->show();
}

void snap_manager::help()
{
    if(f_help == nullptr)
    {
        f_help = new snap_manager_help(this);
    }
    f_help->show();
}

void snap_manager::decode_utf8()
{
    if(f_decode_utf8 == nullptr)
    {
        f_decode_utf8 = new snap_manager_decode_utf8(this);
    }
    f_decode_utf8->show();
}

void snap_manager::snapTest()
{
    // retrieve the current values
    QLineEdit *l = getChild<QLineEdit>(this, "snapServerHost");
    f_snap_host = l->text();
    if(f_snap_host.isEmpty())
    {
        f_snap_host = "localhost";
    }
    l = getChild<QLineEdit>(this, "snapServerPort");
    if(l->text().isEmpty())
    {
        f_snap_port = 4004;
    }
    else
    {
        f_snap_port = l->text().toInt();
    }

    QListWidget *console = getChild<QListWidget>(this, "snapServerConsole");
    console->clear();
    console->addItem("snap::server version: " + QString(snap::server::version()));
    console->addItem("Host: " + f_snap_host);
    console->addItem("Port: " + QString::number(f_snap_port));

    // reconnect with the new info
    // note: the disconnect does nothing if not already connected
    tcp_client_server::tcp_client::pointer_t socket;
    try
    {
         socket.reset(new tcp_client_server::tcp_client(f_snap_host.toUtf8().data(), f_snap_port));
    }
    catch(tcp_client_server::tcp_client_server_runtime_error const&)
    {
        console->addItem("Connection Failed.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to connect to the Snap! Server (connection error).\n\nPlease verify that a Snap! server is running at the specified address.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    // send the #INFO command
    if(socket->write("#INFO\n", 6) != 6)
    {
        console->addItem("Unknown state.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to communicate with the Snap! Server (write error).", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    // read the results of the #INFO command
    bool started(false);
    for(;;)
    {
        // versions are expected to be relatively small so 256 chars per line is enough
        std::string buf;
        int r(socket->read_line(buf));
        if(r <= 0)
        {
            // note that r == 0 is not an error but it should not happen
            // (i.e. I/O is blocking so we should not return too soon.)
            console->addItem("Unknown state.");
            QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to communicate with the Snap! Server (read error).", QMessageBox::Ok, this);
            msg.exec();
            return;
        }
        if(!started)
        {
            if(buf != "#START")
            {
                console->addItem("Connected with an invalid status.");
                QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was able to communicate with the Snap! Server but got unexpected protocol data.", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
            started = true;
        }
        else if(buf == "#END")
        {
            // got the #END mark, we're done
            break;
        }
        else
        {
            QString const line(QString::fromUtf8(buf.c_str(), buf.length()));
            int const equal_pos(line.indexOf('='));
            if(equal_pos <= 0)
            {
                // zero is an error too since `name` would be empty
                console->addItem("Connected with an invalid status.");
                QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was able to communicate with the Snap! Server but got unexpected variable data.", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
            QString const name(line.left(equal_pos));
            QString const value(line.mid(equal_pos + 1).trimmed());
            if(name == "VERSION")
            {
                console->addItem("Live Snap Server v" + value);
            }
            else if(name == "OS")
            {
                console->addItem("Operating System: " + value);
            }
            else if(name == "QT")
            {
                console->addItem("Snap Server compiled with Qt v" + value);
            }
            else if(name == "RUNTIME_QT")
            {
                console->addItem("Snap Server running with Qt v" + value);
            }
            else if(name == "LIBTLD")
            {
                console->addItem("Snap Server compiled with libtld v" + value);
            }
            else if(name == "RUNTIME_LIBTLD")
            {
                console->addItem("Snap Server running with libtld v" + value);
            }
            else if(name == "LIBCASSWRAPPER")
            {
                console->addItem("Snap Server compiled with libcasswrapper v" + value);
            }
            else if(name == "RUNTIME_LIBCASSWRAPPER")
            {
                console->addItem("Snap Server running with libcasswrapper v" + value);
            }
            else if(name == "LIBQTSERIALIZATION")
            {
                console->addItem("Snap Server compiled with libQtSerialization v" + value);
            }
            else if(name == "RUNTIME_LIBQTSERIALIZATION")
            {
                console->addItem("Snap Server running with libQtSerialization v" + value);
            }
            else
            {
                console->addItem("Unknown variable: " + name + "=" + value);
            }
        }
    }
}

void snap_manager::snapStats()
{
    // retrieve the current values
    QLineEdit *l = getChild<QLineEdit>(this, "snapServerHost");
    f_snap_host = l->text();
    if(f_snap_host.isEmpty())
    {
        f_snap_host = "localhost";
    }
    l = getChild<QLineEdit>(this, "snapServerPort");
    if(l->text().isEmpty())
    {
        f_snap_port = 4004;
    }
    else
    {
        f_snap_port = l->text().toInt();
    }

    QListWidget *console = getChild<QListWidget>(this, "snapServerConsole");
    console->clear();
    console->addItem("snap::server version: " + QString(snap::server::version()));
    console->addItem("Host: " + f_snap_host);
    console->addItem("Port: " + QString::number(f_snap_port));

    // reconnect with the new info
    // note: the disconnect does nothing if not already connected
    tcp_client_server::tcp_client::pointer_t socket;
    try
    {
         socket.reset(new tcp_client_server::tcp_client(f_snap_host.toUtf8().data(), f_snap_port));
    }
    catch(tcp_client_server::tcp_client_server_runtime_error const&)
    {
        console->addItem("Connection Failed.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to connect to the Snap! Server (connection error).\n\nPlease verify that a Snap! server is running at the specified address.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    // send the #STATS command
    if(socket->write("#STATS\n", 7) != 7)
    {
        console->addItem("Unknown state.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to communicate with the Snap! Server (write error).", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    // read the results of the #STATS command
    bool started(false);
    for(;;)
    {
        // versions are expected to be relatively small so 256 chars per line is enough
        std::string buf;
        int r(socket->read_line(buf));
        if(r <= 0)
        {
            // note that r == 0 is not an error but it should not happen
            // (i.e. I/O is blocking so we should not return too soon.)
            console->addItem("Unknown state.");
            QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to communicate with the Snap! Server (read error).", QMessageBox::Ok, this);
            msg.exec();
            return;
        }
        if(!started)
        {
            if(buf != "#START")
            {
                console->addItem("Connected with an invalid status.");
                QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was able to communicate with the Snap! Server but got unexpected protocol data.", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
            started = true;
        }
        else if(buf == "#END")
        {
            // got the #END mark, we're done
            break;
        }
        else
        {
            QString const line(QString::fromUtf8(buf.c_str(), buf.length()));
            int const equal_pos(line.indexOf('='));
            if(equal_pos <= 0)
            {
                // zero is an error too since `name` would be empty
                console->addItem("Connected with an invalid status.");
                QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was able to communicate with the Snap! Server but got unexpected variable data.", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
            QString const name(line.left(equal_pos));
            QString const value(line.mid(equal_pos + 1).trimmed());
            if(name == "VERSION")
            {
                console->addItem("Live Snap Server v" + value);
                // add an empty line before the stats
                console->addItem(" ");
            }
            else if(name == "CONNECTIONS_COUNT")
            {
                console->addItem("Connections: " + value);
            }
            else
            {
                console->addItem("Unknown variable: " + name + "=" + value);
            }
        }
    }
}

void snap_manager::on_f_cassandraConnectButton_clicked()
{
    f_cassandraConnectButton->setEnabled( false );
    f_cassandraDisconnectButton->setEnabled( false );

    if(!f_session)
    {
        f_session = Session::create();
    }

    // save the old values
    QString old_host(f_cassandra_host);
    int old_port(f_cassandra_port);

    // retrieve the current values
    QLineEdit *l = getChild<QLineEdit>(this, "cassandraHost");
    f_cassandra_host = l->text();
    if(f_cassandra_host.isEmpty())
    {
        f_cassandra_host = "localhost";
    }
    l = getChild<QLineEdit>(this, "cassandraPort");
    if(l->text().isEmpty())
    {
        f_cassandra_port = 9042;
    }
    else
    {
        f_cassandra_port = l->text().toInt();
    }

    // if old != new then connect to new
    if(f_cassandra_host == old_host && f_cassandra_port == old_port && f_session->isConnected())
    {
        // nothing changed, stay put
        on_f_cassandraDisconnectButton_clicked();
        return;
    }

    QListWidget *console = getChild<QListWidget>(this, "cassandraConsole");
    console->clear();
    console->addItem("libcasswrapper version: " + QString(LIBCASSWRAPPER_LIBRARY_VERSION_STRING));
    console->addItem("Host: " + f_cassandra_host);
    console->addItem("Port: " + QString::number(f_cassandra_port));

    f_tabs->setTabEnabled(TAB_DOMAINS, false);
    f_tabs->setTabEnabled(TAB_WEBSITES, false);
    f_tabs->setTabEnabled(TAB_SITES, false);

    // reconnect with the new info
    // note: the disconnect does nothing if not already connected
    f_session->disconnect();
    try
    {
        f_session->connect( f_cassandra_host, f_cassandra_port, useSSLCB->isChecked() );
    }
    catch( const std::exception& ex )
    {
        // did not work...
        console->addItem( QString("Not connected! Error=[%1]").arg(QString(ex.what())) );
        QMessageBox msg
            ( QMessageBox::Critical
            , "Connection to Cassandra"
            , "Snap! Manager was not able to connect to your Cassandra Cluster.\n"
              "Please verify that it is up and running and accessible (no firewall) from this computer."
            , QMessageBox::Ok
            , this
            );
        msg.exec();

        // give user a chance to try again with another IP or
        // possibly to start the Cassandra server
        on_f_cassandraDisconnectButton_clicked();
        return;
    }

    // read and display the Cassandra information
    auto q = Query::create( f_session );
    q->query( "SELECT cluster_name,native_protocol_version FROM system.local" );
    q->start();
    console->addItem("Cluster Name: "     + q->getVariantColumn("cluster_name").toString());
    console->addItem("Protocol Version: " + q->getVariantColumn("native_protocol_version").toString());
    q->end();

    // read all the contexts so the findContext() works
    SessionMeta::pointer_t meta( SessionMeta::create(f_session) );
    meta->loadSchema();
    //
    const auto& keyspace_list( meta->getKeyspaces() );
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    //
    const auto& snap_keyspace_iter( keyspace_list.find(context_name) );
    if( snap_keyspace_iter == keyspace_list.end() )
    {
        // we connected to the database, but it is not initialized yet
        // offer the user to do the initialization now
        //
        console->addItem("The \"" + context_name + "\" context is not defined.");

        if(f_createcontext_window == nullptr)
        {
            f_createcontext_window = new snap_manager_createcontext(this);
            connect( f_createcontext_window.data(), &snap_manager_createcontext::createContext,
                     this, &snap_manager::create_context );
            connect( f_createcontext_window.data(), &snap_manager_createcontext::disconnectRequested,
                     this, &snap_manager::cassandraDisconnectButton_clicked );
        }
        f_createcontext_window->show();
        return;
    }

    //
    // also check for the 2 main tables
    snap::name_t names[2] = { snap::name_t::SNAP_NAME_DOMAINS, snap::name_t::SNAP_NAME_WEBSITES /*, snap::name_t::SNAP_NAME_SITES*/ };
    for(int i = 0; i < 2; ++i)
    {
        QString const table_name(snap::get_name(names[i]));
        const auto& table_list( snap_keyspace_iter->second->getTables() );
        if( table_list.find(table_name) == table_list.end() )
        {
            // we connected to the database, but it is not properly initialized
            console->addItem("The \"" + table_name + "\" table is not defined.");
            QMessageBox msg(QMessageBox::Critical, "Connection to Cassandra", "Snap! Manager was able to connect to your Cassandra Cluster but it does not include a \"" + table_name + "\" table. The Snap! Server creates the necessary context and tables, have you run it?", QMessageBox::Ok, this);
            msg.exec();

            // give user a chance to try again with another IP or
            // possibly to start the Cassandra server
            on_f_cassandraDisconnectButton_clicked();
            return;
        }
    }

    context_is_valid();
}


void snap_manager::onCurrentTabChanged( int index )
{
    if( index == TAB_DOMAINS )
    {
        loadDomains();
    }
    else if( index == TAB_WEBSITES )
    {
        loadWebsites();
    }
    else if( index == TAB_SITES )
    {
        loadSites();
    }

    startQuery();
}


void snap_manager::context_is_valid()
{
#if 0
    // TODO: call these functions when their respective tab is clicked instead!
    loadDomains();
    loadSites();
#endif

    // we just need to be connected for TAB_SITES
    f_tabs->setTabEnabled(TAB_DOMAINS,  true);
    //f_tabs->setTabEnabled(TAB_WEBSITES, true);
    f_tabs->setTabEnabled(TAB_SITES,    true);

    f_cassandraDisconnectButton->setEnabled( true );
}


void snap_manager::on_f_cassandraDisconnectButton_clicked()
{
    cassandraDisconnectButton_clicked();
}


void snap_manager::cassandraDisconnectButton_clicked()
{
    f_cassandraConnectButton->setEnabled( false );
    f_cassandraDisconnectButton->setEnabled( false );

    // disconnect by deleting the object altogether
    f_session->disconnect();
    f_session.reset();

    QListWidget *console = getChild<QListWidget>(this, "cassandraConsole");
    console->clear();
    console->addItem("libcasswrapper version: " + QString(LIBCASSWRAPPER_LIBRARY_VERSION_STRING));
    console->addItem("Not connected.");

    f_tabs->setTabEnabled(TAB_DOMAINS, false);
    f_tabs->setTabEnabled(TAB_WEBSITES, false);
    f_tabs->setTabEnabled(TAB_SITES, false);

    // this doesn't get cleared otherwise
    f_domain_list->clearSelection();
    f_domain_filter_string->setText("");
    f_domain_org_name = "";
    f_domain_name->setText("");
    f_domain_org_rules = "";
    f_domain_rules->setText("");

    // just in case, reset the sites widgets too
    f_sites_org_name = "";
    f_sites_name->setText("");
    f_sites_parameters->setEnabled(false);
    f_sites_parameter_name->setEnabled(false);
    f_sites_parameter_name->setText("");
    f_sites_parameter_value->setEnabled(false);
    f_sites_parameter_value->setText("");
    f_sites_parameter_type->setEnabled(false);
    f_sites_parameter_type->setCurrentIndex(1);
    f_sites_new->setEnabled(false);
    f_sites_save->setEnabled(false);
    f_sites_delete->setEnabled(false);
    f_sites_apply->setEnabled(false);
    f_sites_revert->setEnabled(false);

    f_cassandraConnectButton->setEnabled( true );
}


/** \brief Create query object and return it.
 *
 * Pass in a query string and the table name to query against. Make sure your query string
 * contains QString placeholders (%1, %2, etc) for the context (keyspace) and table name.
 *
 * For example:
 *  SELECT key,column1,value FROM %1.%2 WHERE key = ?
 *
 * This will fill in the context name (SNAP_NAME_CONTEXT) and the table name based on the parameter.
 * Also, the query will be pushed onto the query stack which you can use to keep track of the query
 * object and pop off the stack when it is complete.
 *
 * You'll have the chance to bind your variables on the returned query object.
 *
 * \param table_name    name of the table to substitute in %2
 * \param q_str         query to run
 */
Query::pointer_t snap_manager::createQuery
    ( const QString& q_str
    )
{
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    auto query = Query::create(f_session);
    query->query( q_str
            .arg(context_name)
            , q_str.count('?')
            );
    connect( query.get(), &Query::queryFinished, this, &snap_manager::onQueryFinished );
    return query;
}

Query::pointer_t snap_manager::createQuery
    ( const QString& table_name
    , const QString& q_str
    )
{
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    auto query = Query::create(f_session);
    query->query( q_str
            .arg(context_name)
            .arg(table_name)
            , q_str.count('?')
            );
    connect( query.get(), &Query::queryFinished, this, &snap_manager::onQueryFinished );
    return query;
}


void snap_manager::addQuery( Query::pointer_t q )
{
    f_queryQueue.push( q );
}


void snap_manager::startQuery()
{
    if( f_queryQueue.empty() )
    {
        return;
    }

    auto& front(f_queryQueue.front());
    //QListWidget * console = getChild<QListWidget>(this, "cassandraConsole");
    //console->addItem( QString("Starting query [%1].").arg(front->description()) );
    front->start( false /*don't block*/ );
}


/** \brief Get the result of the current query
 *
 * This method adds a line to the output area indicating that the query has completed.
 * If there was an error, it is logged and the user is notified by message box.
 */
bool snap_manager::getQueryResult( Query::pointer_t q )
{
    try
    {
        //console->addItem( QString("[%1] has finished!").arg(q->description()) );
        if( !q->queryActive() )
        {
            q->getQueryResult();
        }
        return true;
    }
    catch( const std::exception& ex )
    {
        QListWidget * console = getChild<QListWidget>(this, "cassandraConsole");
        console->addItem( QString("Query Error: [%1]").arg(ex.what()) );
        QMessageBox::critical( this, tr("Query Error!"), ex.what() );
    }

    return false;
}


/** \brief Event handler for finished queries on the stack
 *
 * When a query is finished, this method is then called. The console is
 * logged to, then the bottom query, which just completed, is ended and popped.
 * Then the cycle is started again on the new bottom query.
 */
void snap_manager::onQueryFinished( Query::pointer_t q )
{
    f_queryQueue.pop();

    snap::NOTUSED(getQueryResult( q ));

    startQuery();
}


/** \brief Create the snap_websites context and first few tables.
 *
 * This function creates the snap_websites context.
 *
 * The strategy is defined as a number which represents the selection
 * in the QComboBox of the dialog we just shown to the user. The
 * values are:
 *
 * \li 0 -- Simple
 * \li 1 -- Local
 * \li 2 -- Network
 *
 * \warning
 * It is assumed that you checked all the input parameters validity:
 *
 * \li the replication_factor is under or equal to the number of Cassandra nodes
 * \li the strategy can only be 0, 1, or 2
 * \li the data_centers list cannot be empty
 * \li the host_name must match [a-zA-Z_][a-zA-Z_0-9]*
 *
 * \param[in] replication_factor  Number of times the data will be duplicated.
 * \param[in] strategy  The strategy used to manage the cluster.
 * \param[in] data_centers  List of data centers, one per line, used if
 *                          strategy is not Simple (0).
 * \param[in] host_name  The name of a host to runn a snap server instance.
 */
void snap_manager::create_context(int replication_factor, int strategy, snap::snap_string_list const & data_centers )
{
    // when called here we have f_session defined but no context yet
    //
    QString query_str( "CREATE KEYSPACE %1\n" );

    // this is the default for contexts, but just in case we were
    // to change that default at a later time...
    //
    query_str += QString("WITH durable_writes = true\n");

    // for developers testing with a few nodes in a single data center,
    // SimpleStrategy is good enough; for anything larger ("a real
    // cluster",) it won't work right
    query_str += QString("AND replication =\n");
    if(strategy == 0 /*"simple"*/)
    {
        query_str += QString( "\t{ 'class': 'SimpleStrategy', 'replication_factor': '1' }\n" );
    }
    else
    {
        query_str += QString( "\t{ 'class': 'NetworkTopologyStrategy',\n" );

        // else strategy == 1 /*"network"*/
        //
        QString s;
        QString const replication(QString("%1").arg(replication_factor));
        int const max_names(data_centers.size());
        for(int idx(0); idx < max_names; ++idx)
        {
            if( !s.isEmpty() )
            {
                s += ",\n";
            }
            s += QString("\t\t'%1': '%2'").arg(data_centers[idx]).arg(replication);
        }
        query_str += s + "}\n";
    }

    auto query( createQuery(query_str) );
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    query->setDescription( QString("Create %1 context").arg(context_name) );
    addQuery(query);

    // now we want to add the "domains" and "websites" tables to be
    // complete; also not having the "sites" table can cause problem
    // for that tab, create it now too.
    //
    create_table(snap::get_name(snap::name_t::SNAP_NAME_DOMAINS),  "List of domain rules");
    create_table(snap::get_name(snap::name_t::SNAP_NAME_WEBSITES), "List of website rules");
    create_table(snap::get_name(snap::name_t::SNAP_NAME_SITES),    "Various global settings for websites");

    connect( f_queryQueue.back().get(), &Query::queryFinished, this, &snap_manager::onContextCreated );
    startQuery();
}


void snap_manager::onContextCreated( Query::pointer_t /*q*/ )
{
    //getQueryResult(q);
    context_is_valid();
}


void snap_manager::create_table(QString const & table_name, QString const & comment)
{

    // use RAII at some point, although an exception and we exit anyway...
    casswrapper::Session::pointer_t save_session(f_session);

    {
        f_session = casswrapper::Session::create();

        // increase timeout to 5 min. while creating tables
        // (must be done before the connect() below)
        //
        f_session->setTimeout(5 * 60 * 1000);
        f_session->connect( f_cassandra_host, f_cassandra_port, useSSLCB->isChecked() );

        QString query_str( "CREATE TABLE %1.%2 (key blob, column1 blob, value blob, PRIMARY KEY ((key), column1))\n" );

        // this is the default for contexts, but just in case we were
        // to change that default at a later time...
        //
        query_str += QString("WITH comment = '%1'\n").arg(comment);
        query_str += QString("AND memtable_flush_period_in_ms = 3600000\n");
        query_str += QString("AND gc_grace_seconds = 864000\n");
        query_str += QString("AND compaction =\n");
        query_str += QString( "\t{ 'class': 'SizeTieredCompactionStrategy', "
                              "'min_threshold': '4', "
                              "'max_threshold': '22'}\n" );
        auto query( createQuery(table_name, query_str) );
        query->setDescription( QString("Create [%1] table, comment=[%2]")
            .arg(table_name)
            .arg(comment) );
        addQuery(query);
    }

    // restore "normal" session
    f_session = save_session;
}


void snap_manager::initialize_website()
{
    if(f_initialize_website_window == nullptr)
    {
        f_initialize_website_window = new snap_manager_initialize_website(this);
    }
    f_initialize_website_window->add_status("Enter a URI and port,\nthen click on Send Request.", true);
    f_initialize_website_window->show();
}

void snap_manager::loadDomains()
{
    if( f_domain_list->currentIndex().isValid() )
    {
        f_current_domain_index = f_domain_list->currentIndex().row();
    }
    else
    {
        f_current_domain_index = -1;
    }

    // we just checked to know whether the table existed so it cannot fail here
    // however the index table could be missing...
    f_domain_model.init( f_session, "", "" );
    f_domain_model.doQuery();
}


void snap_manager::onDomainsLoaded()
{
    // at first some of the entries are disabled
    // until a select is made or New is clicked
    f_domain_name->setEnabled(false);
    f_domain_org_name = ""; // not editing, this is new
    f_domain_name->setText("");
    f_domain_rules->setEnabled(false);
    f_domain_org_rules = "";
    f_domain_rules->setText("");
    f_domain_save->setEnabled(false);
    f_domain_cancel->setEnabled(false);
    f_domain_delete->setEnabled(false);

    // allow user to go to that tab
    f_tabs->setTabEnabled(TAB_DOMAINS, true);
    f_tabs->setTabEnabled(TAB_WEBSITES, false); // we lose focus so we want to reset that one

    if( f_current_domain_index != -1 )
    {
        const QModelIndex idx( f_domain_model.index( f_current_domain_index, 0 ) );
        if( idx.isValid() )
        {
            f_domain_list->setCurrentIndex( idx );
        }
    }
}

#if 0
void snap_manager::onLoadDomains( Query::pointer_t q )
{
    if( !getQueryResult(q) )
    {
        return;
    }

    QStringList keys;
    QStringList domains;
    while( q->nextRow() )
    {
        const QByteArray key( q->getByteArrayColumn("key") );
        const QByteArray ba( q->getByteArrayColumn("column1") );
        QString str( QString("%1 - %2")
                     .arg(QString::fromUtf8(key.constData(), key.size()))
                     .arg(QString::fromUtf8(ba.constData(), ba.size() ))
                     );
        domains << str; //QString::fromUtf8( ba.constData(), ba.size() );
    }

    f_domain_list->addItems( domains );

    if( q->nextPage( false ) )
    {
        // Return and we'll process the next waiting query
        return;
    }

    // at first some of the entries are disabled
    // until a select is made or New is clicked
    f_domain_name->setEnabled(false);
    f_domain_org_name = ""; // not editing, this is new
    f_domain_name->setText("");
    f_domain_rules->setEnabled(false);
    f_domain_org_rules = "";
    f_domain_rules->setText("");
    f_domain_save->setEnabled(false);
    f_domain_cancel->setEnabled(false);
    f_domain_delete->setEnabled(false);

    // allow user to go to that tab
    f_tabs->setTabEnabled(TAB_DOMAINS, true);
    f_tabs->setTabEnabled(TAB_WEBSITES, false); // we lose focus so we want to reset that one
}
#endif

void snap_manager::domainWithSelection()
{
    // now there is a selection, everything is enabled
    f_domain_name->setEnabled(true);
    f_domain_rules->setEnabled(true);
    f_domain_save->setEnabled(true);
    f_domain_cancel->setEnabled(true);
    f_domain_delete->setEnabled(true);

    // this is "complicated" since we will have to use the
    // f_domain_org_name until the user saves since the name
    // may change in between...
    bool enable_websites = f_domain_org_name != "";
    f_tabs->setTabEnabled(TAB_WEBSITES, enable_websites);
#if 0
    if(enable_websites)
    {
        // TODO: call that function when the tab is clicked instead!
        loadWebsites();
    }
#endif
}

bool snap_manager::domainChanged()
{
    // if something changed we want to warn the user before going further
    if(f_domain_org_name != f_domain_name->text()
    || f_domain_org_rules != f_domain_rules->toPlainText())
    {
        QMessageBox msg(QMessageBox::Critical, "Domain Modified", "You made changes to this entry and did not Save it yet. Do you really want to continue? If you click Ok you will lose your changes.", QMessageBox::Ok | QMessageBox::Cancel, this);
        int const choice(msg.exec());
        if(choice != QMessageBox::Ok)
        {
            return false;
        }
    }

    return true;
}

void snap_manager::on_domainFilter_clicked()
{
    // make sure the user did not change something first
    if(domainChanged())
    {
        // user is okay with losing changes or did not make any
        // the following applies the filter (Apply button)
        loadDomains();
        startQuery();
    }
}

void snap_manager::on_domainSelectionChanged( const QModelIndex & /*selected*/, const QModelIndex & /*deselected*/ )
{
    const QString text( f_domain_model.data( f_domain_list->currentIndex() ).toString());

    // same domain? if so, skip on it
    if(f_domain_org_name == text && !f_domain_org_name.isEmpty())
    {
        return;
    }

#if 0
    // check whether the current info was modified
    if(!domainChanged())
    {
        // user canceled his action
        // TODO: we need to reset the item selection...
        QList<QListWidgetItem *> items(f_domain_list->findItems(f_domain_org_name, Qt::MatchExactly));
        if(items.count() > 0)
        {
            f_domain_list->setCurrentItem(items[0]);
        }
        else
        {
            f_domain_list->clearSelection();
        }
        return;
    }
#endif

    f_domain_org_name = text;
    f_domain_name->setText(f_domain_org_name);

    QString const table_name              ( snap::get_name( snap::name_t::SNAP_NAME_DOMAINS));
    QString const core_original_rules_name( snap::get_name( snap::name_t::SNAP_NAME_CORE_ORIGINAL_RULES));

    auto query = createQuery( table_name, "SELECT value FROM %1.%2 WHERE key = ? AND column1 = ?" );
    query->setDescription( QString("Retrieving rules for domain [%1]").arg(f_domain_org_name) );
    size_t num = 0;
    query->bindByteArray( num++, f_domain_org_name.toUtf8() );
    query->bindByteArray( num++, core_original_rules_name.toUtf8() );
    query->start();

    if( query->nextRow() )
    {
        f_domain_org_rules = query->getVariantColumn(0).toString();
    }
    else
    {
        // this case happens after a delete (i.e. the row still exist but is empty)
        f_domain_org_rules = "";
    }
    f_domain_rules->setText(f_domain_org_rules);

    query->end();

    domainWithSelection();
}


void snap_manager::on_domainNew_clicked()
{
    // check whether the current info was modified
    if(!domainChanged())
    {
        // user canceled his action
        return;
    }

    f_domain_list->clearSelection();

    f_domain_org_name = ""; // not editing, this is new
    f_domain_name->setText("");
    f_domain_org_rules = "";
    f_domain_rules->setText(
            "main {\n"
            "  required host = \"www\\.\";\n"
            "};\n"
        );

    domainWithSelection();
    f_domain_delete->setEnabled(false);
}

void snap_manager::on_domainSave_clicked()
{
    QString name(f_domain_name->text());
    if(name.isEmpty())
    {
        QMessageBox msg(QMessageBox::Critical, "Name Missing", "You cannot create a new domain entry without giving the domain a valid name.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    QString rules(f_domain_rules->toPlainText());
    if(rules.isEmpty())
    {
        QMessageBox msg(QMessageBox::Critical, "Rules Missing", "Adding a domain requires you to enter at least one rule.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    if(name != f_domain_org_name || rules != f_domain_org_rules)
    {
        // make sure the domain name is correct (i.e. domain + TLD)
        tld_result r;
        tld_info info;
        // save in temporary buffer otherwise we'd lose the string pointers
        // in the tld_info structure
        QByteArray str(name.toUtf8());
        const char *d = str.data();
        r = tld(d, &info);
        if(r != TLD_RESULT_SUCCESS)
        {
            QMessageBox msg(QMessageBox::Critical, "Invalid TLD in Domain Name", "The TLD must be a known TLD. The tld() function could not determine the TLD of this domain name. Please check the domain name and make the necessary adjustments.", QMessageBox::Ok, this);
            msg.exec();
            return;
        }
        // TODO: accept a period at the beginning (although we want to remove it)
        //       so .snapwebsites.org would become snapwebsites.org
        for(; d < info.f_tld; ++d)
        {
            if(*d == '.')
            {
                QMessageBox msg(QMessageBox::Critical, "Invalid sub-domain in Domain Name", "Your domain name cannot include any sub-domain names. Instead, the rules determine how the sub-domains are used and the attached websites.", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
        }

        // domain name is considered valid for now
        // check the rules
        snap::snap_uri_rules domain_rules;
        QByteArray compiled_rules;
        if(!domain_rules.parse_domain_rules(rules, compiled_rules))
        {
            QMessageBox msg(QMessageBox::Critical, "Invalid Domain Rules", "An error was detected in your domain rules: " + domain_rules.errmsg(), QMessageBox::Ok, this);
            msg.exec();
            return;
        }

#if 0
        // This doesn't make a lot of sense in CQL land, since INSERT and UPDATE do basically the same thing.
        //
        QString const table_name              ( snap::get_name( snap::name_t::SNAP_NAME_DOMAINS));
        QString const core_original_rules_name( snap::get_name( snap::name_t::SNAP_NAME_CORE_ORIGINAL_RULES));

        auto query = createQuery( table_name, "SELECT * FROM %1.%2 WHERE key = ? AND column1 = ?" );
        query->setDescription( QString("Retrieving rules for domain [%1] to see if it already exists").arg(name) );
        size_t num = 0;
        query->bindByteArray( num++, name.toUtf8()                     );
        query->bindByteArray( num++, core_original_rules_name.toUtf8() );
        query->start();
        if( query->rowCount() > 0 )
        {
            if(f_domain_org_name.isEmpty())
            {
                // TODO: looks like the same message in both cases!
                QMessageBox msg( QMessageBox::Critical
                                , "Domain Name already defined"
                                , "You asked to create a new Domain Name and yet you specified a Domain Name that is already defined in the database. Please change the Domain Name or Cancel and then edit the existing name."
                                , QMessageBox::Ok
                                , this
                                );
                msg.exec();
            }
            else
            {
                QMessageBox msg( QMessageBox::Critical
                                , "Domain Name already defined"
                                , "You asked to create a new Domain Name and yet you specified a Domain Name that is already defined in the database. Please change the Domain Name or Cancel and then edit the existing name."
                                , QMessageBox::Ok
                                , this
                                );
                msg.exec();
            }
            return;
        }
#endif
        saveDomain();
    }
}


void snap_manager::saveDomain()
{
    QString const name                    ( f_domain_name->text());
    QString const rules                   ( f_domain_rules->toPlainText());
    QString const table_name              ( snap::get_name( snap::name_t::SNAP_NAME_DOMAINS));
    QString const core_rules_name         ( snap::get_name( snap::name_t::SNAP_NAME_CORE_RULES));
    QString const core_original_rules_name( snap::get_name( snap::name_t::SNAP_NAME_CORE_ORIGINAL_RULES));

    // domain name is considered valid for now
    // check the rules
    snap::snap_uri_rules domain_rules;
    QByteArray compiled_rules;
    domain_rules.parse_domain_rules(rules, compiled_rules);

    // (*table)[name][QString("core::original_rules")] = casswrapper::Value(rules);
    auto query = createQuery( table_name, "INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)" );
    query->setDescription( QString("Update core rules for %1").arg(name) );
    size_t num = 0;
    query->bindByteArray( num++, name.toUtf8() );
    query->bindByteArray( num++, core_original_rules_name.toUtf8() );
    query->bindByteArray( num++, rules.toUtf8() );
    addQuery(query);

    // (*table)[name][QString("core::rules")] = casswrapper::Value(compiled_rules);
    query = createQuery( table_name, "INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)" );
    query->setDescription( QString("Update core rules for %1").arg(name) );
    num = 0;
    query->bindByteArray( num++, name.toUtf8() );
    query->bindByteArray( num++, core_rules_name.toUtf8() );
    query->bindByteArray( num++, compiled_rules );
    connect( query.get(), &Query::queryFinished, this, &snap_manager::onFinishedSaveDomain );
    addQuery(query);

    startQuery();

    f_domain_name->setEnabled(false);
    f_domain_rules->setEnabled(false);
    f_domain_save->setEnabled(false);
    f_domain_cancel->setEnabled(false);
    f_domain_delete->setEnabled(false);
}

void snap_manager::onFinishedSaveDomain( Query::pointer_t /*q*/ )
{
    QString const name(f_domain_name->text());
    QString const rules(f_domain_rules->toPlainText());

#if 0
    // the data is now in the database, add it to the table too
    if(f_domain_org_name == "")
    {
        f_domain_list->addItem(name);

        // make sure we select that item too
        QList<QListWidgetItem *> items(f_domain_list->findItems(name, Qt::MatchExactly));
        if(items.count() > 0)
        {
            f_domain_list->setCurrentItem(items[0]);
        }
    }
#endif

    f_domain_org_name  = name;
    f_domain_org_rules = rules;

    f_domain_model.doQuery();

    domainWithSelection();
}


void snap_manager::on_domainCancel_clicked()
{
    // check whether the current info was modified
    if(!domainChanged())
    {
        // user canceled his action
        return;
    }

    // restore the original values
    f_domain_name->setText(f_domain_org_name);
    f_domain_rules->setText(f_domain_org_rules);

    if(f_domain_org_name.length() == 0)
    {
        // if we had nothing selected, reset everything
        f_domain_name->setEnabled(false);
        f_domain_rules->setEnabled(false);
        f_domain_save->setEnabled(false);
        f_domain_cancel->setEnabled(false);
        f_domain_delete->setEnabled(false);
    }
}


void snap_manager::on_domainDelete_clicked()
{
    const QString& domain_name(f_domain_name->text());

    // verify that the user really wants to delete this domain
    QMessageBox msg
        ( QMessageBox::Critical
        , "Delete Domain"
        , "<font color=\"red\"><b>WARNING:</b></font> You are about to delete domain \"" + domain_name + "\" and ALL of its websites definitions. Are you absolutely sure you want to do that?"
        , QMessageBox::Ok | QMessageBox::Cancel
        , this
        );
    if( msg.exec() != QMessageBox::Ok )
    {
        return;
    }

    f_domain_name->setEnabled(false);
    f_domain_rules->setEnabled(false);
    f_domain_save->setEnabled(false);
    f_domain_cancel->setEnabled(false);
    f_domain_delete->setEnabled(false);

    QString const domains_table_name(snap::get_name(snap::name_t::SNAP_NAME_DOMAINS));
    QString const websites_table_name(snap::get_name(snap::name_t::SNAP_NAME_WEBSITES));
    QString const row_index_name(snap::get_name(snap::name_t::SNAP_NAME_INDEX));         // *index* entry, which is now deprectated

    // Go through the websites table and drop all entries that are of the form:
    //
    // 		websitename.domainname.com
    //
    // Then, delete the domain name from the domains tables.
    //
    QStringList website_rows_to_drop;
    //
    {
        auto query = createQuery( websites_table_name, "SELECT DISTINCT key FROM %1.%2" );
        query->setDescription( QString("Delete all sub-domains in index") );
        query->setPagingSize(g_paging_size);
        query->start();
        //
        while( query->nextRow() )
        {
            const QByteArray key( query->getByteArrayColumn(0) );
            if( key == row_index_name )
            {
                // dump deprecated index rows
                website_rows_to_drop << key.data();
                continue;
            }

            const char *d = key.data();
            tld_info info;
            tld_result r( tld( d, &info ) );
            //
            if( r != TLD_RESULT_SUCCESS )
            {
                // If this is a bad entry, remove it too
                //
                website_rows_to_drop << key.data();
                continue;
            }

            const char *domain = d; // by default assume no sub-domain
            for(; d < info.f_tld; ++d)
            {
                if(*d == '.')
                {
                    domain = d + 1;
                }
            }

            if( domain == domain_name )
            {
                // This is the row in the websites table to drop:
                website_rows_to_drop << key.data();
            }
        }
        query->end();
    }

    // Delete each website name from the table.
    //
    for( auto website_name : website_rows_to_drop )
    {
        auto query = createQuery( websites_table_name, "DELETE FROM %1.%2 WHERE key = ?" );
        query->setDescription( QString("Drop website row %1.").arg(website_name) );
        query->bindByteArray( 0, website_name.toUtf8() );
        addQuery(query);
    }

    // Drop the domain in the domains table:
    //
    {
        auto query = createQuery( domains_table_name, "DELETE FROM %1.%2 WHERE key = ?" );
        query->setDescription( QString("Drop domain entry for domain %1.").arg(domain_name) );
        query->bindByteArray( 0, domain_name.toUtf8() );
        addQuery(query);
    }

    startQuery();
}


void snap_manager::onFinishedDeleteDomain( Query::pointer_t /*q*/ )
{
    f_domain_list->clearSelection();
    f_domain_model.init( f_session, "", "" );
    f_domain_model.doQuery();

    // mark empty
    f_domain_org_name = "";
    f_domain_name->setText("");
    f_domain_org_rules = "";
    f_domain_rules->setText("");

    // in effect we just lost our selection
    f_domain_name->setEnabled(false);
    f_domain_rules->setEnabled(false);
    f_domain_save->setEnabled(false);
    f_domain_cancel->setEnabled(false);
    f_domain_delete->setEnabled(false);

    f_tabs->setTabEnabled(TAB_WEBSITES, false);
}


void snap_manager::loadWebsites()
{
    if( f_website_list->currentIndex().isValid() )
    {
        f_current_website_index = f_website_list->currentIndex().row();
    }
    else
    {
        f_current_website_index = -1;
    }

    // we just checked to know whether the table existed so it cannot fail here
    f_website_model.init( f_session, "", "" );
    f_website_model.setDomainOrgName( f_domain_org_name );
    f_website_model.doQuery();
}


void snap_manager::onWebsitesLoaded()
{
    // at first some of the entries are disabled
    // until a select is made or New is clicked
    f_website_name->setEnabled(false);
    f_website_rules->setEnabled(false);
    f_website_save->setEnabled(false);
    f_website_cancel->setEnabled(false);
    f_website_delete->setEnabled(false);

    f_website_org_name = "";
    f_website_org_rules = "";
    f_website_name->setText("");
    f_website_rules->setText("");

    if( f_current_website_index != -1 )
    {
        const QModelIndex idx( f_website_model.index( f_current_website_index, 0 ) );
        if( idx.isValid() )
        {
            f_website_list->setCurrentIndex( idx );
        }
    }
}


#if 0
void snap_manager::onLoadWebsites( Query::pointer_t q )
{
    if( !getQueryResult(q) )
    {
        return;
    }

    const int mid_pos(f_domain_org_name.length() + 2);
    while( q->nextRow() )
    {
        const QString key( q->getStringColumn(0) );

        // the cell key is actually the row name which is the domain name
        // which is exactly what we want to display in our list!
        // (although it starts with the domain name and a double colon that
        // we want to remove)
        if( key.length() <= mid_pos)
        {
            // note that the length of the key is at least 4 additional
            // characters but at this point we don't make sure that the
            // key itself is fully correct (it should be)
            QMessageBox msg(QMessageBox::Warning, "Invalid Website Index", "Somehow we have found an invalid entry in the list of websites. It is suggested that you regenerate the index. Note that this index is not used by the Snap server itself.", QMessageBox::Ok, this);
            continue;
        }
        f_website_list->addItem( key.mid(mid_pos) );
    }

    if( q->nextPage( false ) )
    {
        return;
    }

    q->end();

    // at first some of the entries are disabled
    // until a select is made or New is clicked
    f_website_name->setEnabled(false);
    f_website_rules->setEnabled(false);
    f_website_save->setEnabled(false);
    f_website_cancel->setEnabled(false);
    f_website_delete->setEnabled(false);

    f_website_org_name = "";
    f_website_org_rules = "";
    f_website_name->setText("");
    f_website_rules->setText("");
}
#endif

void snap_manager::websiteWithSelection()
{
    // now there is a selection, everything is enabled
    f_website_name->setEnabled(true);
    f_website_rules->setEnabled(true);
    f_website_save->setEnabled(true);
    f_website_cancel->setEnabled(true);
    f_website_delete->setEnabled(true);
}

bool snap_manager::websiteChanged()
{
    // if something changed we want to warn the user before going further
    if(f_website_org_name != f_website_name->text()
    || f_website_org_rules != f_website_rules->toPlainText())
    {
        QMessageBox msg(QMessageBox::Critical, "Website Modified", "You made changes to this entry and did not Save it yet. Do you really want to continue? If you click Ok you will lose your changes.", QMessageBox::Ok | QMessageBox::Cancel, this);
        int choice = msg.exec();
        if(choice != QMessageBox::Ok)
        {
            return false;
        }
    }

    return true;
}

void snap_manager::on_websiteSelectionChanged( const QModelIndex & /*selected*/, const QModelIndex & /*deselected*/ )
{
    // check whether the current info was modified
    if(!websiteChanged())
    {
        // user canceled his action
        // TODO: we need to reset the item selection...
        return;
    }

    auto curidx( f_website_list->currentIndex() );
    if( !curidx.isValid() )
    {
        return;
    }

    if( f_website_model.rowCount() == 0 )
    {
        return;
    }

    const QString text                    ( f_website_model.data( curidx ).toString());
    const QString core_original_rules_name( snap::get_name( snap::name_t::SNAP_NAME_CORE_ORIGINAL_RULES));

    f_website_org_name = text;
    f_website_name->setText(f_website_org_name);

    QString const table_name(snap::get_name(snap::name_t::SNAP_NAME_WEBSITES));

    auto query = createQuery( table_name, "SELECT value FROM %1.%2 WHERE key = ? AND column1 = ?" );
    query->setDescription( QString("Get websites from domain [%1].").arg(f_website_org_name));
    size_t num = 0;
    query->bindByteArray( num++, f_website_org_name.toUtf8() );
    query->bindByteArray( num++, core_original_rules_name.toUtf8() );
    connect( query.get(), &Query::queryFinished, this, &snap_manager::onLoadWebsite );
    addQuery(query);
    startQuery();
}

void snap_manager::onLoadWebsite( Query::pointer_t q )
{
    if( !getQueryResult(q) )
    {
        return;
    }

    if( q->nextRow() )
    {
        f_website_org_rules = q->getVariantColumn(0).toString();
    }
    else
    {
        // this case happens after a delete (i.e. the row still exist but is empty)
        f_website_org_rules = "";
    }

    f_website_rules->setText(f_website_org_rules);

    websiteWithSelection();
}

void snap_manager::on_websiteNew_clicked()
{
    // check whether the current info was modified
    if(!websiteChanged())
    {
        // user canceled his action
        return;
    }

    f_website_list->clearSelection();

    f_website_org_name = ""; // not editing, this is new
    f_website_name->setText("");
    f_website_org_rules = "";
    f_website_rules->setText("");
    f_website_rules->setText(
            "main {\n"
            "  protocol = \"http\";\n"
            "  port = \"80\";\n"
            "};\n"
        );

    websiteWithSelection();
    f_website_delete->setEnabled(false);
}

void snap_manager::on_websiteSave_clicked()
{
    const QString name(f_website_name->text());
    if(name.isEmpty())
    {
        QMessageBox msg(QMessageBox::Critical, "Name Missing", "You cannot create a new website entry without giving the website a valid name.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    const QString rules(f_website_rules->toPlainText());
    if(rules.isEmpty())
    {
        QMessageBox msg(QMessageBox::Critical, "Rules Missing", "Adding a website requires you to enter at least one rule.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    if(name != f_website_org_name || rules != f_website_org_rules)
    {
        // first make sure the domain name corresponds to the domain
        // being edited; it is important for the following reasons:
        //
        // 1) we use that in the website index for this entry
        //
        // 2) the user could not find his website otherwise (plus it may
        //    not correspond to any other domain and would not make it
        //    in the right index)
        //
        bool valid(false);
        if(name.length() > f_domain_org_name.length())
        {
            QString domain(name.mid(name.length() - 1 - f_domain_org_name.length()));
            if(domain == "." + f_domain_org_name)
            {
                valid = true;
            }
        }
        else
        {
            // in this case it has to be exactly equal (i.e. no sub-domain)
            //
            valid = name == f_domain_org_name;
        }
        if(!valid)
        {
            QMessageBox msg(QMessageBox::Critical, "Invalid Domain Name", "The full domain name of a website must end with the exact domain name of the website you are editing.", QMessageBox::Ok, this);
            msg.exec();
            return;
        }

        // make sure the domain name is correct (i.e. at least "domain + TLD")
        tld_result r;
        tld_info info;
        // save in temporary buffer otherwise we'd lose the string pointers
        // in the tld_info structure
        QByteArray str(name.toUtf8());
        const char *d = str.data();
        r = tld(d, &info);
        if(r != TLD_RESULT_SUCCESS)
        {
            QMessageBox msg(QMessageBox::Critical, "Invalid TLD in Full Domain Name", "The TLD must be a known TLD. The tld() function could not determine the TLD of this full domain name. Please check the full domain name and make the necessary adjustments.", QMessageBox::Ok, this);
            msg.exec();
            return;
        }

        // full domain name is considered valid for now
        snap::snap_uri_rules website_rules;
        QByteArray compiled_rules;
        if(!website_rules.parse_website_rules(rules, compiled_rules))
        {
            QMessageBox msg(QMessageBox::Critical, "Invalid Website Rules", "An error was detected in your website rules: " + website_rules.errmsg(), QMessageBox::Ok, this);
            msg.exec();
            return;
        }

        QString const table_name              ( snap::get_name( snap::name_t::SNAP_NAME_WEBSITES));
        QString const core_rules_name         ( snap::get_name( snap::name_t::SNAP_NAME_CORE_RULES));
        QString const core_original_rules_name( snap::get_name( snap::name_t::SNAP_NAME_CORE_ORIGINAL_RULES));

        // Save the results into the websites table
        //(*table)[name][QString("core::original_rules")] = casswrapper::Value(rules);

        auto query = createQuery( table_name, "INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)" );
        query->setDescription( QString("Insert/update original core rules for %1").arg(name) );
        size_t num = 0;
        query->bindByteArray( num++, name.toUtf8() );
        query->bindByteArray( num++, core_original_rules_name.toUtf8() );
        query->bindByteArray( num++, rules.toUtf8() );
        addQuery(query);

        //(*table)[name][QString("core::rules")] = casswrapper::Value(compiled_rules);
        query = createQuery( table_name, "INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)" );
        query->setDescription( QString("Update core rules for %1").arg(name) );
        num = 0;
        query->bindByteArray( num++, name.toUtf8() );
        query->bindByteArray( num++, core_rules_name.toUtf8() );
        query->bindByteArray( num++, compiled_rules );
        connect( query.get(), &Query::queryFinished, this, &snap_manager::onFinishedSaveWebsite );
        addQuery(query);

        // all those are not valid anymore
        f_website_name->setEnabled(false);
        f_website_rules->setEnabled(false);
        f_website_save->setEnabled(false);
        f_website_cancel->setEnabled(false);
        f_website_delete->setEnabled(false);
    }

    startQuery();
}


void snap_manager::onFinishedSaveWebsite( Query::pointer_t /*q*/ )
{
    const QString name  ( f_website_name->text()         );
    const QString rules ( f_website_rules->toPlainText() );

#if 0
    // the data is now in the database, add it to the table too
    if(f_website_org_name == "")
    {
        f_website_list->addItem(name);

        // make sure we select that item too
        QList<QListWidgetItem *> items(f_website_list->findItems(name, Qt::MatchExactly));
        if(items.count() > 0)
        {
            f_website_list->setCurrentItem(items[0]);
        }
    }
#endif
    f_website_model.init( f_session, "", "" );
    f_website_model.setDomainOrgName( f_domain_org_name );
    f_website_model.doQuery();

    f_website_org_name  = name;
    f_website_org_rules = rules;

    // now the delete is available
    f_website_delete->setEnabled(true);
}


void snap_manager::on_websiteCancel_clicked()
{
    // check whether the current info was modified
    if(!websiteChanged())
    {
        // user canceled his action
        return;
    }

    // restore the original values
    f_website_name->setText(f_website_org_name);
    f_website_rules->setText(f_website_org_rules);
}

void snap_manager::on_websiteDelete_clicked()
{
    QString name(f_website_name->text());

    // verify that the user really wants to delete this website
    QMessageBox msg
        ( QMessageBox::Critical
        , "Delete Website"
        , "<font color=\"red\"><b>WARNING:</b></font> You are about to delete website \"" + name + "\". Are you sure you want to do that?"
        , QMessageBox::Ok | QMessageBox::Cancel
        , this
        );
    if( msg.exec() != QMessageBox::Ok )
    {
        return;
    }

    // all those are not valid anymore
    f_website_name->setEnabled(false);
    f_website_rules->setEnabled(false);
    f_website_save->setEnabled(false);
    f_website_cancel->setEnabled(false);
    f_website_delete->setEnabled(false);

    QString const table_name(snap::get_name(snap::name_t::SNAP_NAME_WEBSITES));

    auto query = createQuery( table_name, "DELETE FROM %1.%2 WHERE key = ?" );
    query->setDescription( QString("Delete website") );
    size_t num = 0;
    query->bindByteArray( num++, name.toUtf8() );
    connect( query.get(), &Query::queryFinished, this, &snap_manager::onDeleteWebsite );
    addQuery(query);

    startQuery();
}


void snap_manager::onDeleteWebsite( Query::pointer_t /*q*/ )
{
    //delete f_website_list->currentItem();
    f_website_model.init( f_session, "", "" );
    f_website_model.setDomainOrgName( f_domain_org_name );
    f_website_model.doQuery();

    // all those are not valid anymore
    f_website_name->setEnabled(false);
    f_website_rules->setEnabled(false);
    f_website_save->setEnabled(false);
    f_website_cancel->setEnabled(false);
    f_website_delete->setEnabled(false);

    // mark empty
    f_website_org_name = "";
    f_website_org_rules = "";
    f_website_name->setText("");
    f_website_rules->setText("");
}


bool snap_manager::sitesChanged()
{
#if 0
    // TODO: this always fails, so we need to fix this problem!
    // f_sites_org_parameter_* are never set.
    //
    // if something changed we want to warn the user before going further
    if(f_sites_org_parameter_name != f_sites_parameter_name->text()
    || f_sites_org_parameter_value != f_sites_parameter_value->text()
    || f_sites_org_parameter_type != f_sites_parameter_type->currentIndex())
    {
        QMessageBox msg(QMessageBox::Critical, "Site Parameter Modified", "You made changes to this parameter and did not Save it yet. Do you really want to continue? If you click Ok you will lose your changes.", QMessageBox::Ok | QMessageBox::Cancel, this);
        int choice = msg.exec();
        if(choice != QMessageBox::Ok)
        {
            return false;
        }
    }
#endif

    return true;
}

void snap_manager::loadSites()
{
    // we just checked to know whether the table existed so it cannot fail here
    // however the index table could be missing...

    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    QString const table_name(snap::get_name(snap::name_t::SNAP_NAME_SITES));
    f_sites_table_model.init( f_session, context_name, table_name );
    f_sites_table_model.doQuery();

    // at first some of the entries are disabled
    // until a select is made or New is clicked
    f_params_row_model.clear();
    f_sites_name->setText("");
    f_sites_org_name = "";
    f_sites_parameters->selectionModel()->clearSelection();
    f_sites_parameters->setEnabled(false);
    f_sites_parameter_name->setEnabled(false);
    f_sites_parameter_name->setText("");
    f_sites_parameter_value->setEnabled(false);
    f_sites_parameter_value->setText("");
    f_sites_parameter_type->setEnabled(false);
    f_sites_parameter_type->setCurrentIndex(1);
    f_sites_new->setEnabled(false);
    f_sites_save->setEnabled(false);
    f_sites_delete->setEnabled(false);
    f_sites_apply->setEnabled(false);
    f_sites_revert->setEnabled(false);
}

void snap_manager::on_sitesFilter_clicked()
{
    // make sure the user did not change something first
    if(sitesChanged())
    {
        // warning about the fact that the filter is currently ignored
        if(!f_sites_filter_string->text().isEmpty())
        {
            QMessageBox msg(QMessageBox::Critical, "Internal Error", "WARNING: The *index* for the sites table was not yet defined. The filter will therefore be ignored.", QMessageBox::Ok, this);
            msg.exec();
        }

        // user is okay with losing changes or did not make any
        // the following applies the filter (Apply button)
        loadSites();
    }
}

void snap_manager::onSitesListCurrentChanged( QModelIndex current, QModelIndex /*previous*/)
{
    // same site? if so, skip on it
    const QString text( f_sites_table_model.data(current).toString() );
    if(f_sites_org_name == text && !f_sites_org_name.isEmpty())
    {
        return;
    }

    // check whether the current info was modified
    if(!sitesChanged())
    {
        // user canceled his action
        // TODO: we need to reset the item selection...
        f_sites_list->selectionModel()->reset();
        for( int row = 0; row < f_sites_table_model.rowCount(); ++row )
        {
            QModelIndex idx( f_sites_table_model.index( row, 0 ) );
            if( f_sites_table_model.data(idx).toString() == f_sites_org_name )
            {
                f_sites_list->selectionModel()->select( idx, QItemSelectionModel::Select );
                break;
            }
        }
        return;
    }

    f_sites_org_name = text;
    f_sites_name->setText(f_sites_org_name);

    // IMPORTANT: note that f_sites_org_name changed to the item->text() value
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    QString const table_name(snap::get_name(snap::name_t::SNAP_NAME_SITES));
    f_params_row_model.clear();
    f_params_row_model.init( f_session, context_name, table_name );
    f_params_row_model.setRowKey( f_sites_org_name.toUtf8() );
    f_params_row_model.doQuery();

    auto hh( f_sites_parameters->horizontalHeader() );
    hh->setSectionResizeMode( 0, QHeaderView::ResizeToContents );
    hh->setSectionResizeMode( 1, QHeaderView::Stretch          );

    f_sites_parameters->setEnabled(true);
    f_sites_new->setEnabled(true);
}


void snap_manager::onSitesParamsCurrentChanged( QModelIndex current, QModelIndex previous )
{
    //snap::NOTUSED(current);
    snap::NOTUSED(previous);

    f_sites_delete->setEnabled( current.isValid() );
}


void snap_manager::onSitesParamsDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles )
{
    snap::NOTUSED(topLeft);
    snap::NOTUSED(bottomRight);
    snap::NOTUSED(roles);

    const auto modified( f_params_row_model.isModified() );
    f_sites_apply ->setEnabled( modified );
    f_sites_revert->setEnabled( modified );
}


void snap_manager::onSitesNewClicked( bool checked )
{
    snap::NOTUSED(checked);
    f_sites_parameter_name ->setEnabled( true );
    f_sites_parameter_value->setEnabled( true );
    f_sites_parameter_type ->setEnabled( true );
    f_sites_parameter_type->setCurrentIndex(1);
    f_sites_new->setEnabled(false);
    f_sites_save->setEnabled(true);
}


void snap_manager::onSitesSaveClicked( bool checked )
{
    snap::NOTUSED(checked);

    if( QMessageBox::question
            ( this
              , tr("About to save.")
              , tr("You are about to write a new entry to the database. This cannot be reverted.\nAre you sure you want to continue?")
              , QMessageBox::Yes | QMessageBox::No
              )
            == QMessageBox::Yes )
    {
        QString const table_name(snap::get_name(snap::name_t::SNAP_NAME_SITES));
        const auto& rowKey( f_params_row_model.rowKey() );
        snap::dbutils du( table_name, rowKey.data() );
        QByteArray result;
        du.set_column_value( rowKey.data(), result, f_sites_parameter_value->text() );
        //
        // TODO: implement parameter type...
        //
        auto q = createQuery( table_name, "INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)" );
        size_t num = 0;
        q->bindByteArray( num++, rowKey );
        q->bindByteArray( num++, f_sites_parameter_name->text().toUtf8() );
        q->bindByteArray( num++, result );
        q->start();
        q->end();

        f_sites_new->setEnabled(true);
        f_sites_save->setEnabled(false);
        f_sites_parameter_name->setText(QString());
        f_sites_parameter_value->setText(QString());

        f_params_row_model.clearModified();
        f_params_row_model.doQuery();			// Force a reload
    }
}


void snap_manager::onSitesDeleteClicked( bool clicked )
{
    snap::NOTUSED(clicked);
}


void snap_manager::onSitesApplyClicked( bool clicked )
{
    snap::NOTUSED(clicked);

    QString const table_name(snap::get_name(snap::name_t::SNAP_NAME_SITES));
    const auto& rowKey( f_params_row_model.rowKey() );

    for( const auto& pair : f_params_row_model.modifiedMap() )
    {
        if( !pair.second )
        {
            // Not modified, so continue.
            continue;
        }

        // Get the key and associated value
        //
        QModelIndex key  ( f_params_row_model.index( pair.first, 0 ) );
        QModelIndex value( f_params_row_model.index( pair.first, 1 ) );

        // Update the value in the database
        //
        auto q = createQuery( table_name, "INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)" );
        size_t num = 0;
        q->bindByteArray( num++, rowKey );
        q->bindByteArray( num++, f_params_row_model.data(key).toByteArray()   );
        q->bindByteArray( num++, f_params_row_model.data(value).toByteArray() );
        addQuery(q);
    }

    if( !f_queryQueue.empty() )
    {
        connect( f_queryQueue.back().get(), &Query::queryFinished,
                this, &snap_manager::onSitesParamSaveFinished );
    }
    startQuery();
}


void snap_manager::onSitesRevertClicked( bool clicked )
{
    snap::NOTUSED(clicked);

    if( QMessageBox::question
            ( this
              , tr("Warning!")
              , tr("You are about to lose all of your changes. Are you sure?")
              , QMessageBox::Yes | QMessageBox::No
              )
            == QMessageBox::Yes )
    {
        f_sites_apply->setEnabled(false);
        f_sites_revert->setEnabled(false);
        f_params_row_model.clearModified();
        f_params_row_model.doQuery();			// Force a reload
    }
}


void snap_manager::onSitesParamSaveFinished( Query::pointer_t q )
{
    snap::NOTUSED(q);
    f_params_row_model.clearModified();
    f_sites_apply->setEnabled(false);
    f_sites_revert->setEnabled(false);
}


void snap_manager::closeEvent(QCloseEvent *close_event)
{
    if(!domainChanged())
    {
        close_event->ignore();
        return;
    }
    if(!websiteChanged())
    {
        close_event->ignore();
        return;
    }
    if(!sitesChanged())
    {
        close_event->ignore();
        return;
    }

    close_event->accept();
}

void snap_manager::quit()
{
    if(!domainChanged())
    {
        return;
    }
    if(!websiteChanged())
    {
        return;
    }
    if(!sitesChanged())
    {
        return;
    }
    exit(0);
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName   ( "snap-manager"              );
    app.setApplicationVersion( SNAPWEBSITES_VERSION_STRING );
    app.setOrganizationDomain( "snapwebsites.org"          );
    app.setOrganizationName  ( "M2OSW"                     );

    snap_manager win;
    win.show();

    return app.exec();
}

// vim: ts=4 sw=4 et
