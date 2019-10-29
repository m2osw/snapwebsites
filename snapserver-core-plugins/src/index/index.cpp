// Snap Websites Server -- advanced handling of indexes
// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
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


// self
//
#include "index.h"


// other plugins
//
#include "../links/links.h"
#include "../path/path.h"
#include "../output/output.h"
#include "../taxonomy/taxonomy.h"


// snapwebsites lib
//
#include <snapwebsites/chownnm.h>
#include <snapwebsites/dbutils.h>
#include <snapwebsites/log.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/snap_backend.h>
#include <snapwebsites/snap_expr.h>
#include <snapwebsites/snap_lock.h>


// snapdev
//
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>
#include <snapdev/tokenize_string.h>


// as2js lib
//
#include <as2js/json.h>


// csspp lib
//
#include <csspp/csspp.h>


// boost lib
//
#include <boost/algorithm/string.hpp>


// Qt lib
//
#include <QtCore>
#include <QtSql>


// C++ lib
//
#include <iostream>


// C lib
//
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(index, 1, 0)


/** \brief Get a fixed index name.
 *
 * The index plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_INDEX_DEFAULT_INDEX:
        return "default";

    case name_t::SNAP_NAME_INDEX_NUMBER_OF_RECORDS:
        return "index::number_of_records";

    case name_t::SNAP_NAME_INDEX_ORIGINAL_SCRIPTS: // text format
        return "index::original_scripts";

    case name_t::SNAP_NAME_INDEX_PAGE: // query string name "...?page=..." or "...?page-<index-name>=..."
        return "page";

    case name_t::SNAP_NAME_INDEX_PAGE_SIZE:
        return "index::page_size";

    case name_t::SNAP_NAME_INDEX_REINDEX: // --action index::reindex
        return "reindex";

    case name_t::SNAP_NAME_INDEX_TABLE:
        return "indexes";       // plural because "INDEX" is a CQL keyword

    case name_t::SNAP_NAME_INDEX_THEME: // filter function
        return "index::theme";

//    case name_t::SNAP_NAME_INDEX_SCRIPTS: // compiled -- this is a problem at the moment because we may have "many" scripts in each type
//        return "index::scripts";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_INDEX_...");

    }
    NOTREACHED();
}




namespace
{

/** \brief List of indexes that were deleted.
 *
 * When re-indexing, we need to delete old indexes, otherwise we would not
 * do a very good job.
 *
 * We could delete everything and then rebuild one list at a time. That is
 * fast, but that also means that the website is in a semi-broken state until
 * the rebuild is done.
 *
 * Another way is to delete only the entries we are working on. This way
 * only the pages that correspond to this specific indexing will be broken
 * for a little while. On a large site with many indexes, this can be
 * quite important.
 *
 * Later versions may be able to better handle the situation, but right
 * now we just create new entries, we do not remove old ones.
 *
 * \sa index::reindex()
 */
snap_string_list * g_deleted_entries = nullptr;



/** \brief The name of the file were we save the reindex of types.
 *
 * This file is a list of URL representing types to be processed.
 *
 * \warning
 * At this time, this file is local to each computer. At this point this
 * is incorrect since any backend computer could take this process over
 * and it will likely end with duplicated work.
 */
constexpr char const * const g_reindex_type_cache_filename = "/var/lib/snapwebsites/snapbackend/reindex-types.db";


/** \brief The name of the file where we save the reindex of pages.
 *
 * This file is a list of all the URL to process for a given type.
 * We save all the pages in the file to limit the mount of memory
 * it requires.
 *
 * \araning
 * As the types, the reindex pages file is found on the local computer.
 * This means the reindex process has to be bound to that one backend
 * computer.
 */
constexpr char const * const g_reindex_page_cache_filename = "/var/lib/snapwebsites/snapbackend/reindex-pages.db";



}





/** \brief Initializes an object to access an index with paging capability.
 *
 * This function initializes this paging object with defaults.
 *
 * The \p ipath parameter is the page that represent a Snap index. It
 * will be read later when you call the read_index() function.
 *
 * \note
 * By default the \p index_name parameter is set to the empty string which
 * means that the default index will be paged. You can't change the name of
 * the index once you created this object. Create a different paging_t
 * object to use a different name. The default name is defined by the
 * SNAP_NAME_INDEX_DEFAULT_INDEX variable. At this time the default is
 * "default".
 *
 * \param[in,out] snap  Pointer to the snap_child object.
 * \param[in,out] ipath  The path to the page representing an index.
 * \param[in] index_name  The name of the index to be paged.
 *
 * \sa get_query_string_info()
 */
paging_t::paging_t(snap_child * snap, content::path_info_t & ipath, QString const & index_name)
    : f_snap(snap)
    , f_ipath(ipath)
    , f_index_name(index_name.isEmpty()
                        ? get_name(name_t::SNAP_NAME_INDEX_DEFAULT_INDEX)
                        : index_name)
{
}


/** \brief Read the current page of this index.
 *
 * This function calls the index::read_index() function with the parameters
 * as defined in this paging object.
 *
 * \return The index of records as read using the index plugin.
 *
 * \sa get_start_offset()
 * \sa get_page_size()
 */
index_record_vector_t paging_t::read_index()
{
    int count(get_page_size());
    if(f_maximum_number_of_records > 0
    && count > f_maximum_number_of_records)
    {
        count = f_maximum_number_of_records;
    }
    return index::index::instance()->read_index(
              f_ipath
            , get_index_name()
            , get_start_offset() - 1
            , count
            , f_start_key);
}


/** \brief Retrieve the name of the index.
 *
 * This function returns the name of this paging object. This is the
 * name used to retrieve the current information about the index position
 * from the query string.
 *
 * The name is retrieved from the database using the referenced page.
 * It is valid to not define a name. Without a name, the simple "page"
 * query string variable is used. A name is important if the page is
 * to appear in another which also represents an index.
 *
 * \note
 * The name is cached so calling this function more than once is fast.
 *
 * \param[in] empty_if_default  Return an empty string if the index name
 * represents the default index.
 *
 * \return The name of the index.
 */
QString paging_t::get_index_name(bool empty_if_default) const
{
    if(empty_if_default
    && f_index_name != get_name(name_t::SNAP_NAME_INDEX_DEFAULT_INDEX))
    {
        return QString();
    }

    return f_index_name;
}


/** \brief Set a maximum number of records to gather.
 *
 * This function defines the maximum number of records one wants to show
 * in an index being paged. This value shadows the total number of records
 * defined in the index if that total number is larger.
 *
 * This is particularly useful to control the length an index so it does
 * not go out of hands. For example, if you create one page per day, you
 * may want to show an index of up to 30 entries (nearly one month) instead
 * of all the entries that have been created from the beginning of time.
 *
 * By default this value is set to -1 which means it has no effect. You
 * may call this function with -1 as well.
 *
 * \param[in] maximum_number_of_records  The new maximum number of records.
 */
void paging_t::set_maximum_number_of_records(int32_t maximum_number_of_records)
{
    if(maximum_number_of_records < 1)
    {
        // make sure that turning the "Off" feature is done using exactly -1
        //
        f_maximum_number_of_records = -1;
    }
    else
    {
        f_maximum_number_of_records = maximum_number_of_records;
    }
}


/** \brief Get the current maximum number of records.
 *
 * This function returns the current maximum number of records. By default
 * this value is set to -1 which means the number of records is not going
 * to get clamped.
 *
 * \return The current maximum number of records.
 */
int32_t paging_t::get_maximum_number_of_records() const
{
    return f_maximum_number_of_records;
}


/** \brief Retrieve the total number of records in an index.
 *
 * This function retrieves the total number of records found in an index.
 * This value is defined in the database under the name
 * name_t::SNAP_NAME_INDEX_NUMBER_OF_RECORDS.
 *
 * \note
 * This function always returns a positive number or zero.
 *
 * \note
 * The number is cached so this function can be called any number of
 * times with the same run, it is very fast after the first call.
 *
 * \warning
 * This is not the number of pages. Use the get_total_pages() to
 * determine the total number of pages available in an index.
 *
 * \warning
 * The exact number of records cannot currently be retrieved. This
 * function is clamped to the maximum number of records as defined
 * by set_maximum_number_of_records().
 *
 * \todo
 * Fix the caching of this number of records.
 *
 * \return The number of records in the index.
 */
int32_t paging_t::get_number_of_records() const
{
    if(f_number_of_records < 0)
    {
        // TODO: find a way to cache this number of records
        //       my problem at the moment is that I'm not too
        //       sure when to clear the cache
        //       (the cache being that SNAP_NAME_INDEX_NUMBER_OF_RECORDS field)
        //
        // if the number of records is not (yet) defined in the database
        // then it will be set to zero
        //
        //content::content * content_plugin(content::content::instance());
        //libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
        //f_number_of_records = branch_table->getRow(f_ipath.get_branch_key())
        //                                  ->getCell(get_name(name_t::SNAP_NAME_INDEX_NUMBER_OF_RECORDS))
        //                                  ->getValue().safeInt32Value();
        libdbproxy::libdbproxy::pointer_t cassandra(f_snap->get_cassandra());
        libdbproxy::context::pointer_t context(f_snap->get_context());
        libdbproxy::proxy::pointer_t dbproxy(cassandra->getProxy());

        libdbproxy::order count_index;
        count_index.setCql(QString("SELECT COUNT(*) FROM %1.%2 WHERE key=? AND column1>=? AND column1<?")
                                .arg(context->contextName())
                                .arg(get_name(name_t::SNAP_NAME_INDEX_TABLE))
                          , libdbproxy::order::type_of_result_t::TYPE_OF_RESULT_ROWS);
        count_index.setConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_ONE);    // no need to do a QUORUM count, we should still get a very good approximation

        QString index_key(f_ipath.get_key());
        if(f_index_name != get_name(name_t::SNAP_NAME_INDEX_DEFAULT_INDEX))
        {
            index_key += '#';
            index_key += f_index_name;
        }

        count_index.addParameter(index_key.toUtf8());
        count_index.addParameter(f_start_key.toUtf8());
        QString up_to(f_start_key);
        up_to[up_to.length() - 1] = up_to[up_to.length() - 1].unicode() + 1; // we expect a ':' at the end, change it into ';' (with the `++`, if another character was used, it will work too)
        count_index.addParameter(up_to.toUtf8());

        libdbproxy::order_result const count_index_result(dbproxy->sendOrder(count_index));
        if(count_index_result.succeeded())
        {
            if(count_index_result.resultCount() == 1)
            {
                QByteArray const column1(count_index_result.result(0));
                f_number_of_records = cassvalue::safeUInt64Value(column1);
            }
            else
            {
                SNAP_LOG_FATAL("The number of results for a COUNT(*) is not exactly 1?!");
                // we continue, it's surprising and "wrong" but what can we do?
            }
        }
        else
        {
            SNAP_LOG_WARNING("Error counting indexes for get_number_of_records(); page \"")
                            (f_start_key)
                            ("\" for website \"")
                            (f_ipath.get_key())
                            ("#")
                            (f_index_name)
                            ("\" from table \"")
                            (context->contextName())
                            (".")
                            (get_name(name_t::SNAP_NAME_INDEX_TABLE))
                            ("\".");
        }
    }

    // the total count may have been limited by the programmer
    //
    if(f_maximum_number_of_records == -1
    || f_number_of_records < f_maximum_number_of_records)
    {
        return f_number_of_records;
    }

    return f_maximum_number_of_records;
}


