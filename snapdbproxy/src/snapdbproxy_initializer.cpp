/*
 * Text:
 *      snapdbproxy/src/snapdbproxy_initializer.cpp
 *
 * Description:
 *      On startup, we run this thread once to make sure that all the tables
 *      exist. This process then authorizes the rest of the application to
 *      run normally. As a side effect, this thread determines the list of
 *      existing tables which can then be shared with other applications.
 *
 * License:
 *      Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
 *
 *      https://snapwebsites.org/
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

// self
//
#include "snapdbproxy.h"


// our lib
//
#include <snapwebsites/log.h>

// C++ lib
//
#include <iostream>


// last include
//
#include <snapdev/poison.h>












/** \brief Initialize the snapdbproxy_initialize object.
 *
 * The initializer object will make sure that the Cassandra database
 * cluster has a "snap_websites" context and all the tables as defined
 * in the "*-tables.xml" files.
 *
 * \attention
 * We use our a separate session because we change the timeout and in
 * a multi-threaded environment it would not otherwise be safe
 * (i.e. all the session setTimeout() + connect() calls would need to
 * be serialized.)
 *
 * \param[in] proxy  A pointer to the snapdbproxy.
 * \param[in] cassandra_host_list  The list of hosts we can connect to.
 * \param[in] cassandra_port  Port used to connect to Cassandra.
 * \param[in] use_ssl  Whether the connection to Cassandra requires SSL.
 */
snapdbproxy_initializer::snapdbproxy_initializer
        ( snapdbproxy * proxy
        , QString const & cassandra_host_list
        , int cassandra_port
        , bool use_ssl
        )
    : snap_runner("snapdbproxy_initializer")
    , f_snapdbproxy(proxy)
    , f_session(casswrapper::Session::create())
    , f_cassandra_host_list(cassandra_host_list)
    , f_cassandra_port(cassandra_port)
    , f_use_ssl(use_ssl)
    , f_context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT))
{
}


snapdbproxy_initializer::~snapdbproxy_initializer()
{
    tcp_client_server::cleanup_on_thread_exit();
}


bool snapdbproxy_initializer::continue_running() const
{
    bool const result(snap_runner::continue_running());

    if(!result)
    {
        f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_STOPPED);
    }

    return result;
}


void snapdbproxy_initializer::run()
{
    try
    {
        if(!load_tables())  return;
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_FATAL("thread received std::exception \"")(e.what())("\" while loading tables");
        f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_INVALID);
        return;
    }
    catch(...)
    {
        SNAP_LOG_FATAL("thread received an unknown exception while loading tables");
        f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_INVALID);
        return;
    }

    // in most cases, a function that fails making changes to the database
    // will throw an exception
    //
    // here we catch such exceptions and try again until the initialization
    // happened and we're asked to exit
    //
    float timeout(static_cast<float>(60.0 / 32.0));
    for(int count(0); count < 2000; ++count)
    {
        try
        {
            if(!connect())                  return;
            if(!load_cassandra_tables())    return;
            if(!load_cassandra_indexes())   return;
            if(has_missing_tables())
            {
                if(!obtain_lock())      return;
                if(!create_context())   return;
                if(!create_tables())    return;
            }
            if(has_missing_indexes())
            {
                if(!obtain_lock())      return;
                if(!create_indexes())   return;
            }
            //else -- it looks like we're good already

            f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_READY);

            // exit thread normally
            return;
        }
        catch(casswrapper::cassandra_exception_t const & e)
        {
            SNAP_LOG_WARNING("thread received QCassandraQuery::query_exception \"")(e.what())("\"");
        }
        catch(std::runtime_error const & e)
        {
            // TODO: At this point the connect() fails with an runtime_error
            //       exception... we should be able to catch a more precise
            //       error to make sure that it is indeed the connection
            //       failures we're catching here
            //
            //SNAP_LOG_WARNING("thread received std::exception \"")(e.what())("\""); -- this is too much when Cassandra doesn't respond
        }
        catch(std::exception const & e)
        {
            SNAP_LOG_ERROR("thread received std::exception \"")(e.what())("\"");
            f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_INVALID);
            return;
        }
        catch(...)
        {
            SNAP_LOG_ERROR("thread received an unknown exception");
            f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_INVALID);
            return;
        }

        f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_PAUSE);
        f_locked = false;

        // try again
        //
        sleep(static_cast<int>(timeout));
        if(timeout < 60.0f)
        {
            timeout *= 2.0f;
        }
    }

    SNAP_LOG_ERROR("thread failed initialization after about 1 day");
    f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_INVALID);
}


