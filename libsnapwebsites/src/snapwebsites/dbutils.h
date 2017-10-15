// Snap Websites Server -- manage sessions for users, forms, etc.
// Copyright (C) 2012-2017  Made to Order Software Corp.
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
//
#pragma once

#include <QtCassandra/QCassandraCell.h>
#include <QtCassandra/QCassandraRow.h>
#include <QtCassandra/QCassandraTable.h>

#include <QString>
#include <QByteArray>

namespace snap
{

class dbutils
{
public:
    enum class column_type_t
    {
        CT_int8_value,
        CT_uint8_value,
        CT_int32_value,
        CT_uint32_value,
        CT_int64_value,
        CT_uint64_value,
        CT_float32_value,
        CT_float64_value,
        CT_float64_or_empty_value,
        CT_time_microseconds,
        CT_time_seconds,
        CT_time_microseconds_and_string,
        CT_priority_and_time_microseconds_and_string,
        CT_hexarray_value,
        CT_hexarray_limited_value,
        CT_md5array_value,
        CT_secure_value,
        CT_status_value,
        CT_string_value,
        CT_rights_value
    };

                        dbutils( QString const & table_name, QString const & row_name );

    static void         copy_row(QtCassandra::QCassandraTable::pointer_t ta, QString const & a,  // source
                                 QtCassandra::QCassandraTable::pointer_t tb, QString const & b); // destination

    static QString      byte_to_hex            ( char const         byte );
    static QString      key_to_string          ( QByteArray const & key  );
    static QByteArray   string_to_key          ( QString const &    str  );
    static QString      microseconds_to_string ( int64_t const &    time, bool const full );
    static uint64_t     string_to_microseconds ( QString const &    time );

    int                 get_display_len() const;
    void                set_display_len( int const val );

    QByteArray          get_row_key() const;
    QString             get_row_name( QtCassandra::QCassandraRow::pointer_t p_r ) const;
    QString             get_row_name( const QByteArray& key ) const;

    QByteArray			set_row_name( const QString& name, const QByteArray& orig_key ) const;

    QString             get_column_name ( QtCassandra::QCassandraCell::pointer_t c ) const;
    QString				get_column_name ( const QByteArray& key ) const;

    void                set_column_name ( QByteArray& key, const QString& name ) const;

    QString             get_column_value( QtCassandra::QCassandraCell::pointer_t c, bool const display_only = false ) const;
    QString             get_column_value( const QByteArray& key, const QByteArray& value, bool const display_only = false ) const;

    void                set_column_value( QtCassandra::QCassandraCell::pointer_t c, QString const & v ) const;
    void				set_column_value( const QByteArray& key, QByteArray& value, QString const & v ) const;

    column_type_t       get_column_type( QtCassandra::QCassandraCell::pointer_t c ) const;
    column_type_t       get_column_type( const QByteArray& key ) const;

    QString             get_column_type_name( const QByteArray& key ) const;
    static QString      get_column_type_name( column_type_t val );

private:
    QString             f_tableName;
    QString             f_rowName;
    int                 f_displayLen;

    QString 			get_column_value( const QByteArray& key, const QtCassandra::QCassandraValue& value, bool const display_only ) const;
    void				set_column_value( const QByteArray& key, QtCassandra::QCassandraValue& cvalue, QString const & v ) const;
};

}
// namespace snap

// vim: ts=4 sw=4 et
