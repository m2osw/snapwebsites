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
#include "RowModel.h"


// snapwebsites
//
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_exception.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// Qt lib
//
#include <QtCore>


// last include
//
#include <snapdev/poison.h>




using namespace casswrapper;


RowModel::RowModel()
{
}


void RowModel::doQuery()
{
    f_dbutils = std::make_shared<snap::dbutils>( f_tableName, QString::fromUtf8(f_rowKey.data()) );

    auto q = Query::create(f_session);
    q->query(
        QString("SELECT column1 FROM %1.%2 WHERE key = ?")
            .arg(f_keyspaceName)
            .arg(f_tableName)
        , 1
        );
    q->setPagingSize( 10 );
    q->bindByteArray( 0, f_rowKey );

    query_model::doQuery( q );
}


bool RowModel::fetchFilter( const QByteArray& key )
{
    QString const col_key( f_dbutils->get_column_name( key ) );

    if( !f_filter.isEmpty() )
    {
        if( f_filter.indexIn( col_key ) == -1 )
        {
            return false;
        }
    }
    //
    return true;
}


Qt::ItemFlags RowModel::flags( const QModelIndex & idx ) const
{
    // Editing is disabled for now.
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if( idx.column() == 0 )
    {
        f |= Qt::ItemIsEditable;
    }
    return f;
#if 0
    snap::NOT_USED(idx);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
#endif
}


QVariant RowModel::data( QModelIndex const & idx, int role ) const
{
    if( role == Qt::UserRole )
    {
        return query_model::data( idx, role );
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    if( idx.column() < 0 || idx.column() > 1 )
    {
        Q_ASSERT(false);
        return QVariant();
    }

    auto const column_name( *(f_rows.begin() + idx.row()) );
    return f_dbutils->get_column_name( column_name );
}


bool RowModel::setData( const QModelIndex & index, const QVariant & new_col_variant, int role )
{
    if( role != Qt::EditRole )
    {
        return false;
    }

    try
    {
        QByteArray value;
        QByteArray new_col_key;
        f_dbutils->set_column_name( new_col_key, new_col_variant.toString() );

        // First, get the value from the current record.
        //
        {
            const QString q_str(
                    QString("SELECT value FROM %1.%2 WHERE key = ? AND column1 = ?")
                    .arg(f_keyspaceName)
                    .arg(f_tableName)
                    );
            auto q = Query::create( f_session );
            q->query( q_str );
            int num = 0;
            q->bindByteArray( num++, f_rowKey );
            q->bindByteArray( num++, f_rows[index.row()] );
            q->start();
            if( q->nextRow() )
            {
                value = q->getByteArrayColumn(0);
            }
            q->end();
        }

        // Next, insert the new value with the new column key. This creates a new record.
        {
            // We must convert the value of the cell from the old format, whatever it is, to the
            // format of the new column key.
            //
            QByteArray new_value;
            try
            {
                QString str_val( f_dbutils->get_column_value( f_rows[index.row()], value ) );
                f_dbutils->set_column_value( new_col_key, new_value, str_val );
            }
            catch( snap::snap_exception const & )
            {
                // It must have not liked the conversion...
                //
                new_value = f_dbutils->get_column_value( f_rows[index.row()], value ).toUtf8();
            }
            catch( ... )
            {
                throw;
            }

            // Now do the query:
            //
            auto q = Query::create( f_session );
            q->query(
                    QString("INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)")
                    .arg(f_keyspaceName)
                    .arg(f_tableName)
                   );
            int num = 0;
            q->bindByteArray( num++, f_rowKey    );
            q->bindByteArray( num++, new_col_key );
            q->bindByteArray( num++, new_value   );
            q->start();
            q->end();
        }

        // Remove the old column key record.
        {
            auto q = Query::create( f_session );
            q->query(
                    QString("DELETE FROM %1.%2 WHERE key = ? AND column1 = ?")
                    .arg(f_keyspaceName)
                    .arg(f_tableName)
                   );
            int num = 0;
            q->bindByteArray( num++, f_rowKey            );
            q->bindByteArray( num++, f_rows[index.row()] );
            q->start();
            q->end();
        }

        // Change the row value
        //
        f_rows[index.row()] = new_col_key;

        Q_EMIT dataChanged( index, index );

        return true;
    }
    catch( std::exception const & except )
    {
        displayError( except, tr("Cannot write data to database.") );
        return false;
    }

    return false;
}


bool RowModel::insertRows ( int row, int count, const QModelIndex & parent_index )
{
    try
    {
        beginInsertRows( parent_index, row, row+count );
        for( int i = 0; i < count; ++i )
        {
            const QByteArray newcol( QString("New column %1").arg(i).toUtf8() );
            f_rows.insert( f_rows.begin() + (row+i), newcol);

            // TODO: this might be pretty slow. I need to utilize the "prepared query" API.
            //
            auto q = Query::create( f_session );
            q->query(
                        QString("INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)")
                        .arg(f_keyspaceName)
                        .arg(f_tableName)
                        );
            int num = 0;
            q->bindByteArray( num++, f_rowKey    );
            q->bindByteArray( num++, newcol      );
            q->bindByteArray( num++, "New Value" );
            q->start();
            q->end();
        }
        endInsertRows();
    }
    catch( std::exception const & except )
    {
        displayError( except, tr("Cannot insert new rows!") );
        return false;
    }

    return true;
}

bool RowModel::removeRows( int row, int count, const QModelIndex & )
{
    // Make a list of the keys we will drop
    //
    QList<QByteArray> key_list;
    for( int idx = 0; idx < count; ++idx )
    {
        const QByteArray key(f_rows[idx + row]);
        key_list << key;
    }

    try
    {
        // Drop each key
        //
        for( auto key : key_list )
        {
            // TODO: this might be pretty slow. I need to utilize the "prepared query" API.
            //
            auto q = Query::create( f_session );
            q->query(
                        QString("DELETE FROM %1.%2 WHERE key = ? AND column1 = ?")
                        .arg(f_keyspaceName)
                        .arg(f_tableName)
                        , 2
                        );
            q->bindByteArray( 0, f_rowKey );
            q->bindByteArray( 1, key      );
            q->start();
            q->end();
        }

        // Remove the columns from the model
        //
        beginRemoveRows( QModelIndex(), row, row+count );
        f_rows.erase( f_rows.begin()+row, f_rows.begin()+row+count );
        endRemoveRows();
    }
    catch( std::exception const & except )
    {
        displayError( except, tr("Cannot write data to database.") );
        return false;
    }

    return true;
}


// vim: ts=4 sw=4 et
