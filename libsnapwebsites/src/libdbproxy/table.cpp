// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "libdbproxy/libdbproxy.h"
#include "libdbproxy/table.h"
#include "libdbproxy/context.h"

#include <casswrapper/schema.h>


// snapdev lib
//
#include    <snapdev/qstring_extensions.h>


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include <iostream>
#include <stdexcept>
#include <sstream>

#include <QtCore>

#include <unistd.h>


namespace libdbproxy
{


/** \class table
 * \brief Defines a table and may hold a Cassandra column family definition.
 *
 * In Cassandra, a table is called a column family. Mainly because
 * each row in a Cassandra table can have a different set of columns
 * whereas a table is usually viewed as a set of rows that all have
 * the exact same number of columns (but really, even in SQL the set
 * of columns can be viewed as varying since columns can be set to
 * NULL and that has [nearly] the same effect as not defining a column
 * in Cassandra.)
 *
 * This class defines objects that can hold all the column family
 * information so as to create new ones and read existing ones.
 *
 * The name of a table is very limited (i.e. letters, digits, and
 * underscores, and the name must start with a letter.) The maximum
 * length is not specified, but it is relatively short as it is
 * used as a filename that will hold the data of the table.
 *
 * Whenever trying to get a row, the default behavior is to create
 * a new row if it doesn't exist yet. If you need to know whether a
 * row exists, make sure you use the exists() function.
 *
 * \note
 * A table can be created, updated, and dropped. In all those cases, the
 * functions return once the Cassandra instance with which you are
 * connected is ready.
 *
 * \sa exists()
 */


/** \var table::f_from_cassandra
 * \brief Whether the table is a memory table or a server table.
 *
 * A table read from the Cassandra server or created with the
 * create() function is marked as being from Cassandra.
 * All other tables are considered memory tables.
 *
 * A memory table can be used as a set of global variables
 * with a format similar to a Cassandra table.
 *
 * If you define a new table with the intend to call the
 * create() function, avoid saving data in the new table
 * as it won't make it to the database. (This may change
 * in the future though.)
 */

/** \var table::f_schema
 * \brief The table private data: CfDef
 *
 * A table is always part of a specific context. You can only create a
 * new table using a function from your context objects.
 *
 * This is a bare pointer since you cannot delete the context and hope
 * the table remains (i.e. when the context goes, the table goes!)
 */

/** \var table::f_context
 * \brief The context that created this table.
 *
 * A table is always part of a specific context. You can only create a
 * new table using a function from your context objects.
 *
 * This is a bare pointer since you cannot delete the context and hope
 * the table remains (i.e. when the context goes, the table goes!)
 * However, you may keep a shared pointer to a table after the table
 * was deleted. In that case, the f_context pointer is set to NULL
 * and calling functions on that table may result in an exception
 * being raised.
 */

/** \var table::f_rows
 * \brief Set of rows.
 *
 * The system caches rows in this map. The map index is the row key. You can
 * clear the table using the clearCache() function.
 */

/** \brief Initialize a table object.
 *
 * This function initializes a table object.
 *
 * All the parameters are set to the defaults as defined in the Cassandra
 * definition of the CfDef message. You can use the different functions to
 * change the default values.
 *
 * Note that the context and table name cannot be changed later. These
 * are fixed values that follow the Cassandra behavior.
 *
 * A table name must be composed of letters (A-Za-z), digits (0-9) and
 * underscores (_). It must start with a letter. The corresponding lexical
 * expression is: /^[A-Za-z][A-Za-z0-9_]*$\/
 *
 * The length of a table name is also restricted, however it is not specified
 * by Cassandra. I suggest you never use a name of more than 200 letters.
 * The length is apparently limited by the number of characters that can be
 * used to name a file on the file system you are using.
 *
 * \param[in] context  The context where this table definition is created.
 * \param[in] table_name  The name of the table definition being created.
 */
table::table(context::pointer_t context, const QString& table_name)
    : f_context(context)
{
    // cache the name because we need it for each other we send
    //
    f_context_name = context->contextName();

    // verify the name here (faster than waiting for the server and good documentation)
    //
    // Note: we support uppercase names, however, this is only because
    //       there is still one system table that uses such... uppercase
    //       requires us to use double quotes around names each time we
    //       access a table so it is some extra overhead.
    //
    bool has_quotes(false);
    bool has_uppercase(false);
    bool quotes_are_valid(false);
    int const max(table_name.length());
    for(int idx(0); idx < max; ++idx)
    {
        ushort c(table_name.at(idx).unicode());
        switch(c)
        {
        case '"':
            if(idx == 0)
            {
                has_quotes = true;
            }
            else if(idx == max - 1)
            {
                if(!has_quotes)
                {
                    throw exception(QString("'%1' is not a valid table name (it cannot end with a double quote (\") if it does not start with a double quote.)")
                            .arg(table_name).toUtf8().data());
                }
                quotes_are_valid = true;
            }
            else
            {
                throw exception(QString("'%1' is not a valid table name (a table name can be surrounded by double quotes, but it cannot itself include a double quote.)")
                            .arg(table_name).toUtf8().data());
            }
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '_':
            if(idx == 0
            || (idx == 1 && has_quotes))
            {
                throw exception(QString("'%1' is not a valid table name (a table name cannot start with a digit or an underscore (_), even when quoted.)")
                            .arg(table_name).toUtf8().data());
            }
            break;

        default:
            // lowercase are always fine
            if(c >= 'a' && c <= 'z')
            {
                break;
            }
            if(c >= 'A' && c <= 'Z')
            {
                has_uppercase = true;
                break;
            }
            // the regex shown in the error message is simplified, the double
            // quotes must not appear at all or be defined at the start AND
            // the end
            //
            throw exception(QString("'%1' is an invalid table name (does not match \"?[a-zA-Z][a-zA-Z0-9_]*\"?)").arg(table_name).toUtf8().data());

        }
    }
    if(has_quotes && !quotes_are_valid)
    {
        throw exception(QString("'%1' is not a valid table name (it cannot start with a double quote (\") if it does not end with a double quote.)")
                .arg(table_name).toUtf8().data());
    }

    if(has_uppercase && !has_quotes)
    {
        // surround the name with double quotes...
        f_table_name = QString("\"%1\"").arg(table_name);
    }
    else
    {
        f_table_name = table_name;
    }
    f_proxy = context->parentCassandra()->getProxy();
}


/** \brief Clean up the table object.
 *
 * This function ensures that all resources allocated by the
 * table are released.
 *
 * Note that does not in any way destroy the table in the
 * Cassandra cluster.
 */
table::~table()
{
    try
    {
        // do an explicit clearCache() so we can try/catch otherwise we
        // could get a throw in the destructor
        //
        clearCache();
    }
    catch(const exception&)
    {
        // ignore, not much else we can do in a destructor
    }
}


/** \brief Return the name of the context attached to this table definition.
 *
 * This function returns the name of the context attached to this table
 * definition.
 *
 * Note that it is not possible to rename a context and therefore this
 * name will never change.
 *
 * To get a pointer to the context, use the cluster function context()
 * with this name. Since each context is unique, it will always return
 * the correct pointer.
 *
 * \return The name of the context this table definition is defined in.
 */
const QString& table::contextName() const
{
    return f_context_name;
}


/** \brief Retrieve the name of this table.
 *
 * This function returns the name of this table. Note that the
 * name cannot be changed.
 *
 * \return The table name.
 */
QString table::tableName() const
{
    if(f_table_name[0] == '"')
    {
        // remove the quotes if present
        return f_table_name.mid(1, f_table_name.length() - 2);
    }
    return f_table_name;
}


const casswrapper::schema::Value::map_t& table::fields() const
{
    return f_schema->getFields();
}


casswrapper::schema::Value::map_t& table::fields()
{
    return f_schema->getFields();
}


/** \brief Mark this table as from Cassandra.
 *
 * This very case happens when the user creates a new context that,
 * at the time of calling context::create(), includes
 * a list of table definitions.
 *
 * In that case we know that the context is being created, but not
 * the tables because the server does it transparently in one go.
 */
void table::setFromCassandra()
{
    f_from_cassandra = true;
}

/** \brief This is an internal function used to parse a CfDef structure.
 *
 * This function is called internally to parse a CfDef object.
 * The data is saved in this table.
 *
 * \param[in] data  The pointer to the CfDef object.
 */
void table::parseTableDefinition( casswrapper::schema::TableMeta::pointer_t table_meta )
{
    f_schema         = table_meta;
    f_from_cassandra = true;
}

#if 0
/** \brief Prepare a table definition.
 *
 * This function transforms a libdbproxy table definition into
 * a Cassandra CfDef structure.
 *
 * The parameter is passed as a void * because we do not want to define
 * the thrift types in our public headers.
 *
 * \param[in] data  The CfDef were the table is to be saved.
 */
void table::prepareTableDefinition( CfDef* cf ) const
{
    *cf = *f_private;

    // copy the columns
    cf->column_metadata.clear();
    for( auto c : f_column_definitions )
    {
        ColumnDef col;
        c->prepareColumnDefinition( &col );
        cf->column_metadata.push_back(col);
    }
    cf->__isset.column_metadata             = !cf->column_metadata.empty();
    cf->__isset.compaction_strategy_options = !cf->compaction_strategy_options.empty();
    cf->__isset.compression_options         = !cf->compression_options.empty();
}
#endif


namespace
{
    QString map_to_json( const std::map<std::string,std::string>& map )
    {
        QString ret;
        for( const auto& pair : map )
        {
            if( !ret.isEmpty() )
            {
                ret += ",";
            }
            ret += QString("'%1':'%2'").arg(pair.first.c_str()).arg(pair.second.c_str());
        }
        return ret;
    }
}


QString table::getTableOptions() const
{
    QString query_string;
    for( const auto& pair : f_schema->getFields() )
    {
        query_string += QString("AND %1=%2\n")
                .arg(pair.first)
                .arg(pair.second.output())
                ;
    }

    return query_string;
}


/** \brief Create a Cassandra table.
 *
 * This function creates a Cassandra table in the context as specified
 * when you created the table object.
 *
 * If you want to declare a set of columns, this is a good time to do
 * it too (there is not QColumnDefinition::create() function!) By
 * default, columns use the default validation type as defined using
 * the setComparatorType() for their name and the
 * setDefaultValidationClass() for their data. It is not required to
 * define any column. In that case they all make use of the exact
 * same data.
 *
 * The table cannot already exist or an error will be thrown by the
 * Cassandra server. If the table is being updated, use the update()
 * function instead.
 *
 * Note that when you create a new context, you can create its tables
 * at once by defining tables before calling the context::create()
 * function.
 *
 * Creating a new table:
 *
 * \code
 * libdbproxy::table::pointer_t table(context->table("qt_cassandra_test_table"));
 * table->setComment("Our test table.");
 * table->setColumnType("Standard"); // Standard or Super
 * table->setKeyValidationClass("BytesType");
 * table->setDefaultValidationClass("BytesType");
 * table->setComparatorType("BytesType");
 * table->setKeyCacheSavePeriodInSeconds(14400); // unused in 1.1+
 * table->setMemtableFlushAfterMins(60); // unused in 1.1+
 * // Memtable defaults are dynamic and usually a better bet
 * //table->setMemtableThroughputInMb(247); // unused in 1.1+
 * //table->setMemtableOperationsInMillions(1.1578125); // unused in 1.1+
 * table->setGcGraceSeconds(864000);
 * table->setMinCompactionThreshold(4);
 * table->setMaxCompactionThreshold(22);
 * table->setReplicateOnWrite(1);
 * table->create();
 * \endcode
 *
 * \note
 * Once the table->create(); function returns, the table was created in the
 * Cassandra node you are connect with, but it was not yet replicated. In
 * order to use the table, the replication automatically happens behind the scenes.
 * In previous version of Cassandra, it was necessary to wait for this replication
 * to finish, but now with modern versions, that is no longer and issue.
 *
 * \sa update()
 * \sa context::create()
 */
void table::create()
{
    // TODO: this is actually wrong because it only creates the table
    //       it should be capable of either creating the table or altering
    //       it because the libQtCassandra user may have changed some
    //       parameters
    //
    //       so if the table exists, we should switch to ALTER TABLE ...
    //       command instead (for Snap! we do not ever tweak table
    //       parameters dynamically, so we are good for now.)
    //
    QString query_string( QString( "CREATE TABLE IF NOT EXISTS %1.%2"
        "(key BLOB,column1 BLOB,value BLOB,PRIMARY KEY(key, column1))"
        "WITH COMPACT STORAGE"
        " AND CLUSTERING ORDER BY(column1 ASC)" )
                .arg(f_context_name)
                .arg(f_table_name)
            );
    query_string += getTableOptions();

    // 1) Load existing tables from the database,
    // 2) Create the table using the query string,
    // 3) Add this object into the list.
    //
    order create_table;
    create_table.setCql(query_string, order::type_of_result_t::TYPE_OF_RESULT_SUCCESS);
    create_table.setTimeout(5 * 60 * 1000);
    create_table.setClearClusterDescription(true);
    order_result const create_table_result(f_proxy->sendOrder(create_table));
    if(!create_table_result.succeeded())
    {
        throw exception("table creation failed");
    }

    f_from_cassandra = true;
}


/** \brief Truncate a Cassandra table.
 *
 * The truncate() function removes all the rows from a Cassandra table
 * and clear out the cached data (rows and cells.)
 *
 * If the table is not connected to Cassandra, then nothing happens with
 * the Cassandra server.
 *
 * If you want to keep a copy of the cache, you will have to retrieve a
 * copy of the rows map using the getRows() function.
 *
 * \sa getRows()
 * \sa clearCache()
 */
void table::truncate()
{
    if( !f_from_cassandra )
    {
        return;
    }

    const QString query_string(
        QString("TRUNCATE %1.%2")
            .arg(f_context_name)
            .arg(f_table_name)
            );

    order truncate_table;
    truncate_table.setCql(query_string, order::type_of_result_t::TYPE_OF_RESULT_SUCCESS);
    truncate_table.setClearClusterDescription(true);
    order_result const truncate_table_result(f_proxy->sendOrder(truncate_table));
    if(!truncate_table_result.succeeded())
    {
        throw exception("table truncation failed");
    }

    clearCache();
}


/** \brief Clear the memory cache.
 *
 * This function clears the memory cache. This means all the rows and
 * their cells will be deleted from this table. The memory cache doesn't
 * affect the Cassandra database.
 *
 * After a clear, you can retrieve fresh data (i.e. by directly loading the
 * data from the Cassandra database.)
 *
 * Note that if you kept shared pointers to rows and cells defined in
 * this table, accessing those is likely going to generate an exception.
 */
void table::clearCache()
{
    closeCursor();

    f_rows.clear();
}


/** \brief Close the current cursor.
 *
 * This function closes the current cursor (i.e. the cursor used
 * to gather a set of rows and their data from a table.)
 */
void table::closeCursor()
{
    if(f_cursor_index >= 0)
    {
        // Note: the "CLOSE" CQL string is ignored
        //
        order close_cursor;
        close_cursor.setCql("CLOSE", order::type_of_result_t::TYPE_OF_RESULT_CLOSE);
        close_cursor.setCursorIndex(f_cursor_index);
        f_cursor_index = -1;
        order_result close_cursor_result(f_proxy->sendOrder(close_cursor));
        if(!close_cursor_result.succeeded())
        {
            throw exception("table::closeCursor(): closing cursor failed.");
        }
    }
}


void table::addRow( const QByteArray& row_key, const QByteArray& column_key, const QByteArray& data )
{
    row::pointer_t  new_row  ( new row( shared_from_this(), row_key ) );
    cell::pointer_t new_cell ( new_row->getCell( column_key ) );
    new_cell->assignValue( value(data) );

    // Now add to the map.
    //
    f_rows[row_key] = new_row;
}


void table::startBatch()
{
    order start_batch;
    start_batch.setCql("START_BATCH", order::type_of_result_t::TYPE_OF_RESULT_BATCH_DECLARE);

    order_result start_batch_result(f_proxy->sendOrder(start_batch));
    start_batch_result.swap(start_batch_result);
    if(!start_batch_result.succeeded())
    {
        throw exception("start batch failed");
    }

    f_batch_index = int32Value(start_batch_result.result(0));
    if(f_batch_index < 0)
    {
        throw logic_exception("received a negative number as batch index!");
    }
}


void table::commitBatch()
{
    if(f_batch_index >= 0)
    {
        // Note: the "CLOSE" CQL string is ignored
        //
        order commit_batch;
        commit_batch.setCql("COMMIT_BATCH", order::type_of_result_t::TYPE_OF_RESULT_BATCH_COMMIT);
        commit_batch.setBatchIndex(f_batch_index);
        f_batch_index = -1;
        order_result commit_batch_result(f_proxy->sendOrder(commit_batch));
        if(!commit_batch_result.succeeded())
        {
            throw exception("table::commitBatch(): batch submission failed.");
        }
    }
}


void table::rollbackBatch()
{
    if(f_batch_index >= 0)
    {
        order batch;
        batch.setCql("ROLLBACK_BATCH", order::type_of_result_t::TYPE_OF_RESULT_BATCH_ROLLBACK);
        batch.setBatchIndex(f_batch_index);
        f_batch_index = -1;
        order_result batch_result(f_proxy->sendOrder(batch));
        if(!batch_result.succeeded())
        {
            throw exception("table::commitBatch(): batch submission failed.");
        }
    }
}


/** \brief Read a set of rows as defined by the row predicate.
 *
 * This function reads a set of rows as defined by the row predicate.
 *
 * To change the consistency for this read, check out the
 * cell_predicate::setConsistencyLevel() function.
 *
 * If the table is not connected to Cassandra (i.e. the table is
 * a memory table) then nothing happens.
 *
 * Remember that if you are querying without checking for any column
 * you will get "empty" rows in your results (see dropRow() function
 * for more information and search for TombStones in Cassandra.)
 * This was true in version 0.8.0 to 1.1.5. It may get fixed at some
 * point.
 *
 * Note that the function updates the predicate so the next call
 * returns the following rows as expected.
 *
 * \warning
 * This function MAY NOT "WORK RIGHT" if your cluster was defined using
 * the RandomPartitioner. Rows are not sorted by key when the
 * RandomPartitioner is used. Instead, the rows are sorted by their
 * MD5 sum. Also the system may add additional data before or
 * after that MD5 and the slice range cannot anyway provide that
 * MD5 to the system. If you want to query sorted slices of your
 * rows, you must create your cluster with another partitioner.
 * Search for partitioner in conf/cassandra.yaml in the
 * Cassandra tarball.
 * See also: http://ria101.wordpress.com/2010/02/22/cassandra-randompartitioner-vs-orderpreservingpartitioner/
 *
 * \param[in,out] row_predicate  The row predicate, which has to be allocated on the heap using shared_ptr.
 *
 * \return The number of rows read.
 *
 * \sa row_predicate (see detailed description of row predicate for an example)
 * \sa cell_predicate::setConsistencyLevel()
 * \sa dropRow()
 */
uint32_t table::readRows( row_predicate::pointer_t row_predicate )
{
    if( !row_predicate )
    {
        throw exception("row_predicate is nullptr!");
    }

    size_t idx(0);
    order_result selected_rows_result;

    f_rows.clear();

    if( f_cursor_index != -1 )
    {
        // Note: the "FETCH" is ignored, only the type is used in this case
        //
        order select_more_rows;
        select_more_rows.setCql("FETCH", order::type_of_result_t::TYPE_OF_RESULT_FETCH);
        select_more_rows.setCursorIndex(f_cursor_index);
        order_result select_more_rows_result(f_proxy->sendOrder(select_more_rows));
        selected_rows_result.swap(select_more_rows_result);
        if(!selected_rows_result.succeeded())
        {
            throw exception("select rows failed");
        }

        if(selected_rows_result.resultCount() == 0)
        {
            closeCursor();
            return 0;
        }
    }
    else
    {
        QString query_string( QString("SELECT key,column1,value FROM %1.%2")
                       .arg(f_context_name)
                       .arg(f_table_name)
                       );
        // Note: with the proxy we do not care about the bind_count
        //       but the appendQuery() function does the same thing
        //
        int bind_count = 0;
        row_predicate->appendQuery( query_string, bind_count );
        if(row_predicate->allowFiltering())
        {
            query_string += " ALLOW FILTERING";
        }
        //
//std::cerr << "query=[" << query_string.toUtf8().data() << "]" << std::endl;

        // setup the consistency level
        consistency_level_t consistency_level( parentContext()->parentCassandra()->defaultConsistencyLevel() );
        consistency_level_t const default_consistency_level(consistency_level);
        consistency_level = row_predicate->consistencyLevel();
        if( consistency_level == CONSISTENCY_LEVEL_DEFAULT )
        {
            consistency_level = row_predicate->cellPredicate()->consistencyLevel();
            if( consistency_level == CONSISTENCY_LEVEL_DEFAULT )
            {
                consistency_level = default_consistency_level;
            }
        }

        // create a CURSOR
        order select_rows;
        select_rows.setCql(query_string, order::type_of_result_t::TYPE_OF_RESULT_DECLARE);
        select_rows.setColumnCount(3);
        select_rows.setConsistencyLevel(consistency_level);

        //
        row_predicate->bindOrder( select_rows );
        select_rows.setPagingSize( row_predicate->count() );

        order_result select_rows_result(f_proxy->sendOrder(select_rows));
        selected_rows_result.swap(select_rows_result);
        if(!selected_rows_result.succeeded())
        {
            throw exception("select rows failed");
        }

        if(selected_rows_result.resultCount() < 1)
        {
            throw exception("select rows did not return a cursor index");
        }
        f_cursor_index = int32Value(selected_rows_result.result(0));
        if(f_cursor_index < 0)
        {
            throw logic_exception("received a negative number as cursor index");
        }
//std::cerr << "got a cursor = " << f_cursor_index << "\n";

        // ignore parameter one, it is not a row of data
        idx = 1;
    }

    auto re(row_predicate->rowNameMatch());

    size_t const max_results(selected_rows_result.resultCount());
#ifdef _DEBUG
    if((max_results - idx) % 3 != 0)
    {
        // the number of results must be a multiple of 3, although on
        // the SELECT (first time in) we expect one additional result
        // which represents the cursor index
        throw logic_exception("the number of results must be an exact multipled of 3!");
    }
#endif
    size_t result_size = 0;
    for(; idx < max_results; idx += 3, ++result_size )
    {
        const QByteArray row_key( selected_rows_result.result( idx + 0 ) );

        if( !re.isEmpty() )
        {
            const QString row_name( QString::fromUtf8(row_key.data()) );
            if( re.indexIn(row_name) == -1 )
            {
                continue;
            }
        }

        const QByteArray column_key( selected_rows_result.result( idx + 1 ) );
        const QByteArray data      ( selected_rows_result.result( idx + 2 ) );

        addRow( row_key, column_key, data );
    }

    return result_size;
}


row::pointer_t table::getRow(const char* row_name)
{
    return getRow( QByteArray(row_name,qstrlen(row_name)) );
}


/** \brief Search for a row or create a new one.
 *
 * This function searches for a row or, if it doesn't exist, create
 * a new row.
 *
 * Note that unless you set the value of a column in this row, the
 * row will never appear in the Cassandra cluster.
 *
 * This function accepts a name for the row. The name is a UTF-8 string.
 *
 * \param[in] row_name  The name of the row to search or create.
 *
 * \return A shared pointer to the matching row or a null pointer.
 */
row::pointer_t table::getRow(const QString& row_name)
{
    return getRow(row_name.toUtf8());
}

/** \brief Search for a row or create a new one.
 *
 * This function searches for a row or, if it doesn't exist, create
 * a new row.
 *
 * Note that unless you set the value of a column in this row, the
 * row will never appear in the Cassandra cluster.
 *
 * This function assigns the row a binary key.
 *
 * \param[in] row_key  The name of the row to search or create.
 *
 * \return A shared pointer to the matching row. Throws if not found in the map.
 *
 * \sa readRows()
 */
row::pointer_t table::getRow(const QByteArray& row_key)
{
    // row already exists?
    rows::iterator ri(f_rows.find(row_key));
    if(ri != f_rows.end())
    {
        return ri.value();
    }

    // this is a new row, allocate it
    row::pointer_t c(new row(shared_from_this(), row_key));
    f_rows.insert(row_key, c);
    return c;
}


/** \brief Retrieve the entire set of rows defined in this table.
 *
 * This function returns a constant reference to the map listing all
 * the rows currently defined in memory for this table.
 *
 * This can be used to determine how many rows are defined in memory
 * and to scan all the data.
 *
 * \return A constant reference to a map of rows. Throws if readRows() has not first been called.
 */
const rows& table::getRows()
{
    return f_rows;
}


/** \brief Search for a row.
 *
 * This function searches for a row. If it doesn't exist, then a NULL
 * pointer is returned (use the operator bool() function on the shared pointer.)
 *
 * The function can be used to check whether a given row was already created
 * in memory without actually creating it.
 *
 * This function accepts a row name viewed as a UTF-8 string.
 *
 * \warning
 * This function does NOT attempt to read the row from the Cassandra database
 * system. It only checks whether the row already exists in memory. To check
 * whether the row exists in the database, use the exists() function instead.
 *
 * \param[in] row_name  The name of the row to check for.
 *
 * \return A shared pointer to the row, may be NULL (operator bool() returning true)
 *
 * \sa exists()
 * \sa getRow()
 */
row::pointer_t table::findRow(const char* row_name) const
{
    return findRow( QByteArray(row_name, qstrlen(row_name)) );
}


/** \brief Search for a row.
 *
 * This function searches for a row. If it doesn't exist, then a NULL
 * pointer is returned (use the operator bool() function on the shared pointer.)
 *
 * The function can be used to check whether a given row was already created
 * in memory without actually creating it.
 *
 * This function accepts a row name viewed as a UTF-8 string.
 *
 * \warning
 * This function does NOT attempt to read the row from the Cassandra database
 * system. It only checks whether the row already exists in memory. To check
 * whether the row exists in the database, use the exists() function instead.
 *
 * \param[in] row_name  The name of the row to check for.
 *
 * \return A shared pointer to the row, may be NULL (operator bool() returning true)
 *
 * \sa exists()
 * \sa getRow()
 */
row::pointer_t table::findRow(const QString& row_name) const
{
    return findRow( row_name.toUtf8() );
}


/** \brief Search for a row.
 *
 * This function searches for a row. If it doesn't exist, then a NULL
 * pointer is returned (use the operator bool() function on the shared pointer.)
 *
 * The function can be used to check whether a given row was already created
 * in memory without actually creating it.
 *
 * This function accepts a row key which is a binary buffer.
 *
 * \warning
 * This function does NOT attempt to read the row from the Cassandra database
 * system. It only checks whether the row already exists in memory. To check
 * whether the row exists in the database, use the exists() function instead.
 *
 * \param[in] row_key  The binary key of the row to search for.
 *
 * \return A shared pointer to the row, may be NULL (operator bool() returning true)
 *
 * \sa exists()
 * \sa getRow()
 */
row::pointer_t table::findRow(const QByteArray& row_key) const
{
    auto ri(f_rows.find(row_key));
    if(ri == f_rows.end())
    {
        row::pointer_t null;
        return null;
    }
    return *ri;
}


/** \brief Check whether a row exists.
 *
 * This function checks whether the named row exists.
 *
 * \param[in] row_name  The row name to transform to UTF-8.
 *
 * \return true if the row exists in memory or the Cassandra database.
 */
bool table::exists(const char *row_name) const
{
    return exists(QByteArray(row_name, qstrlen(row_name)));
}


/** \brief Check whether a row exists.
 *
 * This function checks whether the named row exists.
 *
 * \param[in] row_name  The row name to transform to UTF-8.
 *
 * \return true if the row exists in memory or the Cassandra database.
 */
bool table::exists(const QString& row_name) const
{
    return exists(row_name.toUtf8());
}


/** \brief Check whether a row exists.
 *
 * This function checks whether a row exists. First it checks whether
 * it exists in memory. If not, then it checks in the Cassandra database.
 *
 * Empty keys are always viewed as non-existant and this function
 * returns false in that case.
 *
 * \warning
 * If you can avoid calling this function, all the better. It may be
 * very slow against a table that has many tombstones since it will
 * have to go through those to know whether there is at least one valid
 * row.
 *
 * \warning
 * If you dropped the row recently, IT STILL EXISTS. This is a "bug" in
 * Cassandra and there isn't really a way around it except by testing
 * whether a specific cell exists in the row. We cannot really test for
 * a cell here since we cannot know what cell exists here. So this test
 * really only tells you that (1) the row was never created; or (2) the
 * row was drop "a while back" (the amount of time it takes for a row
 * to completely disappear is not specified and it looks like it can take
 * days.) [This warning may not apply since we now use CQL]
 *
 * \todo
 * At this time there isn't a way to specify the consistency level of the
 * calls used by this function. The libdbproxy default is used.
 *
 * \param[in] row_key  The binary key of the row to check for.
 *
 * \return true if the row exists in memory or in Cassandra.
 */
bool table::exists(const QByteArray& row_key) const
{
    // an empty key cannot represent a valid row
    if(row_key.size() == 0)
    {
        return false;
    }

    auto ri(f_rows.find(row_key));
    if(ri != f_rows.end())
    {
        // row exists in memory
        return true;
    }

    auto pred( std::make_shared<row_key_predicate>() );
    pred->setRowKey(row_key);
    pred->setCount(1); // read as little as possible (TBD verify that works even with many tombstones)

    class save_current_cursor_index_t
    {
    public:
        save_current_cursor_index_t(table *table, int32_t& cursor_index)
            : f_table(table)
            , f_cursor_index_ref(cursor_index)
            , f_saved_cursor_index(cursor_index)
        {
            // simulate the closure of the current cursor index if open
            //
            f_cursor_index_ref = -1;
        }

        save_current_cursor_index_t(save_current_cursor_index_t const & rhs) = delete;

        ~save_current_cursor_index_t()
        {
            try
            {
                f_table->closeCursor();
            }
            catch(const exception&)
            {
                // not muc we can do here (destructor and exceptions do not
                // work together)
                //
                // exception can happen if we lose the network connection
                // and try to close the cursor
            }
            f_cursor_index_ref = f_saved_cursor_index;
        }

        save_current_cursor_index_t & operator = (save_current_cursor_index_t const & rhs) = delete;

    private:
        table *             f_table = nullptr;
        int32_t &           f_cursor_index_ref;
        int32_t             f_saved_cursor_index = 0;
    };
    save_current_cursor_index_t save_cursor_index(const_cast<table *>(this), const_cast<table *>(this)->f_cursor_index);

    // TODO: we should be able to do that without using the full fledge
    //       readRows() with a cursor + fetch etc. since we just want to
    //       know whether at least one entry exists we could just do one
    //       SELECT and save its result; then we would avoid the
    //       "save_current_cursor_index_t" problem
    //
    return const_cast<table *>(this)
            ->readRows( std::static_pointer_cast<row_predicate>(pred) ) != 0;
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the named row doesn't exist yet,
 * then it is created first.
 *
 * The reference is writable so you make write to a cell in this row.
 *
 * This function accepts a UTF-8 name for this row reference.
 *
 * \param[in] row_name  The name of the row to retrieve.
 *
 * \return A reference to a row.
 */
row& table::operator [] (const char *row_name)
{
    // in this case we may create the row and that's fine!
    return *getRow(row_name);
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the named row doesn't exist yet,
 * then it is created first.
 *
 * The reference is writable so you make write to a cell in this row.
 *
 * This function accepts a UTF-8 name for this row reference.
 *
 * \param[in] row_name  The name of the row to retrieve.
 *
 * \return A reference to a row.
 */
row& table::operator [] (const QString& row_name)
{
    // in this case we may create the row and that's fine!
    return *getRow(row_name);
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the keyed row doesn't exist yet,
 * then it is created first.
 *
 * The reference is writable so you make write to a cell in this row.
 *
 * This function accepts a binary key for this row reference.
 *
 * \param[in] row_key  The binary key of the row to retrieve.
 *
 * \return A reference to a row.
 */
row& table::operator[] (const QByteArray& row_key)
{
    // in this case we may create the row and that's fine!
    return *getRow(row_key);
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the named row doesn't exist yet,
 * then the function raises an error.
 *
 * The reference is read-only (constant) so you may retrieve a cell value
 * from it, but not modify the cell.
 *
 * This function accepts a name as the row reference. The name is viewed as
 * a UTF-8 string.
 *
 * \exception exception
 * The function checks whether the named row exists. If not, then this error
 * is raised because the function is constant and cannot create a new row.
 *
 * \param[in] row_name  The name of the row to retrieve.
 *
 * \return A constant reference to a row.
 */
const row& table::operator[] (const char *row_name) const
{
    const row::pointer_t p_row(findRow(row_name));
    if(!p_row) {
        throw exception("row does not exist so it cannot be read from");
    }
    return *p_row;
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the named row doesn't exist yet,
 * then the function raises an error.
 *
 * The reference is read-only (constant) so you may retrieve a cell value
 * from it, but not modify the cell.
 *
 * This function accepts a name as the row reference.
 *
 * \exception exception
 * The function checks whether the named row exists. If not, then this error
 * is raised because the function is constant and cannot create a new row.
 *
 * \param[in] row_name  The name of the row to retrieve.
 *
 * \return A constant reference to a row.
 */
const row& table::operator[] (const QString& row_name) const
{
    const row::pointer_t p_row(findRow(row_name));
    if( !p_row ) {
        throw exception("row does not exist so it cannot be read from");
    }
    return *p_row;
}

/** \brief Retrieve a table row.
 *
 * This function retrieves a table row. If the named row doesn't exist yet,
 * then the function raises an error.
 *
 * The reference is read-only (constant) so you may retrieve a cell value
 * from it, but not modify the cell.
 *
 * This function accepts a binary key as the row reference.
 *
 * \exception exception
 * The function checks whether the named row exists. If not, then this error
 * is raised because the function is constant and cannot create a new row.
 *
 * \param[in] row_key  The binary key of the row to retrieve.
 *
 * \return A constant reference to a row.
 */
const row& table::operator[] (const QByteArray& row_key) const
{
    const row::pointer_t p_row(findRow(row_key));
    if(!p_row) {
        throw exception("row does not exist so it cannot be read from");
    }
    return *p_row;
}


/** \brief Drop the named row.
 *
 * This function is the same as the dropRow() that takes a row_key parameter.
 * It simply transforms the row name into a row key and calls that other
 * function.
 *
 * \param[in] row_name  Specify the name of the row to drop.
 */
void table::dropRow(const char *row_name)
{
    dropRow( QByteArray(row_name, qstrlen(row_name)) );
}


/** \brief Drop the named row.
 *
 * This function is the same as the dropRow() that takes a row_key parameter.
 * It simply transforms the row name into a row key and calls that other
 * function.
 *
 * \param[in] row_name  Specify the name of the row to drop.
 */
void table::dropRow(const QString& row_name)
{
    dropRow(row_name.toUtf8());
}


/** \brief Drop the row from the Cassandra database.
 *
 * This function deletes the specified row and its data from the Cassandra
 * database and from memory.
 *
 * In regard to getting the row deleted from memory, you are expected to
 * use a weak pointer as follow:
 *
 * \code
 * ...
 * {
 *     QWeakPointer<libdbproxy::row> getRow(table.getRow(row_key)));
 *     ...
 *     table.dropRow(row_key);
 * }
 * ...
 * \endcode
 *
 * Note that Cassandra doesn't actually remove the row from its database until
 * the next time it does a garbage collection. Still, if there is a row you do
 * not need, drop it.
 *
 * The timestamp \p mode can be set to value::TIMESTAMP_MODE_DEFINED
 * in which case the value defined in the \p timestamp parameter is used by the
 * Cassandra remove() function.
 *
 * By default the \p mode parameter is set to
 * value::TIMESTAMP_MODE_AUTO which means that we'll make use of
 * the current time (i.e. only a row created after this call will exist.)
 *
 * The consistency level is set to CONSISTENCY_LEVEL_ALL since you are likely
 * willing to delete the row on all the nodes. However, I'm not certain this
 * is the best choice here. So the default may change in the future. You
 * may specify CONSISTENCY_LEVEL_DEFAULT in which case the libdbproxy object
 * default is used.
 *
 * \warning
 * Remember that a row doesn't actually get removed from the Cassandra database
 * until the next Garbage Collection runs. This is important for all your data
 * centers to be properly refreshed. This also means a readRows() will continue
 * to return the deleted row unless you check for a specific column that has
 * to exist. In that case, the row is not returned since all the columns ARE
 * deleted (or at least hidden in some way.) This function could be called
 * truncate(), however, all empty rows are really removed at some point.
 *
 * \warning
 * After a row was dropped, you cannot use the row object anymore, even if you
 * kept a shared pointer to it. Calling functions of a dropped row is likely
 * to get you a run-time exception. Note that all the cells defined in the
 * row are also dropped and are also likely to generate a run-time exception
 * if you kept a shared pointer on any one of them.
 *
 * \param[in] row_key  Specify the key of the row.
 */
void table::dropRow(const QByteArray& row_key)
{
    remove( row_key );
    f_rows.remove( row_key );
}


/** \brief Get the pointer to the parent object.
 *
 * \return Shared pointer to the cassandra object.
 */
context::pointer_t table::parentContext() const
{
    context::pointer_t context(f_context.lock());
    if(context == nullptr)
    {
        throw std::runtime_error("this table was dropped and is not attached to a context anymore");
    }

    return context;
}


/** \brief Save a cell value that changed.
 *
 * This function calls the context insertValue() function to save the new value that
 * was defined in a cell. The idea is so that when the user alters the value of a cell,
 * it directly updates the database. If the row does not exist, an exception is thrown.
 *
 * \param[in] row_key     The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[in] value       The new value of the cell.
 */
void table::insertValue( const QByteArray& row_key, const QByteArray& column_key, const value& value )
{
    if( !f_from_cassandra )
    {
        return;
    }

    // We expect all of our orders to be serialized within a session, to
    // ensure such a serialization, we have to ourselves specify the
    // TIMESTAMP parameter. This also means a DROP may have problems
    // and it adds some slowness.
    //
    int64_t const timestamp(libdbproxy::timeofday());
    QString query_string(QString("INSERT INTO %1.%2(key,column1,value)VALUES(?,?,?)USING TIMESTAMP %3")
        .arg(f_context_name)
        .arg(f_table_name)
        .arg(timestamp)
        );

    // setup the consistency level
    consistency_level_t consistency_level( value.consistencyLevel() );
    if( consistency_level == CONSISTENCY_LEVEL_DEFAULT )
    {
        consistency_level = parentContext()->parentCassandra()->defaultConsistencyLevel();
    }

    // define TTL only if the user defined it (Cassandra uses a 'null' when
    // undefined and that's probably better than having either a really large
    // value or 0 if that would work as 'permanent' in Cassandra.)
    //
    if(value.ttl() != value::TTL_PERMANENT)
    {
        query_string += QString(" AND TTL %1").arg(value.ttl());
    }

    order insert_value;
    insert_value.setCql( query_string, (f_batch_index == -1)
                         ? order::type_of_result_t::TYPE_OF_RESULT_SUCCESS
                         : order::type_of_result_t::TYPE_OF_RESULT_BATCH_ADD
                       );
    insert_value.setConsistencyLevel( consistency_level );
    insert_value.setBatchIndex(f_batch_index);

    insert_value.addParameter( row_key );
    insert_value.addParameter( column_key );
    insert_value.addParameter( value.binaryValue() );

    order_result const insert_value_result(f_proxy->sendOrder(insert_value));
    if(!insert_value_result.succeeded())
    {
        SNAP_LOG_ERROR
            << "unable to insert a value into the table for query: '"
            << query_string
            << "'"
            << SNAP_LOG_SEND;
        throw exception("inserting a value failed");
    }
}





/** \brief Get a cell value from Cassandra.
 *
 * This function calls the context getValue() function to retrieve a value
 * from Cassandra.
 *
 * The \p value parameter is not modified unless some data can be retrieved
 * from Cassandra.
 *
 * \param[in] row_key  The key used to identify the row.
 * \param[in] column_key  The key used to identify the column.
 * \param[out] value  The new value of the cell.
 *
 * \return false when the value was not found in the database, true otherwise
 */
bool table::getValue(const QByteArray& row_key, const QByteArray& column_key, value& value)
{
    const QString query_string( QString("SELECT value FROM %1.%2 WHERE key=? AND column1=?")
                         .arg(f_context_name)
                         .arg(f_table_name) );

    // setup the consistency level
    consistency_level_t consistency_level( value.consistencyLevel() );
    if( consistency_level == CONSISTENCY_LEVEL_DEFAULT )
    {
        consistency_level = parentContext()->parentCassandra()->defaultConsistencyLevel();
    }

    order get_value;
    get_value.setCql(query_string, order::type_of_result_t::TYPE_OF_RESULT_ROWS);
    get_value.setConsistencyLevel( consistency_level );

    get_value.addParameter( row_key );
    get_value.addParameter( column_key );

    order_result const get_value_result(f_proxy->sendOrder(get_value));
    if(!get_value_result.succeeded())
    {
        throw exception("retrieving a value failed");
    }

    if(get_value_result.resultCount() == 0)
    {
        value.setNullValue();
        return false;
    }

    value = get_value_result.result(0);

    return true;
}


/** \brief Count columns.
 *
 * This function counts a the number of columns that match a specified
 * \p column_predicate.
 *
 * If you want to use a different consistency, also indicate such in
 * the \p column_predicate parameter.
 *
 * \param[in] row_key  The row for which this data is being counted.
 * \param[in] column_predicate  The predicate to use to count the cells.
 *
 * \return The number of columns in this row.
 */
int32_t table::getCellCount
    ( const QByteArray& row_key
    , cell_predicate::pointer_t column_predicate
    )
{
    if( f_rows.find( row_key ) == f_rows.end() )
    {
        const QString query_string ( QString("SELECT COUNT(*)AS count FROM %1.%2")
            .arg(f_context_name)
            .arg(f_table_name)
            );

        // setup the consistency level
        consistency_level_t consistency_level( column_predicate
                                ? column_predicate->consistencyLevel()
                                : CONSISTENCY_LEVEL_DEFAULT );
        if( consistency_level == CONSISTENCY_LEVEL_DEFAULT )
        {
            consistency_level = parentContext()->parentCassandra()->defaultConsistencyLevel();
        }

        order cell_count;
        cell_count.setCql(query_string, order::type_of_result_t::TYPE_OF_RESULT_ROWS);
        cell_count.setPagingSize(column_predicate ? column_predicate->count() : 100);
        cell_count.setConsistencyLevel(consistency_level);
        order_result cell_count_result(f_proxy->sendOrder(cell_count));
        if(!cell_count_result.succeeded()
        || cell_count_result.resultCount() != 1)
        {
            throw exception("cell count failed");
        }

        return int32Value(cell_count_result.result(0));
    }

    // return the count from the memory cache
    return f_rows[row_key]->getCells().size();
}

/** \brief Delete a Cell from a table row.
 *
 * This function removes a cell from the Cassandra database as specified
 * by the parameters.
 *
 * \param[in] row_key           The row in which the cell is to be removed.
 * \param[in] column_key        The cell to be removed.
 * \param[in] consistency_level The consistency level to specify when dropping the row with respect to other data centers.
 */
void table::remove
    ( const QByteArray& row_key
    , const QByteArray& column_key
    , consistency_level_t consistency_level
    )
{
    if( !f_from_cassandra || !f_proxy )
    {
        return;
    }

    const QString query_string(
        QString("DELETE FROM %1.%2 WHERE key=? AND column1=?")
            .arg(f_context_name)
            .arg(f_table_name)
            );

    order drop_cell;
    drop_cell.setCql( query_string, (f_batch_index == -1)
                         ? order::type_of_result_t::TYPE_OF_RESULT_SUCCESS
                         : order::type_of_result_t::TYPE_OF_RESULT_BATCH_ADD
                       );
    drop_cell.setBatchIndex(f_batch_index);
    drop_cell.setConsistencyLevel(consistency_level);
    drop_cell.setTimestamp(libdbproxy::timeofday()); // make sure it gets deleted no matter when it was created
    drop_cell.addParameter(row_key);
    drop_cell.addParameter(column_key);
    order_result const drop_cell_result(f_proxy->sendOrder(drop_cell));
    if(!drop_cell_result.succeeded())
    {
        throw exception("drop cell failed");
    }
}

/** \brief Delete a Cell from a table row.
 *
 * This function removes a cell from the Cassandra database as specified
 * by the parameters.
 *
 * \param[in] row_key           The row in which the cell is to be removed.
 */
void table::remove( const QByteArray& row_key )
{
    if( !f_from_cassandra || !f_proxy )
    {
        return;
    }

    const QString query_string(
        QString("DELETE FROM %1.%2 WHERE key=?")
            .arg(f_context_name)
            .arg(f_table_name)
            );

    order drop_cell;
    drop_cell.setCql( query_string, (f_batch_index == -1)
                         ? order::type_of_result_t::TYPE_OF_RESULT_SUCCESS
                         : order::type_of_result_t::TYPE_OF_RESULT_BATCH_ADD
                       );
    drop_cell.setBatchIndex(f_batch_index);
    drop_cell.setConsistencyLevel(parentContext()->parentCassandra()->defaultConsistencyLevel());
    drop_cell.setTimestamp(libdbproxy::timeofday()); // make sure it gets deleted no matter when it was created
    drop_cell.addParameter(row_key);
    order_result const drop_cell_result(f_proxy->sendOrder(drop_cell));
    if(!drop_cell_result.succeeded())
    {
        throw exception("drop cell failed");
    }
}

} // namespace libdbproxy

// vim: ts=4 sw=4 et