bool snapdbproxy_initializer::load_tables()
{
    SNAP_LOG_TRACE("load tables");

    // user may specify multiple paths separated by a colon
    //
    snap::snap_config parameters("snapdbproxy");
    QString const table_paths(parameters[snap::get_name(snap::name_t::SNAP_NAME_CORE_PARAM_TABLE_SCHEMA_PATH)]);
    snap::snap_string_list paths(table_paths.split(':'));
    if(paths.isEmpty())
    {
        // a default if not defined
        //
        paths << "/usr/lib/snapwebsites/tables";
    }
    for(auto p : paths)
    {
        f_tables.load(p);
    }

    return continue_running();
}


bool snapdbproxy_initializer::connect()
{
    SNAP_LOG_TRACE("connect to Cassandra for initialization purposes");

    // this thread uses its own special session because the default
    // session has a very short timeout which would fail all the time
    // while creating the context and tables
    //
    f_session->setTimeout(INITIALIZER_SESSION_TIMEOUT);

    // attempt connecting
    //
    f_session->connect(f_cassandra_host_list
                     , f_cassandra_port
                     , f_use_ssl);

    return continue_running();
}


bool snapdbproxy_initializer::load_cassandra_tables()
{
    SNAP_LOG_TRACE("load the name of each table from Cassandra");

    auto q = casswrapper::Query::create(f_session);
    q->query("SELECT table_name FROM system_schema.tables WHERE keyspace_name=?");
    q->bindVariant(0, QVariant(f_context_name));
    q->start();

    // get the complete list of existing tables
    //
    f_existing_tables.clear();
    do
    {
        while(q->nextRow())
        {
            f_existing_tables << q->getVariantColumn("table_name").toString();
        }
    }
    while(q->nextPage());

    q->end();

    return continue_running();
}


bool snapdbproxy_initializer::load_cassandra_indexes()
{
    SNAP_LOG_TRACE("load the name of each secondary index from Cassandra");

    auto q = casswrapper::Query::create(f_session);
    q->query("SELECT index_name FROM system_schema.indexes WHERE keyspace_name=?");
    q->bindVariant(0, QVariant(f_context_name));
    q->start();

    // get the complete list of existing tables
    //
    f_existing_indexes.clear();
    do
    {
        while(q->nextRow())
        {
            f_existing_indexes << q->getVariantColumn("index_name").toString();
        }
    }
    while(q->nextPage());

    q->end();

    return continue_running();
}


bool snapdbproxy_initializer::has_missing_tables()
{
    SNAP_LOG_TRACE("any missing tables?");

    snap::snap_tables::table_schema_t::map_t const & schemas(f_tables.get_schemas());
    for(auto s : schemas)
    {
        if(s.second.get_drop())
        {
            if(f_existing_tables.contains(s.first))
            {
                SNAP_LOG_TRACE("at least table named \"")
                              (s.first)
                              ("\" exists when it should have been dropped.");
                return true;
            }
        }
        else
        {
            if(!f_existing_tables.contains(s.first))
            {
                SNAP_LOG_TRACE("at least table named \"")
                              (s.first)
                              ("\" is missing.");
                return true;
            }
        }
    }

    return false;
}


