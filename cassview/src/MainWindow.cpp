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


// self
//
#include "MainWindow.h"

#include "AboutDialog.h"
#include "DisplayException.h"
#include "InputDialog.h"
#include "SettingsDialog.h"


// snapwebsites lib
//
#include <snapwebsites/dbutils.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// Qt lib
//
#include <QMessageBox>


// C++ lib
//
#include <iostream>


// last include
//
#include <snapdev/poison.h>





using namespace casswrapper;

MainWindow::MainWindow(QWidget *p)
    : QMainWindow(p)
    , f_row_context_menu(this)
    , f_col_context_menu(this)
{
    setupUi(this);

    QSettings const settings( this );
    restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
    restoreState   ( settings.value( "state"   , saveState()    ).toByteArray() );

    f_mainSplitter->restoreState( settings.value( "splitterState", f_mainSplitter->saveState() ).toByteArray() );
    f_mainSplitter->setStretchFactor( 0, 0 );
    f_mainSplitter->setStretchFactor( 1, 0 );
    f_mainSplitter->setStretchFactor( 2, 1 );

    f_context = settings.value("snap_keyspace", "snap_websites").toString();

    f_session = Session::create();
    connectCassandra();
    //
    f_contextModel = std::make_shared<KeyspaceModel>();
    f_contextModel->setCassandra( f_session, f_context );
    //
    fillTableList();

    f_tables->setCurrentIndex( 0 );
    f_rowsView->setContextMenuPolicy( Qt::CustomContextMenu );
    f_cellsView->setContextMenuPolicy( Qt::CustomContextMenu );
    f_cellsView->setWordWrap( true ); // this is true by default anyway, and it does not help when we have a column with a super long string...

    action_InsertRow->setEnabled( false );
    action_DeleteRows->setEnabled( false );
    action_InsertColumn->setEnabled( false );
    action_DeleteColumns->setEnabled( false );

    f_row_context_menu.addAction( action_InsertRow  );
    f_row_context_menu.addAction( action_DeleteRows );

    f_col_context_menu.addAction( action_InsertColumn  );
    f_col_context_menu.addAction( action_DeleteColumns );

    connect( f_tables
           , static_cast<void(QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged)  // See docs. This is overloaded.
           , this
           , &MainWindow::onTablesCurrentIndexChanged
    );

    connect( f_filterEdit,                  &QLineEdit::returnPressed,              this, &MainWindow::on_f_applyFilter_clicked    );
    connect( qApp,                          &QApplication::aboutToQuit,             this, &MainWindow::onAboutToQuit               );
}


MainWindow::~MainWindow()
{
}


namespace
{
    void displayError( const std::exception& except, const QString& caption, const QString& message )
    {
        DisplayException de( except.what(), caption, message );
        de.displayError();
    }

    void displayError( const QString& what, const QString& caption, const QString& message )
    {
        DisplayException de( what.toUtf8().data(), caption, message );
        de.displayError();
    }
}


void MainWindow::connectCassandra()
{
    QSettings const settings;
    QString const host( settings.value( "cassandra_host" ).toString() );
    int     const port( settings.value( "cassandra_port" ).toInt()    );
    try
    {
        f_session->connect( host, port, settings.value( "use_ssl", true ).toBool() );
        //
        //qDebug() << "Working on Cassandra Cluster Named"    << f_session->clusterName();
        //qDebug() << "Working on Cassandra Protocol Version" << f_session->protocolVersion();

        QString const hostname( tr("%1:%2").arg(host).arg(port) );
        setWindowTitle( tr("Cassandra View [%1]").arg(hostname) );
        f_connectionBtn->setText( hostname );
    }
    catch( std::exception const & except )
    {
        displayError( except
                    , tr("Connection Error")
                    , tr("Cannot connect to Cassandra server at '%1:%2'! Please check your connection information in the settings.")
                      .arg(host)
                      .arg(port)
                    );
        on_action_Settings_triggered();
    }
}


