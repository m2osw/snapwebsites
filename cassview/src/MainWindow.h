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

#include "CassandraModel.h"
#include "KeyspaceModel.h"
#include "TableModel.h"
#include "RowModel.h"

#include <casswrapper/session.h>
#include <QtGui>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include "ui_MainWindow.h"
class MainWindow
    : public QMainWindow
    , Ui::MainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    virtual ~MainWindow() override;

private slots:
    void onShowRowsContextMenu( const QPoint& pos );
    void onShowCellsContextMenu( const QPoint& pos );
    void onCellsModelReset();
    void onAboutToQuit();
    void onTablesCurrentIndexChanged(const QString &table_name);
    void onTableModelQueryFinished();
    void onRowModelQueryFinished();
    void onRowsCurrentChanged( const QModelIndex & current, const QModelIndex & /*previous*/ );
    void onCellsCurrentChanged( const QModelIndex & current, const QModelIndex & /*previous*/ );
    void onSectionClicked( int section );
    void onExceptionCaught( const QString & what, const QString & message );
    void on_action_Settings_triggered();
    void on_action_About_triggered();
    void on_action_AboutQt_triggered();
    void on_action_InsertRow_triggered();
    void on_action_DeleteRows_triggered();
    void on_action_InsertColumn_triggered();
    void on_action_DeleteColumns_triggered();
    void on_f_connectionBtn_clicked();
    void on_f_applyFilter_clicked();
    void on_f_clearFilter_clicked();
    void on_f_refreshView_clicked();

private:
    typedef casswrapper::Session::pointer_t cassandra_t;
    typedef std::shared_ptr<KeyspaceModel>  keyspace_model_t;
    typedef std::shared_ptr<TableModel>     table_model_t;
    typedef std::shared_ptr<RowModel>       row_model_t;

    void connectCassandra ();
    void fillTableList    ();
    void saveValue        ();
    void saveValue        ( const QModelIndex &index );

    cassandra_t      f_session = cassandra_t();
    keyspace_model_t f_contextModel = keyspace_model_t();
    table_model_t    f_tableModel = table_model_t();
    row_model_t      f_rowModel = row_model_t();
    QString          f_context = QString();
    QMenu            f_row_context_menu;
    QMenu            f_col_context_menu;
};
#pragma GCC diagnostic pop


// vim: ts=4 sw=4 et
