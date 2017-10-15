/*
 * Text:
 *      src/QCassandraContext.cpp
 *
 * Description:
 *      Handling of Cassandra Keyspace which is a context.
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

#include "QtCassandra/QCassandraContext.h"
#include "QtCassandra/QCassandra.h"

#include <casswrapper/schema.h>

#include <stdexcept>
#include <unistd.h>

#include <sstream>

#include <QtCore>
//#include <QDebug>


namespace QtCassandra
{

/** \class QCassandraContext
 * \brief Hold a Cassandra keyspace definition.
 *
 * This class defines objects that can hold all the necessary information
 * for a Cassandra keyspace definition.
 *
 * A keyspace is similar to a context in the sense that to work on a keyspace
 * you need to make it the current context. Whenever you use a context, this
 * class automatically makes it the current context. This works well in a non
 * threaded environment. In a threaded environment, you want to either make
 * sure that only one thread makes use of the Cassandra objects or that you
 * protect all the calls. This library does not.
 *
 * You may think of this context as one database of an SQL environment. If
 * you have used OpenGL, this is very similar to the OpenGL context.
 */

/** \typedef QCassandraContext::QCassandraContextOptions
 * \brief A map of context options.
 *
 * This map defines options as name / value pairs.
 *
 * Only known otion names should be used or a protocol error may result.
 */

/** \var QCassandraContext::f_schema
 * \brief The pointer to the casswrapper::schema meta object.
 *
 * This pointer is a shared pointer to the private definition of
 * the Cassandra context (i.e. a keyspace definition.)
 *
 * The pointer is created at the time the context is created.
 */

/** \var QCassandraContext::f_cassandra
 * \brief A pointer back to the QCassandra object.
 *
 * The bare pointer is used by the context to access the cassandra
 * private object and make the context the current context. It is
 * a bare pointer because the QCassandra object cannot be deleted
 * without the context getting deleted first.
 *
 * Note that when deleting a QCassandraContext object, you may still
 * have a shared pointer referencing the object. This means the
 * object itself will not be deleted. In that case, the f_cassandra
 * parameter becomes NULL and calling functions that make use of
 * it throw an error.
 *
 * \note
 * If you look at the implementation, many functions call the
 * makeCurrent() which checks the f_cassandra pointer, thus these
 * functions don't actually test the pointer.
 */

/** \var QCassandraContext::f_tables
 * \brief List of tables.
 *
 * A list of the tables defined in this context. The tables may be created
 * in memory only.
 *
 * The list is a map using the table binary key as the its own key.
 */

/** \brief Initialize a QCassandraContext object.
 *
 * This function initializes a QCassandraContext object.
 *
 * Note that the constructor is private. To create a new context, you must
 * use the QCassandra::context() function.
 *
 * All the parameters are set to the defaults as defined in the Cassandra
 * definition of the KsDef message. You can use the different functions to
 * change the default values.
 *
 * A context name must be composed of letters (A-Za-z), digits (0-9) and
 * underscore (_). It must start with a letter. The corresponding lexical
 * expression is: /[A-Za-z][A-Za-z0-9_]*\/
 *
 * \note
 * A context can be created, updated, and dropped. In all those cases, the
 * functions return once the Cassandra instance with which you are
 * connected is ready.
 *
 * \param[in] cassandra  The QCassandra object owning this context.
 * \param[in] context_name  The name of the Cassandra context.
 *
 * \sa contextName()
 * \sa QCassandra::context()
 */
QCassandraContext::QCassandraContext(QCassandra::pointer_t cassandra, const QString& context_name)
    //: f_schema(std::make_shared<casswrapper::schema::SessionMeta::KeyspaceMeta>())
    : f_cassandra(cassandra)
    , f_context_name(context_name)
      //f_tables() -- auto-init
{
    // verify the name here (faster than waiting for the server and good documentation)
    QRegExp re("[A-Za-z][A-Za-z0-9_]*");
    if(!re.exactMatch(context_name))
    {
        throw QCassandraException("invalid context name (does not match [A-Za-z][A-Za-z0-9_]*)");
    }

    resetSchema();
}

/** \brief Clean up the QCassandraContext object.
 *
 * This function ensures that all resources allocated by the
 * QCassandraContext are released.
 *
 * Note that does not in any way destroy the context in the
 * Cassandra cluster.
 */
QCassandraContext::~QCassandraContext()
{
}