void MainWindow::onAboutToQuit()
{
    saveValue();

    QSettings settings( this );
    settings.setValue( "geometry",      saveGeometry()                );
    settings.setValue( "state",         saveState()                   );
    settings.setValue( "splitterState", f_mainSplitter->saveState()   );
}


void MainWindow::fillTableList()
{
    f_tableModel = std::make_shared<TableModel>();
    f_rowModel   = std::make_shared<RowModel>();

    f_tables->setModel    ( f_contextModel.get() );
    f_rowsView->setModel  ( f_tableModel.get()   );
    f_cellsView->setModel ( f_rowModel.get()     );

    f_tableModel->setSortModel( true );

    connect( f_tableModel.get(), &TableModel::exceptionCaught, this, &MainWindow::onExceptionCaught         );
    connect( f_tableModel.get(), &TableModel::queryFinished,   this, &MainWindow::onTableModelQueryFinished );
    connect( f_rowModel.get(),   &RowModel::exceptionCaught,   this, &MainWindow::onExceptionCaught         );
    connect( f_rowModel.get(),   &RowModel::modelReset,        this, &MainWindow::onCellsModelReset         );
    connect( f_rowModel.get(),   &RowModel::queryFinished,     this, &MainWindow::onRowModelQueryFinished   );

    connect( f_rowsView,                    &QListView::customContextMenuRequested, this, &MainWindow::onShowRowsContextMenu       );
    connect( f_rowsView->selectionModel(),  &QItemSelectionModel::currentChanged,   this, &MainWindow::onRowsCurrentChanged        );
    connect( f_cellsView,                   &QListView::customContextMenuRequested, this, &MainWindow::onShowCellsContextMenu      );
    connect( f_cellsView->selectionModel(), &QItemSelectionModel::currentChanged,   this, &MainWindow::onCellsCurrentChanged       );

    auto doc( f_valueEdit->document() );
    doc->clear();
    f_valueGroup->setTitle( tr("Value") );

    f_contextEdit->setText( f_context );
}


void MainWindow::onShowRowsContextMenu( const QPoint& mouse_pos )
{
    QPoint global_pos( f_rowsView->mapToGlobal(mouse_pos) );
    f_row_context_menu.popup(global_pos);
}


void MainWindow::onShowCellsContextMenu( const QPoint& mouse_pos )
{
    if( !f_rowsView->selectionModel()->hasSelection() )
    {
        // Do nothing, as something must be selected in the rows!
        return;
    }

    QPoint global_pos( f_cellsView->mapToGlobal(mouse_pos) );
    f_col_context_menu.popup(global_pos);
}


void MainWindow::onCellsModelReset()
{
    //f_cellsView->resizeColumnsToContents();
    setCursor( Qt::ArrowCursor );
}


void MainWindow::onTableModelQueryFinished()
{
    setCursor( Qt::ArrowCursor );
}


void MainWindow::onRowModelQueryFinished()
{
    setCursor( Qt::ArrowCursor );
}


void MainWindow::on_action_Settings_triggered()
{
    saveValue();

    try
    {
        SettingsDialog dlg(this);
        if( dlg.exec() == QDialog::Accepted )
        {
            connectCassandra();
            fillTableList();
        }
    }
    catch( const std::exception& except )
    {
        displayError( except
                      , tr("Connection Error")
                      , tr("Error connecting to the server!")
                      );
    }
}