bool snapdbproxy_initializer::has_missing_indexes()
{
    SNAP_LOG_TRACE("any missing indexes?");

    snap::snap_tables::table_schema_t::map_t const & schemas(f_tables.get_schemas());
    for(auto s : schemas)
    {
        if(!s.second.get_drop())
        {
            snap::snap_tables::secondary_index_t::map_t const & indexes(s.second.get_secondary_indexes());
            for(auto i : indexes)
            {
                if(!f_existing_indexes.contains(i.first))
                {
                    SNAP_LOG_TRACE("at least the secondary index named \"")
                                  (s.second.get_name())
                                  ("\" is missing");
                    return true;
                }
            }
        }
    }

    return false;
}


bool snapdbproxy_initializer::obtain_lock()
{
    if(!f_locked)
    {
        SNAP_LOG_TRACE("one or more tables or indexes are missing, get a lock");

        // request for the lock
        //
        f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_LOCK);

        // then poll for a change in status
        //
        // TODO: this is a REALLY UGLY poll, we need to fix that at some point
        //
        snapdbproxy::status_t status(snapdbproxy::status_t::STATUS_START);
        for(;;)
        {
            status = f_snapdbproxy->get_status();
            if(status != snapdbproxy::status_t::STATUS_LOCK)
            {
                break;
            }
            sleep(1);
        }

        // make sure we've got the correct status
        //
        if(status != snapdbproxy::status_t::STATUS_CONTEXT)
        {
            // something's wrong
            //
            throw std::runtime_error("lock obtension failed. (got status: "
                                   + std::to_string(static_cast<int>(status))
                                   + ") This can happen in the rather rare cases where the lock could not be obtained (maybe snaplock was not running or the cluster quorum was not reached?) However, once the database is setup, it should never happen again.");
        }

        f_locked = true;
    }

    return continue_running();
}


bool snapdbproxy_initializer::create_context()
{
    SNAP_LOG_TRACE("create the \"")
                  (f_context_name)
                  ("\" context");

    // if we found at least one table for our context in the list of
    // existing tables, then we know we already have the context and
    // thus there is no need to create it
    //
    if(f_existing_tables.isEmpty())
    {
        auto q = casswrapper::Query::create(f_session);
        q->query(QString("CREATE KEYSPACE IF NOT EXISTS %1"
                        " WITH durable_writes = true"
                         " AND replication = { 'class': 'NetworkTopologyStrategy', 'dc1': '1' }")
                     .arg(f_context_name));
        q->start();
        q->end();
    }

    return continue_running();
}


bool snapdbproxy_initializer::create_tables()
{
    f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_TABLES);
    SNAP_LOG_TRACE("creating tables");

    snap::snap_tables::table_schema_t::map_t const & schemas(f_tables.get_schemas());
    for(auto s : schemas)
    {
        bool const exists(f_existing_tables.contains(s.first));
        bool const drop(s.second.get_drop());
        if(!exists && !drop)
        {
            create_table(s.second);
        }
        else if(exists && drop)
        {
            drop_table(s.second);
        }
        else
        {
            // TODO: we should have code to detect changes
            //
            SNAP_LOG_TRACE("existing table \"")
                          (s.first)
                          ("\" is not going to be modified");
        }
    }

    // TODO: should we look into removing dropped tables from the list once
    //       done with them like this?
    //
    // table.reset(); // lose those references

    SNAP_LOG_TRACE("tables are ready");

    return continue_running();
}


