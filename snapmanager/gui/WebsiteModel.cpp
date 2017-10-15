//===============================================================================
// Copyright (c) 2015-2017 by Made to Order Software Corporation
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

#include "WebsiteModel.h"
#include <snapwebsites/snapwebsites.h>

#include <libtld/tld.h>

#include <QMessageBox>
#include <QSettings>
#include <QVariant>

#include <iostream>
#include <exception>

#include <snapwebsites/poison.h>

using namespace casswrapper;


WebsiteModel::WebsiteModel()
{
}


void WebsiteModel::doQuery()
{
    f_dbutils = std::make_shared<snap::dbutils>( f_tableName, "" );

    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    QString const table_name(snap::get_name(snap::name_t::SNAP_NAME_WEBSITES));

    auto q = Query::create(f_session);
    q->query(
        QString("SELECT DISTINCT key FROM %1.%2")
            .arg(context_name)
            .arg(table_name)
        );
    q->setPagingSize( 100 );

    query_model::doQuery( q );
}


bool WebsiteModel::fetchFilter( const QByteArray& key )
{
    if( !query_model::fetchFilter( key ) )
    {
        return false;
    }

    QString const row_index_name(snap::get_name(snap::name_t::SNAP_NAME_INDEX));
    if( key == row_index_name )
    {
        // Ignore *index* entries
        return false;
    }

    const char *d = key.data();
    tld_info info;
    tld_result r( tld( d, &info ) );
    //
    if( r == TLD_RESULT_SUCCESS )
    {
        const char *domain = d; // by default assume no sub-domain
        for(; d < info.f_tld; ++d)
        {
            if(*d == '.')
            {
                domain = d + 1;
            }
        }

        return domain == f_domain_org_name;
    }

    return false;
}


QVariant WebsiteModel::data( QModelIndex const & idx, int role ) const
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


void WebsiteModel::fetchCustomData( Query::pointer_t q )
{
    const QByteArray value(q->getByteArrayColumn(0));
    f_sortMap[f_dbutils->get_row_name(value)] = value;
}


// vim: ts=4 sw=4 et
