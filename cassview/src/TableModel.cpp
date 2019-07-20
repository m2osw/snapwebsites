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
#include "TableModel.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// Qt lib
//
#include <QSettings>
#include <QVariant>


// C++ lib
//
#include <iostream>
#include <exception>


// last include
//
#include <snapdev/poison.h>




using namespace casswrapper;


TableModel::TableModel()
{
}


void TableModel::doQuery()
{
    f_dbutils = std::make_shared<snap::dbutils>( f_tableName, "" );

    auto q = Query::create(f_session);
    q->query(
        QString("SELECT DISTINCT key FROM %1.%2")
            .arg(f_keyspaceName)
            .arg(f_tableName)
            );
    q->setPagingSize( 10 );

    query_model::doQuery( q );
}


bool TableModel::fetchFilter( const QByteArray& key )
{
    QString const row_name( f_dbutils->get_row_name( key ) );

    if( !f_filter.isEmpty() )
    {
        if( f_filter.indexIn( row_name ) == -1 )
        {
            return false;
        }
    }
    //
    return true;
}


QVariant TableModel::data( QModelIndex const & idx, int role ) const
{
    if( role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::UserRole )
    {
        return QVariant();
    }

    if( static_cast<int>(f_sortMap.size()) <= idx.row() )
    {
        return QVariant();
    }

    if( f_sortModel )
    {
        auto iter = f_sortMap.begin();
        for( int i = 0; i < idx.row(); ++i) iter++;
        if( role == Qt::UserRole )
        {
            return iter->second;
        }
        else
        {
            return iter->first;
        }
    }
    else
    {
        if( role == Qt::UserRole )
        {
            return query_model::data( idx, role );
        }
        else
        {
            return f_dbutils->get_row_name( f_rows[idx.row()] );
        }
    }
}


void TableModel::fetchCustomData( Query::pointer_t q )
{
    if( !f_sortModel )
    {
        return;
    }

    const QByteArray value(q->getByteArrayColumn(0));
    f_sortMap[f_dbutils->get_row_name(value)] = value;
}


// vim: ts=4 sw=4 et
