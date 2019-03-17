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

#include <casswrapper/schema.h>
#include <QtGui>

#include <memory>
#include <vector>

class KeyspaceModel
    : public QAbstractListModel
{
    Q_OBJECT

public:
    typedef std::vector<QString> string_list_t;

                    KeyspaceModel() {}

    void            setCassandra( casswrapper::Session::pointer_t c, const QString& keyspace_name );

    Qt::ItemFlags   flags       ( const QModelIndex & index ) const;
    QVariant        data        ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    QVariant        headerData  ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    int             rowCount    ( const QModelIndex & parent = QModelIndex() ) const;

private:
    void            reset();

    string_list_t   f_tableNames = string_list_t();
};


// vim: ts=4 sw=4 et