void QCassandraContext::resetSchema()
{
    f_schema = std::make_shared<casswrapper::schema::SessionMeta::KeyspaceMeta>();

    casswrapper::schema::Value replication;
    auto& replication_map(replication.map());
    replication_map["class"]              = QVariant("SimpleStrategy");
    replication_map["replication_factor"] = QVariant(1);

    auto& field_map(f_schema->getFields());
    field_map["replication"]    = replication;
    field_map["durable_writes"] = QVariant(true);
}


/** \brief Retrieve the name of this context.
 *
 * This function returns the name of this context.
 *
 * Note that the name cannot be modified. It is set by the constructor as
 * you create a QCassandraContext.
 *
 * \return A QString with the context name.
 */
const QString& QCassandraContext::contextName() const
{
    return f_context_name;
}


const casswrapper::schema::Value::map_t& QCassandraContext::fields() const
{
    return f_schema->getFields();
}


casswrapper::schema::Value::map_t& QCassandraContext::fields()
{
    return f_schema->getFields();
}


/** \brief Retrieve a table definition by name.
 *
 * This function is used to retrieve a table definition by name.
 * If the table doesn't exist, it gets created.
 *
 * Note that if the context is just a memory context (i.e. it does not yet
 * exist in the Cassandra cluster,) then the table is just created in memory.
 * This is useful to create a new context with all of its tables all at
 * once. The process is to call the QCassandra::context() function to get
 * an in memory context, and then call this table() function for each one of
 * the table you want to create. Finally, to call the create() function to
 * actually create the context and its table in the Cassandra cluster.
 *
 * You can test whether the result is null with the isNull() function
 * of the std::shared_ptr<> class.
 *
 * \param[in] table_name  The name of the table to retrieve.
 *
 * \return A shared pointer to the table definition found or a null shared pointer.
 */
QCassandraTable::pointer_t QCassandraContext::table(const QString& table_name)
{
    QCassandraTable::pointer_t t( findTable( table_name ) );
    if( t != QCassandraTable::pointer_t() )
    {
        return t;
    }

    // this is a new table, allocate it
    t.reset( new QCassandraTable(shared_from_this(), table_name) );
    f_tables.insert( table_name, t );
    return t;
}


/** \brief Retrieve a reference to the tables.
 *
 * This function retrieves a constant reference to the map of table definitions.
 * The list is read-only, however, it is strongly suggested that you make a copy
 * if your code is going to modifiy tables later (i.e. calling table() may
 * affect the result of this call if you did not first copy the map.)
 *
 * \return A reference to the table definitions of this context.
 */
const QCassandraTables& QCassandraContext::tables()
{
#if 0
    if( f_tables.empty() )
    {
        loadTables();
    }
#endif

    return f_tables;
}

/** \brief Search for a table.
 *
 * This function searches for a table. If it exists, its shared pointer is
 * returned. Otherwise, it returns a NULL pointer (i.e. the
 * std::shared_ptr<>::operator bool() function returns true.)
 *
 * \note
 * Since the system reads the list of existing tables when it starts, this
 * function returns tables that exist in the database and in memory only.
 *
 * \param[in] table_name  The name of the table to retrieve.
 *
 * \return A shared pointer to the table.
 */
QCassandraTable::pointer_t QCassandraContext::findTable(const QString& table_name)
{
#if 0
    if( f_tables.empty() )
    {
        loadTables();
    }
#endif

    QCassandraTables::const_iterator it(f_tables.find(table_name));
    if(it == f_tables.end()) {
        QCassandraTable::pointer_t null;
        return null;
    }
    return *it;
}

/** \brief Retrieve a table reference from a context.
 *
 * The array operator searches for a table by name and returns
 * its reference. This is useful to access data with array like
 * syntax as in:
 *
 * \code
 * context[table_name][column_name] = value;
 * \endcode
 *
 * \exception QCassandraException
 * If the table doesn't exist, this function raises an exception
 * since otherwise the reference would be a NULL pointer.
 *
 * \param[in] table_name  The name of the table to retrieve.
 *
 * \return A reference to the named table.
 */
QCassandraTable& QCassandraContext::operator [] (const QString& table_name)
{
    QCassandraTable::pointer_t ptable( findTable(table_name) );
    if( !ptable ) {
        throw QCassandraException("named table was not found, cannot return a reference");
    }

    return *ptable;
}


#if 0
/** \brief Set the replication factor.
 *
 * This function sets the replication factor of the context.
 *
 * \deprecated
 * Since version 1.1 of Cassandra, the context replication
 * factor is viewed as a full option. This function automatically
 * sets the factor using the setDescriptionOption() function.
 * This means calling the setDescriptionOptions()
 * and overwriting all the options has the side effect of
 * cancelling this call. Note that may not work right with
 * older version of Cassandra. Let me know if that's the case.
 *
 * \param[in] factor  The new replication factor.
 */
