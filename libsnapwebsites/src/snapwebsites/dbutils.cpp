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

#include "snapwebsites/dbutils.h"

#include "snapwebsites/snap_exception.h"
#include "snapwebsites/qstring_stream.h"
#include "snapwebsites/log.h"
#include "snapwebsites/mkgmtime.h"
#include "snapwebsites/snap_string_list.h"

#include <iostream>

#include <uuid/uuid.h>

#include "snapwebsites/poison.h"


namespace snap
{

namespace
{
    char hex_to_dec(ushort c)
    {
        if(c >= '0' && c <= '9')
        {
            return static_cast<char>(c - '0');
        }
        if(c >= 'a' && c <= 'f')
        {
            return static_cast<char>(c - 'a' + 10);
        }
        if(c >= 'A' && c <= 'F')
        {
            return static_cast<char>(c - 'A' + 10);
        }
        throw snap_exception( "error: invalid hexadecimal digit, it cannot be converted." );
    }
}


dbutils::dbutils( QString const & table_name, QString const & row_name )
    : f_tableName(table_name)
    , f_rowName(row_name)
    , f_displayLen(64)
{
}


/** \brief Copy all the cells from one row to another.
 *
 * This function copies all the cells from one row to another. It does not
 * try to change anything in the process. The destination (source?) should
 * be tweaked as required on return.
 *
 * \warning
 * This function does not delete anything, if other fields already existed
 * in the destination, then it stays there.
 *
 * \param[in] ta  The source table.
 * \param[in] a  The name of the row to copy from.
 * \param[in] tb  The destination table.
 * \param[in] b  The name of the row to copy to.
 */
void dbutils::copy_row(QtCassandra::QCassandraTable::pointer_t ta, QString const & a, // source
                       QtCassandra::QCassandraTable::pointer_t tb, QString const & b) // destination
{
    // just in case there is still a previous query, clear the cache ahead of time
    QtCassandra::QCassandraRow::pointer_t source_row(ta->row(a));
    source_row->clearCache();
    QtCassandra::QCassandraRow::pointer_t destination_row(tb->row(b));
    auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
    column_predicate->setCount(100); // we have to copy everything also it is likely very small (i.e. 10 fields...)
    column_predicate->setIndex(); // behave like an index
    for(;;)
    {
        source_row->readCells(column_predicate);
        QtCassandra::QCassandraCells const source_cells(source_row->cells());
        if(source_cells.isEmpty())
        {
            // done
            break;
        }
        // handle one batch
        for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
                nc != source_cells.end();
                ++nc)
        {
            QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
            QByteArray cell_key(source_cell->columnKey());
            destination_row->cell(cell_key)->setValue(source_cell->value());
        }
    }
}



QByteArray dbutils::get_row_key() const
{
    QByteArray row_key;

    if(!f_rowName.isEmpty() && f_tableName == "files")
    {
        if(f_rowName == "new" || f_rowName == "javascripts" || f_rowName == "css" || f_rowName == "images")
        {
            row_key = f_rowName.toLatin1();
        }
        else
        {
            // these rows make use of MD5 sums so we have to convert them
            QByteArray const str(f_rowName.toUtf8());
            char const * s(str.data());
            while(s[0] != '\0' && s[1] != '\0')
            {
                char const c(static_cast<char>((hex_to_dec(s[0]) << 4) | hex_to_dec(s[1])));
                row_key.append(c);
                s += 2;
            }
        }
    }
    else if( f_tableName == "users" )
    {
        QtCassandra::QCassandraValue const identifier(f_rowName);
        if( identifier.stringValue() == "*index_row*" || identifier.stringValue() == "*id_row*" )
        {
            return identifier.binaryValue();
        }
        else
        {
            QByteArray new_key;
            QtCassandra::appendInt64Value( new_key, static_cast<int64_t>(f_rowName.toLong()) );
            return new_key;
        }
    }
    else
    {
        row_key = f_rowName.toLatin1();
    }

    return row_key;
}


/** \brief Transform a byte in an hexadecimal number.
 *
 * This function transforms a byte (a number from 0 to 255) to an ASCII
 * representation using hexadecimal.
 *
 * \param[in] byte  The byt to transform.
 *
 * \return The resulting string.
 */
QString dbutils::byte_to_hex( char const byte )
{
    const QString hex(QString("%1").arg(byte & 255, 2, 16, QChar('0')));
    return hex;
}


/** \brief Transform a binary key to hexadecimal.
 *
 * This function transforms each byte of a binary key to an ASCII string
 * of hexadecimal numbers using the byte_to_hex() function.
 *
 * \param[in] key  The key to transform.
 *
 * \return The resulting string.
 */
QString dbutils::key_to_string( QByteArray const & key )
{
    QString ret;
    int const max_length(key.size());
    for(int i(0); i < max_length; ++i)
    {
        ret += byte_to_hex( key[i] );
    }
    return ret;
}


QByteArray dbutils::string_to_key( QString const & str )
{
    QByteArray ret;
    snap_string_list numList( str.split(' ') );

    // TODO: we may want to seperate functions to clearly know whether
    //       we are checking an MD5 or not
    //
    if(numList.size() == 1 && str.length() == 32)
    {
        for(int i(0); i < 32; i += 2)
        {
            QString val(str.mid(i, 2));
            bool ok( false );
            ret.push_back( static_cast<char>( val.toInt( &ok, 16 ) ) );
            if( !ok )
            {
                throw snap_exception( "Cannot convert to number! Not base 16." );
            }
        }
    }
    else
    {
        for( auto const & str_num : numList )
        {
            bool ok( false );
            ret.push_back( static_cast<char>( str_num.toInt( &ok, 16 ) ) );
            if( !ok )
            {
                throw snap_exception( "Cannot convert to number! Not base 16 or too large." );
            }
        }
    }

    return ret;
}


