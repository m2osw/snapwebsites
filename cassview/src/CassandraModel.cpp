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

#include "CassandraModel.h"

#include <iostream>

using namespace casswrapper;
using namespace casswrapper::schema;


void CassandraModel::reset()
{
    beginResetModel();
    endResetModel();
}


void CassandraModel::setCassandra( Session::pointer_t c )
{
    f_sessionMeta = SessionMeta::create( c );
    f_sessionMeta->loadSchema();
    reset();
}


Qt::ItemFlags CassandraModel::flags( const QModelIndex & /*idx*/ ) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant CassandraModel::data( const QModelIndex & idx, int role ) const
{
    if( !f_sessionMeta )
    {
        return QVariant();
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    try
    {
        const auto& keyspace_list( f_sessionMeta->getKeyspaces() );
        auto iter = keyspace_list.begin();
        for( int r = 0; r < idx.row(); ++r ) iter++;
        const QString& keyspace_name( iter->first );
        return keyspace_name;
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return QVariant();
}


QVariant CassandraModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
    // TODO
    return "Row Name";
}


int CassandraModel::rowCount( const QModelIndex & /*parent*/ ) const
{
    if( !f_sessionMeta )
    {
        return 0;
    }

    try
    {
        const auto& keyspace_list( f_sessionMeta->getKeyspaces() );
        return keyspace_list.size();
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return 0;
}


// vim: ts=4 sw=4 et