void QCassandraContext::setReplicationFactor(int32_t factor)
{
    // since version 1.1 of Cassandra, the replication factor
    // defined in the structure is ignored
    QString value(QString("%1").arg(factor));
    setDescriptionOption("replication_factor", value);
}

/** \brief Unset the replication factor.
 *
 * This function unsets the replication factor in case it was set.
 * In general it is not necessary to call this function unless you
 * are initializing a new context and you want to make sure that
 * the default replication factor is used.
 */
void QCassandraContext::unsetReplicationFactor()
{
    eraseDescriptionOption("replication_factor");
}

/** \brief Check whether the replication factor is defined.
 *
 * This function retrieves the current status of the replication factor parameter.
 *
 * \return True if the replication factor parameter is defined.
 */
bool QCassandraContext::hasReplicationFactor() const
{
    return f_private->__isset.replication_factor;
}

/** \brief Retrieve the current replication factor.
 *
 * This function reads and return the current replication factor of
 * the context.
 *
 * If the replication factor is not defined, zero is returned.
 *
 * \return The current replication factor.
 */
int32_t QCassandraContext::replicationFactor() const
{
    if(f_private->__isset.replication_factor) {
        return f_private->replication_factor;
    }
    return 0;
}

/** \brief Set whether the writes are durable.
 *
 * Temporary and permanent contexts can be created. This option defines
 * whether it is one of the other. Set to true to create a permanent
 * context (this is the default.)
 *
 * \param[in] durable_writes  Set whether writes are durable.
 */
void QCassandraContext::setDurableWrites(bool durable_writes)
{
    f_private->__set_durable_writes(durable_writes);
}

/** \brief Unset the durable writes flag.
 *
 * This function marks the durable write flag as not set. This does
 * not otherwise change the flag. It will just not be sent over the
 * network and the default will be used when required.
 */
void QCassandraContext::unsetDurableWrites()
{
    f_private->__isset.durable_writes = false;
}

/** \brief Check whether the durable writes is defined.
 *
 * This function retrieves the current status of the durable writes parameter.
 *
 * \return True if the durable writes parameter is defined.
 */
bool QCassandraContext::hasDurableWrites() const
{
    return f_private->__isset.durable_writes;
}

/** \brief Retrieve the durable write flag.
 *
 * This function returns the durable flag that determines whether a
 * context is temporary (false) or permanent (true).
 *
 * \return The current durable writes flag status.
 */
bool QCassandraContext::durableWrites() const
{
    if(f_private->__isset.durable_writes) {
        return f_private->durable_writes;
    }
    return false;
}


/** \brief Prepare the context.
 *
 * This function prepares the context so it can be copied in a
 * keyspace definition later used to create a keyspace or to
 * update an existing keyspace.
 *
 * \todo
 * Verify that the strategy options are properly defined for the strategy
 * class (i.e. the "replication_factor" parameter is only for SimpleStrategy
 * and a list of at least one data center for other strategies.) However,
 * I'm not 100% sure that this is good idea since a user may add strategies
 * that we do not know anything about!
 *
 * \param[out] data  The output keyspace definition.
 *
 * \sa parseContextDefinition()
 */
void QCassandraContext::prepareContextDefinition(KsDef *ks) const
{
    *ks = *f_private;

    if(ks->strategy_class == "") {
        ks->strategy_class = "LocalStrategy";
    }

    // copy the options
    ks->strategy_options.clear();
    for(QCassandraContextOptions::const_iterator
                o = f_options.begin(); o != f_options.end(); ++o)
    {
        ks->strategy_options.insert(
                std::pair<std::string, std::string>(o.key().toUtf8().data(),
                                                    o.value().toUtf8().data()));
    }
    ks->__isset.strategy_options = !ks->strategy_options.empty();

    // copy the tables -- apparently we cannot do that here!
    // instead we have to loop through the table in the previous
    // level and update each column family separately
    ks->cf_defs.clear();
    for( auto t : f_tables )
    {
        CfDef cf;
        t->prepareTableDefinition(&cf);
        ks->cf_defs.push_back(cf);
    }
    //if(ks->cf_defs.empty()) ... problem? it's not optional...
}


/** \brief Generate the replication stanza for the CQL keyspace schema.
 */