/** \brief Define the start offset to use with read_index().
 *
 * This function is used to define the start offset. By default this
 * value is set to -1 meaning that the start page parameter is used
 * instead. This is useful in case you want to show records at any
 * offset instead of an exact page multiple.
 *
 * You make set the parameter back to -1 to ignore it.
 *
 * If the offset is larger than the total number of records present in
 * the index, the read_index() will return an empty index. You may test
 * the limit using the get_number_of_records() function. This function
 * does not prevent you from using an offsets larger than the
 * number of available records.
 *
 * \warning
 * The first record offset is 1, not 0 as generally expected in C/C++.
 *
 * \param[in] start_offset  The offset at which to start showing the index.
 */
void paging_t::set_start_offset(int32_t start_offset)
{
    // any invalid number, convert to -1 (ignore)
    if(start_offset < 1)
    {
        f_start_offset = -1;
    }
    else
    {
        f_start_offset = start_offset;
    }
}


/** \brief Retrieve the start offset.
 *
 * This function returns the start offset. This represents the number
 * of the first record to return to the caller of the read_index() function.
 * The offset may point to an record after the last record in which case the
 * read_index() function will return an empty index of records.
 *
 * If the start offset is not defined (is -1) then this function calculates
 * the start offset using the start page information:
 *
 * \code
 *      return (f_page - 1) * get_page_size() + 1;
 * \endcode
 *
 * Note that since f_page can be set to a number larger than the maximum
 * number of pages, the offset returned in that situation may also be
 * larger than the total number of records present in the index.
 *
 * \note
 * The function returns one for the first record (and NOT zero as generally
 * expected in C/C++).
 *
 * \warning
 * There is no way to retrieve the f_start_offset value directly.
 *
 * \return The start offset.
 */
int32_t paging_t::get_start_offset() const
{
    int const offset(f_start_offset < 1 ? 1 : static_cast<int>(f_start_offset));
    return offset + (f_page - 1) * get_page_size();
}


/** \brief Define the start key to used against column1.
 *
 * This function defines a start key that needs to be matched for the
 * data to be considered as being part of this index.
 *
 * The paging_t object will limit the pages it selects to those that have
 * their index key starting with this string. The index key is the one you
 * defined with your "k=..." key.
 *
 * The end key will be set to your start key plus one (only the last character
 * is increased by 1.)
 *
 * By setting the `start_key` value to an empty string, you remove this
 * search constrain and end up with all the keys for this index.
 *
 * \param[in] start_key  Only select entries which key start with this string.
 *
 * \sa get_start_key()
 */
void paging_t::set_start_key(QString const & start_key)
{
    f_start_key = start_key;
}


/** \brief Return the start of the key.
 *
 * In many cases, the index key starts with a specific string representing
 * a specific list. (Contrary to lists that have a distinct definition for
 * each list.)
 *
 * This string represents that part of the key which distinguish one list
 * from another. For example, if you create elements that are attached
 * to a form of \em parent element, then the \em parent URL or number
 * (once we have a tree it should be a number) could be how you start your
 * key.
 *
 * Say you use a URL and a namespace separator to the other parts of your
 * key. You could have a key which starts this way:
 *
 *     "https://www.example.com/this/page/here::..."
 *
 * The system automatically generates the end key by adding one to the last
 * character of the start key.
 *
 * \return The start key as it stands.
 *
 * \sa set_start_key()
 */
QString const & paging_t::get_start_key() const
{
    return f_start_key;
}


/** \brief Retrieve the query string page information.
 *
 * This function reads the query string page information and saves
 * it in this paging object.
 *
 * The query string name is defined as:
 *
 * \code
 *      page
 *   or
 *      page-<index_name>
 * \endcode
 *
 * If the index name is empty or undefined, then the name of the query
 * string variable is simply "page". If the name is defined, then the
 * system adds a dash and the name of the index.
 *
 * The value of the query string is generally just the page number.
 * The number is expected to be between 1 and the total number of
 * pages available in this index. The number 1 is not required as it
 * is the default.
 *
 * Multiple numbers can be specified by separating them with commas
 * and preceeding them with a letter as follow:
 *
 * \li 'p' -- page number, the 'p' is always optional (i.e. "3" and "p3" are
 *            equivalent)
 * \li 'o' -- start offset, an record number, ignores the page number
 * \li 's' -- page size, the number of records per page
 *
 * For example, to show page 3 of an index named "blog" with 300 records,
 * showing 50 records per page, you can use:
 *
 * \code
 *      page-blog=3,s50
 *   or
 *      page-blog=p3,s50
 * \endcode
 *
 * \sa get_page()
 * \sa get_page_size()
 * \sa get_start_offset()
 */
void paging_t::process_query_string_info()
{
    // define the query string variable name
    QString variable_name(get_name(name_t::SNAP_NAME_INDEX_PAGE));
    QString const index_name(get_index_name(true));
    if(!index_name.isEmpty())
    {
        variable_name += "-";
        variable_name += index_name;
    }

    // check whether such a variable exists in the query string
    //
    if(!f_snap->get_uri().has_query_option(variable_name))
    {
        return;
    }

    // got such, retrieve it
    //
    QString const variable(f_snap->get_uri().query_option(variable_name));
    snap_string_list const params(variable.split(","));
    bool defined_page(false);
    bool defined_size(false);
    bool defined_offset(false);
    for(int idx(0); idx < params.size(); ++idx)
    {
        QString const p(params[idx]);
        if(p.isEmpty())
        {
            continue;
        }
        switch(p[0].unicode())
        {
        case 'p':   // explicit page number
            if(!defined_page)
            {
                defined_page = true;
                bool ok(false);
                int const page(p.mid(1).toInt(&ok));
                if(ok && page > 0)
                {
                    f_page = page;
                }
            }
            break;

        case 's':   // page size (number of records per page)
            if(!defined_size)
            {
                defined_size = true;
                bool ok(false);
                int const size = p.mid(1).toInt(&ok);
                if(ok && size > 0 && size <= index::INDEX_MAXIMUM_RECORDS)
                {
                    f_page_size = size;
                }
            }
            break;

        case 'o':   // start offset (specific number of records)
            if(!defined_offset)
            {
                defined_offset = true;
                bool ok(false);
                int offset = p.mid(1).toInt(&ok);
                if(ok && offset > 0)
                {
                    f_start_offset = offset;
                }
            }
            break;

        case '0': // the page number (like "p123")
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(!defined_page)
            {
                defined_page = true;
                bool ok(false);
                int page = p.toInt(&ok);
                if(ok && page > 0)
                {
                    f_page = page;
                }
            }
            break;

        }
    }
}


/** \brief Generate the query string representing this paging information.
 *
 * This function is used to generate a link to a page as defined by this
 * paging information.
 *
 * The \p page_offset parameter is expected to be zero (0) for a link
 * to the current page. It is expected to be negative to go to a previous
 * page and positive to go to a following page.
 *
 * \param[in] page_offset  The offset to the page to generate a query string for.
 *
 * \return The query string variable and value for the specified page.
 */
QString paging_t::generate_query_string_info(int32_t page_offset) const
{
    QString result(get_name(name_t::SNAP_NAME_INDEX_PAGE));
    QString const index_name(get_index_name(true));
    if(!index_name.isEmpty())
    {
        result += "-";
        result += index_name;
    }
    result += "=";

    int32_t const page_size(get_page_size());

    bool need_comma(false);
    if(f_start_offset > 1)
    {
        // keep using the offset if defined
        int32_t offset(f_start_offset + page_offset * page_size);
        if(offset <= 0)
        {
            offset = 1;
        }
        else if(offset > get_number_of_records())
        {
            offset = get_number_of_records();
        }
        result += QString("o%1").arg(offset);
        need_comma = true;
    }
    else
    {
        int32_t page(f_page + page_offset);
        int32_t const max_pages(get_total_pages());
        if(page > max_pages && max_pages != -1)
        {
            // maximum limit
            page = max_pages;
        }
        if(page < 1)
        {
            // minimum limit
            page = 1;
        }

        if(page != f_page)
        {
            // use the page only if no offset specified
            // also we do not need to specify page=1 since that is the default
            result += QString("%1").arg(page);
            need_comma = true;
        }
    }

    if(page_size != f_default_page_size)
    {
        if(need_comma)
        {
            result += "%2C";
        }
        result += QString("s%1").arg(page_size);
        need_comma = true;
    }

    if(!need_comma)
    {
        // page 1 with default size, add nothing to the query string
        return QString();
    }

    return result;
}


/** \brief Generate the query string to access the first page.
 *
 * This function calculates the query string to send the user to the
 * first page of this index. The first page is often represented by an
 * empty query string so this function may return such when the offset
 * was not specified and no specific page size was defined.
 *
 * \return The query string to send the user to the last page.
 */
QString paging_t::generate_query_string_info_for_first_page() const
{
    if(f_start_offset > 0)
    {
        int32_t const page_size(get_page_size());
        return generate_query_string_info((1 - f_start_offset + page_size - 1) / page_size);
    }

    return generate_query_string_info(1 - f_page);
}


/** \brief Generate the query string to access the last page.
 *
 * This function calculates the query string to send the user to the
 * last page of this index. The last page may be the first page in
 * which case the function may return an empty string.
 *
 * \return The query string to send the user to the last page.
 */
QString paging_t::generate_query_string_info_for_last_page() const
{
    int32_t const max_pages(get_total_pages());
    if(max_pages == -1)
    {
        // this also represents the very first page with the default
        // page size... but without a valid max_pages, what can we do
        // really?
        return QString();
    }

    if(f_start_offset > 0)
    {
        int32_t const page_size(get_page_size());
        return generate_query_string_info((get_number_of_records() - f_start_offset + page_size - 1) / page_size);
    }

    return generate_query_string_info(max_pages - f_page);
}


/** \brief Generate a set of anchors for navigation purposes.
 *
 * This function generates the navigation anchors used to let the
 * end user move between pages quickly.
 *
 * \todo
 * The next / previous anchors make use of characters that the end
 * user should be able to change (since we have access to the index
 * we can define them in the database.)
 *
 * \param[in,out] element  A QDomElement object where we add the navigation
 *                         elements.
 * \param[in] uri  The URI used to generate the next/previous, pages 1, 2, 3...
 * \param[in] next_previous_count  The number of anchors before and after
 *                                 the current page.
 * \param[in] next_previous  Whether to add a next and previous set of anchors.
 * \param[in] first_last  Whether to add a first and last set of anchors.
 * \param[in] next_previous_page  Whether to add a ... for next and previous
 *                                pages.
 */
