// Snap Websites Server -- snap websites server
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

#include "snapwebsites/snap_cassandra.h"

#include "snapwebsites/snapwebsites.h"
#include "snapwebsites/snap_tables.h"
#include "snapwebsites/log.h"

#include <casswrapper/schema.h>

#include <unistd.h>

#include "snapwebsites/poison.h"


namespace snap
{


snap_cassandra::snap_cassandra()
{
    // empty
}


snap_cassandra::~snap_cassandra()
{
}


void snap_cassandra::connect()
{
    // We now connect to our proxy instead. This allows us to have many
    // permanent connections to Cassandra (or some other data store) and
    // not have to have threads (at least the C/C++ driver forces us to
    // have threads for asynchronous and timeout handling...)
    //
    snap_config config("snapdbproxy");
    tcp_client_server::get_addr_port(config["listen"], f_snapdbproxy_addr, f_snapdbproxy_port, "tcp");

//std::cerr << "snap proxy info: " << f_snapdbproxy_addr << " and " << f_snapdbproxy_port << "\n";
    f_cassandra = QtCassandra::QCassandra::create();
    if(!f_cassandra)
    {
        QString const msg("could not create the QCassandra instance.");
        SNAP_LOG_FATAL(msg);
        throw snap_cassandra_not_available_exception(msg);
    }

    // everything setup to QUORUM or we get really strange errors when under
    // load (without much load, it works like a charm with ONE).
    //
    // Note: the low level library forces everything to QUORUM anyway so
    //       this call is not really useful as it stands.
    //
    f_cassandra->setDefaultConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);

    if( !f_cassandra->connect(f_snapdbproxy_addr, f_snapdbproxy_port) )
    {
        QString const msg("could not connect QCassandra to snapdbproxy.");
        SNAP_LOG_FATAL(msg);
        throw snap_cassandra_not_available_exception(msg);
    }
}


void snap_cassandra::disconnect()
{
    f_cassandra.reset();
}


QtCassandra::QCassandraContext::pointer_t snap_cassandra::get_snap_context()
{
    if( !f_cassandra )
    {
        QString msg("You must connect to cassandra first!");
        SNAP_LOG_FATAL(msg);
        throw snap_cassandra_not_available_exception(msg);
    }

    // we need to read all the contexts in order to make sure the
    // findContext() works
    //
    f_cassandra->contexts();
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    return f_cassandra->findContext(context_name);
}


QString snap_cassandra::get_snapdbproxy_addr() const
{
    return f_snapdbproxy_addr;
}


int32_t snap_cassandra::get_snapdbproxy_port() const
{
    return f_snapdbproxy_port;
}


bool snap_cassandra::is_connected() const
{
    if(!f_cassandra)
    {
        return false;
    }
    return f_cassandra->isConnected();
}


QtCassandra::QCassandraTable::pointer_t snap_cassandra::get_table(QString const & table_name)
{
    QtCassandra::QCassandraContext::pointer_t context(get_snap_context());
    if(!context)
    {
        throw snap_cassandra_not_available_exception("The snap_websites context is not available in this Cassandra database.");
    }

    // does table exist?
    QtCassandra::QCassandraTable::pointer_t table(context->findTable(table_name));
    if(!table)
    {
        SNAP_LOG_FATAL("could not find table \"")(table_name)("\" in Cassandra.");
        throw snap_cassandra_not_available_exception(QString("Table \"%1\" does not exist. Did you install a *-tables.xml file for it?").arg(table_name));
    }

    return table;
}


/** \brief Create the tables of snap_websites context.
 *
 * This function reads all the XML files found under the specified
 * path. The path is expected to be defined in the snapdbproxy.conf
 * file under the name table_schema_path.
 *
 * The format of these XML files is defined in the core system XML
 * files definition the core system tables.
 *
 * These XML files define a keyspace of one or more tables.
 *
 * The format is defined in details in libsnapwebsites/src/core-tables.xml.
 *
 * \todo
 * Create an .xsd file to verify all the table files.
 */
