/*
 * Text:
 *      snapbackup.cpp
 *
 * Description:
 *      Reads and describes a Snap database. This ease checking out the
 *      current content of the database as the cassandra-cli tends to
 *      show everything in hexadecimal number which is quite unpractical.
 *      Now we do it that way for runtime speed which is much more important
 *      than readability by humans, but we still want to see the data in an
 *      easy practical way which this tool offers.
 *
 * License:
 *      Copyright (c) 2012-2017 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// our lib
//
#include "snapbackup.h"
#include "snap_table_list.h"

#include <casswrapper/schema.h>
#include <casswrapper/qstring_stream.h>

// 3rd party libs
//
#include <QtCore>
#include <QtSql>

// system
//
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace casswrapper;
using namespace casswrapper::schema;

snapbackup::snapbackup( getopt_ptr_t opt )
    : f_session( Session::create() )
    , f_opt(opt)
{
}


void snapbackup::setSqliteDbFile( const QString& sqlDbFile )
{
    QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );
    if( !db.isValid() )
    {
        const QString error( "QSQLITE database is not valid for some reason!" );
        std::cerr << "QSQLITE not valid!!!" << std::endl;
        throw std::runtime_error( error.toUtf8().data() );
    }
    db.setDatabaseName( sqlDbFile );
    if( !db.open() )
    {
        const QString error( QString("Cannot open SQLite database [%1]!").arg(sqlDbFile) );
        std::cerr << "QSQLITE not open!!!" << std::endl;
        throw std::runtime_error( error.toUtf8().data() );
    }
}


void snapbackup::connectToCassandra()
{
    f_session->setLowWaterMark  ( f_opt->get_long("low-watermark")  );
    f_session->setHighWaterMark ( f_opt->get_long("high-watermark") );

    f_session->connect
            ( f_opt->get_string("host").c_str()
            , f_opt->get_long("port")
            , f_opt->is_defined("use-ssl")
            );
}

void snapbackup::exec( QSqlQuery& q )
{
    if( !q.exec() )
    {
        std::cerr << "lastQuery=[" << q.lastQuery() << "]" << std::endl;
        std::cerr << "query error=[" << q.lastError().text() << "]" << std::endl;
        throw std::runtime_error( q.lastError().text().toUtf8().data() );
    }
}

void snapbackup::storeSchemaEntry( const QString& description, const QString& name, const QString& schema_line )
{
    const QString q_str =
        "INSERT OR REPLACE INTO cql_schema_list "
        "(name,description,schema_line) "
        "VALUES "
        "(:name,:description,:schema_line);"
        ;
    QSqlQuery q;
    q.prepare( q_str );
    //
    q.bindValue( ":name",        name        );
    q.bindValue( ":description", description );
    q.bindValue( ":schema_line", schema_line );
    //
    exec( q );
}


void snapbackup::storeSchema( const QString& context_name )
{
    std::cout << "Generating CQL schema blob..." << std::endl;
    SessionMeta::pointer_t sm( SessionMeta::create(f_session) );
    sm->loadSchema();
    const auto& keyspaces( sm->getKeyspaces() );
    auto snap_iter = keyspaces.find(context_name);
    if( snap_iter == keyspaces.end() )
    {
        throw std::runtime_error(
                QString("Context '%1' does not exist! Aborting!")
                .arg(context_name).toUtf8().data()
                );
    }

    auto kys( snap_iter->second );

    std::cout << "Creating CQL schema blob table..." << std::endl;
    QString q_str = "CREATE TABLE IF NOT EXISTS cql_schema_list "
            "( name TEXT PRIMARY KEY"
            ", description TEXT"
            ", schema_line LONGBLOB"
            ");"
            ;
    QSqlQuery q;
    q.prepare( q_str );
    exec( q );

    std::cout << "Storing schema blob..." << std::endl;
    storeSchemaEntry( "keyspace", context_name, kys->getKeyspaceCql() );
    for( const auto& line : kys->getTablesCql() )
    {
        storeSchemaEntry( "table", line.first, line.second );
    }
}


void snapbackup::restoreSchema( const QString& context_name )
{
    SessionMeta::pointer_t sm( SessionMeta::create(f_session) );
    sm->loadSchema();
    const auto& keyspaces( sm->getKeyspaces() );
    auto snap_iter = keyspaces.find(context_name);
    if( snap_iter != keyspaces.end() )
    {
        if( f_opt->is_defined("force-schema-creation") )
        {
            std::cout << "Context " << context_name << " already exists, but forcing (re)creation as requested." << std::endl;
        }
        else
        {
            std::cout << "Context " << context_name << " already exists, so skipping schema creation." << std::endl;
            return;
        }
    }

    std::cout << "Restoring CQL schema blob..." << std::endl;
    const QString q_str( "SELECT name, description, schema_line FROM cql_schema_list;" );
    QSqlQuery q;
    q.prepare( q_str );
    //
    exec( q );

    std::cout << "Creating keyspace '" << context_name << "', and tables." << std::endl;
    const int name_idx    = q.record().indexOf("name");
    const int desc_idx    = q.record().indexOf("description");
    const int schema_idx  = q.record().indexOf("schema_line");
    for( q.first(); q.isValid(); q.next() )
    {
        const QString desc( q.value(desc_idx).toString() );
        const QString name( q.value(name_idx).toString() );
        const QString schema_string( q.value( schema_idx ).toString() );

        auto cass_query = Query::create( f_session );
        cass_query->query( schema_string );
        cass_query->start( false );
        std::cout << "Creating " << desc << " " << name;
        while( !cass_query->isReady() )
        {
            std::cout << "." << std::flush;
            sleep(1);
        }
        std::cout << "done!" << std::endl;
        cass_query->end();
    }

    std::cout << std::endl << "Database creation finished!" << std::endl;
}


void snapbackup::dropContext( const QString& context_name )
{
    std::cout << QString("Dropping context [%1]...").arg(context_name) << std::endl;

    SessionMeta::pointer_t sm( SessionMeta::create(f_session) );
    sm->loadSchema();
    const auto& keyspaces( sm->getKeyspaces() );
    auto snap_iter = keyspaces.find(context_name);
    if( snap_iter == keyspaces.end() )
    {
        throw std::runtime_error(
                QString("Context '%1' does not exist! Aborting!")
                .arg(context_name).toUtf8().data()
                );
    }

    auto kys( snap_iter->second );

    for( auto table : kys->getTables() )
    {
        const auto table_name( table.first );

        auto q( Query::create(f_session) );
        q->query( QString("DROP TABLE %1.%2").arg(context_name).arg(table_name) );
        q->start( false );
        std::cout << "Dropping table " << table_name;
        while( !q->isReady() )
        {
            std::cout << "." << std::flush;
            sleep(1);
        }
        std::cout << "dropped!" << std::endl;
    }

    {
        auto q( Query::create(f_session) );
        q->query( QString("DROP KEYSPACE %1").arg(context_name) );
        q->start( false );
        std::cout << "Dropping keyspace " << context_name;
        while( !q->isReady() )
        {
            std::cout << "." << std::flush;
            sleep(1);
        }
        std::cout << "dropped!" << std::endl;
    }

    std::cout << std::endl << "Context successfully dropped!" << std::endl;
}


void snapbackup::dumpContext()
{
    setSqliteDbFile( f_opt->get_string("dump-context").c_str() );

    auto db( QSqlDatabase::database() );
    db.transaction();
    const QString context_name( f_opt->get_string("context-name").c_str() );
    storeSchema( context_name );
    storeTables( f_opt->get_long("count"), context_name );
    db.commit();
}


void snapbackup::dropContext()
{
    const QString context_name( f_opt->get_string("context-name").c_str() );
    dropContext( context_name );
}


void snapbackup::restoreContext()
{
    setSqliteDbFile( f_opt->get_string("restore-context").c_str() );

    const QString context_name( f_opt->get_string("context-name").c_str() );
    restoreSchema( context_name );
    restoreTables( context_name );
}


void snapbackup::appendRowsToSqliteDb( int& id, Query::pointer_t cass_query, const QString& table_name )
{
    const QString q_str = QString( "INSERT OR REPLACE INTO %1 "
            "(id, key, column1, value ) "
            "VALUES "
            "(:id, :key, :column1, :value );"
            ).arg(table_name);
    QSqlQuery q;
    q.prepare( q_str );
    //
    q.bindValue( ":id",      id++                                      );
    q.bindValue( ":key",     cass_query->getByteArrayColumn("key")     );
    q.bindValue( ":column1", cass_query->getByteArrayColumn("column1") );
    q.bindValue( ":value",   cass_query->getByteArrayColumn("value")   );
    //
    exec( q );
}

/// \brief Backup snap_websites tables.
//
// This does not dump the Cassandra schema. In order to obtain this, run the following command on a
// Cassandra node:
//
//      cqlsh -e "DESCRIBE snap_websites" > schema.sql
// 
// The above command creates an SQL file that can be reimported into your Cassandra node.
//
// Then you can call this method.
//
void snapbackup::storeTables( const int count, const QString& context_name )
{
    snapTableList   dump_list;

    SessionMeta::pointer_t sm( SessionMeta::create(f_session) );
    sm->loadSchema();
    const auto& keyspaces( sm->getKeyspaces() );
    auto snap_iter = keyspaces.find(context_name);
    if( snap_iter == keyspaces.end() )
    {
        throw std::runtime_error(
                QString("Context '%1' does not exist! Aborting!")
                .arg(context_name).toUtf8().data()
                );
    }

    auto kys( snap_iter->second );

    QStringList tables_to_dump;
    if( f_opt->is_defined("tables") )
    {
        for( int idx = 0; idx < f_opt->size("tables"); ++idx )
        {
            tables_to_dump << QString(f_opt->get_string( "tables", idx ).c_str());
        }

        dump_list.overrideTablesToDump( tables_to_dump );
    }

    auto tables_to_ignore( dump_list.tablesToIgnore() );

    for( auto table : kys->getTables() )
    {
        const auto table_name( table.first );

        if( tables_to_ignore.contains(table_name) )
        {
            continue;
        }

        if( !tables_to_dump.isEmpty() )
        {
            if( !tables_to_dump.contains(table_name) )
            {
                continue;
            }
        }

        QString q_str = QString( "CREATE TABLE IF NOT EXISTS %1 "
                "( id INTEGER PRIMARY KEY"
                ", key LONGBLOB"
                ", column1 LONGBLOB"
                ", value LONGBLOB"
                ");"
                ).arg(table_name);
        QSqlQuery q;
        q.prepare( q_str );
        exec( q );

        std::cout << "Dumping table [" << table_name << "]" << std::endl;

        const QString cql_select_string("SELECT key,column1,value FROM snap_websites.%1");
        q_str = cql_select_string.arg(table_name);

        auto cass_query = Query::create( f_session );
        cass_query->query( q_str );
        cass_query->setPagingSize( count );
        cass_query->start();

        int id = 1;
        do
        {
            while( cass_query->nextRow() )
            {
                appendRowsToSqliteDb( id, cass_query, table_name );
            }
        }
        while( cass_query->nextPage() );
    }
}


/// \brief Restore snap_websites tables.
//
// This assumes that the Cassandra schema has been created already. On backup, follow the instructions
// above snapbackup::storeTable() to create your schema.sql file. Then dump the database.
//
// In order to restore, drop the "snap_websites" context on the Cassandra node you wish to restore.
// Then run the following commands:
//
//      snapdb --drop-context
//      cqlsh -f schema.sql
//
// Then call this method.
//
void snapbackup::restoreTables( const QString& context_name )
{
    snapTableList   dump_list;

    SessionMeta::pointer_t sm( SessionMeta::create(f_session) );
    sm->loadSchema();
    const auto& keyspaces( sm->getKeyspaces() );
    auto snap_iter = keyspaces.find(context_name);
    if( snap_iter == keyspaces.end() )
    {
        throw std::runtime_error(
                QString("Context '%1' does not exist! Aborting!")
                .arg(context_name).toUtf8().data()
                );
    }

    auto kys( snap_iter->second );

    QStringList tables_to_restore;
    if( f_opt->is_defined("tables") )
    {
        for( int idx = 0; idx < f_opt->size("tables"); ++idx )
        {
            tables_to_restore << QString(f_opt->get_string( "tables", idx ).c_str());
        }

        dump_list.overrideTablesToDump( tables_to_restore );
    }

    auto tables_to_ignore( dump_list.tablesToIgnore() );

    for( auto table : kys->getTables() )
    {
        const auto table_name( table.first );

        if( tables_to_ignore.contains(table_name) )
        {
            continue;
        }

        if( !tables_to_restore.isEmpty() )
        {
            if( !tables_to_restore.contains(table_name) )
            {
                continue;
            }
        }

        std::cout << "Restoring table [" << table_name << "]" << std::endl;

        const QString sql_select_string ("SELECT key,column1,value FROM %1");
        QString q_str                   ( sql_select_string.arg(table_name) );
        QSqlQuery q;
        q.prepare( q_str );
        exec( q );

        const int key_idx       = q.record().indexOf("key");
        const int column1_idx   = q.record().indexOf("column1");
        const int value_idx     = q.record().indexOf("value");
        for( q.first(); q.isValid(); q.next() )
        {
            const QByteArray key       ( q.value( key_idx       ).toByteArray() );
            const QByteArray column1   ( q.value( column1_idx   ).toByteArray() );
            const QByteArray value     ( q.value( value_idx     ).toByteArray() );

            const QString cql_insert_string("INSERT INTO snap_websites.%1 (key,column1,value) VALUES (?,?,?);");
            const QString qstr( cql_insert_string.arg(table_name) );

            auto cass_query = Query::create( f_session );
            cass_query->query( qstr, 3 );
            int bind_num = 0;
            cass_query->bindByteArray( bind_num++, key     );
            cass_query->bindByteArray( bind_num++, column1 );
            cass_query->bindByteArray( bind_num++, value   );

            cass_query->start();
            cass_query->end();
        }
    }
}


// vim: ts=4 sw=4 et