void paging_t::generate_index_navigation(QDomElement element
                                       , snap_uri uri
                                       , int32_t next_previous_count
                                       , bool const next_previous
                                       , bool const first_last
                                       , bool const next_previous_page) const
{
    if(element.isNull())
    {
        return;
    }

    // no navigation necessary if the number of records is limited and
    // that limit is smaller or equal to the size of one page
    if((f_maximum_number_of_records != -1 && f_maximum_number_of_records <= f_page_size)
    || get_number_of_records() <= f_page_size)
    {
        return;
    }

    QDomDocument doc(element.ownerDocument());
    QDomElement ul(doc.createElement("ul"));

    // add a root tag to encompass all the other tags
    //
    QString index_name(get_index_name(true));
    if(!index_name.isEmpty())
    {
        index_name = " " + index_name;
    }
    ul.setAttribute("class", "index-navigation" + index_name);
    element.appendChild(ul);

    // generate the URIs in before/after the current page
    int32_t first(0);
    int32_t last(0);
    int32_t current_index(0);
    snap_string_list qs;
    QString const current_page_query_string(generate_query_string_info(0));
    qs.push_back(current_page_query_string);
    for(int32_t i(-1); i >= -next_previous_count; --i)
    {
        QString const query_string(generate_query_string_info(i));
        if(qs.first() == query_string)
        {
            break;
        }
        if(i < first)
        {
            first = i;
        }
        qs.push_front(query_string);
    }
    current_index = qs.size() - 1;
    for(int32_t i(1); i <= next_previous_count; ++i)
    {
        QString const query_string(generate_query_string_info(i));
        if(qs.last() == query_string)
        {
            break;
        }
        if(i > last)
        {
            last = i;
        }
        qs.push_back(query_string);
    }

    // add the first anchor only if we are not on the first page
    if(first_last && first < 0)
    {
        // add the first button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "index-navigation-first");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info_for_first_page());
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x21E4))));
        anchor.appendChild(text);
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    // add the previous anchor only if we are not on the first page
    if(next_previous && first < 0)
    {
        // add the previous button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "index-navigation-previous");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info(-1));
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x2190))));
        anchor.appendChild(text);
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    if(next_previous_page && first < 0)
    {
        QString const query_string(generate_query_string_info(-1 - next_previous_count));
        if(qs.first() != query_string)
        {
            // add the previous page button
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "index-navigation-previous-page");
            ul.appendChild(li);

            snap_uri anchor_uri(uri);
            anchor_uri.set_query_string(generate_query_string_info(-1 - next_previous_count));
            QDomElement anchor(doc.createElement("a"));
            QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x2026))));
            anchor.appendChild(text);
            anchor.setAttribute("href", "?" + anchor_uri.query_string());
            li.appendChild(anchor);
        }
    }

    // add the navigation links now
    int32_t const max_qs(qs.size());
    for(int32_t i(0); i < max_qs; ++i)
    {
        QString query_string(qs[i]);
        if(i == current_index)
        {
            // the current page (not an anchor)
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "index-navigation-current");
            ul.appendChild(li);
            QDomText text(doc.createTextNode(QString("%1").arg(f_page)));
            li.appendChild(text);
        }
        else if(i < current_index)
        {
            // a previous anchor
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "index-navigation-preceeding-page");
            ul.appendChild(li);

            snap_uri anchor_uri(uri);
            anchor_uri.set_query_string(query_string);
            QDomElement anchor(doc.createElement("a"));
            QDomText text(doc.createTextNode(QString("%1").arg(f_page + i - current_index)));
            anchor.appendChild(text);
            anchor.setAttribute("href", "?" + anchor_uri.query_string());
            li.appendChild(anchor);
        }
        else
        {
            // a next anchor
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "index-navigation-following-page");
            ul.appendChild(li);

            snap_uri anchor_uri(uri);
            anchor_uri.set_query_string(query_string);
            QDomElement anchor(doc.createElement("a"));
            QDomText text(doc.createTextNode(QString("%1").arg(f_page + i - current_index)));
            anchor.appendChild(text);
            anchor.setAttribute("href", "?" + anchor_uri.query_string());
            li.appendChild(anchor);
        }
    }

    if(next_previous_page && last > 0)
    {
        QString const query_string(generate_query_string_info(next_previous_count + 1));
        if(qs.last() != query_string)
        {
            // add the previous page button
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "index-navigation-previous-page");
            ul.appendChild(li);

            snap_uri anchor_uri(uri);
            anchor_uri.set_query_string(generate_query_string_info(next_previous_count + 1));
            QDomElement anchor(doc.createElement("a"));
            QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x2026))));
            anchor.appendChild(text);
            anchor.setAttribute("href", "?" + anchor_uri.query_string());
            li.appendChild(anchor);
        }
    }

    // add the previous anchor only if we are not on the first page
    if(next_previous && last > 0)
    {
        // add the previous button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "index-navigation-next");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info(1));
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x2192))));
        anchor.appendChild(text);
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    // add the last anchor only if we are not on the last page
    if(first_last && last > 0)
    {
        // add the last button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "index-navigation-last");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info_for_last_page());
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x21E5))));
        anchor.appendChild(text);
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    QDomElement div_clear(doc.createElement("div"));
    div_clear.setAttribute("class", "div-clear");
    element.appendChild(div_clear);
}


/** \brief Define the page with which the index shall start.
 *
 * This function defines the start page you want to read with the read_index()
 * function. By default this is set to 1 to represent the very first page.
 *
 * This parameter must be at least 1. If larger than the total number of
 * pages available, then the read_index() will return an empty index.
 *
 * \param[in] page  The page to read with read_index().
 */
void paging_t::set_page(int32_t page)
{
    // make sure this is at least 1
    f_page = std::max(1, page);
}


/** \brief Retrieve the start page.
 *
 * This function retrieves the page number that is to be read by the
 * read_index() function. The first page is represented with 1 and not
 * 0 as normally expected by C/C++.
 *
 * \note
 * The page number returned here will always be 1 or more.
 *
 * \return The start page.
 */
int32_t paging_t::get_page() const
{
    return f_page;
}


/** \brief Calculate the next page number.
 *
 * This function calculates the page number to use to reach the next
 * page. If the current page is the last page, then this function
 * returns -1 meaning that there is no next page.
 *
 * \warning
 * The function returns -1 if the total number of pages is not
 * yet known. That number is known only after you called The
 * read_index() at least once.
 *
 * \return The next page or -1 if there is no next page.
 */
int32_t paging_t::get_next_page() const
{
    int32_t const max_pages(get_total_pages());
    if(f_page >= max_pages || max_pages == -1)
    {
        return -1;
    }
    return f_page + 1;
}


/** \brief Calculate the previous page number.
 *
 * This function calculates the page number to use to reach the
 * previous page. If the current page is the first page, then this
 * function returns -1 meaning that there is no previous page.
 *
 * \return The previous page or -1 if there is no previous page.
 */
int32_t paging_t::get_previous_page() const
{
    if(f_page <= 1)
    {
        return -1;
    }

    return f_page - 1;
}


/** \brief Calculate the total number of pages.
 *
 * This function calculates the total number of pages available in
 * an index. This requires the total number of records available and
 * thus it is known only after the read_index() function was called
 * at least once.
 *
 * Note that an index may be empty. In that case the function returns
 * zero (no pages available.)
 *
 * \return The total number of pages available.
 */
int32_t paging_t::get_total_pages() const
{
    int32_t const page_size(get_page_size());
    return (get_number_of_records() + page_size - f_start_offset) / page_size;
}


/** \brief Set the size of a page.
 *
 * Set the number of records to be presented in a page.
 *
 * The default index paging mechanism only supports a constant
 * number of records per page.
 *
 * By default the number of records in a page is defined using the
 * database name_t::SNAP_NAME_INDEX_PAGE_SIZE from the branch table. This
 * function can be used to force the size of a page and ignore
 * the size defined in the database.
 *
 * \param[in] page_size  The number of records per page for that index.
 *
 * \sa get_page_size()
 */
void paging_t::set_page_size(int32_t page_size)
{
    f_page_size = std::max(1, page_size);
}


/** \brief Retrieve the number of records per page.
 *
 * This function returns the number of records defined in a page.
 *
 * By default the function reads the size of a page for a given index
 * by reading the size from the database. This way it is easy for the
 * website owner to change that size.
 *
 * If the size is not defined in the database, then the DEFAULT_PAGE_SIZE
 * value is used (20 at the time of writing.)
 *
 * If you prefer to enforce a certain size for your index, you may call
 * the set_page_size() function. This way the data will not be hit.
 *
 * \return The number of records defined in one page.
 *
 * \sa set_page_size()
 */
int32_t paging_t::get_page_size() const
{
    if(f_default_page_size < 1)
    {
        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
        f_default_page_size = branch_table->getRow(f_ipath.get_branch_key())
                                          ->getCell(get_name(name_t::SNAP_NAME_INDEX_PAGE_SIZE))
                                          ->getValue().safeInt32Value();
        if(f_default_page_size < 1)
        {
            // not defined in the database, bump it to 20
            f_default_page_size = DEFAULT_PAGE_SIZE;
        }
    }

    if(f_page_size < 1)
    {
        f_page_size = f_default_page_size;
    }

    return f_page_size;
}





/** \class index
 * \brief The index plugin to handle indexes of pages.
 *
 * The index plugin makes use of many references and links and thus it
 * is documented here:
 *
 *
 * 1) Indexes are defined in the pages representing the type of a page.
 *    These pages are also called the taxonomy.
 *
 *    The taxonomy pages are found under this page:
 *
 * \code
 *     /types/taxonomy
 * \endcode
 *
 * We use the taxonomy tree to find all the indexes found on a website.
 * This gives us a way to manage all the indexes in a loop.
 *
 *
 *
 * 2) Records in an index appear in the `index` table. Note that the
 *    same record may appear in more than one index.
 *
 *
 * 3) The index table includes links to all the pages that are part of
 *    the index. These links do not use the standard link capability
 *    because the records are defined in the `index` table instead and
 *    they are sorted by their key. The keys are sorted by Cassandra
 *    using a binary compare (locales are ignored--anyone one table
 *    can only support one locale...)
 *
 * \code
 *    index::records::<sort key>
 * \endcode
 *
 * Important Note: We do not have a double link. However, the value column
 * of the index table has a Cassandra secondary index so we can search with
 * a CQL order
 *
 * \code
 *     SELECT ... WHERE value = '<page URI>'
 * \endcode
 *
 * \note
 * We do not repair index links when a page is cloned. If the clone is
 * to be part of an index the links will be updated accordingly. This
 * means if you do not write specialized code to make sure the clone
 * is an index, the "index::type" link is missing and thus no checks
 * are done to update the index data of the clone which by default will
 * be empty (inexistant may be a better way to describe this one.)
 */




/** \fn index::index_modified(content::path_info_t & ipath)
 * \brief Signal that an index was modified.
 *
 * In some cases you want to immediately be alerted of a change in an index.
 * The functions that modify indexes (add or remove elements from indexes)
 * end by calling this signal. The parameter is the path to the index that
 * changed.
 *
 * Indexes that are newly created get all their elements added and once
 * and then the index_modified() function gets called.
 *
 * Indexes that get many pages added at once, but are not new, will get
 * this signal called once per element added or removed.
 *
 * \note
 * Remember that although you are running in a backend, it is timed
 * and indexes should not take more than 10 seconds to all be worked on
 * before another website gets a chance to be worked on. It is more
 * polite to do the work you need to do quickly or memorize what needs
 * to be done and do it in your backend process instead of the pageindex
 * process if it is to take a quite long time to finish up.
 *
 * \param[in] ipath  The path to the index that was just updated (added/removed
 *                   an record in that index.)
 */




/** \brief Initialize the index plugin.
 *
 * This function is used to initialize the index plugin object.
 */
index::index()
{
}


/** \brief Clean up the index plugin.
 *
 * Ensure the index object is clean before it is gone.
 */
index::~index()
{
}


/** \brief Get a pointer to the index plugin.
 *
 * This function returns an instance pointer to the index plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the index plugin.
 */
