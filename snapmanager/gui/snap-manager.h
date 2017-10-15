// Snap Manager -- Cassandra manager for Snap! Servers
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

#include "snap-manager-initialize-website.h"
#include "snap-manager-createcontext.h"

#include "DomainModel.h"
#include "RowModel.h"
#include "TableModel.h"
#include "WebsiteModel.h"

#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_string_list.h>

#include <casswrapper/query.h>
#include <casswrapper/session.h>

#include <QMap>
#include <QMainWindow>
#include <QMessageBox>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTcpSocket>

#include <queue>

#include "ui_snap-manager-mainwindow.h"

// We do not use namespaces because that doesn't work too well with
// Qt tools such as moc.

class snap_manager : public QMainWindow, public Ui_MainWindow
{
    Q_OBJECT

public:
    snap_manager(QWidget *parent = NULL);
    virtual ~snap_manager();

    void cassandraDisconnectButton_clicked();

private slots:
    void about();
    void help();
    void decode_utf8();
    void snapTest();
    void snapStats();
    void on_f_cassandraConnectButton_clicked();
    void on_f_cassandraDisconnectButton_clicked();
    void initialize_website();
    void OnAboutToQuit();
    void on_domainFilter_clicked();
    void on_domainNew_clicked();
    void on_domainSelectionChanged( const QModelIndex & selected, const QModelIndex & deselected );
    void on_domainSave_clicked();
    void on_domainCancel_clicked();
    void on_domainDelete_clicked();
    void on_websiteNew_clicked();
    void on_websiteSelectionChanged( const QModelIndex& selected, const QModelIndex& deselected );
    void on_websiteSave_clicked();
    void on_websiteCancel_clicked();
    void on_websiteDelete_clicked();
    void on_sitesFilter_clicked();
    void quit();

    void onCurrentTabChanged         ( int index );

    void create_context              ( int replication_factor, int strategy, snap::snap_string_list const & data_centers );
    void onQueryFinished             ( casswrapper::Query::pointer_t q );
    void onContextCreated            ( casswrapper::Query::pointer_t q );
    void onFinishedSaveDomain        ( casswrapper::Query::pointer_t q );
    void onFinishedDeleteDomain      ( casswrapper::Query::pointer_t q );
    void onLoadWebsite               ( casswrapper::Query::pointer_t q );
    void onFinishedSaveWebsite       ( casswrapper::Query::pointer_t q );
    void onDeleteWebsite             ( casswrapper::Query::pointer_t q );

    void onSitesListCurrentChanged   ( QModelIndex current, QModelIndex previous );
    void onSitesParamsCurrentChanged ( QModelIndex current, QModelIndex previous );
    void onSitesParamsDataChanged    ( const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles );
    void onSitesParamSaveFinished    ( casswrapper::Query::pointer_t q );

    void onSitesNewClicked           ( bool checked );
    void onSitesSaveClicked          ( bool checked );
    void onSitesDeleteClicked        ( bool checked );
    void onSitesApplyClicked         ( bool checked );
    void onSitesRevertClicked        ( bool checked );

    void onDomainsLoaded();
    void onWebsitesLoaded();

private:
    enum tabs
    {
        TAB_CONNECTIONS = 0,
        TAB_DOMAINS     = 1,
        TAB_WEBSITES    = 2,
        TAB_SITES       = 3
    };

    QPointer<QWidget>               f_about;
    QPointer<QWidget>               f_help;
    QPointer<snap_manager_createcontext> f_createcontext_window;
    QPointer<snap_manager_initialize_website> f_initialize_website_window;
    QPointer<QWidget>               f_decode_utf8;
    QPointer<QTabWidget>            f_tabs;
    QPointer<QWidget>               f_tab_connect;
    QPointer<QWidget>               f_tab_domain;

    QPointer<QAction>               f_initialize_website;

    // snap domains
    QString                         f_domain_org_name; // the original name (in case user changes it)
    QString                         f_domain_org_rules; // the original rules (to check Cancel properly)
    QPointer<QPushButton>           f_domain_filter;
    QPointer<QLineEdit>             f_domain_filter_string;
    QPointer<QListView>             f_domain_list;
    QPointer<QLineEdit>             f_domain_name;
    QPointer<QTextEdit>             f_domain_rules;
    QPointer<QPushButton>           f_domain_new;
    QPointer<QPushButton>           f_domain_save;
    QPointer<QPushButton>           f_domain_cancel;
    QPointer<QPushButton>           f_domain_delete;

    // snap websites
    QString                         f_website_org_name; // the original name (in case user changes it)
    QString                         f_website_org_rules; // the original rules (to check Cancel properly)
    QPointer<QListView>             f_website_list;
    QPointer<QLineEdit>             f_website_name;
    QPointer<QTextEdit>             f_website_rules;
    QPointer<QPushButton>           f_website_new;
    QPointer<QPushButton>           f_website_save;
    QPointer<QPushButton>           f_website_cancel;
    QPointer<QPushButton>           f_website_delete;

    // snap site parameters
    QString                         f_sites_org_name;
    QPointer<QPushButton>           f_sites_filter;
    QPointer<QLineEdit>             f_sites_filter_string;
    QPointer<QListView>             f_sites_list;
    QPointer<QLineEdit>             f_sites_name;
    QPointer<QTableView>            f_sites_parameters;
    QPointer<QLineEdit>             f_sites_parameter_name;
    QString                         f_sites_org_parameter_value;
    QPointer<QLineEdit>             f_sites_parameter_value;
    int32_t                         f_sites_org_parameter_type = 0;
    QPointer<QComboBox>             f_sites_parameter_type;
    QPointer<QPushButton>           f_sites_new;
    QPointer<QPushButton>           f_sites_save;
    QPointer<QPushButton>           f_sites_delete;
    QPointer<QPushButton>           f_sites_apply;
    QPointer<QPushButton>           f_sites_revert;

    DomainModel                     f_domain_model;
    RowModel						f_params_row_model;
    TableModel						f_sites_table_model;
    WebsiteModel                    f_website_model;
    int                             f_current_domain_index;
    int                             f_current_website_index;

    // snap server
    QString                         f_snap_host;
    int32_t                         f_snap_port = 0;

    // cassandra data
    QString                                     f_cassandra_host;
    int32_t                                     f_cassandra_port = 0;
    casswrapper::Session::pointer_t   f_session;
    QStringList                                 f_domains_to_check;

    std::queue<casswrapper::Query::pointer_t>	f_queryQueue;

    void loadDomains          ();
    void domainWithSelection  ();
    bool domainChanged        ();
    void saveDomain           ();

    void loadWebsites         ();
    void websiteWithSelection ();
    bool websiteChanged       ();

    void loadSites            ();
    bool sitesChanged         ();

    virtual void closeEvent(QCloseEvent *event);

    casswrapper::Query::pointer_t createQuery
        ( const QString& q_str
        );
    casswrapper::Query::pointer_t createQuery
        ( const QString& table_name
        , const QString& q_str
        );
    void addQuery( casswrapper::Query::pointer_t q );
    bool getQueryResult( casswrapper::Query::pointer_t q );
    void startQuery();

    void create_table(QString const & table_name, QString const & comment);
    void context_is_valid();
};



// vim: ts=4 sw=4 et