QString QCassandraContext::generateReplicationStanza() const
{
    QString replication_stanza;
    if( f_private->strategy_class == "SimpleStrategy" )
    {
        replication_stanza = QString("'class': 'SimpleStrategy', 'replication_factor' : %1")
                             .arg(f_options["replication_factor"]);
    }
    else if( f_private->strategy_class == "NetworkTopologyStrategy" )
    {
        QString datacenters;
        for( QString key : f_options.keys() )
        {
            if( !datacenters.isEmpty() )
            {
                datacenters += ", ";
            }
            datacenters += QString("'%1': %2").arg(key).arg(f_options[key]);
        }
        //
        replication_stanza = QString("'class': 'NetworkTopologyStrategy', %1")
                             .arg(datacenters);
    }
    else
    {
        std::stringstream ss;
        ss << "This strategy class, '" << f_private->strategy_class << "', is not currently supported!";
        throw QCassandraException( ss.str().c_str() );
    }

    return replication_stanza;
}
#endif


/** \brief This is an internal function used to parse a KsDef structure.
 *
 * This function is called internally to parse a KsDef object.
 *
 * \param[in] data  The pointer to the KsDef object.
 *
 * \sa prepareContextDefinition()
 */
void QCassandraContext::parseContextDefinition( casswrapper::schema::SessionMeta::KeyspaceMeta::pointer_t keyspace_meta )
{
    f_schema = keyspace_meta;
    for( const auto pair : keyspace_meta->getTables() )
    {
        QCassandraTable::pointer_t t(table(pair.first));
        t->parseTableDefinition(pair.second);
    }
}


/** \brief Make this context the current context.
 *
 * This function marks this context as the current context where further
 * function calls will be made (i.e. table and cell editing.)
 *
 * Note that whenever you call a function that requires this context to
 * be current, this function is called automatically. If the context is
 * already the current context, then no message is sent to the Cassandra
 * server.
 *
 * \sa QCassandra::setContext()
 */
void QCassandraContext::makeCurrent()
{
    parentCassandra()->setCurrentContext( shared_from_this() );
}


QString QCassandraContext::getKeyspaceOptions()
{
    QString q_str;
    for( const auto& pair : f_schema->getFields() )
    {
        if( q_str.isEmpty() )
        {
            q_str = "WITH ";
        }
        else
        {
            q_str += "AND ";
        }
        q_str += QString("%1 = %2\n")
                .arg(pair.first)
                .arg(pair.second.output())
                ;
    }

    return q_str;
}


/** \brief Create a new context.
 *
 * This function is used to create a new context (keyspace) in the current
 * Cassandra cluster. Once created, you can make use of it whether it is
 * attached to the Cassandra cluster or not.
 *
 * If you want to include tables in your new context, then create them before
 * calling this function. It will be faster since you'll end up with one
 * single request.
 *
 * There is an example on how to create a new context with this library:
 *
 * \code
 * QtCassandra::QCassandraContext context("qt_cassandra_test_context");
 * // default strategy is LocalStrategy which you usually do not want
 * context.setStrategyClass("org.apache.cassandra.locator.SimpleStrategy");
 * context.setDurableWrites(true); // by default this is 'true'
 * context.setReplicationFactor(1); // by default this is undefined
 * ...  // add tables before calling create() if you also want tables
 * context.create();
 * \endcode
 *
 * With newer versions of Cassandra (since 1.1) and a network or local
 * strategy you have to define the replication factors using your data
 * center names (the "replication_factor" parameter is ignored in that
 * case):
 *
 * \code
 * context.setDescriptionOption("strategy_class", "org.apache.cassandra.locator.NetworkTopologyStrategy");
 * context.setDescriptionOption("data_center1", "3");
 * context.setDescriptionOption("data_center2", "3");
 * context.setDurableWrites(true);
 * context.create();
 * \endcode
 *
 * Note that the replication factor is not set by default, yet it is a required
 * parameter.
 *
 * Also, the replication factor can be set to 1, although if you have more
 * than one node it is probably a poor choice. You probably want a minimum
 * of 3 for the replication factor, and you probably want a minimum of
 * 3 nodes in any live cluster.
 *
 * \sa QCassandraTable::create()
 */
void QCassandraContext::create()
{
    QString q_str( QString("CREATE KEYSPACE IF NOT EXISTS %1").arg(f_context_name) );
    q_str += getKeyspaceOptions();

    QCassandraOrder create_keyspace;
    create_keyspace.setCql( q_str, QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_SUCCESS );
    create_keyspace.setClearClusterDescription(true);
    QCassandraOrderResult const create_keyspace_result(parentCassandra()->proxy()->sendOrder(create_keyspace));
    if(!create_keyspace_result.succeeded())
    {
        throw QCassandraException("keyspace creation failed");
    }

    for( auto t : f_tables )
    {
        t->create();
    }
}