index * index::instance()
{
    return g_plugin_index_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString index::icon() const
{
    return "/images/index/index-logo-64x64.png";
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString index::description() const
{
    return "Generate indexes of pages using a set of parameters as defined"
          " in said page type.";
}


/** \brief Return our dependencies.
 *
 * This function builds the index of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our index of dependencies.
 */
QString index::dependencies() const
{
    return "|filter|layout|links|messages|output|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t index::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2019, 3, 6, 21, 35, 3, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to
 *                              the database by this update (in micro-seconds).
 */
void index::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the index.
 *
 * This function terminates the initialization of the index plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void index::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(index, "server", server, attach_to_session);
    //SNAP_LISTEN (index, "server", server, register_backend_cron, _1);
    SNAP_LISTEN (index, "server", server, register_backend_action, _1);
    SNAP_LISTEN (index, "content", content::content, create_content, _1, _2, _3);
    SNAP_LISTEN (index, "content", content::content, modified_content, _1);
    SNAP_LISTEN (index, "content", content::content, copy_branch_cells, _1, _2, _3);
    SNAP_LISTEN (index, "links", links::links, modified_link, _1, _2);
    SNAP_LISTEN (index, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN (index, "filter", filter::filter, token_help, _1);

    //SNAP_TEST_PLUGIN_SUITE_LISTEN(index);
}


/** \brief Get the index table.
 *
 * The index table is used to generate indexes sorted using the table
 * `key` and `column1`.
 *
 * The `key` is always the name of the website currently in use. We actually
 * use the type URL (i.e. the index of a page of type "journal" will be
 * found with its `key` set to the taxonomy type named "journal".)
 *
 * `column1` is the value as generated by the index key script. This allows
 * for describing the order in which you prefer to have the data. (i.e. more
 * or less the equivalent to an `ORDER BY` in an SQL database.)
 *
 * You must ensure the uniqueness of this key on a per page basis.
 *
 * The `value` column includes the full URL to the page part of this index.
 * We create a secondary index on this column allowing us a pretty fast way
 * to delete all the entries to a given page which was removed from an
 * index.
 *
 * \return The pointer to the index table.
 */
libdbproxy::table::pointer_t index::get_index_table()
{
    if(!f_index_table)
    {
        f_index_table = f_snap->get_table(get_name(name_t::SNAP_NAME_INDEX_TABLE));
    }
    return f_index_table;
}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the \<page\> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void index::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Signal that a page was created.
 *
 * This function is called whenever the content plugin creates a new page.
 * At that point the page may not yet be complete so we could not handle
 * the possible index updates.
 *
 * So instead the function saves the full key to the page that was just
 * created so indexes that include this page can be updated by the backend
 * as required.
 *
 * \param[in,out] ipath  The path to the page being modified.
 * \param[in] owner  The plugin owner of the page.
 * \param[in] type  The type of the page.
 */
void index::on_create_content(content::path_info_t & ipath, QString const & owner, QString const & type)
{
    NOTUSED(owner);
    NOTUSED(type);

    on_modified_content(ipath); // same as on_modified_content()
}


/** \brief Signal that a page was modified by a new link.
 *
 * This function is called whenever the links plugin modifies a page by
 * adding a link or removing a link. By now the page should be quite
 * complete, outside of other links still missing.
 *
 * \param[in] link  The link that was just created or deleted.
 * \param[in] created  Whether the link was created (true) or deleted (false).
 */
void index::on_modified_link(links::link_info const & link, bool const created)
{
    NOTUSED(created);

    content::path_info_t ipath;
    ipath.set_path(link.key());
    on_modified_content(ipath); // same as on_modified_content()
}


/** \brief Signal that a page was modified.
 *
 * This function is called whenever a plugin modified a page and then called
 * the modified_content() signal of the content plugin.
 *
 * The function needs to save the information so the pageindex backend has
 * a chance to process that modified page.
 *
 * The key used to handle this information includes the following 4 parameters:
 *
 * \li The protocol + website complete domain name
 *
 * The "protocol + website complete domain name"
 * (such as "https://snapwebsites.org/") is used to aggregate the data changes
 * on a per website basis. This is important for the backend processing which
 * happens on one website at a time.
 *
 * \li The current priority
 *
 * The priority is used to handle entries with a lower priority first.
 *
 * The backend is responsible for the final sorting and removal of duplicates.
 * Here we just append data to a journal and let a backend process send the
 * data to the pageindex process. We want to indexes to work so we want the
 * pageindex to acknowledge the fact that it received our requests (and save
 * them in its own journals) so we use a backend. If the pageindex is down
 * a the time we process many indexes, it will cummulate on a computer or
 * another but the data won't be lost.
 *
 * \li The start date + start date offset
 *
 * The time defined by "start date + start date offset" is used to make sure
 * that this page is handled on or after that time (too soon and the page
 * may not yet be ready!)
 *
 * This date is also used by the sorting algorithm.
 *
 * \li The ipath URL
 *
 * The ipath URL represents the page to be updated.
 *
 * This parameter is also used in the sorting algorithm, but in a different
 * way: if two requests are made with the same URL, then only the one with
 * the largest date (start date + start date offset) is kept. The others
 * are dropped.
 *
 * \param[in,out] ipath  The path to the page being modified.
 */
void index::on_modified_content(content::path_info_t & ipath)
{
    // there are times when you may want to debug your code to know which
    // pages are marked as modified; this debug log will help with that
    //
    SNAP_LOG_DEBUG("index detected that page \"")(ipath.get_key())("\" was modified.");

    // save the page URL to a list of pages to manage once down with this
    // access; the 
    //
    f_page << ipath.get_key();
}


/** \brief Capture the last event.
 *
 * This function goes through the list of pages that were created and/or
 * updated to make sure that the index is properly maintained.
 *
 * At that point we assume that the page is modified and in its \em final
 * state for a while.
 *
 * \todo
 * This could be run by an event that happens after we sent all the
 * results to the client so we do not make the client wait on this
 * work. Of course, that means the client may then send another request
 * very quickly hoping that the results are ready when they're not.
 */
void index::on_attach_to_session()
{
    // any new or updated pages?
    //
    for(auto p : f_page)
    {
        content::path_info_t page_ipath;
        page_ipath.set_path(p);

        taxonomy::taxonomy * taxonomy_plugin(taxonomy::taxonomy::instance());

        // check the page type(s)
        //
        libdbproxy::value const index_scripts(taxonomy_plugin->find_type_with(
                  page_ipath
                , content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE)
                , get_name(name_t::SNAP_NAME_INDEX_ORIGINAL_SCRIPTS)
                , content::get_name(content::name_t::SNAP_NAME_CONTENT_CONTENT_TYPES_NAME)));
        if(!index_scripts.nullValue())
        {
            // we found index scripts, handle this page now
            //
            content::path_info_t type_ipath(taxonomy_plugin->get_type_ipath());
            index_pages(page_ipath, type_ipath, index_scripts.stringValue());
        }
    }
}


/** \brief Index the specified page using the specified script.
 *
 * This function executes the index script against the specified page.
 *
 * The index script defines how the page gets indexed. Especially, it
 * defines the list of fields used to sort the pages between each others.
 *
 * Resulting lists that are small enough can then benefit from different
 * `ORDER BY`. If really necessary, you can create multiple indexes to
 * have different `ORDER BY` when the number of pages is likely to
 * grow over 1,000.
 *
 * The index definition is expected to be an array of JSON objects.
 * The object fields define how the corresponding index is generated.
 * We currently support the following:
 *
 * \li `"c"` -- the check script
 * \li `"k"` -- the key script
 * \li `"n"` -- the name of this index
 *
 * The name is used to define the key of the index entry. The key entry
 * is the URL to the type of the page so if you want multiple indexes
 * (different `ORDER BY`) you need to be able to distinguish those
 * indexes and this is done with the name. The name is used after a
 * hash character (`https://example.com/types/taxonomy/...#\<name>`).
 * The name is reused whenever you want to read the index.
 *
 * These are the same as the list, except they appear together in the
 * same field, so we limit the number of access to the database, but
 * also we do not have to carry another structure around to use both
 * scripts.
 *
 * The "k" parameter is required to have an index. If removed, then the
 * index will get deleted. This is how you want to remove an index properly:
 *
 * \code
 *      // 1. create a new index
 *      //
 *      [
 *          {
 *              "c": "<check-script>",
 *              "k": "<key-script>",
 *              "n": "<script-name>"
 *          }
 *      ]
 *
 *      // 2. update a existing index
 *      //
 *      [
 *          {
 *              "c": "<new-check-script>",
 *              "k": "<new-key-script>",
 *              "n": "<script-name>"        // name is immutable
 *          }
 *      ]
 *
 *      // 3. remove an index
 *      //
 *      //    (if name was not defined, then it was viewed as "default"
 *      //    so you can use `"n": "default"` as something is required
 *      //    or the corresponding entries in the index table will remain
 *      //    there forever...)
 *      //
 *      [
 *          {
 *              "n": "<script-name>"        // name is immutable
 *          }
 *      ]
 *
 *      // 4. assuming the index_pages() ran at least once
 *      //    you can now remove the index altogether
 *      //
 * \endcode
 *
 * \todo
 * Look into having the scripts pre-compiled.
 *
 * \todo
 * Have a function to reset an index. Especially, if an index script is
 * modified, then we need to regenerate that entire index. (At this time
 * we have a reindex() which we can run from the command line with
 * "index::reindex")
 *
 * \param[in] page_ipath  The path to the page being indexed.
 * \param[in] type_ipath  The path to the page representing the index.
 * \param[in] scripts  The index scripts to execute.
 *
 * \return true if a delete and/or an insert happened.
 */
void index::index_pages(
              content::path_info_t & page_ipath
            , content::path_info_t & type_ipath
            , QString const & scripts)
{
    // ever had a script here?
    //
    if(scripts.isEmpty())
    {
        return;
    }

    // prepare input
    //
    as2js::String json_string;
    json_string.from_utf8(scripts.toUtf8().data());
    as2js::StringInput::pointer_t scripts_json_input(new as2js::StringInput(json_string));

    // parse input to objects
    //
    as2js::JSON::pointer_t scripts_json(new as2js::JSON);
    as2js::JSON::JSONValue::pointer_t scripts_json_value(scripts_json->parse(scripts_json_input));

    // make sure the parser was happy
    //
    if(!scripts_json_value)
    {
        // TBD: should we just delete our data and start over?
        //
        SNAP_LOG_ERROR("invalid JSON for the index_pages() list of scripts \"")
                      (scripts)
                      ("\".");
        return;
    }

    // get the list of scripts
    //
    // the function throws if the root is not an array
    //
    // TODO: support an object from the top if only one index is defined
    //
    as2js::JSON::JSONValue::array_t const & script_list(scripts_json_value->get_array());

    for(auto s : script_list)
    {
        // 'script' is expected to be an object, we transform that in a map
        // of name/value
        //
        as2js::JSON::JSONValue::object_t const & script(s->get_object());
        variables_t vars;
        for(auto v : script)
        {
            QString const name(QString::fromUtf8(v.first.to_utf8().c_str()));
            QString const value(QString::fromUtf8(v.second->get_string().to_utf8().c_str()));
            vars[name] = value;
        }

        index_one_page(page_ipath, type_ipath, vars);
    }
}


void index::index_one_page(
              content::path_info_t & page_ipath
            , content::path_info_t & type_ipath
            , variables_t const & vars)
{
    QString index_key(type_ipath.get_key());
    if(vars.find("n") != vars.end()
    && vars.at("n") != get_name(name_t::SNAP_NAME_INDEX_DEFAULT_INDEX))
    {
        index_key += "#";
        index_key += vars.at("n");
    }

    if(g_deleted_entries != nullptr
    && !g_deleted_entries->contains(index_key))
    {
        *g_deleted_entries << index_key;

        libdbproxy::libdbproxy::pointer_t cassandra(f_snap->get_cassandra());
        libdbproxy::context::pointer_t context(f_snap->get_context());
        libdbproxy::proxy::pointer_t dbproxy(cassandra->getProxy());

        // DELETE FROM snap_websites.index
        //       WHERE key = '<website>';
        //
        libdbproxy::order delete_index;
        delete_index.setCql(QString("DELETE FROM %1.%2 WHERE key=?")
                                .arg(context->contextName())
                                .arg(get_name(name_t::SNAP_NAME_INDEX_TABLE))
                          , libdbproxy::order::type_of_result_t::TYPE_OF_RESULT_SUCCESS);
        delete_index.setConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_ONE);

        delete_index.addParameter(index_key.toUtf8());

        libdbproxy::order_result const delete_index_result(dbproxy->sendOrder(delete_index));

        // report error, but continue since we're just trying to delete
        //
        if(!delete_index_result.succeeded())
        {
            SNAP_LOG_WARNING("Error deleting indexes for website \"")
                            (page_ipath.get_key())
                            ("\" from table \"")
                            (context->contextName())
                            (".")
                            (get_name(name_t::SNAP_NAME_INDEX_TABLE))
                            ("\"");
        }
    }

    QString key;
    if(!vars.empty()
    && vars.find("k") != vars.end())
    {
        key = get_key_of_index_page(page_ipath, type_ipath, vars);
    }

    {
        // when no key: make sure it's not in any index
        // when there is a key: make sure other keys for the same value get
        //                      deleted before we do a new insert
        //
        libdbproxy::libdbproxy::pointer_t cassandra(f_snap->get_cassandra());
        libdbproxy::context::pointer_t context(f_snap->get_context());
        libdbproxy::proxy::pointer_t dbproxy(cassandra->getProxy());

        // DELETE FROM snap_websites.index
        //       WHERE key = '<website>'
        //         AND value = '<ipath.get_key()>'
        //         AND column1 <> '<key>';  // <- only when updating
        //
        // however, cassandra does not allow a DELETE FROM with such a
        // complicated WHERE on any other column than the `key` column,
        // so we instead have to do a SELECT:
        //
        // SELECT column1 FROM snapwebsites.index
        //               WHERE key = '<website>'
        //                 AND value = '<ipath.get_key()>';
        //
        // From the results of the SELECT send one
        // DELETE per entry found. The DELETE ends up looking like this:
        //
        // DELETE FROM snap_websites.index
        //       WHERE key = '<website>'
        //         AND column1 = '<key>';
        //
        // this DELETE works because we have a second index on the `value`
        // column. Our loop will skip the DELETE when `key` equal `column1`.
        // (to replicate the effect of the `column` <> '<key>'` above.)
        // We can always compare since key will be empty when all entries
        // have to be deleted.
        //
        // Note: the SELECT is unconventional for us so we have to use the
        //       low level interface and emit a CQL command directly
        //
        libdbproxy::order select_index;
        select_index.setCql(QString("SELECT column1 FROM %1.%2 WHERE key=? AND value=?")
                                .arg(context->contextName())
                                .arg(get_name(name_t::SNAP_NAME_INDEX_TABLE))
                          , libdbproxy::order::type_of_result_t::TYPE_OF_RESULT_ROWS);
        select_index.setConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_QUORUM);

        select_index.addParameter(index_key.toUtf8());
        select_index.addParameter(page_ipath.get_key().toUtf8());

        libdbproxy::order_result const select_index_result(dbproxy->sendOrder(select_index));
        if(!select_index_result.succeeded())
        {
            SNAP_LOG_WARNING("Error selecting indexes for deletion; page \"")
                            (index_key)
                            ("\" for website \"")
                            (page_ipath.get_key())
                            ("\" from table \"")
                            (context->contextName())
                            (".")
                            (get_name(name_t::SNAP_NAME_INDEX_TABLE))
                            ("\".");
        }

        QByteArray key_value(key.toUtf8());

        // count should be 0 or 1 in this case
        // although we allow for more, just in case something went wrong
        // at some point (a DELETE failed?!)
        //
        size_t const max_results(select_index_result.resultCount());
        for(size_t idx(0); idx < max_results; ++idx)
        {
            QByteArray const column1(select_index_result.result(idx));
            if(column1 != key_value)    // avoid deleting the key we're about to update (the effect is the same, it's one less CQL order, though)
            {
                libdbproxy::order delete_index;
                delete_index.setCql(QString("DELETE FROM %1.%2 WHERE key=? AND column1=?")
                                        .arg(context->contextName())
                                        .arg(get_name(name_t::SNAP_NAME_INDEX_TABLE))
                                  , libdbproxy::order::type_of_result_t::TYPE_OF_RESULT_SUCCESS);
                delete_index.setConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_ONE);

                delete_index.addParameter(index_key.toUtf8());
                delete_index.addParameter(column1);

                libdbproxy::order_result const delete_index_result(dbproxy->sendOrder(delete_index));

                // report error, but continue since we're just trying to delete
                //
                if(!delete_index_result.succeeded())
                {
                    SNAP_LOG_WARNING("Error deleting indexes pointing to page \"")
                                    (index_key)
                                    ("\" for website \"")
                                    (page_ipath.get_key())
                                    ("\" from table \"")
                                    (context->contextName())
                                    (".")
                                    (get_name(name_t::SNAP_NAME_INDEX_TABLE))
                                    ("\".");
                }
            }
        }
    }

    if(!key.isEmpty())
    {
        // we got a valid key, add this page to the index
        //
        libdbproxy::table::pointer_t index_table(get_index_table());
        index_table->getRow(index_key)
                   ->getCell(key)
                   ->setValue(page_ipath.get_key());
    }
}