QString dbutils::microseconds_to_string ( int64_t const & time, bool const full )
{
    char buf[64];
    struct tm t;
    time_t const seconds(time / 1000000);
    gmtime_r(&seconds, &t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    if(full)
    {
        return QString("%1.%2 (%3)")
                .arg(buf)
                .arg(time % 1000000, 6, 10, QChar('0'))
                .arg(time);
    }
    else
    {
        return QString("%1.%2")
                .arg(buf)
                .arg(time % 1000000, 6, 10, QChar('0'));
    }
}

uint64_t dbutils::string_to_microseconds ( QString const & time )
{
    // TODO: check whether we have the number between parenthesis...
    //       The string may include the microseconds as one 64 bit number
    //       between parenthesis; if present we may want to use that instead?

    // String will be of the form: "%Y-%m-%d %H:%M:%S.%N"
    //
    snap_string_list const datetime_split ( time.split(' ') );
    if(datetime_split.size() < 2)
    {
        return -1;
    }
    snap_string_list const date_split     ( datetime_split[0].split('-') );
    snap_string_list const time_split     ( datetime_split[1].split(':') );
    if(date_split.size() != 3
    || time_split.size() != 3)
    {
        return -1;
    }
    //
    tm to;
    memset(&to, 0, sizeof(to));
    to.tm_sec  = time_split[2].toInt();
    to.tm_min  = time_split[1].toInt();
    to.tm_hour = time_split[0].toInt();
    to.tm_mday = date_split[2].toInt();
    to.tm_mon  = date_split[1].toInt() - 1;
    to.tm_year = date_split[0].toInt() - 1900;

    int64_t ns((time_split[2].toDouble() - to.tm_sec) * 1000000.0);
    //
    time_t const tt( mkgmtime( &to ) );
    return tt * 1000000 + ns;
}


int dbutils::get_display_len() const
{
    return f_displayLen;
}


void dbutils::set_display_len( int const val )
{
    f_displayLen = val;
}


QString dbutils::get_row_name( QtCassandra::QCassandraRow::pointer_t p_r ) const
{
    QByteArray key(p_r->rowKey());
    return get_row_name( key );
}


QString dbutils::get_row_name( const QByteArray& key ) const
{
    if(f_tableName == "files")
    {
        // these are raw MD5 keys
        if( key.size() == 16 )
        {
            return key_to_string( key );
        }

        // except for a few special cases
        // TODO: add tests to make sure only known keys do not get
        //       defined as MD5 sums
    }
    else if( f_tableName == "users" )
    {
        QtCassandra::QCassandraValue const identifier(key);
        if( identifier.stringValue() == "*index_row*" || identifier.stringValue() == "*id_row*" )
        {
            return identifier.stringValue();
        }
        else
        {
            return QString("%1").arg(identifier.safeInt64Value());
        }
    }

    return QString::fromUtf8( key.data() );
}


QByteArray dbutils::set_row_name( const QString& name, const QByteArray& orig_key ) const
{
    if(f_tableName == "files")
    {
        // these are raw MD5 keys
        if( orig_key.size() == 16 )
        {
            return string_to_key( name );
        }
    }

    return name.toUtf8();
}


QString dbutils::get_column_name( QtCassandra::QCassandraCell::pointer_t c ) const
{
    return get_column_name( c->columnKey() );
}


QString dbutils::get_column_name( const QByteArray& key ) const
{
    QString const content_attachment_reference( "content::attachment::reference::" );

    QString name;
    if(f_tableName == "files" && f_rowName == "new")
    {
        name = key_to_string( key );
    }
    else if((f_tableName == "list" && f_rowName != "*standalone*"))
    {
        unsigned char priority(QtCassandra::safeUnsignedCharValue(key, 0));
        QString const time(microseconds_to_string(QtCassandra::safeInt64Value(key, sizeof(unsigned char)), true));
        name = QString("%1 %2 %3")
                .arg(static_cast<int>(priority))
                .arg(time)
                .arg(QtCassandra::stringValue(key, sizeof(unsigned char) + sizeof(uint64_t)));
    }
    else if((f_tableName == "files" && f_rowName == "images"))
    {
        QString const time(microseconds_to_string(QtCassandra::safeInt64Value(key, 0), true));
        name = QString("%1 %2").arg(time).arg(QtCassandra::stringValue(key, sizeof(uint64_t)));
    }
    else if(f_tableName == "branch" && (key.startsWith(content_attachment_reference.toLatin1())) )
    {
        name = content_attachment_reference;
        name += key_to_string( key.mid( content_attachment_reference.length() ) );
    }
    else if((f_tableName == "files" && f_rowName == "javascripts")
         || (f_tableName == "files" && f_rowName == "css"))
    {
        // this row name is "<name>"_"<browser>"_<version as integers>
        int const max_length(key.size());
        int sep(0);
        int i(0);
        for(; i < max_length && sep < 2; ++i)
        {
            if(key[i] == '_')
            {
                ++sep;
            }
            name += key[i];
        }
        // now we have to add the version
        bool first(true);
        for(; i + 3 < max_length; i += 4)
        {
            if(first)
            {
                first = false;
            }
            else
            {
                name += ".";
            }
            name += QString("%1").arg(QtCassandra::safeUInt32Value(key, i));
        }
    }
    else if( f_tableName == "users" )
    {
        QtCassandra::QCassandraValue const identifier(key);
        name = identifier.stringValue();
    }
    else if( (f_tableName == "shorturl" && f_rowName.endsWith("/*index_row*"))
         || f_tableName == "serverstats")
    {
        // special case where the column key is a 64 bit integer
        QtCassandra::QCassandraValue const identifier(key);
        name = QString("%1").arg(identifier.safeInt64Value());
    }
    else if(f_tableName == "tracker"
         || (f_tableName == "backend" && !f_rowName.startsWith("*"))
         || (f_tableName == "firewall" && !f_rowName.startsWith("ip::")))
    {
        QtCassandra::QCassandraValue const start_date(key);
        name = microseconds_to_string(start_date.safeInt64Value(), true);
    }
    else if(f_tableName == "emails" && f_rowName == "bounced_raw")
    {
        QtCassandra::QCassandraValue const start_date(key);

        // 64 bit value (microseconds)
        name = microseconds_to_string(start_date.safeInt64Value(), true);

        // 128 bit UUID
        if( static_cast<size_t>(start_date.size()) >= sizeof(int64_t) + sizeof(uuid_t) )
        {
            QByteArray const bytes(QtCassandra::binaryValue( start_date.binaryValue(), sizeof(int64_t), sizeof(uuid_t) ));
            uuid_t uuid;
            memcpy( uuid, bytes.data(), sizeof(uuid) );
            char unique_key[37];
            uuid_unparse( uuid, unique_key );
            name += " ";
            name += unique_key;
        }
    }
    else
    {
        name = QString::fromUtf8(key.data());
    }

    return name;
}


void dbutils::set_column_name( QByteArray& key, const QString& name ) const
{
    QString const content_attachment_reference( "content::attachment::reference::" );

    if(f_tableName == "files" && f_rowName == "new")
    {
        key = string_to_key( name );
    }
    else if((f_tableName == "list" && f_rowName != "*standalone*"))
    {
        QStringList arr( name.split(' ') );
        if( arr.size() != 3 )
        {
            throw std::runtime_error("expected 3 arguments!");
        }

        const char priority = arr[0][0].toLatin1();
        QtCassandra::appendUnsignedCharValue( key, priority );

        const uint64_t microsec( string_to_microseconds( arr[1] ) );
        QtCassandra::appendInt64Value( key, microsec );
        QtCassandra::appendStringValue( key, arr[2] );
    }
    else if((f_tableName == "files" && f_rowName == "images"))
    {
        QStringList arr( name.split(' ') );
        if( arr.size() != 2 )
        {
            throw std::runtime_error("expected 3 arguments!");
        }

        const uint64_t microsec( string_to_microseconds( arr[0] ) );
        QtCassandra::appendInt64Value( key, microsec );

        QtCassandra::appendStringValue( key, arr[1] );
    }
    else if(f_tableName == "branch" && (name.startsWith(content_attachment_reference.toLatin1())) )
    {
        QtCassandra::appendStringValue( key, content_attachment_reference );
        QtCassandra::appendStringValue( key, name );
    }
    else if((f_tableName == "files" && f_rowName == "javascripts")
         || (f_tableName == "files" && f_rowName == "css"))
    {
        // this row name is "<name>"_"<browser>"_<version as integers>
        QStringList arr( name.split('_') );
        if( arr.size() != 3 )
        {
            throw std::runtime_error( "The column key is expected to be in the form: <name>_<browser>_<version>!" );
        }

        QtCassandra::appendStringValue( key, arr[0] + "_" );       // name
        QtCassandra::appendStringValue( key, arr[1] + "_" );       // browser

        QStringList version( arr[2].split('.') );
        for( auto rev : version )
        {
            QtCassandra::appendUInt32Value( key, static_cast<uint32_t>(rev.toUInt()) );
        }
    }
    else if( f_tableName == "users" )
    {
        QtCassandra::appendStringValue( key, name );
    }
    else if( (f_tableName == "shorturl" && f_rowName.endsWith("/*index_row*"))
             || f_tableName == "serverstats")
    {
        QtCassandra::appendInt64Value( key, static_cast<int64_t>(name.toLong()) );
    }
    else if(f_tableName == "tracker"
         || (f_tableName == "backend" && !f_rowName.startsWith("*"))
         || (f_tableName == "firewall" && !f_rowName.startsWith("ip::")))
    {
        const uint64_t microsec( string_to_microseconds( name ) );
        QtCassandra::appendInt64Value( key, microsec );
    }
    else if(f_tableName == "emails" && f_rowName == "bounced_raw")
    {
        QStringList arr( name.split(' ') );
        if( arr.size() == 2 )
        {
            const uint64_t microsec( string_to_microseconds( arr[0] ) );
            QtCassandra::appendInt64Value( key, microsec );
            //
            uuid_t uuid;
            uuid_parse( arr[1].toUtf8().data(), uuid );
            char unique_key[sizeof(uuid)];
            memcpy( unique_key, uuid, sizeof(uuid) );
            QtCassandra::appendBinaryValue( key, QByteArray(unique_key, sizeof(unique_key)) );
        }
        else
        {
            const uint64_t microsec( string_to_microseconds( name ) );
            QtCassandra::appendInt64Value( key, microsec );
        }
    }
    else
    {
        key = name.toUtf8();
    }
}


dbutils::column_type_t dbutils::get_column_type( QtCassandra::QCassandraCell::pointer_t c ) const
{
    return get_column_type( c->columnKey() );
}

dbutils::column_type_t dbutils::get_column_type( const QByteArray& key ) const
{
    QString const n( get_column_name( key ) );

    if(f_tableName == "revision" && f_rowName.indexOf("#user/") != -1 && n != "content::modified")
    {
        // on error fields are all saved as string, whetever their type
        // (because on errors we cannot save a transformed value as the
        // user entered it...)
        return column_type_t::CT_string_value;
    }

    if(n == "antivirus::enable"
    || n == "content::breadcrumbs_show_current_page"
    || n == "content::breadcrumbs_show_home"
    || n == "content::prevent_delete"
    || n == "core::mx_result"
    || n == "core::test_site"
    || n == "epayment_creditcard::show_address2"
    || n == "epayment_creditcard::show_country"
    || n == "epayment_creditcard::show_phone"
    || n == "epayment_creditcard::show_province"
    || n == "epayment_paypal::debug"
    || n == "epayment_stripe::debug"
    || n == "feed::allow_main_atom_xml"
    || n == "feed::allow_main_rss_xml"
    || n == "feed::publish_full_article"
    || n == "oauth2::enable"
    || n == "password::check_blacklist"
    || n == "password::exists_in_blacklist"
    || n == "permissions::dynamic"
    || n == "permissions::secure_site"
    || n == "users::example"
    || n == "users::multiuser"
    || n == "users::long_sessions"
    || n == "users::password::blocked"
    || (f_tableName == "content" && f_rowName == "*index*")
    || (f_tableName == "list" && f_rowName != "*standalone*")
    || n == "users::soft_administrative_session"
    || n == "users::verify_ignore_user_agent"
    || n == "finball::data_status" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_atm_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_cafe_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_carwash_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_carwashexpress_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_detailshop_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_giftshop_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_lotto_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_lube_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_propane_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_repair_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_retail_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::data_vendingmachine_holiday" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::number_of_cashiers" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::plan" // TODO -- remove at some point since that is a cutomer's field
    || n == "finball::read_terms_n_conditions" // TODO -- remove at some point since that is a customer's field (we'd need to have an XML file instead)
    || n == "finball::applies_to_companies" // TODO -- remove at some point since that is a customer's field (we'd need to have an XML file instead)
    || n == "finball::applies_to_locations" // TODO -- remove at some point since that is a customer's field (we'd need to have an XML file instead)
    )
    {
        // signed 8 bit value
        return column_type_t::CT_int8_value;
    }
    else if(n == "content::final"
         || n.startsWith("content::files::reference::")
         || n == "epayment_paypal::maximum_repeat_failures"
         || n == "sessions::used_up"
         || (f_tableName == "files" && f_rowName == "new")
         || (f_tableName == "files" && f_rowName == "images")
         || (f_tableName == "test_results" && n == "test_plugin::success")
         || (f_tableName == "processing" && n == "content::status_changed")
         )
    {
        // unsigned 8 bit value
        // cast to integer so arg() doesn't take it as a character
        return column_type_t::CT_uint8_value;
    }
    else if(n == "list::number_of_items"
         || (f_tableName == "domains" && f_rowName == "*test_snap_lock*" && n == "counter")
         || n == "finball::memo_count" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         )
    {
        return column_type_t::CT_int32_value;
    }
    else if(n.startsWith("content::attachment::reference::")
         || n == "content::attachment::revision_control::last_branch"
         || n.startsWith("content::attachment::revision_control::last_revision::")
         || n == "content::files::image_height"
         || n == "content::files::image_width"
         || n == "content::files::size"
         || n == "content::files::size::gzip_compressed"
         || n == "content::files::size::minified"
         || n == "content::files::size::minified::gzip_compressed"
         || n == "content::revision_control::attachment::current_branch"
         || n == "content::revision_control::attachment::current_working_branch"
         || n == "content::revision_control::current_branch"
         || n == "content::revision_control::current_working_branch"
         || n == "content::revision_control::last_branch"
         || n == "content::revision_control::attachment::last_branch"
         || n.startsWith("content::revision_control::attachment::current_revision::")
         || n.startsWith("content::revision_control::attachment::current_working_revision::")
         || n.startsWith("content::revision_control::current_revision::")
         || n.startsWith("content::revision_control::current_working_revision::")
         || n.startsWith("content::revision_control::last_revision::")
         || n.startsWith("content::revision_control::attachment::last_revision::")
         || n == "sitemapxml::count"
         || n == "sessions::id"
         || n == "sessions::time_to_live"
         || (f_tableName == "lock_table" && f_rowName == "hosts")
         )
    {
        return column_type_t::CT_uint32_value;
    }
    else if(n == "bookkeeping::counter"
         || n == "content::revision_limits"
         || n == "cookie_consent_silktide::javascript_version"
         || n == "cookie_consent_silktide::consent_duration"
         || n == "feed::teaser_tags"
         || n == "feed::teaser_words"
         || n == "feed::top_maximum_number_of_items_in_any_feed"
         || n == "password::blocked_user_counter"
         || n == "password::blocked_user_counter_lifetime"
         || n == "password::count_failures"
         || n == "password::count_bad_password_503s"
         || n == "password::invalid_passwords_counter"
         || n == "password::invalid_passwords_block_duration"
         || n == "password::invalid_passwords_counter_lifetime"
         || n == "password::invalid_passwords_slowdown"
         || n == "password::minimum_length"
         || n == "password::minimum_length_of_variations"
         || n == "password::minimum_letters"
         || n == "password::minimum_lowercase_letters"
         || n == "password::minimum_spaces"
         || n == "password::minimum_specials"
         || n == "password::minimum_unicode"
         || n == "password::minimum_uppercase_letters"
         || n == "password::minimum_digits"
         || n == "password::minimum_variation"
         || n == "sessions::check_flags"
         || n == "users::administrative_session_duration"
         || n == "users::total_session_duration"
         || n == "users::user_session_duration"
         || (f_tableName == "firewall" && (n.endsWith("::ban_count")
                                        || n.endsWith("::packet_count")
                                        || n.endsWith("::byte_count")))
         )
    {
        return column_type_t::CT_int64_value;
    }
    else if(n.startsWith("content::field_priority::")
         || n == "ecommerce::invoice_number"
         || n == "epayment_paypal::invoice_number"
         || n == "shorturl::identifier"
         || n == "users::identifier"
         || n == "finball::invoice_number" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::unique_email" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "content::journal::timestamp"
         )
    {
        return column_type_t::CT_uint64_value;
    }
    else if(n == "sitemapxml::priority"
         )
    {
        // 32 bit float
        return column_type_t::CT_float32_value;
    }
    else if(n == "epayment::price"
         || n == "epayment::grand_total"
         || n == "finball::atm_fee" // TODO -- remove at some point since that is a cutomer's field
         || n == "finball::company_plan1" // TODO -- remove at some point since that is a cutomer's field
         || n == "finball::company_plan12" // TODO -- remove at some point since that is a cutomer's field
         || n == "finball::company_plan6" // TODO -- remove at some point since that is a cutomer's field
         || n == "finball::location_plan1" // TODO -- remove at some point since that is a cutomer's field
         || n == "finball::location_plan12" // TODO -- remove at some point since that is a cutomer's field
         || n == "finball::location_plan6" // TODO -- remove at some point since that is a cutomer's field
         || n == "finball::lotto_fee" // TODO -- remove at some point since that is a cutomer's field
         || n == "finball::memo_invoice_total_amount" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::minimum_total" // TODO -- remove at some point since that is a cutomer's field
         || n == "finball::promotion_amount" // TODO -- remove at some point since that is a cutomer's field
         )
    {
        // 64 bit float
        return column_type_t::CT_float64_value;
    }
    else if((n.startsWith("finball::data_") && n.endsWith("_count")) // TODO -- remove at some point since that is a cutomer's field
         || (n.startsWith("finball::data_") && n.endsWith("_amount"))) // TODO -- remove at some point since that is a cutomer's field
    {
        // 64 bit float
        return column_type_t::CT_float64_or_empty_value;
    }
    else if((f_tableName == "backend" && f_rowName.startsWith("*"))
         || (f_tableName == "antihammering" && n == "*blocked*")
         || (f_tableName == "layout" && n.startsWith("layout::reference::"))
         || n == "content::created"
         || n == "content::cloned"
         || n == "content::files::created"
         || n == "content::files::secure::last_check"
         || n == "content::files::updated"
         || n == "content::modified"
         || n == "content::updated"
         || (f_tableName == "content" && n == "content::status_changed")
         || n.startsWith("core::last_dynamic_update")
         || n.startsWith("core::last_updated")
         || n == "core::plugin_threshold"
         || n == "core::site_ready"
         || n == "editor::date_edit_widget"
         || n == "editor::dropdown_date_edit_widget"
         || n == "epayment_paypal::last_attempt"
         || n == "epayment_paypal::oauth2_expires"
         || n == "epayment_stripe::created"
         || n == "content::files::creation_time"
         || n == "content::files::modification_time"
         || n == "core::mx_last_checked"
         || n == "images::modified"
         || n == "list::last_updated"
         || n == "permissions::last_updated"
         || n == "sendmail::bounce_arrival_date0"
         || n == "sendmail::bounce_arrival_date1"
         || n == "sendmail::bounce_arrival_date2"
         || n == "sendmail::bounce_arrival_date3"
         || n == "sendmail::bounce_arrival_date4"
         || n.endsWith("::sendmail::created")
         || (f_tableName == "firewall" && (n.endsWith("::created")
                                        || n.endsWith("::modified")
                                        || n.endsWith("::block_limit")))
         || n == "sendmail::unsubscribe_on"
         || n == "sessions::date"
         || n == "snap_software_description::last_update"
         || n == "shorturl::date"
         || n == "users::created_time"
         || n == "users::forgot_password_on"
         || n == "users::login_on"
         || n == "users::logout_on"
         || n == "users::modified"
         || n == "users::password::modified"
         || n.startsWith("users::password::modified_")
         || n == "users::previous_login_on"
         || n == "users::start_date"
         || n == "users::verified_on"
         || (f_tableName == "users" && n.startsWith("users::website_reference::"))
         || n == "sessions::creation_date"
         || (f_tableName == "test_results" && n == "test_plugin::end_date")
         || (f_tableName == "test_results" && n == "test_plugin::start_date")
         || n == "finball::applied_on" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::apply_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::billing_last_updated" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::category_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::correct_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::data_last_modified" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::end_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::payment_entered_on" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::sub_category_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::business_type_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::connection_accepted_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::connection_declined_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::invoice_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::invoice_due_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::invoice_end_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::invoice_paid_on" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::invoice_start_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::memo_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::memo_end_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::memo_paid_on" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::memo_start_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::payment_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::refused_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::report_created" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::start_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         || n == "finball::void_date" // TODO -- remove at some point since that is a customer's type (we'd need to have an XML file instead)
         )
    {
        // 64 bit value (microseconds)
        return column_type_t::CT_time_microseconds;
    }
    else if(n == "sessions::login_limit"
         || n == "sessions::time_limit"
         )
    {
        // 64 bit value (seconds)
        return column_type_t::CT_time_seconds;
    }
    else if(f_tableName == "listref"
         )
    {
        return column_type_t::CT_priority_and_time_microseconds_and_string;
    }
    else if(n == "sessions::random"
         || n == "users::password::salt"
         || n.startsWith("users::password::salt_")
         || n == "users::password"
         || n.startsWith("users::password_")
         )
    {
        // n bit binary value
        return column_type_t::CT_hexarray_value;
    }
    else if(n.startsWith("content::files::data")
         || f_tableName == "layout"
         || n.startsWith("finball::mtd_data_") // TODO -- remove at some point since that is a cutomer's field
         )
    {
        // n bit binary value
        // same as previous only this can be huge so we limit it
        return column_type_t::CT_hexarray_limited_value;
    }
    else if((f_tableName == "revision" && n == "content::attachment")
         || (f_tableName == "files" && f_rowName == "javascripts")
         || (f_tableName == "files" && f_rowName == "css")
         )
    {
        // md5 in binary
        return column_type_t::CT_md5array_value;
    }
    else if(n == "content::files::secure")
    {
        return column_type_t::CT_secure_value;
    }
    else if(n == "content::status")
    {
        return column_type_t::CT_status_value;
    }
    else if(f_tableName == "cache"
        && (n.startsWith("permissions::")))
    {
        return column_type_t::CT_rights_value;
    }
    else if( (f_tableName == "users") && (f_rowName == "*index_row*") )
    {
        return column_type_t::CT_int64_value;
    }

    // all others viewed as strings
    return column_type_t::CT_string_value;
}


QString dbutils::get_column_type_name( column_type_t val )
{
    QString name;
    switch( val )
    {
        case column_type_t::CT_int8_value                                : name = "int8"                                      ; break;
        case column_type_t::CT_uint8_value                               : name = "uint8"                                     ; break;
        case column_type_t::CT_int32_value                               : name = "int32"                                     ; break;
        case column_type_t::CT_uint32_value                              : name = "uint32"                                    ; break;
        case column_type_t::CT_int64_value                               : name = "int64"                                     ; break;
        case column_type_t::CT_uint64_value                              : name = "uint64"                                    ; break;
        case column_type_t::CT_float32_value                             : name = "float32"                                   ; break;
        case column_type_t::CT_float64_value                             : name = "float64"                                   ; break;
        case column_type_t::CT_float64_or_empty_value                    : name = "float64_or_empty"                          ; break;
        case column_type_t::CT_time_microseconds                         : name = "time_microseconds"                         ; break;
        case column_type_t::CT_time_seconds                              : name = "time_seconds"                              ; break;
        case column_type_t::CT_time_microseconds_and_string              : name = "time_microseconds_and_string"              ; break;
        case column_type_t::CT_priority_and_time_microseconds_and_string : name = "priority_and_time_microseconds_and_string" ; break;
        case column_type_t::CT_hexarray_value                            : name = "hexarray"                                  ; break;
        case column_type_t::CT_hexarray_limited_value                    : name = "hexarray_limited"                          ; break;
        case column_type_t::CT_md5array_value                            : name = "md5array"                                  ; break;
        case column_type_t::CT_secure_value                              : name = "secure"                                    ; break;
        case column_type_t::CT_status_value                              : name = "status"                                    ; break;
        case column_type_t::CT_string_value                              : name = "string"                                    ; break;
        case column_type_t::CT_rights_value                              : name = "rights"                                    ; break;
    }
    return name;
}


QString dbutils::get_column_type_name( const QByteArray& key ) const
{
    return get_column_type_name( get_column_type(key) );
}


QString dbutils::get_column_value( QtCassandra::QCassandraCell::pointer_t c, bool const display_only ) const
{
    return get_column_value( c->columnKey(), c->value().binaryValue(), display_only );
}


QString dbutils::get_column_value( const QByteArray& key, const QByteArray& value, bool const display_only ) const
{
    QtCassandra::QCassandraValue cvalue(value);
    return get_column_value( key, cvalue, display_only );
}

QString dbutils::get_column_value( const QByteArray& key, const QtCassandra::QCassandraValue& value, bool const display_only ) const
{
    QString v;
    try
    {
        column_type_t const ct( get_column_type( key ) );
        switch( ct )
        {
            case column_type_t::CT_int8_value:
            {
                // signed 8 bit value
                // cast to integer so arg() doesn't take it as a character
                v = QString("%1").arg(static_cast<int>(value.signedCharValue()));
            }
            break;

            case column_type_t::CT_uint8_value:
            {
                // unsigned 8 bit value
                // cast to integer so arg() doesn't take it as a character
                v = QString("%1").arg(static_cast<int>(value.unsignedCharValue()));
            }
            break;

            case column_type_t::CT_int32_value:
            {
                // 32 bit value
                v = QString("%1").arg(value.int32Value());
            }
            break;

            case column_type_t::CT_uint32_value:
            {
                // 32 bit value
                v = QString("%1").arg(value.uint32Value());
            }
            break;

            case column_type_t::CT_int64_value:
            {
                v = QString("%1").arg(value.int64Value());
            }
            break;

            case column_type_t::CT_uint64_value:
            {
                v = QString("%1").arg(value.uint64Value());
            }
            break;

            case column_type_t::CT_float32_value:
            {
                // 32 bit float
                v = QString("%1").arg(value.floatValue());
            }
            break;

            case column_type_t::CT_float64_value:
            {
                // 64 bit float
                v = QString("%1").arg(value.doubleValue());
            }
            break;

            case column_type_t::CT_float64_or_empty_value:
            {
                // 64 bit float or empty
                if(value.nullValue())
                {
                    v = "";
                }
                else
                {
                    v = QString("%1").arg(value.doubleValue());
                }
            }
            break;

            case column_type_t::CT_time_microseconds:
            {
                // 64 bit value (microseconds)
                int64_t const time(value.safeInt64Value());
                if(time == 0)
                {
                    v = "time not set (0)";
                }
                else
                {
                    v = microseconds_to_string(time, true);
                }
            }
            break;

            case column_type_t::CT_time_microseconds_and_string:
            {
                v = QString("%1 %2")
                            .arg(microseconds_to_string(QtCassandra::safeInt64Value(value.binaryValue(), 0), true))
                            .arg(QtCassandra::stringValue(value.binaryValue(), sizeof(int64_t)));
            }
            break;

            case column_type_t::CT_priority_and_time_microseconds_and_string:
            {
                v = QString("%1 %2 %3")
                            .arg(static_cast<int>(QtCassandra::safeUnsignedCharValue(value.binaryValue(), 0)))
                            .arg(microseconds_to_string(QtCassandra::safeInt64Value(value.binaryValue(), sizeof(unsigned char)), true))
                            .arg(QtCassandra::stringValue(value.binaryValue(), sizeof(unsigned char) + sizeof(int64_t)));
            }
            break;

            case column_type_t::CT_time_seconds:
            {
                // 64 bit value (seconds)
                uint64_t time(value.uint64Value());
                if(time == 0)
                {
                    v = "time not set (0)";
                }
                else
                {
                    char buf[64];
                    struct tm t;
                    time_t const seconds(time);
                    gmtime_r(&seconds, &t);
                    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
                    v = display_only
                            ? QString("%1 (%2)").arg(buf).arg(time)
                            : QString("%1").arg(buf);
                }
            }
            break;

            case column_type_t::CT_hexarray_value:
            case column_type_t::CT_hexarray_limited_value:
            {
                // n bit binary value
                bool const display_limited( display_only && (ct == column_type_t::CT_hexarray_limited_value) );
                const QByteArray& buf(value.binaryValue());
                int const max_length( display_limited ? std::min(f_displayLen, buf.size()): buf.size() );
                if( display_only )
                {
                    v += "(hex) ";
                }
                for(int i(0); i < max_length; ++i)
                {
                    v += byte_to_hex(buf[i]) + " ";
                }
                if( display_limited && (buf.size() > max_length) )
                {
                    v += "...";
                }
            }
            break;

            case column_type_t::CT_md5array_value:
            {
                // md5 in binary
                QByteArray const& buf(value.binaryValue());
                if( display_only )
                {
                    v += "(md5) ";
                }
                v += key_to_string(buf);
            }
            break;

            case column_type_t::CT_secure_value:
            {
                switch(value.signedCharValue())
                {
                case -1:
                    v = "not checked (-1)";
                    break;

                case 0:
                    v = "not secure (0)";
                    break;

                case 1:
                    v = "secure (1)";
                    break;

                default:
                    v = QString("unknown secure status (%1)").arg(value.signedCharValue());
                    break;

                }
            }
            break;

            case column_type_t::CT_status_value:
            {
                uint32_t status(value.uint32Value());
                if(display_only)
                {
                    switch(status & 0x000000FF)
                    {
                    case 0:
                        v = "unknown state";
                        break;

                    case 1:
                        v = "create"; // should not be saved!
                        break;

                    case 2:
                        v = "normal";
                        break;

                    case 3:
                        v = "hidden";
                        break;

                    case 4:
                        v = "moved";
                        break;

                    case 5:
                        v = "deleted";
                        break;

                    default:
                        v = QString("unknown content status (%1)").arg(status & 255);
                        break;

                    }
                    // working is not supported anymore
                    //switch(status & 0x0000FF00)
                    //{
                    //case 0 * 256:
                    //    v += " (unknown working)";
                    //    break;

                    //case 1 * 256:
                    //    // "not working" is not shown
                    //    break;

                    //case 2 * 256:
                    //    v += " (creating)";
                    //    break;

                    //case 3 * 256:
                    //    v += " (cloning)";
                    //    break;

                    //case 4 * 256:
                    //    v += " (removing)";
                    //    break;

                    //case 5 * 256:
                    //    v += " (updating)";
                    //    break;

                    //default:
                    //    v += QString(" (unknown working status: %1)").arg(status & 0x0000FF00);
                    //    break;

                    //}
                }
                else
                {
                    //v = QString("%1 (%2)").arg(status & 255).arg((status / 256) & 255);
                    v = QString("%1").arg(status & 0x000000FF);
                }
            }
            break;

            case column_type_t::CT_string_value:
            {
                v = value.stringValue().replace("\r", "\\r").replace("\n", "\\n");
            }
            break;

            case column_type_t::CT_rights_value:
            {
                // save the time followed by the list of permissions separated
                // by '\n'
                v = QString("%1 %2")
                        .arg(microseconds_to_string(value.safeInt64Value(), true))
                        .arg(value.stringValue(sizeof(int64_t)).replace("\r", "\\r").replace("\n", "\\n"));
            }
            break;

        }
    }
    catch(std::runtime_error const& e)
    {
        SNAP_LOG_ERROR() << "error: caught a runtime exception dealing with \"" << get_column_name(key) << "\" (" << e.what() << ")";
        // TBD: just rethrow?
        //throw;
        v = "ERROR DETECTED";
    }

    return v;
}


void dbutils::set_column_value( QtCassandra::QCassandraCell::pointer_t c, QString const & v ) const
{
    QtCassandra::QCassandraValue cvalue;
    set_column_value( c->columnKey(), cvalue, v );
    c->setValue( cvalue );
}


void dbutils::set_column_value( const QByteArray& key, QByteArray& value, QString const & v ) const
{
    QtCassandra::QCassandraValue cvalue;
    set_column_value( key, cvalue, v );
    value = cvalue.binaryValue();
}


void dbutils::set_column_value( const QByteArray& key, QtCassandra::QCassandraValue& cvalue, QString const & v ) const
{
    switch( get_column_type( key ) )
    {
        case column_type_t::CT_int8_value:
        {
            cvalue.setSignedCharValue( static_cast<signed char>(v.toInt()) );
        }
        break;

        case column_type_t::CT_uint8_value:
        {
            cvalue.setUnsignedCharValue( static_cast<unsigned char>(v.toUInt()) );
        }
        break;

        case column_type_t::CT_int32_value:
        {
            cvalue.setInt32Value( static_cast<int32_t>(v.toLong()) );
        }
        break;

        case column_type_t::CT_uint32_value:
        {
            cvalue.setUInt32Value( static_cast<uint32_t>(v.toULong()) );
        }
        break;

        case column_type_t::CT_int64_value:
        {
            cvalue.setInt64Value( static_cast<uint64_t>(v.toLongLong()) );
        }
        break;

        case column_type_t::CT_uint64_value:
        {
            cvalue.setUInt64Value( static_cast<uint64_t>(v.toULongLong()) );
        }
        break;

        case column_type_t::CT_float32_value:
        {
            cvalue.setFloatValue( v.toFloat() );
        }
        break;

        case column_type_t::CT_float64_value:
        {
            cvalue.setFloatValue( v.toDouble() );
        }
        break;

        case column_type_t::CT_float64_or_empty_value:
        {
            if(v.isEmpty())
            {
                cvalue.setStringValue( "" );
            }
            else
            {
                cvalue.setFloatValue( v.toDouble() );
            }
        }
        break;

        case column_type_t::CT_time_microseconds:
        {
            cvalue.setInt64Value( string_to_microseconds(v) );
        }
        break;

        case column_type_t::CT_time_microseconds_and_string:
        {
            // String will be of the form: "%Y-%m-%d %H:%M:%S.%N string"
            //
            snap_string_list datetime_split ( v.split(' ') );
            if(datetime_split.size() < 2)
            {
                return;
            }
            snap_string_list const date_split     ( datetime_split[0].split('-') );
            snap_string_list const time_split     ( datetime_split[1].split(':') );
            if(date_split.size() != 3)
            {
                return;
            }
            if(time_split.size() != 3)
            {
                return;
            }
            datetime_split.removeFirst();
            datetime_split.removeFirst();
            QString const str(datetime_split.join(" "));
            //
            tm to;
            to.tm_sec  = time_split[2].toInt();
            to.tm_min  = time_split[1].toInt();
            to.tm_hour = time_split[0].toInt();
            to.tm_mday = date_split[2].toInt();
            to.tm_mon  = date_split[1].toInt() - 1;
            to.tm_year = date_split[0].toInt() - 1900; // TODO handle the microseconds decimal number

            int64_t ns((time_split[2].toDouble() - to.tm_sec) * 1000000.0);
            //
            time_t const tt( mkgmtime( &to ) );

            // concatenate the result
            QByteArray tms;
            QtCassandra::appendInt64Value( tms, tt * 1000000 + ns );
            QtCassandra::appendStringValue( tms, str );
            cvalue.setBinaryValue(tms);
        }
        break;

        case column_type_t::CT_priority_and_time_microseconds_and_string:
        {
            // String will be of the form: "priority %Y-%m-%d %H:%M:%S.%N string"
            //
            snap_string_list datetime_split ( v.split(' ') );
            if(datetime_split.size() < 3)
            {
                return;
            }
            QString          const priority_str   ( datetime_split[0] );
            snap_string_list const date_split     ( datetime_split[1].split('-') );
            snap_string_list const time_split     ( datetime_split[2].split(':') );

            bool ok(false);
            int priority(priority_str.toInt(&ok, 10));
            if(!ok)
            {
                return;
            }
            if(date_split.size() != 3)
            {
                return;
            }
            if(time_split.size() != 3)
            {
                return;
            }

            datetime_split.removeFirst();
            datetime_split.removeFirst();
            datetime_split.removeFirst();
            QString const str(datetime_split.join(" "));
            //
            tm to;
            to.tm_sec  = time_split[2].toInt();
            to.tm_min  = time_split[1].toInt();
            to.tm_hour = time_split[0].toInt();
            to.tm_mday = date_split[2].toInt();
            to.tm_mon  = date_split[1].toInt() - 1;
            to.tm_year = date_split[0].toInt() - 1900; // TODO handle the microseconds decimal number

            int64_t ns((time_split[2].toDouble() - to.tm_sec) * 1000000.0);
            //
            time_t const tt( mkgmtime( &to ) );

            // concatenate the result
            QByteArray tms;
            QtCassandra::appendUnsignedCharValue( tms, static_cast<unsigned char>(priority) );
            QtCassandra::appendInt64Value( tms, tt * 1000000 + ns );
            QtCassandra::appendStringValue( tms, str );
            cvalue.setBinaryValue(tms);
        }
        break;

        case column_type_t::CT_time_seconds:
        {
            // String will be of the form: "%Y-%m-%d %H:%M:%S"
            //
            snap_string_list const datetime_split ( v.split(' ') );
            if(datetime_split.size() < 2)
            {
                return;
            }
            snap_string_list const date_split     ( datetime_split[0].split('-') );
            snap_string_list const time_split     ( datetime_split[1].split(':') );
            if(date_split.size() != 3)
            {
                return;
            }
            if(time_split.size() != 3)
            {
                return;
            }
            //
            tm to;
            to.tm_sec  = time_split[2].toInt();
            to.tm_min  = time_split[1].toInt();
            to.tm_hour = time_split[0].toInt();
            to.tm_mday = date_split[2].toInt();
            to.tm_mon  = date_split[1].toInt() - 1;
            to.tm_year = date_split[0].toInt() - 1900;
            //
            time_t const tt( mkgmtime( &to ) );
            cvalue.setUInt64Value( tt );
        }
        break;

        case column_type_t::CT_hexarray_value:
        case column_type_t::CT_hexarray_limited_value:
        {
            cvalue.setBinaryValue( string_to_key( v ) );
        }
        break;

        case column_type_t::CT_md5array_value:
        {
            cvalue.setBinaryValue( string_to_key( v ) );
        }
        break;

        case column_type_t::CT_secure_value:
        {
            signed char cv;
            if( v == "not checked (-1)" || v == "-1" )
            {
                cv = -1;
            }
            else if( v == "not secure (0)" || v == "0" )
            {
                cv = 0;
            }
            else if( v == "secure (1)" || v == "1" )
            {
                cv = 1;
            }
            else
            {
                throw snap_exception( "error: unknown secure value! Must be -1, 0 or 1!" );
            }
            cvalue.setSignedCharValue( cv );
        }
        break;

        case column_type_t::CT_status_value:
        {
            uint32_t cv;
            QString state_name(v);
            //int pos(v.indexOf("("));
            //if(pos != -1)
            //{
            //    state_name = state_name.left(pos).trimmed();
            //}
            state_name = state_name.trimmed();
            if(state_name == "0" || state_name == "unknown" || state_name == "unknown state")
            {
                cv = 0;
            }
            else if(state_name == "1" || state_name == "create")
            {
                cv = 1;
            }
            else if(state_name == "2" || state_name == "normal")
            {
                cv = 2;
            }
            else if(state_name == "3" || state_name == "hidden")
            {
                cv = 3;
            }
            else if(state_name == "4" || state_name == "moved")
            {
                cv = 4;
            }
            else if(state_name == "5" || state_name == "deleted")
            {
                cv = 5;
            }
            else
            {
                throw snap_exception( "error: unknown status state value! Must be between 0 and +5 or a valid name!" );
            }
            //if(pos != -1)
            //{
            //    // there is processing status
            //    QString working_name(v.mid(pos + 1).trimmed());
            //    if(working_name.right(1) == ")")
            //    {
            //        working_name.remove(working_name.length() - 1, 1);
            //    }
            //    if(working_name == "0" || working_name == "unknown" || working_name == "unknown working")
            //    {
            //        cv |= 0 * 256;
            //    }
            //    else if(working_name == "1" || working_name == "not working")
            //    {
            //        cv |= 1 * 256;
            //    }
            //    else if(working_name == "2" || working_name == "creating")
            //    {
            //        cv |= 2 * 256;
            //    }
            //    else if(working_name == "3" || working_name == "cloning")
            //    {
            //        cv |= 3 * 256;
            //    }
            //    else if(working_name == "4" || working_name == "removing")
            //    {
            //        cv |= 4 * 256;
            //    }
            //    else if(working_name == "5" || working_name == "updating")
            //    {
            //        cv |= 5 * 256;
            //    }
            //}
            //else
            //{
            //    // use not working by default
            //    cv |= 1 * 256;
            //}
            cvalue.setUInt32Value( cv );
        }
        break;

        case column_type_t::CT_string_value:
        {
            // all others viewed as strings
            //v = c->value().stringValue().replace("\n", "\\n");
            QString convert( v );
            cvalue.setStringValue( convert.replace( "\\r", "\r" ).replace( "\\n", "\n" ) );
        }
        break;

        case column_type_t::CT_rights_value:
        {
            // save the time followed by the list of permissions separated
            // by '\n'
            QString convert( v );
            QByteArray buffer;
            QtCassandra::setInt64Value( buffer, string_to_microseconds(v) );
            int pos(v.indexOf(')'));
            if(pos > 0)
            {
                ++pos;
                while(v[pos].isSpace())
                {
                    ++pos;
                }
                QtCassandra::appendStringValue( buffer, v.mid(pos + 1) );
            }
            cvalue.setBinaryValue(buffer);
        }
        break;
    }
}


}
// namespace snap

// vim: ts=4 sw=4 et