void snapdbproxy_initializer::create_table(snap::snap_tables::table_schema_t const & schema)
{
    SNAP_LOG_INFO("creating table \"")
                 (schema.get_name())
                 ("\" ...");

    // fields make use of a map
    //
    // for details see:
    // http://docs.datastax.com/en/cql/3.1/cql/cql_reference/tabProp.html
    //
    casswrapper::schema::Value::map_t table_fields;

    // get the model for this table
    //
    snap::snap_tables::model_t const model(schema.get_model());

    // setup the comment for information
    //
    table_fields["comment"] = QVariant(schema.get_name()
                                     + " ("
                                     + snap::snap_tables::model_to_string(model)
                                     + ")");

    // how often we want the mem[ory] tables to be flushed out
    //
    switch(model)
    {
    case snap::snap_tables::model_t::MODEL_LOG:
        // 99% of the time, there is really no need to keep
        // log like data in memory, give it 5 min.
        //
        table_fields["memtable_flush_period_in_ms"] = QVariant(300);
        break;

    case snap::snap_tables::model_t::MODEL_CONTENT:
    case snap::snap_tables::model_t::MODEL_DATA:
    case snap::snap_tables::model_t::MODEL_SESSION:
        // keep the default, which is to disable the memory tables
        // flushing mechanism; this means that data stays in memory
        // as long as space is available for it
        //
        break;

    default:
        // once per hour for most of our tables, because their
        // data is not generally necessary in the memory cache
        //
        table_fields["memtable_flush_period_in_ms"] = QVariant(3600000); // Once per hour
        break;

    }

    // not so sure that we really want a read-repair mechanism
    // to run on any read, but it sounds like it work working
    // that way in older versions and since we use a ONE
    // consistency with our writes, it may be safer to have
    // a read repair at least in the few tables where we have
    // what we consider end user data
    //
    // note that all my tables used to have 0.1 and it worked
    // nicely
    //
    switch(model)
    {
    case snap::snap_tables::model_t::MODEL_CONTENT:
    case snap::snap_tables::model_t::MODEL_DATA:
    case snap::snap_tables::model_t::MODEL_SESSION:
        // 10% of the time, verify that the data being read is
        // consistent (it does not slow down our direct reads,
        // however, it makes Cassandra busier as it checks many
        // values on each node that has a copy of that data)
        //
        table_fields["read_repair_chance"] = QVariant(0.1f);
        break;

    default:
        // keep the default for the others (i.e. no repair)
        //
        break;

    }

    // force a retry on reads that timeout
    //
    // we keep the default for most tables, there are tables
    // where we do not care as much and we can turn that
    // feature off on those
    //
    switch(model)
    {
    case snap::snap_tables::model_t::MODEL_LOG:
        // no retry
        //
        table_fields["speculative_retry"] = QVariant("NONE");
        break;

    default:
        // keep the default for the others (i.e. 99%)
        //
        break;

    }

    // The following sets up how often a table should be checked
    // for tombstones; the models have quite different needs in
    // this area
    //
    // Important notes about potential problems in regard to
    // the Cassandra Gargbage Collection and tombstones not
    // being taken in account:
    //
    //   https://docs.datastax.com/en/cassandra/2.0/cassandra/dml/dml_about_deletes_c.html
    //   http://stackoverflow.com/questions/21755286/what-exactly-happens-when-tombstone-limit-is-reached
    //   http://cassandra-user-incubator-apache-org.3065146.n2.nabble.com/Crash-with-TombstoneOverwhelmingException-td7592018.html
    //
    // Garbage Collection of 1 day (could be a lot shorter for several
    // tables such as the "list", "backend" and "antihammering"
    // tables... we will have to fix that once we have our proper per
    // table definitions)
    switch(model)
    {
    case snap::snap_tables::model_t::MODEL_DATA:
    case snap::snap_tables::model_t::MODEL_LOG: // TBD: we may have problems getting old tombstones removed if too large?
        // default of 10 days for heavy write but nearly no upgrades
        //
        table_fields["gc_grace_seconds"] = QVariant(864000);
        break;

    case snap::snap_tables::model_t::MODEL_QUEUE:
        // 1h and we want a clean up; this is important in queue
        // otherwise the tombstones build up very quickly
        //
        table_fields["gc_grace_seconds"] = QVariant(3600);
        break;

    default:
        // 1 day, these tables need cleaning relatively often
        // because they have quite a few updates
        //
        table_fields["gc_grace_seconds"] = QVariant(86400);
        break;

    }

    // data can be compressed, in a few cases, there is really
    // no need for such though
    //
    switch(model)
    {
    case snap::snap_tables::model_t::MODEL_QUEUE:
        // no compression for queues
        //
        // The documentation says to use "" for "no compression"
        //
        {
            casswrapper::schema::Value compression;
            auto & compression_map(compression.map());
            compression_map["sstable_compression"] = QVariant(QString());
            table_fields["compression"] = compression;
        }
        break;

    case snap::snap_tables::model_t::MODEL_LOG:
        // data that we do not generally re-read can be
        // ultra-compressed only it will be slower to
        // decompress such data
        //
        // TBD: we could enlarge block size to 1Mb, it would
        //      help in terms of compression, but slow down
        //      (dramatically?) in term of speed and it forces
        //      that much memory to be used too...
        //
        {
            casswrapper::schema::Value compression;
            auto & compression_map(compression.map());
            compression_map["sstable_compression"] = QVariant("DeflateCompressor");
            //compression_map["chunk_length_kb"] = QVariant(64);
            //compression_map["crc_check_chance"] = QVariant(1.0); // 100%
            table_fields["compression"] = compression;
        }
        break;

    default:
        // leave the default (LZ4Compressor at the moment)
        break;

    }

    // Define the compaction mechanism; in most cases we want to
    // use the Leveled compation as it looks like there is no real
    // advantages to using the other compaction methods available
    //
    switch(model)
    {
    case snap::snap_tables::model_t::MODEL_QUEUE:
        {
            // we choose Data Tiered Compaction for queues because
            // Cassandra is smart enough to place rows with similar
            // timeout dates within the same file and just delete
            // an sstable file when all data within is past its
            // deadline
            //
            casswrapper::schema::Value compaction;
            auto & compaction_map(compaction.map());
            compaction_map["class"] = QVariant("DateTieredCompactionStrategy");
            compaction_map["min_threshold"] = QVariant(4);
            compaction_map["max_threshold"] = QVariant(10);
            compaction_map["tombstone_threshold"] = QVariant(0.02); // 2%
            table_fields["compaction"] = compaction;

            table_fields["bloom_filter_fp_chance"] = QVariant(0.1f); // suggested default is 0.01 for date compaction, but I do not see the point of having tables 10x the size
        }
        break;

    case snap::snap_tables::model_t::MODEL_DATA:
    case snap::snap_tables::model_t::MODEL_LOG:
        {
            // tables that have mainly just writes are better
            // handled with a Size Tiered Compaction (50% less I/O)
            //
            casswrapper::schema::Value compaction;
            auto & compaction_map(compaction.map());
            compaction_map["class"] = QVariant("SizeTieredCompactionStrategy");
            //compaction_map["min_threshold"] = QVariant(4);
            //compaction_map["max_threshold"] = QVariant(32);
            //compaction_map["tombstone_threshold"] = QVariant(0.2); // 20%
            table_fields["compaction"] = compaction;

            table_fields["bloom_filter_fp_chance"] = QVariant(0.1f); // suggested default is 0.01 for date compaction, but I do not see the point of having tables 10x the size
        }
        break;

    default:
        {
            casswrapper::schema::Value compaction;
            auto & compaction_map(compaction.map());
            compaction_map["class"] = QVariant("LeveledCompactionStrategy");
            //compaction_map["tombstone_threshold"] = QVariant(0.2); // 20%
            table_fields["compaction"] = compaction;

            table_fields["bloom_filter_fp_chance"] = QVariant(0.1f); // suggested for leveled compaction
        }
        break;

    }

    QString query_string(QString("CREATE TABLE IF NOT EXISTS %1.%2")
                .arg(f_context_name)
                .arg(schema.get_name()));

    bool with(false);
    switch(schema.get_kind())
    {
    //case snap::snap_tables::kind_t::KIND_THRIFT:
    default:
        // allow sorting against a "column1"
        //
        query_string += "(key BLOB,column1 BLOB,value BLOB,PRIMARY KEY(key, column1))"
                            "WITH CLUSTERING ORDER BY(column1 ASC)";
        with = true;
        break;

    case snap::snap_tables::kind_t::KIND_BLOB:
        // no "column1" at all (not required for the blobs)
        //
        query_string += "(key BLOB,value BLOB,PRIMARY KEY(key))";
        break;

    }

    // do not compact the columns if a secondary index is going to be created
    //
    if(schema.get_secondary_indexes().empty())
    {
        query_string += QString(" %1 COMPACT STORAGE")
                            .arg(with ? "AND" : "WITH");
        with = true;
    }

    for(auto const & f : table_fields)
    {
        query_string += QString(" %1 %2=%3\n")
                                .arg(with ? "AND" : "WITH")
                                .arg(f.first)
                                .arg(f.second.output());
        with = true;
    }

    auto q = casswrapper::Query::create(f_session);
    q->query(query_string);
    q->start();
    q->end();

    // if we reach here, the table was created as expected
    //
    SNAP_LOG_INFO("table \"")
                 (schema.get_name())
                 ("\" was created successfully.");
}