QString index::get_key_of_index_page(
              content::path_info_t & page_ipath
            , content::path_info_t & type_ipath
            , variables_t const & vars)
{
    // if we have no scripts at all, then we do nothing (this is not part of
    // an index)
    //
    // WARNING: this is not 100% correct to update existing scripts;
    //          i.e. it will not remove the items already present in the
    //          index table
    //
    if(vars.empty()
    || vars.find("k") == vars.end())
    {
        // without at least a key script, do nothing
        //
        return QString();
    }

    // if there is no check script then we assume that all pages of that
    // specific type are always included in this index
    //
    // WARNING: notice that this is the opposite of the list behavior which
    //          is to assume false by default in this case.
    //
    if(vars.find("c") != vars.end())
    {
        // compile and execute the check script to see whether this page has
        // to be included in the index
        //
        snap_expr::expr::expr_pointer_t e(new snap_expr::expr);
        if(!e->compile(vars.at("c")))
        {
            // script could not be compiled (invalid script!)
            //
            // TODO: generate an error message to the admin
            //
            SNAP_LOG_ERROR("Error compiling check script: \"")
                          (vars.at("c"))
                          ("\".");
            return QString();
        }

        // run the script with this path
        //
        snap_expr::variable_t result;
        snap_expr::variable_t::variable_map_t variables;
        snap_expr::variable_t var_path("path");
        var_path.set_value(page_ipath.get_cpath());
        variables["path"] = var_path;
        snap_expr::variable_t var_page("page");
        var_page.set_value(page_ipath.get_key());
        variables["page"] = var_page;
        snap_expr::variable_t var_index("index");
        var_index.set_value(type_ipath.get_key());
        variables["index"] = var_index;
        snap_expr::functions_t functions;
        e->execute(result, variables, functions);

#if 0
#ifdef DEBUG
        {
            content::content *content_plugin(content::content::instance());
            libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
            libdbproxy::value script(branch_table->getRow(branch_key)->getCell(get_name(name_t::SNAP_NAME_INDEX_ORIGINAL_TEST_SCRIPT))->getValue());
            SNAP_LOG_TRACE() << "script [" << script.stringValue() << "] result [" << (result.is_true() ? "true" : "false") << "]";
        }
#endif
#endif

        if(!result.is_true())
        {
            // not included
            //
            return QString();
        }
    }

    // okay, it looks like this page has to be included so we are moving
    // forward with it
    //
    {
        snap_expr::expr::expr_pointer_t e(new snap_expr::expr);

        if(!e->compile(vars.at("k")))
        {
            // script could not be compiled (invalid script!)
            //
            // TODO: generate an error message to admin
            //
            SNAP_LOG_ERROR("Error compiling key script: \"")
                          (vars.at("k"))
                          ("\".");
            return QString();
        }

        // run the script with this path
        //
        snap_expr::variable_t result;
        snap_expr::variable_t::variable_map_t variables;
        snap_expr::variable_t var_path("path");
        var_path.set_value(page_ipath.get_cpath());
        variables["path"] = var_path;
        snap_expr::variable_t var_page("page");
        var_page.set_value(page_ipath.get_key());
        variables["page"] = var_page;
        snap_expr::variable_t var_index("index");
        var_index.set_value(type_ipath.get_key());
        variables["index"] = var_index;
        snap_expr::functions_t functions;
        e->execute(result, variables, functions);

        return result.get_string("*result*");
    }
}


///** \brief Change the start date offset to increase latency.
// *
// * The offset is defined in microseconds. It defines the amount of time
// * it takes before the index plugin is allowed to process that page. By
// * default is is set to INDEX_PROCESSING_LATENCY, which at time of writing
// * is 10 seconds.
// *
// * In most cases you do not need to change this value. However, if you
// * are working with a special plugin that needs to create many pages,
// * especially permissions to change who has access to those pages, then
// * the process may take more or around the default 10 seconds. In that
// * case, you want to change the start date offset with a (much) larger
// * amount.
// *
// * You should never call this function directly. Instead look into
// * using the RAII class safe_start_date_offset, which will automatically
// * restore the default offset once you are done.
// *
// * \code
// *      {
// *          // set your my_new_offset value to the amount in microseconds
// *          // you want the index plugin to wait before processing your
// *          // new content
// *          //
// *          index::safe_start_date_offset saved_offset(my_new_offset);
// *
// *          content::content::instance()->create_content(...);
// *      }
// * \endcode
// *
// * \note
// * The minimum value of offset_us is INDEX_PROCESSING_LATENCY.
// * We also clamp to a maximum of 24h.
// *
// * \param[in] offset_us  The offset to add to the start date of records
// *                       added to the index.
// */
//void index::set_start_date_offset(int64_t offset_us)
//{
//    if(offset_us < INDEX_PROCESSING_LATENCY)
//    {
//        f_start_date_offset = INDEX_PROCESSING_LATENCY;
//    }
//    else if(offset_us > 24LL * 60LL * 60LL * 1000000LL)
//    {
//        f_start_date_offset = 24LL * 60LL * 60LL * 1000000LL;
//    }
//    else
//    {
//        f_start_date_offset = offset_us;
//    }
//}
//
//
///** \brief Retrieve the start date offset.
// *
// * By default, the act of creating or modifying a page is registered for
// * immediate processing by the index plugin.
// *
// * There are cases, however, where an record is created and needs some time
// * before getting 100% ready. This offset defines how long the index plugin
// * should wait.
// *
// * The default wait is INDEX_PROCESSING_LATENCY, which at time of writing is
// * 10 seconds.
// *
// * \return The offset to add to the start date when registering a page for
// *         reprocessing after modification, in microseconds.
// */
//int64_t index::get_start_date_offset() const
//{
//    return f_start_date_offset;
//}


