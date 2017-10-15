//===============================================================================
// Copyright (c) 2011-2017 by Made to Order Software Corporation
// 
// All Rights Reserved.
// 
// The source code in this file ("Source Code") is provided by Made to Order Software Corporation
// to you under the terms of the GNU General Public License, version 2.0
// ("GPL").  Terms of the GPL can be found in doc/GPL-license.txt in this distribution.
// 
// By copying, modifying or distributing this software, you acknowledge
// that you have read and understood your obligations described above,
// and agree to abide by those obligations.
// 
// ALL SOURCE CODE IN THIS DISTRIBUTION IS PROVIDED "AS IS." THE AUTHOR MAKES NO
// WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
// COMPLETENESS OR PERFORMANCE.
//===============================================================================

#include "TableModel.h"
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>

#include <QSettings>
#include <QVariant>

#include <iostream>
#include <exception>

#include <snapwebsites/poison.h>

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


void TableModel::fetchCustomData( Query::pointer_t q )
{
    const QByteArray value(q->getByteArrayColumn(0));
    f_sortMap[f_dbutils->get_row_name(value)] = value;
}


// vim: ts=4 sw=4 et