/** \brief Update a context with new properties.
 *
 * This function defines a new set of properties in the specified context.
 * In general, the context will be searched in the cluster definitions,
 * updated in memory then this function called.
 */
void QCassandraContext::update()
{
    QString q_str( QString("ALTER KEYSPACE %1").arg(f_context_name) );
    q_str += getKeyspaceOptions();

    QCassandraOrder alter_keyspace;
    alter_keyspace.setCql( q_str, QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_SUCCESS );
    alter_keyspace.setClearClusterDescription(true);
    QCassandraOrderResult const alter_keyspace_result(parentCassandra()->proxy()->sendOrder(alter_keyspace));
    if(!alter_keyspace_result.succeeded())
    {
        throw QCassandraException("keyspace creation failed");
    }
}


/** \brief Drop this context.
 *
 * This function drops this context in the Cassandra database.
 *
 * Note that contexts are dropped by name so we really only use the name of
 * the context in this case.
 *
 * The QCassandraContext object is still valid afterward, although, obviously
 * no data can be read from or written to the Cassandra server since the
 * context is gone from the cluster.
 *
 * You may change the parameters of the context and call create() to create
 * a new context with the same name.
 *
 * \warning
 * If the context does not exist in Cassandra, this function call
 * raises an exception in newer versions of the Cassandra system
 * (in version 0.8 it would just return silently.) You may want to
 * call the QCassandra::findContext() function first to know whether
 * the context exists before calling this function.
 *
 * \sa QCassandra::dropContext()
 * \sa QCassandra::findContext()
 */
void QCassandraContext::drop()
{
    QString q_str(QString("DROP KEYSPACE IF EXISTS %1").arg(f_context_name));

    QCassandraOrder drop_keyspace;
    drop_keyspace.setCql( q_str, QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_SUCCESS );
    drop_keyspace.setClearClusterDescription(true);
    QCassandraOrderResult const drop_keyspace_result(parentCassandra()->proxy()->sendOrder(drop_keyspace));
    if(!drop_keyspace_result.succeeded())
    {
        throw QCassandraException("drop keyspace failed");
    }

    resetSchema();
    f_tables.clear();
}


/** \brief Drop the specified table from the Cassandra database.
 *
 * This function sends a message to the Cassandra server so the named table
 * gets droped from it.
 *
 * The function also deletes the table from memory (which means all its
 * rows and cells are also deleted.) Do not use the table after this call,
 * even if you kept a shared pointer to it. You may create a new one
 * with the same name though.
 *
 * Note that tables get dropped immediately from the Cassandra database
 * (contrary to rows).
 *
 * \param[in] table_name  The name of the table to drop.
 */
void QCassandraContext::dropTable(const QString& table_name)
{
    if(f_tables.find(table_name) == f_tables.end())
    {
        return;
    }

    // keep a shared pointer on the table
    QCassandraTable::pointer_t t(table(table_name));

    // remove from the Cassandra database
    makeCurrent();

    QString q_str(QString("DROP TABLE IF EXISTS %1.%2").arg(f_context_name).arg(table_name));

    QCassandraOrder drop_table;
    drop_table.setCql( q_str, QCassandraOrder::type_of_result_t::TYPE_OF_RESULT_SUCCESS );
    drop_table.setTimeout(5 * 60 * 1000);
    drop_table.setClearClusterDescription(true);
    QCassandraOrderResult const drop_table_result(parentCassandra()->proxy()->sendOrder(drop_table));
    if(!drop_table_result.succeeded())
    {
        throw QCassandraException("drop table failed");
    }

    // disconnect all the cached data from this table
    f_tables.remove(table_name);
}


/** \brief Clear the context cache.
 *
 * This function clears the context cache. This means all the tables, their
 * rows, and the cells of those rows all get cleared. None of these can be
 * used after this call even if you kept a shared pointer to any of these
 * objects.
 */
void QCassandraContext::clearCache()
{
    f_tables.clear();
    parentCassandra()->retrieveContextMeta( shared_from_this(), f_context_name );
}


/** \brief Get the pointer to the parent object.
 *
 * \return Shared pointer to the cassandra object.
 */
QCassandra::pointer_t QCassandraContext::parentCassandra() const
{
    QCassandra::pointer_t cassandra(f_cassandra.lock());
    if(cassandra == nullptr)
    {
        throw QCassandraException("this context was dropped and is not attached to a cassandra cluster anymore");
    }

    return cassandra;
}


} // namespace QtCassandra
// vim: ts=4 sw=4 et