/** \brief Read a set of URIs from an index.
 *
 * This function reads a set of URIs from the index specified by \p ipath.
 * The \p ipath parameter is expected to be a valid taxonomy type. If the
 * type has multiple names, you must specify the name of the index you
 * want to read unless you want to read the default index in which case
 * you can use the empty string ("" or QString()).
 *
 * The first index returned is defined by \p start. It is inclusive and the
 * very first index is number 0.
 *
 * The maximum number of indexes returned is defined by \p count. The number
 * may be set of -1 to returned as many records as there is available starting
 * from \p start. However, the function limits \p count to 10,000 records
 * so if the returned index is exactly 10,000 records, it is not unlikely that
 * you did not get all the records after the \p start point.
 *
 * The sorting of the records is done through the use of the index key as
 * defined in the JSON data of the index field. If you want to use an index
 * sorted in a different order you will have to read many entries and sort
 * those before returning them to the client or create a distinct index with
 * the proper key (in which case Cassandra will do the sorting for you with
 * the disadvantage of even more duplication of your data.)
 *
 * The \p count parameter cannot be set to zero. The function throws if you
 * do that.
 *
 * TODO: review this doc. from here
 *
 * \todo
 * Note that at this point this function reads ALL the records from 0 to start
 * and throw them away. Later we'll add sub-indexes that will allow us to
 * reach any record very quickly. The sub-index will be something like this:
 *
 * \code
 *     index::index::100 = <key of the 100th record>
 *     index::index::200 = <key of the 200th record>
 *     ...
 * \endcode
 *
 * That way we can go to record 230 be starting the index scan at the 200th
 * record. We read the index::index:200 and us that key to start reading the
 * index (i.e. in the column predicate would use that key as the start key.)
 *
 * When an index name is specified, the \em page query string is checked for
 * a parameter that starts with that name, followed by a dash and a number.
 * Multiple indexes can exist on a web page, and each index may be at a
 * different page. In this way, each index can define a different page
 * number, you only have to make sure that all the indexes that can appear
 * on a page have a different name.
 *
 * The syntax of the query string for pages is as follow:
 *
 * \code
 *      page-<name>=<number>
 * \endcode
 *
 * \exception snap_logic_exception
 * The function raises the snap_logic_exception exception if the start or
 * count values are incompatible. The start parameter must be positive or
 * zero. The count value must be position (larger than 0) or -1 to use
 * the system maximum allowed.
 *
 * \param[in,out] ipath  The path to the index to be read.
 * \param[in] index_name  The name of the index to read.
 * \param[in] start  The first record to be returned (must be 0 or larger).
 * \param[in] count  The number of records to return (-1 for the maximum allowed).
 * \param[in] start_key  How the key must or the empty string.
 *
 * \return The index of records
 */