void MainWindow::onTablesCurrentIndexChanged(QString const & table_name)
{
    saveValue();

    fillTableList();

    if( table_name.isEmpty() )
    {
        return;
    }

    QString filter_text( f_filterEdit->text( ) );
    QRegExp filter( filter_text );
    if(!filter.isValid() && !filter_text.isEmpty())
    {
        // reset the filter
        filter = QRegExp();

        QMessageBox::warning( QApplication::activeWindow()
            , tr("Warning!")
            , tr("Warning!\nThe filter regular expression is not valid. It will not be used.")
            , QMessageBox::Ok
            );
    }

    try
    {
        f_tableModel->init
                ( f_session
                , f_context
                , table_name
                , filter
                );
        f_tableModel->doQuery();
    }
    catch( const std::exception& except )
    {
        displayError( except
                    , tr("Connection Error")
                    , tr("Error connecting to the server!")
                    );
    }

    action_InsertRow->setEnabled( true );
    action_DeleteRows->setEnabled( true );

    setCursor( Qt::WaitCursor );
}


void MainWindow::on_f_applyFilter_clicked()
{
    QString const table_name( f_tables->currentText( ) );
    onTablesCurrentIndexChanged( table_name );
    setCursor( Qt::WaitCursor );
}


void MainWindow::on_f_clearFilter_clicked()
{
    f_filterEdit->clear();
    QString const table_name( f_tables->currentText( ) );
    onTablesCurrentIndexChanged( table_name );
    setCursor( Qt::WaitCursor );
}


void MainWindow::on_f_refreshView_clicked()
{
    QString const table_name( f_tables->currentText( ) );
    onTablesCurrentIndexChanged( table_name );
    setCursor( Qt::WaitCursor );
}


void MainWindow::onExceptionCaught( const QString & what, const QString & message )
{
    displayError( what
                  , tr("Exception Caught!")
                  , message
                  );
    setCursor( Qt::ArrowCursor );
}


void MainWindow::saveValue()
{
    auto selected_cells( f_cellsView->selectionModel()->selectedRows() );
    if( selected_cells.size() == 1 )
    {
        saveValue( selected_cells[0] );
    }
}


void MainWindow::saveValue( const QModelIndex& index )
{
    try
    {
        auto doc( f_valueEdit->document() );
        if( doc->isModified() && index.isValid() )
        {
            int response = QMessageBox::Yes;
            QSettings settings;
            if( settings.value("prompt_before_commit",true).toBool() )
            {
                response = QMessageBox::question
                        ( this
                          , tr("Data has been changed!")
                          , tr("Are you sure you want to save the changes?")
                          , QMessageBox::Yes | QMessageBox::No
                          );
            }
            if( response == QMessageBox::Yes )
            {
                const QByteArray column_key(f_rowModel->data(index, Qt::UserRole).toByteArray());
                QByteArray value;
                snap::dbutils du( f_rowModel->tableName(), QString::fromUtf8(f_rowModel->rowKey().data()) );
                du.set_column_value( column_key, value, doc->toPlainText().toUtf8() );

                const QString q_str(
                            QString("UPDATE %1.%2 SET value = ? WHERE key = ? AND column1 = ?")
                            .arg(f_rowModel->keyspaceName())
                            .arg(f_rowModel->tableName())
                            );
                auto query = Query::create( f_session );
                query->query( q_str, 3 );
                int num = 0;
                query->bindByteArray( num++, value                );
                query->bindByteArray( num++, f_rowModel->rowKey() );
                query->bindByteArray( num++, column_key           );
                query->start();
                query->end();
            }
        }
    }
    catch( const std::exception& except )
    {
        displayError( except
                    , tr("Database Error")
                    , tr("Cannot write value data to server!")
                    );
    }
}


void MainWindow::onRowsCurrentChanged( const QModelIndex & current, const QModelIndex & /*previous*/ )
{
    try
    {
        saveValue();

        auto doc( f_valueEdit->document() );
        doc->clear();
        f_valueGroup->setTitle( tr("Value") );
        f_rowModel->clear();

        if( current.isValid() )
        {
            const QByteArray row_key( f_tableModel->data(current,Qt::UserRole).toByteArray() );

            f_rowModel->init
                ( f_session
                  , f_tableModel->keyspaceName()
                  , f_tableModel->tableName()
                );
            f_rowModel->setRowKey( row_key );
            f_rowModel->doQuery();

            action_InsertColumn->setEnabled( true );
            action_DeleteColumns->setEnabled( true );
            setCursor( Qt::WaitCursor );
        }
    }
    catch( const std::exception& except )
    {
        displayError( except
                    , tr("Connection Error")
                    , tr("Error connecting to the server!")
                    );
    }
}