void snap_cassandra::create_table_list()
{
    QtCassandra::QCassandraContext::pointer_t context(get_snap_context());
    if(!context)
    {
        throw snap_cassandra_not_available_exception("The snap_websites context is not available in this Cassandra database.");
    }

    snap_tables tables;

    // user may specify multiple paths separated by a colon
    //
    snap_config parameters("snapdbproxy");
    QString const table_paths(parameters[get_name(name_t::SNAP_NAME_CORE_PARAM_TABLE_SCHEMA_PATH)]);
    snap_string_list paths(table_paths.split(':'));
    if(paths.isEmpty())
    {
        // a default if not defined
        //
        paths << "/usr/lib/snapwebsites/tables";
    }
    for(auto p : paths)
    {
        tables.load(p);
    }

    // now we have all the tables loaded
    //
    snap_tables::table_schema_t::map_t const & schemas(tables.get_schemas());
    SNAP_LOG_INFO("check existence of ")(schemas.size())(" tables...");
    for(auto const & s : schemas)
    {
        QString const table_name(s.second.get_name());

        // does table exist?
        QtCassandra::QCassandraTable::pointer_t table(context->findTable(table_name));
        if(!table
        && !s.second.get_drop())
        {
            SNAP_LOG_INFO("creating table \"")(table_name)("\"");

            // create table
            //
            // setup the name in the "constructor"
            //
            table = context->table(table_name);

            // other fields make use of a map
            //
            // for details see:
            // http://docs.datastax.com/en/cql/3.1/cql/cql_reference/tabProp.html
            //
            auto & table_fields(table->fields());

            // setup the comment for information
            //
            table_fields["comment"] = QVariant(s.second.get_name());

            // how often we want the mem[ory] tables to be flushed out
            //
            snap_tables::model_t const model(s.second.get_model());
            switch(model)
            {
            case snap_tables::model_t::MODEL_LOG:
                // 99% of the time, there is really no need to keep
                // log like data in memory, give it 5 min.
                //
                table_fields["memtable_flush_period_in_ms"] = QVariant(300);
                break;

            case snap_tables::model_t::MODEL_CONTENT:
            case snap_tables::model_t::MODEL_DATA:
            case snap_tables::model_t::MODEL_SESSION:
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
            case snap_tables::model_t::MODEL_CONTENT:
            case snap_tables::model_t::MODEL_DATA:
            case snap_tables::model_t::MODEL_SESSION:
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
            case snap_tables::model_t::MODEL_LOG:
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
            case snap_tables::model_t::MODEL_DATA:
            case snap_tables::model_t::MODEL_LOG: // TBD: we may have problems getting old tombstones removed if too large?
                // default of 10 days for heavy write but nearly no upgrades
                //
                table_fields["gc_grace_seconds"] = QVariant(864000);
                break;

            case snap_tables::model_t::MODEL_QUEUE:
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
            case snap_tables::model_t::MODEL_QUEUE:
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

            case snap_tables::model_t::MODEL_LOG:
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
            case snap_tables::model_t::MODEL_QUEUE:
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

            case snap_tables::model_t::MODEL_DATA:
            case snap_tables::model_t::MODEL_LOG:
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

            try
            {
                table->create();
            }
            catch(std::runtime_error const & e)
            {
                if(strcmp(e.what(), "table creation failed") != 0)
                {
                    // we do not know anything about that exception,
                    // let it pass propagate...
                    //
                    throw;
                }
                // passthrough on "basic" errors... because the gossiping
                // in Cassandra in regard to creating tables can generate
                // a safe race condition (note that other changes to the
                // schema may not be as safe...)
                //
                // See https://issues.apache.org/jira/browse/CASSANDRA-5025
                SNAP_LOG_TRACE("Marking a pause while schema gets synchronized after a CREATE TABLE.");
                sleep(10);
            }
        }
        else if(table
             && s.second.get_drop())
        {
            SNAP_LOG_INFO("droping table \"")(table_name)("\"");

            table.reset(); // lose that reference
            try
            {
                // this can take forever and it will work just fine, but
                // the Cassandra cluster is likely to timeout on us
                // and throw an error so we use a try/catch
                //
                context->dropTable(table_name);
            }
            catch(QtCassandra::QCassandraException const & e)
            {
                SNAP_LOG_WARNING("an exception was raised with \"")(e.what())("\"");
                sleep(10);
            }
        }
    }

    SNAP_LOG_DEBUG("all tables are ready now.");
}



}
// namespace snap
// vim: ts=4 sw=4 et
