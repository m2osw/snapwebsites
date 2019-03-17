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

#include <casswrapper/query_model.h>

#include <snapwebsites/dbutils.h>

#include <memory>

class RowModel
    : public casswrapper::query_model
{
    Q_OBJECT

public:
                          RowModel();

    const QByteArray&     rowKey() const                     { return f_rowKey; }
    void                  setRowKey( const QByteArray& key ) { f_rowKey = key;  }

    virtual bool          fetchFilter( const QByteArray& key ) override;

    virtual QVariant      data  ( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
    virtual Qt::ItemFlags flags ( const QModelIndex & idx ) const override;

    // Write access
    //
    bool                  setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole ) override;

    // Resizable methods
    //
    bool                  insertRows( int row, int count, const QModelIndex & parent = QModelIndex() ) override;
    virtual bool          removeRows( int row, int count, const QModelIndex & parent = QModelIndex() ) override;

    void                  doQuery();

private:
    QByteArray                      f_rowKey = QByteArray();
    std::shared_ptr<snap::dbutils>  f_dbutils = std::shared_ptr<snap::dbutils>();
};


// vim: ts=4 sw=4 et