void snapdbproxy_initializer::drop_table(snap::snap_tables::table_schema_t const & schema)
{
    SNAP_LOG_INFO("droping table \"")(schema.get_name())("\"");

    // this can take forever and it will work just fine, but
    // the Cassandra cluster is likely to timeout on us
    // and throw an error so we use a try/catch
    //
    auto q = casswrapper::Query::create(f_session);
    q->query(QString("DROP TABLE IF EXISTS %1.%2")
                            .arg(f_context_name)
                            .arg(schema.get_name()));
    q->start();
    q->end();
}


bool snapdbproxy_initializer::create_indexes()
{
    f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_TABLES);
    SNAP_LOG_TRACE("creating indexes");

    // our secondary indexes are defined in our tables so here too
    // we loop through our table schemas
    //
    snap::snap_tables::table_schema_t::map_t const & schemas(f_tables.get_schemas());
    for(auto s : schemas)
    {
        // ignore dropped tables
        //
        if(!s.second.get_drop())
        {
            snap::snap_tables::secondary_index_t::map_t const & indexes(s.second.get_secondary_indexes());
            for(auto i : indexes)
            {
                bool const exists(f_existing_indexes.contains(i.first));
                if(!exists)
                {
                    create_index(s.second, i.second);
                }
                else
                {
                    // TODO: we should have code to detect changes
                    //
                    SNAP_LOG_TRACE("existing index \"")
                                  (i.first)
                                  ("\" is not going to be modified");
                }
            }
        }
    }

    // TODO: should we look into removing dropped tables from the list once
    //       done with them like this?
    //
    // table.reset(); // lose those references

    SNAP_LOG_TRACE("tables are ready");

    return continue_running();
}


void snapdbproxy_initializer::create_index(
              snap::snap_tables::table_schema_t const & schema
            , snap::snap_tables::secondary_index_t const & index)
{
    QString name(index.get_name());
    if(name.isEmpty())
    {
        name = index.get_column();
    }

    SNAP_LOG_INFO("creating index \"")
                 (name)
                 ("\" ...");

    QString const query_string(QString("CREATE INDEX IF NOT EXISTS %1_%2_index ON %3.%1(%4)")
                .arg(schema.get_name())
                .arg(name)
                .arg(f_context_name)
                .arg(index.get_column()));

    auto q = casswrapper::Query::create(f_session);
    q->query(query_string);
    q->start();
    q->end();

    // if we reach here, the table was created as expected
    //
    SNAP_LOG_INFO("index \"")
                 (name)
                 ("\" was created successfully.");
}





// vim: ts=4 sw=4 et
