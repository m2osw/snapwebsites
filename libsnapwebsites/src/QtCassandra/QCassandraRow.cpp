/*
 * Text:
 *      src/QCassandraRow.cpp
 *
 * Description:
 *      Handling of rows. There is no class representing a row in Cassandra.
 *      A row is just a key. We have this object to allow for our C++ array
 *      syntax to access the Cassandra data.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2017 Made to Order Software Corp.
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

#include "QtCassandra/QCassandraRow.h"
#include "QtCassandra/QCassandraTable.h"
#include "QtCassandra/QCassandraContext.h"
#include "QtCassandra/QCassandra.h"

#include <iostream>
#include <stdexcept>

namespace QtCassandra
{

/** \class QCassandraRow
 * \brief The row class to hold a set of cells.
 *
 * These objects are created by the QCassandraTable whenever data is
 * being read or written to a cell. Rows have a binary key (may be set
 * as a UTF-8 string) and a map of cells indexed by the names of the
 * cells.
 *
 * The name of a row is limited to nearly 64Kb. Although, if you have
 * a table with very long names, you may want to consider computing
 * an md5sum or equivalent. This is important if you want to avoid
 * slowing down the Cassandra server. Searching through a large set
 * of 60Kb keys is much slower than doing the same through their
 * md5sums. Of course, that means you lose the automatic sorting of
 * the original key.
 *
 * By default, most of the functions will create a new cell. If you
 * need to test the existance without creating a cell, use the
 * exists() function with the column name of the cell check out.
 *
 * \sa exists()
 */


/** \typedef QCassandraRow::composite_column_names_t
 * \brief The set of column names.
 *
 * This type is used to declare a set of composite column names as
 * used by the compositeCell() functions.
 *
 * The type simply defines an array of column names. Each name can be any
 * valid column name key. In other words, a QCassandraValue with any value.
 * However, to be compatible with CQL and the CLI and possibly other
 * Cassandra features, you want to limit your names to things that do not
 * include the colon characters. The CQL and CLI make use of names
 * separated by colons (i.e. "blah:foo:123".) However, the Cassandra
 * cluster itself has no limit to the content of the names except for
 * their length which is 65536 bytes each.
 */


/** \var QCassandraRow::f_table
 * \brief The table this row is part of.
 *
 * This bare pointer is used to access the table this row is part of.
 * It is a bare pointer because you cannot create a row without having
 * a table and the table keeps a tight reference on the row.
 */


/** \var QCassandraRow::f_key
 * \brief The binary key of the row.
 *
 * The binary key of the row is set to UTF-8 when defined with a
 * string. Otherwise it is defined as specified by the user.
 */


/** \var QCassandraRow::f_cells
 * \brief The array of cells defined in this row.
 *
 * This is a map of cells. Cells are names and values pairs.
 *
 * The values are defined with a timestamp and ttl value.
 */


/** \brief Initialize a QCassandraRow object.
 *
 * This function initializes a QCassandraRow object. You must specify the
 * key of the row and that's the only parameter that a row supports at
 * this time.
 *
 * The key of the row is a binary buffer of data. It must be at least 1 byte
 * and at most 64Kb minus 1 (65535 bytes).
 *
 * A row is composed of multiple cells (called columns in Cassandra.)
 *
 * \exception QCassandraException
 * The key of the row cannot be empty or more than 64Kb. If that happens,
 * this exception is raised.
 *
 * \param[in] table  The parent table of this row.
 * \param[in] row_key  The key of this row.
 */
QCassandraRow::QCassandraRow(std::shared_ptr<QCassandraTable> table, const QByteArray& row_key)
    : f_table(table)
    , f_key(row_key)
    //f_cells() -- auto-init
{
    if(f_key.size() == 0) {
        throw QCassandraException("row key cannot be empty");
    }
    if(f_key.size() > 65535) {
        throw QCassandraException("row key is more than 64Kb");
    }
}


/** \brief Clean up the QCassandraRow object.
 *
 * This function ensures that all resources allocated by the
 * QCassandraRow are released.
 */