index_record_vector_t index::read_index(
              content::path_info_t & ipath
            , QString const & index_name
            , int start
            , int count
            , QString const & start_key)
{
    index_record_vector_t result;

    if(count == -1 || count > INDEX_MAXIMUM_RECORDS)
    {
        count = INDEX_MAXIMUM_RECORDS;
    }
    if(start < 0 || count <= 0)
    {
        throw snap_logic_exception(QString("index::read_index(\"%1\", %2, %3) called with invalid start and/or count values...")
                    .arg(ipath.get_key())
                    .arg(start)
                    .arg(count));
    }

    libdbproxy::table::pointer_t index_table(get_index_table());

    QString index_key(ipath.get_key());
    if(!index_name.isEmpty()
    && index_name != get_name(name_t::SNAP_NAME_INDEX_DEFAULT_INDEX))
    {
        index_key += "#";
        index_key += index_name;
    }

    libdbproxy::row::pointer_t index_row(index_table->getRow(index_key));
    index_row->clearCache();

    QString end_key(start_key);
    if(!end_key.isEmpty())
    {
        // the key is not empty, increment the last character by one
        //
        ushort const l(end_key[end_key.length() - 1].unicode());
        if(l == 0xFFFF)
        {
            QChar const c('\0');
            end_key += c;
        }
        else
        {
            end_key[end_key.length() - 1] = l + 1;
        }
    }

    auto column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
    column_predicate->setCount(std::min(100, count)); // optimize the number of cells transferred
    column_predicate->setIndex(); // behave like an index
    column_predicate->setStartCellKey(start_key); // limit the loading to user defined range
    column_predicate->setEndCellKey(end_key);
    for(;;)
    {
        // clear the cache before reading the next load
        //
        index_row->readCells(column_predicate);
        libdbproxy::cells const cells(index_row->getCells());
        if(cells.empty())
        {
            // all columns read
            //
            return result;
        }
        for(libdbproxy::cells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
        {
            if(start > 0)
            {
                --start;
            }
            else
            {
                // we keep the sort key in the index
                //
                index_record_t record;
                record.set_sort_key(cell_iterator.key());
                record.set_uri(cell_iterator.value()->getValue().stringValue());
                result.push_back(record);
                if(result.size() >= count)
                {
                    // we got the count we wanted, return now
                    //
                    return result;
                }
            }
        }
    }
    NOTREACHED();
}


///** \brief Register the CRON actions supported by the index plugin.
// *
// * This function registers this plugin CRON actions as defined below:
// *
// * \li reindex
// *
// * The "reindex" is used by the backend to go through all the pages of
// * all the existing indexes and reindex them _from scratch_ (it won't delete
// * all and rebuild all, but it will go through the entire set of indexes and
// * make sure that we do not have any missing items and extra items.)
// *
// * \param[in,out] actions  The vector of supported actions where we add
// *                         our own actions.
// */
//void index::on_register_backend_cron(server::backend_action_set & actions)
//{
//    //actions.add_action(get_name(name_t::SNAP_NAME_INDEX_REINDEX), this);
//}


/** \brief Register the various index actions.
 *
 * This function registers this plugin as supporting the following
 * one time actions:
 *
 * \li index::reindex
 *
 * The "index::reindex" goes through the list of index definitions and
 * reindex each list as required.
 *
 * \code
 * snapbackend https://example.com/ \
 *          --action index::reindex
 * \endcode
 *
 * The system automatically searches all the .../content-types/... definitions
 * for index declarations. The process first deletes the existing indexes
 * and then regenerates them. This allows for editing your indexes and
 * getting the correct table again. The delete happens one entry at a time
 * so that way other entries are still functional while the one being worked
 * on gets rebuilt in full.
 *
 * \param[in,out] actions  The set of actions where we add ourselves.
 */
void index::on_register_backend_action(server::backend_action_set & actions)
{
    actions.add_action(get_name(name_t::SNAP_NAME_INDEX_REINDEX), this);
}


/** \brief Run an index action.
 *
 * This function understands the following commands:
 *
 * \li index::reindex -- check all the indexes
 *
 * The reindex function is used to force a full reindexing of the entire
 * database of a website. This is useful if you have an existing database
 * and want add new indexes or if somehow it looks like indexes are
 * currently out of whack and need a little help.
 *
 * \param[in] action  The action this function is being called with.
 */
void index::on_backend_action(QString const & action)
{
    if(action == get_name(name_t::SNAP_NAME_INDEX_REINDEX))
    {
        f_backend = dynamic_cast<snap_backend *>(f_snap);
        if(!f_backend)
        {
            throw index_exception_no_backend("index::on_backend_action(): could not determine the snap_backend pointer for the listjournal action");
        }

        reindex();
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        //
        throw snap_logic_exception(QString("index.cpp:on_backend_action(): index::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


/** \brief Go through all the content-types to reindex all indexes.
 *
 * This function goes through all the `content-types` to reindex all
 * the existing indexes. It is useful to run this process once in a
 * while to make sure that your indexes do not get out of sync.
 *
 * This function is a loop going through all the `content-types`
 * recursively.
 */
void index::reindex()
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    //libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());

    QString const site_key(f_snap->get_site_key_with_slash());
    QString const root_key(site_key + "types/taxonomy/system/content-types");

    snap_string_list paths;

    {
        std::fstream reindex_type_cache;
        reindex_type_cache.open(g_reindex_type_cache_filename, std::ios_base::in);
        if(reindex_type_cache.is_open())
        {
            // if there is a file, read it and use those paths
            //
            std::string line;
            while(getline(reindex_type_cache, line))
            {
                boost::algorithm::trim(line);
                paths << QString::fromUtf8(line.c_str());
            }
        }
    }

    // if still empty, start over from the root
    // (i.e. deleting the file is a way to start over)
    //
    if(paths.empty())
    {
        SNAP_LOG_INFO("Restarting processing from root (")(root_key)(")");
        paths << root_key;
    }

    QString const children_name(content::get_name(content::name_t::SNAP_NAME_CONTENT_CHILDREN));
    QString const page_name(content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE));
    QString const original_scripts_name(get_name(name_t::SNAP_NAME_INDEX_ORIGINAL_SCRIPTS));

    // BUG: This is not incremental and it deletes the previous work so we
    //      really can't have it here at the moment. If you know you need
    //      a DELETE, do it manually before running the reindex.
    //
    //      Also it should be a form of RAII because the pointer needs to be
    //      deleted when we return from this function. Finally, that pointer
    //      should only be used by the reindex() and none of the other
    //      functions. It's wrong too in that sense.
    //
    //if(g_deleted_entries == nullptr)
    //{
    //    g_deleted_entries = new snap_string_list;
    //}

    // the amount of time one process can take to process all its lists
    //
    auto get_timeout = [&](auto const & field_name, int64_t default_timeout)
        {
            QString const loop_timeout_str(f_snap->get_server_parameter(field_name));
            if(!loop_timeout_str.isEmpty())
            {
                // time in seconds in .conf
                //
                bool ok(false);
                int64_t const loop_timeout_sec(loop_timeout_str.toLongLong(&ok, 10) * 1000000LL);
                if(ok && loop_timeout_sec >= 1000000LL) // valid and at least 1 second
                {
                    return loop_timeout_sec;
                }
                SNAP_LOG_WARNING("invalid number or timeout too small (under 1s) in ")(field_name);
            }
            return default_timeout;
        };
    int64_t const loop_timeout(get_timeout("index::reindex_timeout", 60LL * 60LL * 1000000LL));
    int64_t const loop_start_time(f_snap->get_current_date());

    auto get_number = [&](auto const & field_name, uint32_t default_number)
        {
            QString const number_str(f_snap->get_server_parameter(field_name));
            if(!number_str.isEmpty())
            {
                // time in seconds in .conf
                //
                bool ok(false);
                int64_t const number(number_str.toLongLong(&ok, 10));
                if(ok && number >= 10) // valid and at least 1 second
                {
                    return static_cast<uint32_t>(number);
                }
                SNAP_LOG_WARNING("invalid number in ")(field_name)(" (")(number_str)(")");
            }
            return default_number;
        };
    uint32_t const max_count(get_number("index::reindex_max_count", 100));
    f_count_processed = 0;

    while(!paths.isEmpty())
    {
        SNAP_LOG_INFO("reindexing working on index \"")
                     (paths[0])
                     ("\".");

        content::path_info_t type_ipath;
        type_ipath.set_path(paths[0]);

        // type exists (it should always exist unless someone just deleted
        // it under this process feet)
        //
        if(content_table->exists(type_ipath.get_key()))
        {
            // process the list of pages attached to this type
            {
                libdbproxy::row::pointer_t row(content_table->getRow(type_ipath.get_key()));
                libdbproxy::value const index_scripts(row->getCell(original_scripts_name)->getValue());
                QString const scripts(index_scripts.stringValue());
                if(!scripts.isEmpty())
                {
                    std::fstream reindex_page_cache;
                    reindex_page_cache.open(g_reindex_page_cache_filename, std::ios_base::in);
                    if(reindex_page_cache.is_open())
                    {
                        reindex_page_cache.close();
                    }
                    else
                    {
                        reindex_page_cache.open(g_reindex_page_cache_filename, std::ios_base::out | std::ios_base::trunc);
                        if(reindex_page_cache.is_open())
                        {
                            links::link_info info(page_name
                                                , false
                                                , type_ipath.get_key()
                                                , type_ipath.get_branch());
                            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                            links::link_info page_info;
                            while(link_ctxt->next_link(page_info))
                            {
                                reindex_page_cache << page_info.key() << std::endl;
                            }
                            reindex_page_cache.close();
                        }
                    }

                    reindex_page_cache.open(g_reindex_page_cache_filename, std::ios_base::in | std::ios_base::out);
                    if(reindex_page_cache.is_open())
                    {
                        for(;;)
                        {
                            std::fstream::pos_type const start_pos(reindex_page_cache.tellg());
                            std::string line;
                            if(!getline(reindex_page_cache, line))
                            {
                                unlink(g_reindex_page_cache_filename);
                                break;
                            }
                            boost::algorithm::trim(line);
                            if(line.empty())
                            {
                                // skip empty lines
                                //
                                continue;
                            }
                            content::path_info_t page_ipath;
                            page_ipath.set_path(QString::fromUtf8(line.c_str()));
                            index_pages(page_ipath, type_ipath, scripts);

                            // page was processed, remove it from the file
                            // by overwriting it with spaces
                            //
                            std::fstream::pos_type const end_pos(reindex_page_cache.tellg());
                            reindex_page_cache.seekp(start_pos);
                            std::string const spaces(end_pos - start_pos - 1, ' ');
                            reindex_page_cache.write(spaces.data(), spaces.length());
                            reindex_page_cache.seekg(end_pos);

                            ++f_count_processed;
                            if(f_count_processed >= max_count)
                            {
                                SNAP_LOG_WARNING("Stopping the reindex processing after ")(max_count)(" pages were processed.");
                                return;
                            }

                            if(f_backend->stop_received())
                            {
                                SNAP_LOG_WARNING("Stopping the reindex processing because the parent backend process asked us to.");
                                return;
                            }

                            // limit the time we work
                            //
                            int64_t const loop_time_spent(f_snap->get_current_date() - loop_start_time);
                            if(loop_time_spent > loop_timeout)
                            {
                                SNAP_LOG_WARNING("Stopping the reindex processing after ")(loop_timeout / 1000000LL)(" seconds.");
                                return;
                            }
                        }
                    }
                }
#if 0
                else
                {
                    // TODO: run a DELETE if there are no scripts
                    //       only we do not have the index name so
                    //       I'm not too sure how to do it at this
                    //       time, maybe with a ALLOW FILTERING,
                    //       but we can't filter on the key anyway...
                    //
                    //       next this is going to be a rather slow
                    //       process so it should be optional
                    //
                    libdbproxy::libdbproxy::pointer_t cassandra(f_snap->get_cassandra());
                    libdbproxy::context::pointer_t context(f_snap->get_context());
                    libdbproxy::proxy::pointer_t dbproxy(cassandra->getProxy());

                    // DELETE FROM snap_websites.index
                    //       WHERE key = '<website>';
                    //
                    libdbproxy::order delete_index;
                    delete_index.setCql(QString("DELETE FROM %1.%2 WHERE key=?")
                                            .arg(context->contextName())
                                            .arg(get_name(name_t::SNAP_NAME_INDEX_TABLE))
                                      , libdbproxy::order::type_of_result_t::TYPE_OF_RESULT_SUCCESS);
                    delete_index.setConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_ONE);

                    QString index_key = type_ipath.get_key().toUtf8();
                    // the name is required, but it's gone?!
                    //if(vars.find("n") != vars.end()
                    //&& vars.at("n") != get_name(name_t::SNAP_NAME_INDEX_DEFAULT_INDEX))
                    //{
                    //    index_key += "#";
                    //    index_key += vars.at("n");
                    //}

                    delete_index.addParameter(index_key.toUtf8());

                    libdbproxy::order_result const delete_index_result(dbproxy->sendOrder(delete_index));

                    // report error, but continue since we're just trying to delete
                    //
                    if(!delete_index_result.succeeded())
                    {
                        SNAP_LOG_ERROR("Error deleting indexes for website \"")
                                      (page_ipath.get_key())
                                      ("\" from table \"")
                                      (context->contextName())
                                      (".")
                                      (get_name(name_t::SNAP_NAME_INDEX_TABLE))
                                      ("\"");
                    }
                }
#endif
            }

            // read the next level (children)
            {
                links::link_info info(children_name, false, type_ipath.get_key(), type_ipath.get_branch());
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                links::link_info child_info;
                while(link_ctxt->next_link(child_info))
                {
                    paths << child_info.key();
                }
            }
        }
        else
        {
            // TODO: should this be an error instead?
            //       the root page and all of its children should exist!
            //
            SNAP_LOG_WARNING("could not access \"")
                            (type_ipath.get_key())
                            ("\".");
        }

        paths.removeAt(0);

        if(paths.empty())
        {
            unlink(g_reindex_type_cache_filename);
        }
        else
        {
            std::fstream reindex_cache;
            reindex_cache.open(g_reindex_type_cache_filename, std::ios_base::out | std::ios_base::trunc);
            if(reindex_cache.is_open())
            {
                for(auto p : paths)
                {
                    reindex_cache << p << std::endl;
                }
            }
        }
    }

    SNAP_LOG_INFO("reindexing complete.");

    // Note: if the delete doesn't happen, it's not a big deal, the reindex
    //       is just a one time backend run so we would leak the memory and
    //       then exit right after anyway...
    //
    delete g_deleted_entries;
    g_deleted_entries = nullptr;
}


///** \brief Implementation of the backend process signal.
// *
// * This function captures the backend processing signal which is sent
// * by the server whenever the backend tool is run against a cluster.
// *
// * The index plugin refreshes indexes of pages on websites when it receives
// * this signal assuming that the website has the parameter PROCESS_INDEX
// * defined.
// *
// * This backend may end up taking a lot of processing time and may need to
// * run very quickly (i.e. within seconds when a new page is created or a
// * page is modified). For this reason we also offer an action which supports
// * the PING signal.
// *
// * This backend process will actually NOT run if the PROCESS_INDEXES parameter
// * is not defined as a site parameter. With the command line:
// *
// * \code
// * snapbackend [--config snapserver.conf] --param PROCESS_INDEXES=1
// * \endcode
// *
// * At this time the value used with PROCESS_INDEX is not tested, however, it
// * is strongly recommended you use 1.
// *
// * It is also important to mark the index as a standalone index to avoid
// * parallelism which is NOT checked by the backend at this point (because
// * otherwise you take the risk of losing the index updating process
// * altogether.) So you want to run this command once:
// *
// * \code
// * snapbackend [--config snapserver.conf] --action standaloneindex http://my-website.com/
// * \endcode
// *
// * Make sure to specify the URI of your website because otherwise all the
// * sites will be marked as standalone sites!
// *
// * Note that if you create a standalone site, then you have to either
// * allow its processing with the PROCESS_INDEXES parameter, or you have
// * to start it with the pageindex and its URI:
// *
// * \code
// * snapbackend [--config snapserver.conf] --action pageindex http://my-website.com/
// * \endcode
// */
//void index::on_backend_process()
//{
//    SNAP_LOG_TRACE("backend_process: update indexes.");
//}






/** \brief Replace a [index::...] token with the contents of an index.
 *
 * This function replaces the index tokens with themed indexes.
 *
 * The supported tokens are:
 *
 * \code
 * [index::theme(path="<index path>", theme="<theme name>", start="<start>", count="<count>")]
 * \endcode
 *
 * Theme the index defined at \<index path\> with the theme \<theme name\>.
 * You may skip some records and start with record \<start\> instead of record 0.
 * You may specified the number of records to display with \<count\>. Be
 * careful because by default all the records are shown (Although there is a
 * system limit which at this time is 10,000 which is still a very LARGE
 * index!) The theme name, start, and count parameters are optional.
 * The path is mandatory. It can be empty if the root page was transformed
 * into an index.
 *
 * \param[in,out] ipath  The path to the page being worked on.
 * \param[in,out] xml  The XML document used with the layout.
 * \param[in,out] token  The token object, with the token name and optional parameters.
 */
void index::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(xml);

    // an index::... token?
    if(!token.is_namespace("index::"))
    {
        return;
    }

    if(token.is_token(get_name(name_t::SNAP_NAME_INDEX_THEME)))
    {
        // index::theme expects one to four parameters
        if(!token.verify_args(1, 4))
        {
            return;
        }

        // Path
        filter::filter::parameter_t path_param(token.get_arg("path", 0, filter::filter::token_t::TOK_STRING));
        if(token.f_error)
        {
            return;
        }
        if(path_param.f_value.isEmpty())
        {
            token.f_error = true;
            token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> index 'path' (first parameter) of the index::theme() function cannot be an empty string.</span>";
            return;
        }

        // Theme
        QString theme("qrc:/xsl/index/default"); // default theming, simple <ul>{<li>...</li>}</ul> index
        if(token.has_arg("theme", 1))
        {
            filter::filter::parameter_t theme_param(token.get_arg("theme", 1, filter::filter::token_t::TOK_STRING));
            if(token.f_error)
            {
                return;
            }
            // if user included the ".xsl" extension, ignore it
            if(theme_param.f_value.endsWith(".xsl"))
            {
                theme_param.f_value = theme_param.f_value.left(theme_param.f_value.length() - 4);
            }
            if(!theme_param.f_value.isEmpty())
            {
                theme = theme_param.f_value;
            }
        }

        // Start
        int start(0); // start with very first index
        if(token.has_arg("start", 2))
        {
            filter::filter::parameter_t start_param(token.get_arg("start", 2, filter::filter::token_t::TOK_INTEGER));
            if(token.f_error)
            {
                return;
            }
            bool ok(false);
            start = start_param.f_value.toInt(&ok, 10);
            if(!ok)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> index start (third parameter) of the index::theme() function must be a valid integer.</span>";
                return;
            }
            if(start < 0)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> index start (third parameter) of the index::theme() function must be a positive integer or zero.</span>";
                return;
            }
        }

        // Count
        int count(-1); // all records
        if(token.has_arg("count", 3))
        {
            filter::filter::parameter_t count_param(token.get_arg("count", 3, filter::filter::token_t::TOK_INTEGER));
            if(token.f_error)
            {
                return;
            }
            bool ok(false);
            count = count_param.f_value.toInt(&ok, 10);
            if(!ok)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> index 'count' (forth parameter) of the index::theme() function must be a valid integer.</span>";
                return;
            }
            if(count != -1 && count <= 0)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> index 'count' (forth parameter) of the index::theme() function must be a valid integer large than zero or -1.</span>";
                return;
            }
        }

        content::path_info_t index_ipath;
        index_ipath.set_path(path_param.f_value);

        token.f_replacement = generate_index(ipath, index_ipath, start, count, theme);
    }
}