void MainWindow::onCellsCurrentChanged( const QModelIndex & current, const QModelIndex & previous )
{
    try
    {
        if( previous.isValid() )
        {
            saveValue( previous );
        }

        auto doc( f_valueEdit->document() );
        doc->clear();

        if( current.isValid() )
        {
            const QByteArray column_key(f_rowModel->data(current, Qt::UserRole).toByteArray());
            const QString q_str(
                    QString("SELECT value FROM %1.%2 WHERE key = ? AND column1 = ?")
                    .arg(f_rowModel->keyspaceName())
                    .arg(f_rowModel->tableName())
                    );
            auto query = Query::create( f_session );
            query->query( q_str );
            int num = 0;
            query->bindByteArray( num++, f_rowModel->rowKey() );
            query->bindByteArray( num++, column_key );
            query->start();

            snap::dbutils du( f_rowModel->tableName(), QString::fromUtf8(f_rowModel->rowKey().data()) );
            const QString value = du.get_column_value( column_key, query->getByteArrayColumn(0) );

            doc->setPlainText( value );
            doc->setModified( false );

            f_valueGroup->setTitle( tr("Value [%1]").arg(du.get_column_type_name(column_key)) );
        }
    }
    catch( const std::exception& except )
    {
        displayError( except
                , tr("Connection Error")
                , tr("Error connecting to the server!")
                );
    }
}


void MainWindow::on_action_About_triggered()
{
    AboutDialog dlg( this );
    dlg.exec();
}

void MainWindow::on_action_AboutQt_triggered()
{
    QMessageBox::aboutQt( this );
}


void MainWindow::onSectionClicked( int /*section*/ )
{
    //f_cellsView->resizeColumnToContents( section );
}


void MainWindow::on_action_InsertRow_triggered()
{
    QMessageBox::critical( this, tr("Notice"), tr("Row insertion has been disabled for now.") );
}


void MainWindow::on_action_DeleteRows_triggered()
{
    QMessageBox::critical( this, tr("Notice"), tr("Row deletion has been disabled for now.") );
}


void MainWindow::on_action_InsertColumn_triggered()
{
    //QMessageBox::critical( this, tr("Notice"), tr("Column insertion has been disabled for now.") );
    f_rowModel->insertRows( 0, 1 );
}


void MainWindow::on_action_DeleteColumns_triggered()
{
    try
    {
        const QModelIndexList selectedItems( f_cellsView->selectionModel()->selectedRows() );
        if( !selectedItems.isEmpty() )
        {
            QMessageBox::StandardButton result
                    = QMessageBox::warning( QApplication::activeWindow()
                    , tr("Warning!")
                    , tr("Warning!\nYou are about to remove %1 columns from row '%2', in table '%3'.\nThis cannot be undone!")
                      .arg(selectedItems.size())
                      .arg(QString::fromUtf8(f_rowModel->rowKey().data()))
                      .arg(f_rowModel->tableName())
                    , QMessageBox::Ok | QMessageBox::Cancel
                    );
            if( result == QMessageBox::Ok )
            {
                f_rowModel->removeRows( selectedItems[0].row(), selectedItems.size() );
            }
        }
    }
    catch( const std::exception& except )
    {
        displayError( except
                    , tr("Connection Error")
                    , tr("Error connecting to the server!")
                    );
    }
}


void MainWindow::on_f_connectionBtn_clicked()
{
    on_action_Settings_triggered();
}


// vim: ts=4 sw=4 et