QCassandraRow::~QCassandraRow()
{
    try
    {
        // do an explicit clearCache() so we can try/catch otherwise we
        // could get a throw in the destructor
        //
        clearCache();
    }
    catch(const QCassandraException&)
    {
        // ignore, not much else we can do in a destructor
    }
}


/** \brief Retrieve the name of the row.
 *
 * This function returns the name of the row as specified in the
 * constructor.
 *
 * The name cannot be changed.
 *
 * Note that if you created the row with a binary key (i.e. a
 * QByteArray parameter) then you CANNOT retrieve the row name.
 * Instead, use the rowKey() function.
 *
 * \return A string with the key name.
 *
 * \sa rowKey()
 */
QString QCassandraRow::rowName() const
{
    return QString::fromUtf8(f_key.data());
}


/** \brief Retrieve the row key.
 *
 * This function returns the key of this row. The key is a binary buffer
 * of data. This function works whether the row was created with a name
 * or a key.
 *
 * Note that when creating a row with a binary key, you cannot retrieve
 * it using the rowName() function.
 *
 * \return A buffer of data representing the row key.
 *
 * \sa rowName()
 */
const QByteArray& QCassandraRow::rowKey() const
{
    return f_key;
}


/** \brief Retrieve the number of cells defined in this row.
 *
 * This function retrieves the number of cells currently defined in this row,
 * depending on the specified predicate (by default, all the cells.)
 *
 * This counts the number of cells available in the Cassandra database. It
 * may be different from the number of cells in the memory cache. (i.e. the
 * value returned by cells().size())
 *
 * \param[in] column_predicate  The predicate used to select which columns to count.
 *
 * \return The number of cells defined in this row.
 *
 * \note This method no longer changes the row set from the query!
 *
 * \sa cells(), QCassandraTable::readRows()
 */
int QCassandraRow::cellCount( const QCassandraCellPredicate::pointer_t column_predicate )
{
    //return static_cast<int>(f_cells.size());
    return parentTable()->getCellCount(f_key, column_predicate);
}


/** \brief Read the cells as defined by a default column predicate.
 *
 * This function is the same as the readCells() with a column predicate
 * only it uses a default predicate which is to read all the columns
 * available in that row, with a limit of 100 cells.
 *
 * In this case the predicate has no column names or range boundaries
 * and no index capabilities. This mode should be used on rows which you
 * know have a limited number of cells. Otherwise, you should use the
 * other readCells() version.
 *
 * To know how many cells were read, save the returned value. When you
 * use the count() function on the map returned by the cells() function,
 * you actually get the total number of cells ever read since the
 * creation of this row in memory or the last call to clearCache().
 * Most importantly, the cellCount() function returns the total number of
 * cells available in the Cassandra database. Not the number of cells
 * currently available in memory.
 *
 * \note
 * The number of columns read (the value returned by this function) may be
 * smaller than the total number of columns available in memory if you did
 * not clear the cache first.
 *
 * \return The number of cells read from Cassandra, can be zero.
 *
 * \sa cellCount()
 * \sa cells()
 * \sa clearCache()
 */
uint32_t QCassandraRow::readCells()
{
    return f_cells.size();
}


/** \brief Read the cells as defined by the predicate.
 *
 * This function reads a set of cells as specified by the specified
 * predicate. If you use the default QCassandraCellPredicate, then
 * the first 100 cells are read.
 *
 * If you are using columns as an index, then the column_predicate
 * parameter gets modified by this function. The start column name
 * is updated with the name of the last row read on each iteration.
 *
 * See the QCassandraCellPredicate for more information on how to
 * select columns.
 *
 * This function is often called to read an entire row in memory all
 * at once (this is faster than reading the row one value at a time
 * if you anyway are likely to read most of the columns.) However,
 * this function should not be used that way if the row includes an
 * index.
 *
 * \param[in,out] column_predicate  The predicate used to select which columns to read.
 *
 * \note This method no longer changes the row set from the query!
 *
 * \sa setIndex(), QCassandraTable::readRows()
 */
