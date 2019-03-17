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

#include "KeyspaceModel.h"

#include <iostream>

using namespace casswrapper;
using namespace casswrapper::schema;


void KeyspaceModel::reset()
{
    beginResetModel();
    endResetModel();
}

void KeyspaceModel::setCassandra( Session::pointer_t c, const QString& keyspace_name )
{
    auto sessionMeta = SessionMeta::create( c );
    sessionMeta->loadSchema();

    f_tableNames.clear();
    const auto& keyspaces( sessionMeta->getKeyspaces() );
    const auto& keyspace ( keyspaces.find(keyspace_name) );
    if( keyspace != keyspaces.end() )
    {
        for( const auto& pair : keyspace->second->getTables() )
        {
            f_tableNames.push_back( pair.first );
        }
    }

    reset();
}


Qt::ItemFlags KeyspaceModel::flags( const QModelIndex & /*idx*/ ) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant KeyspaceModel::data( const QModelIndex & idx, int role ) const
{
    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    return f_tableNames[idx.row()];
}


QVariant KeyspaceModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
    // TODO
    return "Row Name";
}


int KeyspaceModel::rowCount( const QModelIndex & /*parent*/ ) const
{
    return static_cast<int>(f_tableNames.size());
}


// vim: ts=4 sw=4 et