void index::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("index::theme",
        "Display a themed index. The token accepts 1 to 4 parameters:"
        " the path to the index (mandatory) [path], the name of a theme"
        " (\"default\" otherwise) [theme], the first record to display"
        " [start] (the very first record is number 0), the number of"
        " records to display [count].");
}



/** \brief Generate an index.
 *
 * This function generates the index defined by index_ipath from \p start
 * up to <code>start + count - 1</code> using the specified \p theme.
 *
 * The \p ipath represents the object for which the index is being created.
 *
 * \param[in,out] ipath  Object for which the index is being generated.
 * \param[in,out] index_ipath  The index object from which the index is created.
 * \param[in] start  The start element.
 * \param[in] count  The number of records to display.
 * \param[in] start_key  How the key must or the empty string.
 * \param[in] theme  The theme used to generate the output.
 *
 * \return The resulting HTML of the index or an empty string.
 */
QString index::generate_index(
              content::path_info_t & ipath
            , content::path_info_t & index_ipath
            , int start
            , int count
            , QString const & start_key
            , QString const & theme)
{
    QString const index_cpath(index_ipath.get_cpath());
    if(index_cpath == "admin"
    || index_cpath.startsWith("admin/"))
    {
        // although we are just viewing indexes, only "administer" is
        // used when visiting pages under /admin...
        //
        index_ipath.set_parameter("action", "administer");
    }
    else
    {
        // we are just viewing this index
        //
        index_ipath.set_parameter("action", "view");
    }

    quiet_error_callback index_error_callback(f_snap, true);
    plugin * index_plugin(path::path::instance()->get_plugin(index_ipath, index_error_callback));
    if(!index_error_callback.has_error() && index_plugin)
    {
        layout::layout_content * index_content(dynamic_cast<layout::layout_content *>(index_plugin));
        if(index_content == nullptr)
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Plugin Missing",
                    QString("Plugin \"%1\" does not know how to handle an index assigned to it.").arg(index_plugin->get_plugin_name()),
                    "index::generate_index() -- the \"index\" plugin does not derive from layout::layout_content.");
            NOTREACHED();
        }

        // IMPORTANT NOTE: We do not check the maximum with the count
        //                 because our indexes may expend with time

        // read the index of records
        //
        // TODO: use a paging_t object to read the index so we can
        //       append a navigation and handle the page parameter
        //
        paging_t paging(f_snap, index_ipath);
        paging.set_start_offset(start + 1);
        paging.set_maximum_number_of_records(count);
        paging.process_query_string_info();
        paging.set_start_key(start_key);
        index_record_vector_t records(paging.read_index());
        snap_child::post_file_t f;

        // Load the index body
        f.set_filename(theme + "-index-body.xsl");
        if(!f_snap->load_file(f) || f.get_size() == 0)
        {
            index_ipath.set_parameter("error", "1");
            return QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> index theme (%1-index-body.xsl) could not be loaded.</span>")
                                        .arg(theme);
        }
        QString const index_body_xsl(QString::fromUtf8(f.get_data()));

        // Load the index theme
        f.set_filename(theme + "-index-theme.xsl");
        if(!f_snap->load_file(f) || f.get_size() == 0)
        {
            index_ipath.set_parameter("error", "1");
            return QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> index theme (%1-index-theme.xsl) could not be loaded.</span>")
                                        .arg(theme);
        }
        QString const index_theme_xsl(QString::fromUtf8(f.get_data()));

        // Load the record body
        f.set_filename(theme + "-record-body.xsl");
        if(!f_snap->load_file(f) || f.get_size() == 0)
        {
            index_ipath.set_parameter("error", "1");
            return QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> index theme (%1-record-theme.xsl) could not be loaded.</span>")
                                        .arg(theme);
        }
        QString const record_body_xsl(QString::fromUtf8(f.get_data()));

        // Load the record theme
        f.set_filename(theme + "-record-theme.xsl");
        if(!f_snap->load_file(f) || f.get_size() == 0)
        {
            index_ipath.set_parameter("error", "1");
            return QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> index theme (%1-record-theme.xsl) could not be loaded.</span>")
                                        .arg(theme);
        }
        QString const record_theme_xsl(QString::fromUtf8(f.get_data()));

        layout::layout * layout_plugin(layout::layout::instance());
        QDomDocument index_doc(layout_plugin->create_document(index_ipath, index_plugin));
        layout_plugin->create_body(index_doc, index_ipath, index_body_xsl, index_content);
        // TODO: fix this problem (i.e. /products, /feed...)
        // The following is a "working" fix so we can generate an index
        // for the page that defines the index, but of course, in
        // that case we have the "wrong" path... calling with the
        // index_ipath generates a filter loop problem
        //content::path_info_t random_ipath;
        //random_ipath.set_path("");
        //layout_plugin->create_body(index_doc, random_ipath, index_body_xsl, index_content);

        QDomElement body(snap_dom::get_element(index_doc, "body"));
        QDomElement index_element(index_doc.createElement("index"));
        body.appendChild(index_element);

        QString const main_path(f_snap->get_uri().path());
        content::path_info_t main_ipath;
        main_ipath.set_path(main_path);

        // now theme the index
        //
        int const max_records(records.size());
        for(int i(0), item(1); i < max_records; ++i)
        {
            index_error_callback.clear_error();
            content::path_info_t record_ipath;
            record_ipath.set_path(records[i].get_uri());
            if(record_ipath.get_parameter("action").isEmpty())
            {
                // the default action on a link is "view" unless it
                // references an administrative task under /admin
                //
                if(record_ipath.get_cpath() == "admin" || record_ipath.get_cpath().startsWith("admin/"))
                {
                    record_ipath.set_parameter("action", "administer");
                }
                else
                {
                    record_ipath.set_parameter("action", "view");
                }
            }
            // whether we are attempting to display this record
            // (opposed to the test when going to the page or generating
            // the index in the first place)
            //
            record_ipath.set_parameter("mode", "display");
            plugin * record_plugin(path::path::instance()->get_plugin(record_ipath, index_error_callback));
            layout_content * l(dynamic_cast<layout_content *>(record_plugin));
            if(!index_error_callback.has_error() && record_plugin)
            {
                if(l)
                {
                    // put each box in a filter tag so that way we have
                    // a different owner and path for each
                    //
                    QDomDocument record_doc(layout_plugin->create_document(record_ipath, record_plugin));
                    QDomElement record_root(record_doc.documentElement());
                    record_root.setAttribute("item", item);

                    FIELD_SEARCH
                        (content::field_search::command_t::COMMAND_ELEMENT, snap_dom::get_element(record_doc, "metadata"))
                        (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)

                        // snap/head/metadata/desc[@type="index_uri"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, index_ipath.get_key())
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=index_uri]/data")

                        // snap/head/metadata/desc[@type="index_path"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, index_cpath)
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=index_path]/data")

                        // snap/head/metadata/desc[@type="box_uri"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, ipath.get_key())
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=box_uri]/data")

                        // snap/head/metadata/desc[@type="box_path"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, ipath.get_cpath())
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=box_path]/data")

                        // snap/head/metadata/desc[@type="main_page_uri"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, main_ipath.get_key())
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=main_page_uri]/data")

                        // snap/head/metadata/desc[@type="main_page_path"]/data
                        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, main_ipath.get_cpath())
                        (content::field_search::command_t::COMMAND_SAVE, "desc[type=main_page_path]/data")

                        // retrieve names of all the boxes
                        ;

//SNAP_LOG_WARNING("create body for record ")(i)(" with item ")(item);
                    layout_plugin->create_body(record_doc, record_ipath, record_body_xsl, l);
//std::cerr << "source to be parsed [" << record_doc.toString(-1) << "]\n";
                    QDomElement record_body(snap_dom::get_element(record_doc, "body"));
                    record_body.setAttribute("item", item);
//SNAP_LOG_WARNING("apply theme to record ")(i)(" with item ")(item);
                    QString const themed_record(layout_plugin->apply_theme(record_doc, record_theme_xsl, theme));
//std::cerr << "themed record [" << themed_record << "]\n";

                    // add that result to the item document
                    //
                    QDomElement record(index_doc.createElement("record"));
                    index_element.appendChild(record);
                    snap_dom::insert_html_string_to_xml_doc(record, themed_record);

                    ++item; // item only counts records added to the output
                }
                else
                {
                    SNAP_LOG_ERROR("the record_plugin pointer for \"")
                                  (record_plugin->get_plugin_name())
                                  ("\" is not a layout_content");
                }
            }
        }

        // we cannot use "navigation" as the name of this tag since it is
        // used for the navigation links defined in the header.
        //
        QDomElement navigation_tag(index_doc.createElement("index-navigation-tags"));
        body.appendChild(navigation_tag);
        paging.generate_index_navigation(navigation_tag, f_snap->get_uri(), 5, true, true, true);

//std::cerr << "resulting XML [" << index_doc.toString(-1) << "]\n";

        // now theme the index as a whole
        // we add a wrapper so we can use /node()/* in the final theme
//SNAP_LOG_WARNING("apply index theme");
        return layout_plugin->apply_theme(index_doc, index_theme_xsl, theme);
    }
    // else index is not accessible (permission "problem")
//else SNAP_LOG_FATAL("index::on_replace_token() index \"")(index_ipath.get_key())("\" is not accessible.");

    return QString();
}


void index::on_generate_boxes_content(content::path_info_t & page_cpath, content::path_info_t & ipath, QDomElement & page, QDomElement & box)
{
    NOTUSED(page_cpath);

    output::output::instance()->on_generate_main_content(ipath, page, box);
}


void index::on_copy_branch_cells(libdbproxy::cells & source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    NOTUSED(destination_branch);

    libdbproxy::cells left_cells;

    // check cells we support
    //
    bool has_index(false);
    for(libdbproxy::cells::const_iterator nc(source_cells.begin());
            nc != source_cells.end();
            ++nc)
    {
        libdbproxy::cell::pointer_t source_cell(*nc);
        QByteArray cell_key(source_cell->columnKey());

        if(cell_key == get_name(name_t::SNAP_NAME_INDEX_ORIGINAL_SCRIPTS))
        {
            has_index = true;

            // copy our fields as is
            //
            destination_row->getCell(cell_key)->setValue(source_cell->getValue());
        }
        else
        {
            // keep the other branch fields as is, other plugins can handle
            // them as required by implementing this signal
            //
            // note that the map is a map of shared pointers so it is fast
            // to make a copy like this
            //
            left_cells[cell_key] = source_cell;
        }
    }

    // TODO: we need to do something about it but how?!
    // (right now we do not copy types so we should be good for a while)
    //if(has_index)
    //{
    //    // make sure the (new) index is checked so we actually get an index
    //    content::path_info_t ipath;
    //    ipath.set_path(destination_row->rowName());
    //    on_modified_content(ipath);
    //}

    // overwrite the source with the cells we allow to copy "further"
    source_cells = left_cells;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