uint32_t QCassandraRow::readCells( QCassandraCellPredicate::pointer_t column_predicate )
{
    size_t idx(0);
    QCassandraOrderResult selected_cells_result;

    f_cells.clear();

    if( f_cursor_index != -1 )
    {
        // Note: the "FETCH" is ignored, only the type is used in this case
        //
        QCassandraOrder select_more_cells;
        select_more_cells.setCql("FETCH", QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_FETCH);
        select_more_cells.setCursorIndex(f_cursor_index);
        QCassandraOrderResult select_more_cells_result(parentTable()->proxy()->sendOrder(select_more_cells));
        selected_cells_result.swap(select_more_cells_result);
        if(!selected_cells_result.succeeded())
        {
            throw QCassandraException("select cells failed");
        }

        if(selected_cells_result.resultCount() == 0)
        {
            closeCursor();
            return 0;
        }
    }
    else
    {
        auto row_predicate = std::make_shared<QCassandraRowKeyPredicate>();
        row_predicate->setRowKey( f_key );

        // setup the consistency level
        consistency_level_t consistency_level( parentTable()->parentContext()->parentCassandra()->defaultConsistencyLevel() );
        if( column_predicate )
        {
            consistency_level_t const default_consistency_level(consistency_level);
            consistency_level = column_predicate->consistencyLevel();
            if( consistency_level == CONSISTENCY_LEVEL_DEFAULT )
            {
                // reset back to default if not defined in the column_predicate
                consistency_level = default_consistency_level;
            }
        }

        // prepare the CQL order
        QString query_string( QString("SELECT column1,value FROM %1.%2")
                       .arg(parentTable()->contextName())
                       .arg(parentTable()->tableName())
                       );
        int bind_count = 0;
        if( column_predicate )
        {
            row_predicate->setCellPredicate( column_predicate );
        }
        row_predicate->appendQuery( query_string, bind_count );

        // WARNING: the row_predicate we create right here, when the
        //          allow filtering flag can only be set by the caller
        //          in the column_predicate
        //
        if(column_predicate->allowFiltering())
        {
            query_string += " ALLOW FILTERING";
        }

        //
//std::cerr << "query=[" << query_string.toUtf8().data() << "]" << std::endl;
        QCassandraOrder select_cells;
        select_cells.setCql(query_string, QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_DECLARE);
        select_cells.setConsistencyLevel( consistency_level );
        select_cells.setColumnCount(2);

        row_predicate->bindOrder( select_cells );

        //
        if( column_predicate )
        {
            select_cells.setPagingSize( column_predicate->count() );
        }
        //

        QCassandraOrderResult select_cells_result(parentTable()->proxy()->sendOrder(select_cells));
        selected_cells_result.swap(select_cells_result);
        if(!selected_cells_result.succeeded())
        {
            throw QCassandraException("select cells failed");
        }

        if(selected_cells_result.resultCount() < 1)
        {
            throw QCassandraException("select cells did not return a cursor index");
        }
        f_cursor_index = int32Value(selected_cells_result.result(0));
        if(f_cursor_index < 0)
        {
            throw QCassandraLogicException("received a negative number as cursor index");
        }

        // ignore parameter one, it is not a row of data
        idx = 1;
    }

    size_t const max_results(selected_cells_result.resultCount());
#ifdef _DEBUG
    if((max_results - idx) % 2 != 0)
    {
        // the number of results must be a multiple of 3, although on
        // the SELECT (first time in) we expect one additional result
        // which represents the cursor index
        throw QCassandraLogicException("the number of results must be an exact multipled of 2!");
    }
#endif
    size_t result_size = 0;
    for(; idx < max_results; idx += 2, ++result_size )
    {
        const QByteArray column_key( selected_cells_result.result( idx + 0 ) );
        const QByteArray data      ( selected_cells_result.result( idx + 1 ) );

        QCassandraCell::pointer_t new_cell( cell( column_key ) );
        new_cell->assignValue( QCassandraValue(data) );
    }

    return result_size;
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a cell from this row. If the cell
 * does not exist, it is created.
 *
 * Note that the cell is not saved in the Cassandra database
 * unless you save a value in it (and assuming the context does
 * not only exist in memory.)
 *
 * This function accepts a column name. The UTF-8 version of it is used to
 * retrieve the data from Cassandra.
 *
 * \param[in] column_name  The name of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 */
QCassandraCell::pointer_t QCassandraRow::cell(const char *column_name)
{
    return cell( QByteArray(column_name, qstrlen(column_name)) );
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a cell from this row. If the cell
 * does not exist, it is created.
 *
 * Note that the cell is not saved in the Cassandra database
 * unless you save a value in it (and assuming the context does
 * not only exist in memory.)
 *
 * This function accepts a column name. The UTF-8 version of it is used to
 * retrieve the data from Cassandra.
 *
 * \param[in] column_name  The name of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 */
QCassandraCell::pointer_t QCassandraRow::cell(const QString& column_name)
{
    return cell(column_name.toUtf8());
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a cell from this row. If the cell
 * does not exist, it is created.
 *
 * Note that the cell is not saved in the Cassandra database
 * unless you save a value in it (and assuming the context does
 * not only exist in memory.)
 *
 * This function makes use of a binary key to reference the cell.
 *
 * \note
 * This function cannot be used to read a composite column unless
 * you know how to build the QByteArray to do so. I suggest you
 * use the compositeCell() function instead.
 *
 * \param[in] column_key  The binary key of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 *
 * \sa compositeCell()
 * \sa compositeCell() const
 */
QCassandraCell::pointer_t QCassandraRow::cell(const QByteArray& column_key)
{
    // column already exists?
    QCassandraCells::iterator ci(f_cells.find(column_key));
    if(ci != f_cells.end()) {
        return ci.value();
    }

    // this is a new column, allocate it
    QCassandraCell::pointer_t c(new QCassandraCell(shared_from_this(), column_key));
    f_cells.insert(column_key, c);
    return c;
}


/** \brief Retrieve the map of cells.
 *
 * This function returns a constant reference to the map of cells defined in
 * the row.
 *
 * This map does not generally represent all the cells of a row as only those
 * that you already accessed in read or write mode will be defined in memory.
 *
 * \note
 * The order of the cells in memory may not be the same as the order of the
 * cells in the database. This is especially true if the data is not integers.
 * (i.e. floating point numbers, UTF-8 or other encodings with characters
 * that are not ordered in the same order as the bytes representing them,
 * etc.) This also depends on the definition of the type in Cassandra.
 * Positive integers should always be properly sorted. Negative integers may
 * be a problem if you use the "wrong" type in Cassandra. FYI, the order of
 * the map of cells uses the QByteArray < operator.
 *
 * \warning
 * When reading cells from a row representing an index, you probably want to
 * clear the cells already read before the next read. This is done with the
 * clearCache() function. Then cells().isEmpty() returns true if no more
 * cells can be read from the database.
 *
 * \warning
 * The dropCell() function actually deletes the cell from this map and
 * therefore the map becomes invalid if you use an iterator in a loop.
 * Make sure you reset the iterator each time.
 *
 * \return The map of cells referenced by column keys.
 *
 * \sa clearCache()
 */
const QCassandraCells& QCassandraRow::cells() const
{
    return f_cells;
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a cell from this row. If the cell
 * does not exist, it returns a NULL pointer (i.e. isNull() on the
 * shared pointer returns true.)
 *
 * This function accepts a column name. The UTF-8 version of it is used to
 * retrieve the data from Cassandra.
 *
 * \warning
 * This function does NOT attempt to read the cell from the Cassandra database
 * system. It only checks whether the cell already exists in memory. To check
 * whether the cell exists in the database, use the exists() function instead.
 *
 * \param[in] column_name  The name of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 *
 * \sa cell()
 * \sa exists()
 */
QCassandraCell::pointer_t QCassandraRow::findCell(const QString& column_name) const
{
    return findCell(column_name.toUtf8());
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a cell from this row. If the cell
 * does not exist, it returns a NULL pointer (i.e. isNull() on the
 * shared pointer returns true.)
 *
 * This function makes use of a binary key to reference the cell.
 *
 * \warning
 * This function does NOT attempt to read the cell from the Cassandra database
 * system. It only checks whether the cell already exists in memory. To check
 * whether the cell exists in the database, use the exists() function instead.
 *
 * \param[in] column_key  The binary key of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 *
 * \sa cell()
 * \sa exists()
 */
QCassandraCell::pointer_t QCassandraRow::findCell(const QByteArray& column_key) const
{
    QCassandraCells::const_iterator ci(f_cells.find(column_key));
    if(ci == f_cells.end()) {
        QCassandraCell::pointer_t null;
        return null;
    }
    return *ci;
}


/** \brief Check whether a cell exists in this row.
 *
 * The check is happening in memory first. If the cell doesn't exist in memory,
 * then the row checks in the Cassandra database.
 *
 * \todo
 * Look into why a cell is created when just checking for its existance.
 *
 * \bug
 * At this time this function CREATES the cell if it did not yet
 * exist!
 *
 * \param[in] column_name  The column name.
 *
 * \return true if the cell exists, false otherwise.
 */
bool QCassandraRow::exists(const char * column_name) const
{
    return exists(QString(column_name));
}


/** \brief Check whether a cell exists in this row.
 *
 * The check is happening in memory first. If the cell doesn't exist in memory,
 * then the row checks in the Cassandra database.
 *
 * \todo
 * Look into why a cell is created when just checking for its existance.
 *
 * \bug
 * At this time this function CREATES the cell if it did not yet
 * exist!
 *
 * \param[in] column_name  The column name.
 *
 * \return true if the cell exists, false otherwise.
 */
bool QCassandraRow::exists(const QString& column_name) const
{
    return exists(column_name.toUtf8());
}


/** \brief Check whether a cell exists in this row.
 *
 * The check is happening in memory first. If the cell doesn't exist in memory,
 * then the row checks in the Cassandra database.
 *
 * \param[in] column_key  The column binary key.
 *
 * \return true if the cell exists, false otherwise.
 */
bool QCassandraRow::exists(const QByteArray& column_key) const
{
    QCassandraCells::const_iterator ci(f_cells.find(column_key));
    if(ci != f_cells.end())
    {
        // exists in the cache already
        return true;
    }

    // This is already done by the readRows() method of QCassandraTable.
    // If you have an instance of this class without going through the
    // QCassandraTable interface, it won't work right.
    //
    // try reading this cell
    QCassandraValue value;
    try
    {
        if(!parentTable()->getValue(f_key, column_key, value))
        {
            return false;
        }
    }
    catch( const std::exception& )
    {
        return false;
    }

    // since we just got the value, we might as well cache it
    //
    QCassandraCell::pointer_t c(const_cast<QCassandraRow *>(this)->cell(column_key));
    c->assignValue(value);

    return true;
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a reference to a cell from this row in
 * array syntax.
 *
 * This version returns a writable cell and it creates a new cell
 * when one with the specified name doesn't already exist.
 *
 * This function accepts a column name. The UTF-8 version of it is used to
 * retrieve the data from Cassandra.
 *
 * \param[in] column_name  The name of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 */
QCassandraCell& QCassandraRow::operator [] (const char * column_name)
{
    return *cell(column_name);
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a reference to a cell from this row in
 * array syntax.
 *
 * This version returns a writable cell and it creates a new cell
 * when one with the specified name doesn't already exist.
 *
 * This function accepts a column name. The UTF-8 version of it is used to
 * retrieve the data from Cassandra.
 *
 * \param[in] column_name  The name of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 */
QCassandraCell& QCassandraRow::operator [] (const QString& column_name)
{
    return *cell(column_name);
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a reference to a cell from this row in
 * array syntax.
 *
 * This version returns a writable cell and it creates a new cell
 * when one with the specified name doesn't already exist.
 *
 * This function makes use of a binary key to reference the cell.
 *
 * \param[in] column_key  The binary key of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 */
QCassandraCell& QCassandraRow::operator [] (const QByteArray& column_key)
{
    return *cell(column_key);
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a constant reference to a cell from this row
 * in array syntax.
 *
 * This version returns a read-only cell. If the cell doesn't exist,
 * the funtion raises an exception.
 *
 * This function accepts a column name. The UTF-8 version of it is used to
 * retrieve the data from Cassandra.
 *
 * \exception QCassandraException
 * This function requires that the cell being accessed already exist
 * in memory. If not, this exception is raised.
 *
 * \param[in] column_name  The name of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 */
const QCassandraCell& QCassandraRow::operator [] (const char* column_name) const
{
    return operator [] (QString(column_name));
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a constant reference to a cell from this row
 * in array syntax.
 *
 * This version returns a read-only cell. If the cell doesn't exist, 
 * the funtion raises an exception.
 *
 * This function accepts a column name. The UTF-8 version of it is used to
 * retrieve the data from Cassandra.
 *
 * \exception QCassandraException
 * This function requires that the cell being accessed already exist
 * in memory. If not, this exception is raised.
 *
 * \param[in] column_name  The name of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 */
const QCassandraCell& QCassandraRow::operator [] (const QString& column_name) const
{
    return operator [] (column_name.toUtf8());
}


/** \brief Retrieve a cell from the row.
 *
 * This function retrieves a cell from this row in array syntax.
 *
 * This version returns a writable cell and it creates a new cell
 * when one with the specified name doesn't already exist.
 *
 * This function makes use of a binary key to reference the cell.
 *
 * \param[in] column_key  The binary key of the column referencing this cell.
 *
 * \return A shared pointer to the cell.
 */
const QCassandraCell& QCassandraRow::operator [] (const QByteArray& column_key) const
{
    QCassandraCell::pointer_t p_cell( findCell(column_key) );
    if( !p_cell )
    {
        throw QCassandraException("named column while retrieving a cell was not found, cannot return a reference");
    }

    return *p_cell;
}


/** \brief Clear the cached cells.
 *
 * This function is used to clear all the cells that were cached in this row.
 *
 * As a side effect, all the QCassandraCell objects are unparented which means
 * that you cannot use them anymore (doing so raises an exception.)
 */
void QCassandraRow::clearCache()
{
    closeCursor();

    f_cells.clear();
}


/** \brief Close the current cursor.
 *
 * This function closes the current cursor (i.e. the cursor used
 * to gather a set of rows and their data from a table.)
 */
void QCassandraRow::closeCursor()
{
    if(f_cursor_index >= 0)
    {
        // Note: the "CLOSE" CQL string is ignored
        //
        QCassandraOrder close_cursor;
        close_cursor.setCql("CLOSE", QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_CLOSE);
        close_cursor.setCursorIndex(f_cursor_index);
        QCassandraOrderResult close_cursor_result(parentTable()->proxy()->sendOrder(close_cursor));
        if(!close_cursor_result.succeeded())
        {
            throw QCassandraException("QCassandraRow::closeCursor(): closing cursor failed.");
        }
        f_cursor_index = -1;
    }
}


/** \brief Drop the named cell.
 *
 * This function is the same as the dropCell() that accepts a QByteArray
 * as its column key. It simply calls it after changing the column name into
 * a key.
 *
 * \param[in] column_name  The name of the column to drop.
 */
void QCassandraRow::dropCell(const char * column_name)
{
    dropCell(QByteArray(column_name, qstrlen(column_name)));
}


/** \brief Drop the named cell.
 *
 * This function is the same as the dropCell() that accepts a QByteArray
 * as its column key. It simply calls it after changing the column name into
 * a key.
 *
 * \param[in] column_name  The name of the column to drop.
 */
void QCassandraRow::dropCell(const QString& column_name)
{
    dropCell(column_name.toUtf8());
}


/** \brief Drop the specified cell from the Cassandra database.
 *
 * This function deletes the specified cell and its data from the Cassandra
 * database and from memory. To delete the cell immediately you want to set
 * the timestamp to now (i.e. use QCassandra::timeofday() with the DEFINED
 * mode as mentioned below.)
 *
 * The timestamp \p mode can be set to QCassandraValue::TIMESTAMP_MODE_DEFINED
 * in which case the value defined in the \p timestamp parameter is used by the
 * Cassandra remove() function.
 *
 * By default the \p mode parameter is set to
 * QCassandraValue::TIMESTAMP_MODE_AUTO which means that the timestamp
 * value of the cell f_value parameter is used. This will NOT WORK RIGHT
 * if the timestamp of the cell value was never set properly (i.e. you
 * never read the cell from the Cassandra database and never called
 * the setTimestamp() function on the cell value.) You have two choices here.
 * You may specify the timestamp on the dropCell() call, or you may change
 * the cell timestamp:
 *
 * \code
 *     // this is likely to fail if you created the cell in this session
 *     row->dropCell(cell_name);
 *
 *     // define the timestamp in the cell
 *     cell->setTimestamp(QCassandra::timeofday());
 *
 *     // define the timestamp in the dropCell() call
 *     row->dropCell(cell_name, QCassandraValue::TIMESTAMP_MODE_DEFINED, QCassandra::timeofday());
 * \endcode
 *
 * The consistency level of the cell f_value is also passed to the Cassandra
 * remove() function. This means that by default you'll get whatever the
 * default is from your QCassandra object, the default in the value or
 * whatever the value was when you last read the value. To change that
 * default you can retrieve the cell and set the consistency level as follow:
 *
 * \code
 *     QCassandraCell::pointer_t c(f_row->cell(f_cell));
 * \endcode
 *
 * These 2 lines of code do NOT create the cell in the Cassandra cluster.
 * It only creates it in memory unless it was read earlier in which case
 * the cached copy is returned.
 *
 * \warning
 * The corresponding cell is marked as dropped, whether you kept a shared
 * pointer of that cell does not make it reusable. You must forget about it
 * after this call.
 *
 * \warning
 * If you used the cells() function to retrieve a reference to the list of
 * cells as a reference, that list is broken when this function returns.
 * Make sure to re-retrieve the list after each call to the dropCell()
 * function.
 *
 * \param[in] column_key  A shared pointer to the cell to remove.
 * \param[in] mode  Specify the timestamp mode.
 * \param[in] timestamp  Specify the timestamp to remove only cells that are equal or older.
 *
 * \sa cell()
 * \sa cells()
 * \sa QCassandra::timeofday()
 */
void QCassandraRow::dropCell( const QByteArray& column_key )
{
    QCassandraCell::pointer_t c(cell(column_key));

    parentTable()->remove( f_key, column_key, c->consistencyLevel() );
    f_cells.remove(column_key);
}


/** \brief Get the pointer to the parent object.
 *
 * \return Shared pointer to the table object with owns this object.
 */
QCassandraTable::pointer_t QCassandraRow::parentTable() const
{
    QCassandraTable::pointer_t table(f_table.lock());
    if(table == nullptr)
    {
        throw QCassandraException("this row was dropped and is not attached to a table anymore");
    }

    return table;
}


/** \brief Save a cell value that changed.
 *
 * This function calls the table insertValue() function to save the new value that
 * was defined in a cell.
 *
 * \param[in] column_key  The key used to identify the column.
 * \param[in] value  The new value of the cell.
 */
void QCassandraRow::insertValue(const QByteArray& column_key, const QCassandraValue& value)
{
    parentTable()->insertValue(f_key, column_key, value);
}


/** \brief Get a cell value from Cassandra.
 *
 * This function calls the table getValue() function to retrieve the currrent
 * value that defined in a cell.
 *
 * If the cell does not exist, then value is set to the Null value.
 *
 * \param[in] column_key  The key used to identify the column.
 * \param[out] value  To return the value of the cell.
 *
 * \return false when the value was not found in the database, true otherwise
 */
bool QCassandraRow::getValue(const QByteArray& column_key, QCassandraValue& value)
{
    return parentTable()->getValue(f_key, column_key, value);
}


/** \brief Add a value to a Cassandra counter.
 *
 * This function calls the table addValue() function to add the specified
 * value to the Cassandra counter that is defined in a cell.
 *
 * If the cell counter does not exist yet, then value is set to the specified
 * value.
 * 
 * \note this is a synonym for QCassandraRow::insertValue(), since counters
 * are automatically sensed and handled with an "UPDATE" instead of an "INSERT".
 *
 * \param[in] column_key  The key used to identify the column.
 * \param[in] value  To value to add to this counter.
 */
void QCassandraRow::addValue(const QByteArray& column_key, int64_t value)
{
    return parentTable()->insertValue(f_key, column_key, value);
}


} // namespace QtCassandra
// vim: ts=4 sw=4 et
