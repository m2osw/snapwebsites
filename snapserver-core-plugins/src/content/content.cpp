// Snap Websites Server -- all the user content and much of the system content
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


/** \file
 * \brief The implementation of the content plugin.
 *
 * The implementation of the content plugin handles the content, branch, and
 * revision tables in a way that gives other plugins access to all the data
 * without themselves having to directly peek and poke at the data.
 *
 * This allows the content plugin a way to control that modified data does
 * general all the necessary "side effects" as expected in the system. The
 * main problem we have when modifying one or more fields in a propagation
 * of the information. By using the path_info_t and the content plugin to
 * make all data changes we ensure that the related signals get emitted
 * and thus that all plugins get a chance to do further updates as they
 * require to make to finish up the work (i.e. when changing a title and
 * that page is part of a list which shows that title, we want the list
 * plugin to kick in and fix the corresponding list.)
 */

#include "content.h"

//#include "../messages/messages.h" -- we now have 2 levels (messages and output) so we could include messages.h there

#include <snapwebsites/compression.h>
#include <snapwebsites/dbutils.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/snap_magic.h>
#include <snapwebsites/snap_image.h>
#include <snapwebsites/snap_lock.h>
#include <snapwebsites/snap_version.h>

#include <iostream>

#include <openssl/md5.h>

#include <QFile>
#include <QTextStream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(content, 1, 0)

/** \brief Get a fixed content name.
 *
 * The content plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    // Note: <branch>.<revision> are actually replaced by a full version
    //       when dealing with JavaScript and CSS files (Version: field)
    switch(name)
    {
    case name_t::SNAP_NAME_CONTENT_ACCEPTED:
        return "content::accepted";

    case name_t::SNAP_NAME_CONTENT_ATTACHMENT:
        return "content::attachment";

    case name_t::SNAP_NAME_CONTENT_ATTACHMENT_FILENAME:
        return "content::attachment::filename";

    case name_t::SNAP_NAME_CONTENT_ATTACHMENT_MIME_TYPE:
        return "content::attachment::mime_type";

    case name_t::SNAP_NAME_CONTENT_ATTACHMENT_PATH_END:
        return "path";

    case name_t::SNAP_NAME_CONTENT_ATTACHMENT_PLUGIN: // this is a forward definition...
        return "attachment";

    case name_t::SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE:
        return "content::attachment::reference";

    case name_t::SNAP_NAME_CONTENT_BODY:
        return "content::body";

    case name_t::SNAP_NAME_CONTENT_BRANCH:
        return "content::branch";

    case name_t::SNAP_NAME_CONTENT_BRANCH_TABLE:
        return "branch";

    case name_t::SNAP_NAME_CONTENT_BREADCRUMBS_SHOW_CURRENT_PAGE:
        return "content::breadcrumbs_show_current_page";

    case name_t::SNAP_NAME_CONTENT_BREADCRUMBS_SHOW_HOME:
        return "content::breadcrumbs_show_home";

    case name_t::SNAP_NAME_CONTENT_BREADCRUMBS_HOME_LABEL:
        return "content::breadcrumbs_home_label";

    case name_t::SNAP_NAME_CONTENT_BREADCRUMBS_PARENT:
        return "content::breadcrumbs_parent";

    case name_t::SNAP_NAME_CONTENT_CACHE_CONTROL:
        return "content::cache_control";

    case name_t::SNAP_NAME_CONTENT_CACHE_TABLE:
        return "cache";

    case name_t::SNAP_NAME_CONTENT_CHILDREN:
        return "content::children";

    case name_t::SNAP_NAME_CONTENT_CLONE:
        return "content::clone";

    case name_t::SNAP_NAME_CONTENT_CLONED:
        return "content::cloned";

    case name_t::SNAP_NAME_CONTENT_CONTENT_TYPES:
        return "Content Types";

    case name_t::SNAP_NAME_CONTENT_CONTENT_TYPES_NAME:
        return "content-types";

    case name_t::SNAP_NAME_CONTENT_COPYRIGHTED:
        return "content::copyrighted";

    case name_t::SNAP_NAME_CONTENT_CREATED:
        return "content::created";

    case name_t::SNAP_NAME_CONTENT_DIRRESOURCES:
        return "dirresources";

    case name_t::SNAP_NAME_CONTENT_ERROR_FILES:
        return "error_files";

    case name_t::SNAP_NAME_CONTENT_EXTRACTRESOURCE:
        return "extractresource";

    case name_t::SNAP_NAME_CONTENT_DESCRIPTION:
        return "content::description";

    case name_t::SNAP_NAME_CONTENT_DESTROYPAGE:
        return "destroypage";

    case name_t::SNAP_NAME_CONTENT_FIELD_PRIORITY:
        return "content::field_priority";

    case name_t::SNAP_NAME_CONTENT_FILES_COMPRESSOR: // NOT USED -- we actually may compress the file with many different compressors instead of just one so this is useless
        return "content::files::compressor";         // I keep the field because I have an update that deletes them in all files

    case name_t::SNAP_NAME_CONTENT_FILES_CREATED:
        return "content::files::created";

    case name_t::SNAP_NAME_CONTENT_FILES_CREATION_TIME:
        return "content::files::creation_time";

    case name_t::SNAP_NAME_CONTENT_FILES_CSS:
        return "css";

    case name_t::SNAP_NAME_CONTENT_FILES_DATA:
        return "content::files::data";

    case name_t::SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED:
        return "content::files::data::gzip_compressed";

    case name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED:
        return "content::files::data::minified";

    case name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED_GZIP_COMPRESSED:
        return "content::files::data::minified::gzip_compressed";

    case name_t::SNAP_NAME_CONTENT_FILES_DEPENDENCY:
        return "content::files::dependency";

    case name_t::SNAP_NAME_CONTENT_FILES_FILENAME:
        return "content::files::filename";

    case name_t::SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT:
        return "content::files::image_height";

    case name_t::SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH:
        return "content::files::image_width";

    case name_t::SNAP_NAME_CONTENT_FILES_JAVASCRIPTS:
        return "javascripts";

    case name_t::SNAP_NAME_CONTENT_FILES_MIME_TYPE:
        return "content::files::mime_type";

    case name_t::SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME:
        return "content::files::modification_time";

    case name_t::SNAP_NAME_CONTENT_FILES_NEW:
        return "new";

    case name_t::SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE:
        return "content::files::original_mime_type";

    case name_t::SNAP_NAME_CONTENT_FILES_REFERENCE:
        return "content::files::reference";

    case name_t::SNAP_NAME_CONTENT_FILES_SECURE: // -1 -- unknown, 0 -- unsecure, 1 -- secure
        return "content::files::secure";

    case name_t::SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK:
        return "content::files::secure::last_check";

    case name_t::SNAP_NAME_CONTENT_FILES_SECURITY_REASON:
        return "content::files::security_reason";

    case name_t::SNAP_NAME_CONTENT_FILES_SIZE:
        return "content::files::size";

    case name_t::SNAP_NAME_CONTENT_FILES_SIZE_GZIP_COMPRESSED:
        return "content::files::size::gzip_compressed";

    case name_t::SNAP_NAME_CONTENT_FILES_SIZE_MINIFIED:
        return "content::files::size::minified";

    case name_t::SNAP_NAME_CONTENT_FILES_SIZE_MINIFIED_GZIP_COMPRESSED:
        return "content::files::size::minified::gzip_compressed";

    case name_t::SNAP_NAME_CONTENT_FILES_TABLE:
        return "files";

    case name_t::SNAP_NAME_CONTENT_FILES_UPDATED:
        return "content::files::updated";

    case name_t::SNAP_NAME_CONTENT_FINAL:
        return "content::final";

    case name_t::SNAP_NAME_CONTENT_FORCERESETSTATUS:
        return "forceresetstatus";

    case name_t::SNAP_NAME_CONTENT_INDEX:
        return "*index*";

    case name_t::SNAP_NAME_CONTENT_ISSUED:
        return "content::issued";

    case name_t::SNAP_NAME_CONTENT_JOURNAL_TABLE:
        return "journal";

    case name_t::SNAP_NAME_CONTENT_JOURNAL_TIMESTAMP:
        return "content::journal::timestamp";

    case name_t::SNAP_NAME_CONTENT_JOURNAL_URL:
        return "content::journal::url";

    case name_t::SNAP_NAME_CONTENT_LONG_TITLE:
        return "content::long_title";

    case name_t::SNAP_NAME_CONTENT_MINIMAL_LAYOUT_NAME:
        return "notheme";

    case name_t::SNAP_NAME_CONTENT_MODIFIED:
        return "content::modified";

    case name_t::SNAP_NAME_CONTENT_NEWFILE:
        return "newfile";

    case name_t::SNAP_NAME_CONTENT_ORIGINAL_PAGE:
        return "content::original_page";

    case name_t::SNAP_NAME_CONTENT_OUTPUT_PLUGIN: // this a forward declaration of the name of the "output" plugin...
        return "output";

    case name_t::SNAP_NAME_CONTENT_PAGE:
        return "content::page";

    case name_t::SNAP_NAME_CONTENT_PAGE_TYPE:
        return "content::page_type";

    case name_t::SNAP_NAME_CONTENT_PARENT:
        return "content::parent";

    case name_t::SNAP_NAME_CONTENT_PREVENT_DELETE:
        return "content::prevent_delete";

    case name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER:
        return "content::primary_owner";

    case name_t::SNAP_NAME_CONTENT_PROCESSING_TABLE:
        return "processing";

    case name_t::SNAP_NAME_CONTENT_REBUILDINDEX:
        return "rebuildindex";

    case name_t::SNAP_NAME_CONTENT_RESETSTATUS:
        return "resetstatus";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL: // content::revision_control::...
        return "content::revision_control";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH: // content::revision_control::current_branch [uint32_t]
        return "current_branch";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH_KEY: // content::revision_control::current_branch_key [string]
        return "current_branch_key";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION: // content::revision_control::current_revision::<branch>::<locale> [uint32_t]
        return "current_revision";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY: // content::revision_control::current_revision_key::<branch>::<locale> [string]
        return "current_revision_key";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH: // content::revision_control::current_working_branch [uint32_t]
        return "current_working_branch";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH_KEY: // content::revision_control::current_working_branch_key [string]
        return "current_working_branch_key";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION: // content::revision_control::current_working_revision::<branch>::<locale> [uint32_t]
        return "current_working_revision";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY: // content::revision_control::current_working_revision_key::<branch>::<locale> [string]
        return "current_working_revision_key";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH: // content::revision_control::last_branch [uint32_t]
        return "last_branch";

    case name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION: // content::revision_control::last_revision::<branch>::<locale> [uint32_t]
        return "last_revision";

    case name_t::SNAP_NAME_CONTENT_REVISION_LIMITS:
        return "content::revision_limits";

    case name_t::SNAP_NAME_CONTENT_REVISION_TABLE:
        return "revision";

    case name_t::SNAP_NAME_CONTENT_SECRET_TABLE:
        return "secret";

    case name_t::SNAP_NAME_CONTENT_SHORT_TITLE:
        return "content::short_title";

    case name_t::SNAP_NAME_CONTENT_SINCE:
        return "content::since";

    case name_t::SNAP_NAME_CONTENT_STATUS:
        return "content::status";

    case name_t::SNAP_NAME_CONTENT_STATUS_CHANGED:
        return "content::status_changed";

    case name_t::SNAP_NAME_CONTENT_SUBMITTED:
        return "content::submitted";

    case name_t::SNAP_NAME_CONTENT_TABLE: // pages, tags, comments, etc.
        return "content";

    case name_t::SNAP_NAME_CONTENT_TAG:
        return "content";

    case name_t::SNAP_NAME_CONTENT_TITLE:
        return "content::title";

    case name_t::SNAP_NAME_CONTENT_TRASHCAN:
        return "content::trashcan";

    case name_t::SNAP_NAME_CONTENT_UNTIL:
        return "content::until";

    case name_t::SNAP_NAME_CONTENT_UPDATED:
        return "content::updated";

    case name_t::SNAP_NAME_CONTENT_VARIABLE_REVISION:
        return "revision";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_CONTENT_...");

    }
    NOTREACHED();
}



namespace
{

/** \brief Extensions we accept as JavaScript file extensions.
 *
 * This table lists JavaScript extensions that we understand as
 * acceptable JavaScript extensions. This table is used to make
 * sure JavaScript files get added to the right place when
 * uploaded to the website.
 */
char const * js_extensions[] =
{
    // longer first
    ".min.js",
    ".org.js",
    ".js",
    //".as", -- TODO allow AS files as original JS files (see as2js)
    nullptr
};

char const * css_extensions[] =
{
    // longer first
    ".min.css",
    ".org.css",
    //".scss", -- TODO allow SCSS files as original CSS files (see csspp)
    ".css",
    nullptr
};

} // no name namespace






/** \brief Initialize the content plugin.
 *
 * This function is used to initialize the content plugin object.
 */
content::content()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the content plugin.
 *
 * Ensure the content object is clean before it is gone.
 */
content::~content()
{
}


/** \brief Get a pointer to the content plugin.
 *
 * This function returns an instance pointer to the content plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the content plugin.
 */
content * content::instance()
{
    return g_plugin_content_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString content::settings_path() const
{
    return "/settings/info";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString content::icon() const
{
    return "/images/snap/content-logo-64x64.png";
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
QString content::description() const
{
    return "Manage nearly all the content of your website. This plugin handles"
        " your pages, the website taxonomy (tags, categories, permissions...)"
        " and much much more.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString content::dependencies() const
{
    return "|server|";
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
int64_t content::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    // DO NOT CHANGE THE DATES ON THOSE ENTRIES
    SNAP_PLUGIN_UPDATE(2015, 7, 3, 20, 54, 18, remove_files_compressor);

    // This entry can get a newer date as things evolve
    SNAP_PLUGIN_UPDATE(2015, 9, 10, 3, 35, 19, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Delete the content::files::compressor field.
 *
 * When I first thought of having a compressed version of each file,
 * I put that field to name the compressor. The fact is that different
 * clients may not support the compressor we select. So instead I
 * create two fields per compression method.
 *
 * For example, for the GZIP compressor, I use:
 *
 * \li content::files::data::gzip_compressed
 * \li content::files::size::gzip_compressed
 *
 * Certain file formats allow for minification first. We do so against
 * XML, HTL, JavaScript and CSS documents. In that case we also offer a
 * separate field for each version:
 *
 * \li content::files::data::minified
 * \li content::files::size::minified
 *
 * And that version can itself be minified:
 *
 * \li content::files::data::minified::gzip_compressed
 * \li content::files::size::minified::gzip_compressed
 *
 * So... the "content::files::compressor" field is not required. Not
 * only that, so far I created it with a direct 'char const *' pointer
 * which means 0x01 was saved in that field instead of the intended
 * string.
 *
 * What we could (should) add is a field that gives us the order in which
 * the compressors are sorted (i.e. smallest version first, largets last)
 * so that way we do not have to check each size field one by one to know
 * which of the version to select and send to the user.
 */
void content::remove_files_compressor(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    libdbproxy::table::pointer_t files_table(get_files_table());
    files_table->clearCache();

    auto row_predicate = std::make_shared<libdbproxy::row_predicate>();
    QString const site_key(f_snap->get_site_key_with_slash());
    // process 100 in a row
    row_predicate->setCount(100);
    for(;;)
    {
        uint32_t const count(files_table->readRows(row_predicate));
        if(count == 0)
        {
            // no more files to process
            break;
        }
        libdbproxy::rows const rows(files_table->getRows());
        for(libdbproxy::rows::const_iterator o(rows.begin());
                o != rows.end(); ++o)
        {
            QString const key(QString::fromUtf8(o.key().data()));
            (*o)->dropCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_COMPRESSOR));
        }
    }
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void content::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);
}


/** \brief Initialize the content.
 *
 * This function terminates the initialization of the content plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void content::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(content, "server", server, save_content);
    SNAP_LISTEN(content, "server", server, add_snap_expr_functions, _1);
    SNAP_LISTEN(content, "server", server, register_backend_action, _1);
    SNAP_LISTEN0(content, "server", server, backend_process);
    SNAP_LISTEN(content, "server", server, load_file, _1, _2);
    SNAP_LISTEN(content, "server", server, table_is_accessible, _1, _2);

    SNAP_TEST_PLUGIN_SUITE_LISTEN(content);
}


/** \brief Initialize the content table.
 *
 * This function creates the content table if it does not already exist.
 * Otherwise it simply initializes the f_content_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The content table is the one that includes the tree representing the
 * entire content of all the websites. Since tables can grow as big as
 * we want, this is not a concern. The content table looks like a tree
 * although each row represents one leaf at any one level (the row keys
 * are the site key with slash + path).
 *
 * The data in a row of the content table includes two branch and revision
 * references: the current branch/revision and the current working
 * branch revision. The working version is the one the website administrator
 * edits until it looks good and then publish that version so it becomes
 * the current branch/revision.
 *
 * Branch zero is special in that it is used by the system to define the
 * data from the various content.xml files (hard coded data.)
 *
 * Branch one and up are reserved for the user, although a few other branch
 * numbers are reserved to indicate errors.
 *
 * The revision information makes use of one entry for the current branch,
 * and one entry for the current revision per branch and language. This is
 * then repeated for the current working branch and revisions.
 *
 * \code
 * content::revision_control::current_branch = <branch>
 * content::revision_control::current_revision::<branch>::<language> = <revision>
 * content::revision_control::current_working_branch = <branch>
 * content::revision_control::current_working_revision::<branch>::<language> = <revision>
 * content::revision_control::last_revision::<branch>::<language> = <revision>
 * \endcode
 *
 * \return The pointer to the content table.
 */
libdbproxy::table::pointer_t content::get_content_table()
{
    if(!f_content_table)
    {
        f_content_table = f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_TABLE));
    }
    return f_content_table;
}


/** \brief Initialize the secret table.
 *
 * This function creates the secret table if it does not already exist.
 * Otherwise it simply initializes the f_secret_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The secret table is used in parallel to the content table, only it is
 * used to save fields that should never appear to the end user. We use
 * this mechanism to save data such as tokens sent by a gateway offering
 * an OAuth2 login capability.
 *
 * The most important part here is that the secret table is NOT accessible
 * from the filter and any similar plugin. In other words, an end user
 * cannot write an expression which will peek in this table. The data is
 * viewed as being internal data only.
 *
 * Since this table is viewed as the content table, you should really
 * only have global data (i.e. one instance of the data per page, and
 * not one instance per branch, and not one instance per revision.)
 * This reduces the amount of secret data saved in your datbase since
 * editing such a page would otherwise duplicate the data once per
 * branch and/or once per revision. Secrete data does not get
 * duplicated.
 *
 * \note
 * This table should really only be used for data that should never be
 * visible in a page or a list. Plugins must use necessary precautions
 * to prevent end users from reading from this table, and to make use
 * of this table when they handle sensitive data.
 *
 * \return The pointer to the secret table.
 */
libdbproxy::table::pointer_t content::get_secret_table()
{
    if(!f_secret_table)
    {
        f_secret_table = f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_SECRET_TABLE));
    }
    return f_secret_table;
}


/** \brief Initialize the processing table.
 *
 * This function creates the processing table if it does not already exist.
 * Otherwise it simply initializes the f_processing_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The processing table is used to save all the URI of pages being processed
 * one way or the other. This allows the backend process to delete all
 * statuses (over 10 minutes old.)
 *
 * The data is set to the start date so we do not have to read anything
 * more to know whether we need to process that entry.
 *
 * \return The pointer to the content table.
 */
libdbproxy::table::pointer_t content::get_processing_table()
{
    if(!f_processing_table)
    {
        f_processing_table = f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_PROCESSING_TABLE));
    }
    return f_processing_table;
}


/** \brief Initialize the cache table.
 *
 * This function creates the cache table if it does not already exist.
 * Otherwise it simply initializes the f_cache_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The cache table is used to save preprocessed data for content, branch,
 * and revision tables. The rows are the same as those found in the content
 * table. The cached data is saved using various field names.
 *
 * Each plugin may cache data in a variety of ways. At this point there is
 * no specific scheme defining how the data saved in the cache table should
 * be handled. In all cases, though, there needs to be a way to invalidate
 * the cache (i.e. save the date when the cache was created so you can
 * detect whether it is still valid or not.)
 *
 * \return The pointer to the content table.
 */
libdbproxy::table::pointer_t content::get_cache_table()
{
    if(!f_cache_table)
    {
        f_cache_table = f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_CACHE_TABLE));
    }
    return f_cache_table;
}


/** \brief Initialize the branch table.
 *
 * This function creates the branch table if it does not exist yet. Otherwise
 * it simple initializes the f_branch_table variable member before returning
 * it.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The branch table is the one that includes the links of the page at
 * a specific branch level (links cannot be defined on a per revision basis.)
 * It is referenced from the content table for the current branch and
 * current working branch. Older branches may be accessed by using branch
 * identifiers smaller than the largest branch in existance (i.e.
 * content::current_working_branch in most cases.) Intermediate branches
 * may have been deleted (in most cases because they were so old.)
 *
 * The branch table is similar to the content table in that it looks like a
 * tree although it includes one row per branch.
 *
 * The key used by a branch is defined as follow:
 *
 * \code
 * <site-key>/<path>#<branch>
 * \endcode
 *
 * The '#' is used because it cannot appear in a path (i.e. the browser
 * cannot send you a request with a # in it, it is not legal.)
 *
 * The content table has references to the current branch and the current
 * working branch as follow:
 *
 * \code
 * content::revision_control::current_branch_key = <site-key>/<path>#1
 * content::revision_control::current_working_branch_key = <site-key>/<path>#1
 * \endcode
 *
 * Note that for attachments we do use a language, most often "xx", but there
 * are pictures created with text on them and thus you have to have a
 * different version for each language for pictures too.
 *
 * Note that \<language> never represents a programming language here. So if
 * an attachment is a JavaScript file, the language can be set to "en" if
 * it includes messages in English, but it is expected that all JavaScript
 * files be assigned language "xx". This also applies to CSS files which are
 * likely to all be set to "xx".
 *
 * \return The pointer to the branch table.
 */
libdbproxy::table::pointer_t content::get_branch_table()
{
    if(!f_branch_table)
    {
        f_branch_table = f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_BRANCH_TABLE));
    }
    return f_branch_table;
}


/** \brief Initialize the revision table.
 *
 * This function creates the revision table if it does not exist yet.
 * Otherwise it simple initializes the f_revision_table variable member
 * and returns its value.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The revision table is the one that includes the actual content of the
 * websites. It is referenced from the content table for the current
 * revision and current working revision. Older revisions can be listed
 * or tried with the exists() function.
 *
 * The revision table is similar to the content table in that it looks like
 * a tree although it includes one row per revision.
 *
 * The key is defined as follow:
 *
 * \code
 * <site-key>/<path>#<language>/<branch>.<revision>
 * \endcode
 *
 * The content table includes a couple of revision references: the
 * current revision and the current working revision.
 *
 * \code
 * content::revision_control::current_revision_key::<branch>::<language> = <site-key>/<path>#<language>/<branch>.<revision>
 * content::revision_control::current_working_revision_key::<branch>::<language> = <site-key>/<path>#<language>/<branch>.<revision>
 * \endcode
 *
 * Note that \<language> never represents a programming language here.
 *
 * \return The pointer to the revision table.
 */
libdbproxy::table::pointer_t content::get_revision_table()
{
    if(!f_revision_table)
    {
        f_revision_table = f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_REVISION_TABLE));
    }
    return f_revision_table;
}


/** \brief Initialize the files table.
 *
 * This function creates the files table if it doesn't exist yet. Otherwise
 * it simple initializes the f_files_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The table is used to list all the files from all the websites managed
 * by this Snap! server. Note that the files are listed for all the
 * websites, by website & filename, when new and need to be checked
 * (anti-virus, etc.) and maybe a few other things later.
 *
 * \li Rows are MD5 sums of the files, this is used as the key in the
 *     content table
 * \li '*new*' includes MD5 sums of files to be checked (anti-virus, ...)
 * \li '*index*' lists of files by 'site key + filename'
 *
 * \return The pointer to the files table.
 */
libdbproxy::table::pointer_t content::get_files_table()
{
    if(!f_files_table)
    {
        f_files_table = f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_FILES_TABLE));
    }
    return f_files_table;
}


/** \brief Retrieve the snap_child pointer.
 *
 * This function returns the snap_child object pointer. It is generally used
 * internally by sub-classes to gain access to the outside world.
 *
 * \return A pointer to the snap_child running this process.
 */
snap_child * content::get_snap()
{
    if(!f_snap)
    {
        // in case someone tries to get this while in the on_bootstrap()
        // function (which should not happen...)
        throw content_exception_content_not_initialized("content::get_snap() called before f_snap got initialized");
    }
    return f_snap;
}


/** \brief Create a page at the specified path.
 *
 * This function creates a page in the database at the specified path.
 * The page will be ready to be used once all the plugins had a chance
 * to run their own on_create_content() function.
 *
 * Note that if the page (as in, the row as defined by the path) already
 * exists then the function returns immediately.
 *
 * The full key for the page makes use of the site key which cannot already
 * be included in the path.
 *
 * The type of a new page must be specified. By default, the type is set
 * to "page". Specific modules may offer additional types. The three offered
 * by the content plugin are:
 *
 * \li "page" -- a standard user page.
 * \li "administration-page" -- in general any page under /admin.
 * \li "system-page" -- a page created by the content.xml which is not under
 *                      /admin.
 *
 * The page type MUST be just the type. It may be a path since a type
 * of page may be a sub-type of an basic type. For example, a "blog"
 * type would actually be a page and thus the proper type to pass to
 * this function is "page/blog" and not a full path or just "blog".
 * We force you in this way so any plugin can test the type without
 * having to frantically test all sorts of cases.
 *
 * The create function always generates  a new revision. If the specified
 * branch exists, then the latest revision + 1 is used. Otherwise, revision
 * zero (0) is used. When the system creates content it always uses
 * SPECIAL_VERSION_SYSTEM_BRANCH as the branch number (which is zero).
 *
 * \param[in] ipath  The path of the new page.
 * \param[in] owner  The name of the plugin that is to own this page.
 * \param[in] type  The type of page.
 *
 * \return true if the signal is to be propagated.
 */
bool content::create_content_impl(path_info_t & ipath, QString const & owner, QString const & type)
{
    libdbproxy::table::pointer_t content_table(get_content_table());
    libdbproxy::table::pointer_t branch_table(get_branch_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const key(ipath.get_key());

    // create the row
    QString const primary_owner(get_name(name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER));
    libdbproxy::row::pointer_t row(content_table->getRow(key));
    if(row->exists(primary_owner))
    {
        // it already exists, but it could have been deleted or moved before
        // in which case we need to resurrect the page back to NORMAL
        //
        // the editor allowing creating such a page should have asked the
        // end user first to know whether the page should indeed be
        // "undeleted".
        //
        path_info_t::status_t status(ipath.get_status());
        if(status.get_state() == path_info_t::status_t::state_t::DELETED
        || status.get_state() == path_info_t::status_t::state_t::MOVED)
        {
            // restore to a NORMAL page
            //
            // TODO: here we probably need to force a new branch so the
            //       user would not see the old revisions by default...
            //
            SNAP_LOG_WARNING("Re-instating (i.e. \"Undeleting\") page \"")(ipath.get_key())("\" as we received a create_content() request on a deleted page.");
            status.reset_state(path_info_t::status_t::state_t::NORMAL);
            ipath.set_status(status);
        }

        // the row already exists, this is considered created.
        // (we may later want to have a repair_content signal
        // which we could run as an action from the backend...)
        // however, if it were created by an add_xml() call,
        // then the on_create_content() of all the other plugins
        // should probably be called (i.e. f_updating is true then)
        //
        return f_updating;
    }

    // note: we do not need to test whether the home page ("") allows
    //       for children; if not we would have a big problem!
    if(!ipath.get_cpath().isEmpty())
    {
        // parent path is the path without the last "/..." part
        int const pos(ipath.get_cpath().lastIndexOf('/'));
        if(pos >= 0)
        {
            QString const parent_key(site_key + ipath.get_cpath().left(pos));
            if(is_final(parent_key))
            {
                // the user was trying to add content under a final leaf
                f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN, "Final Parent",
                        QString("Page \"%1\" cannot be added under \"%2\" since \"%2\" is marked as final.")
                                    .arg(key).arg(parent_key),
                        "The parent row does not allow for further children.");
                NOTREACHED();
            }
        }
    }

    // first, we want to save the status
    //
    // This is not required anymore because a page with a primary owner
    // is automatically viewed as in the CREATE state
    //
    //path_info_t::status_t status(ipath.get_status());
    //status.reset_state(path_info_t::status_t::state_t::CREATE);
    //ipath.set_status(status);

    // save the owner
    row->getCell(primary_owner)->setValue(owner);

    SNAP_LOG_DEBUG("Started creation of page \"")(ipath.get_key())("\".");

    // setup first branch
    snap_version::version_number_t const branch_number(ipath.get_branch());

    set_branch(key, branch_number, false);
    set_branch(key, branch_number, true);
    set_branch_key(key, branch_number, true);
    set_branch_key(key, branch_number, false);

    snap_version::version_number_t const revision_number(ipath.get_revision());
    if(revision_number != snap_version::SPECIAL_VERSION_UNDEFINED
    && revision_number != snap_version::SPECIAL_VERSION_INVALID
    && revision_number != snap_version::SPECIAL_VERSION_EXTENDED)
    {
        QString const locale(ipath.get_locale());
        set_current_revision(key, branch_number, revision_number, locale, false);
        set_current_revision(key, branch_number, revision_number, locale, true);
        set_revision_key(key, branch_number, revision_number, locale, true);
        set_revision_key(key, branch_number, revision_number, locale, false);
    }

    // add the different basic content dates setup
    int64_t const start_date(f_snap->get_start_date());
    row->getCell(get_name(name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);

    libdbproxy::row::pointer_t branch_row(branch_table->getRow(ipath.get_branch_key()));
    branch_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
    branch_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_MODIFIED))->setValue(start_date);

    // link the page to its type (very important for permissions)
    links::links * links_plugin(links::links::instance());
    {
        // TODO We probably should test whether that content-types exists
        //      because if not it's certainly completely invalid (i.e. the
        //      programmer mistyped the type [again].)
        //
        //      However, we have to be very careful as the initialization
        //      process may not be going in the right order and thus not
        //      have created the type yet when this starts to happen.
        //
        QString const destination_key(site_key + "types/taxonomy/system/content-types/" + (type.isEmpty() ? "page" : type));
        path_info_t destination_ipath;
        destination_ipath.set_path(destination_key);
        QString const link_name(get_name(name_t::SNAP_NAME_CONTENT_PAGE_TYPE));
        QString const link_to(get_name(name_t::SNAP_NAME_CONTENT_PAGE));
        bool const source_unique(true);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, key, branch_number);
        links::link_info destination(link_to, destination_unique, destination_key, destination_ipath.get_branch());
        links_plugin->create_link(source, destination);
    }

    // link this entry to its parent automatically
    // first we need to remove the site key from the path
    snap_version::version_number_t child_branch(branch_number);
    snap_version::version_number_t parent_branch;
    snap_string_list parts(ipath.get_segments());
    while(parts.count() > 0)
    {
        QString const src(QString("%1%2").arg(site_key).arg(parts.join("/")));
        parts.pop_back();
        QString const dst(QString("%1%2").arg(site_key).arg(parts.join("/")));

        // TBD: 2nd parameter should be true or false?
        parent_branch = get_current_branch(dst, true);

        // TBD: is the use of the system branch always correct here?
        links::link_info source(get_name(name_t::SNAP_NAME_CONTENT_PARENT), true, src, child_branch);
        links::link_info destination(get_name(name_t::SNAP_NAME_CONTENT_CHILDREN), false, dst, parent_branch);
// TODO only repeat if the parent did not exist, otherwise we assume the
//      parent created its own parent/children link already.
//SNAP_LOG_DEBUG("parent/children [")(src)("]/[")(dst)("]");
        links_plugin->create_link(source, destination);

        child_branch = parent_branch;
    }

    SNAP_LOG_DEBUG("Creation of page \"")(ipath.get_key())("\" in content plugin is ready for other modules to react.");

    return true;
}


/** \brief Function called after all the other plugins signal were called.
 *
 * This function gives a chance to the content plugin to fix the status
 * to NORMAL since on creation it is set to CREATING instead.
 *
 * \param[in] path  The path of the new page.
 * \param[in] owner  The name of the plugin that is to own this page.
 * \param[in] type  The type of page.
 */
void content::create_content_done(path_info_t & ipath, QString const & owner, QString const & type)
{
    NOTUSED(owner);
    NOTUSED(type);

    SNAP_LOG_DEBUG("Finalization of page \"")(ipath.get_key())("\" in content plugin (i.e. create_content_done() function) is running now.");

    // now the page was created and is ready to be used
    // (although the revision data is not yet available...
    // but at this point we do not have a good way to handle
    // that part yet.)
    //
    path_info_t::status_t status(ipath.get_status());
    if(status.get_state() == path_info_t::status_t::state_t::CREATE)
    {
        status.set_state(path_info_t::status_t::state_t::NORMAL);
        ipath.set_status(status);
    }

    // the page now exists and is considered valid so add it to the content
    // index for all the have access to
    //
    libdbproxy::table::pointer_t content_table(get_content_table());
    libdbproxy::value ready;
    ready.setSignedCharValue(1);
    content_table->getRow(get_name(name_t::SNAP_NAME_CONTENT_INDEX))->getCell(ipath.get_key())->setValue(ready);

    SNAP_LOG_DEBUG("Page \"")(ipath.get_key())("\" creation was completed.");
}


/** \brief Create a page which represents an attachment (A file).
 *
 * This function creates a page that represents an attachment with the
 * specified \p file. The new file path is saved in the \p file
 * object. On a successfully return you can retrieve the attachment
 * path with the get_attachment_cpath() function.
 *
 * This function prepares the file and sends a create_content() event
 * to create the actual content entry if it does not exist yet.
 *
 * Note that the MIME type of the file is generated using the magic
 * database. The \p attachment_type information is the one saved in the
 * page referencing that file. However, only the one generated by magic
 * is considered official.
 *
 * \note
 * It is important to understand that we only save each file ONCE,
 * in the database. This is accomplished by create_attachment() by
 * computing the MD5 sum of the file and then checking whether the
 * file was previously loaded. If so, then the existing copy is used
 * (even if it was uploaded by someone else on another website!)
 *
 * Possible cases when creating an attachment:
 *
 * \li The file does not yet exist in the files table; in that case we
 *     simply create it;
 *
 * \li If the file already existed, we do not add it again (obviously)
 *     and we can check whether it was already attached to that very
 *     same page; if so then we have nothing else to do (files have
 *     references of all the pages were they are attachments);
 *
 * \li When adding a JavaScript or a CSS file, the version and browser
 *     information also gets checked; it is extracted from the file itself
 *     and used to version the file in the database (in the content row);
 *     note that each version of a JavaScript or CSS file ends up in
 *     the database (just like with a tool such as SVN or git); this
 *     version information replaces the branch and revision information
 *     normally used by Snap!.
 *
 * \warning
 * All files are versioned (if not a JavaScript or CSS file, then the
 * standard Snap! branch and revision numbers are used.) By default you
 * will access such a file with the bare filename (i.e. the filename
 * without the version included in the filename). You may also include
 * the name of the browser and the version in the filename or you can
 * use the "branch" and "revision" query strings (see qs_branch and
 * qs_revision) to load a very specific version of a file. Either way
 * you will be directed to the same page in the content table. The fork
 * happens when the file to output is searched. This mechanism also
 * includes minification (.min.) and compression (.gz) schemes.
 *
 * \code
 *  // access the file as "editor.js" on the website
 *  http://snapwebsites.org/js/editor/editor.js
 *
 *  // saved the file as editor_1.2.3.js in files
 *  files["editor_1.2.3.js"]
 * \endcode
 *
 * This is particularly confusing because the server is capable of
 * recognizing a plethora of filenames that all resolve to the same
 * file in the files table only "tweaked" as required internally.
 * Tweaked here means reformatted as requested.
 *
 * \code
 *  // minimized version 1.2.3, current User Agent
 *  http://snapwebsites.org/js/editor/editor_1.2.3.min.js
 *
 *  // original version, compressed, current User Agent
 *  http://snapwebsites.org/js/editor/editor_1.2.3.org.js.gz
 *
 *  // specifically the version for Internet Explorer
 *  http://snapwebsites.org/js/editor/editor_1.2.3_ie.min.js
 *
 *  // the same with query strings
 *  http://snapwebsites.org/js/editor/editor.js?v=1.2.3&b=ie&e=min
 *
 *  // for images, you upload a JPEG and you can access it as a PNG...
 *  http://snapwebsites.org/some/page/image.png
 *
 *  // for images, you upload a 300x900 page, and access it as a 100x300 image
 *  http://snapwebsites.org/some/page/image.png?d=100x300
 * \endcode
 *
 * The supported fields are: [FIXME--this is not quite true, we still have
 * to implement some parts too...]
 *
 * \li \<name> -- the name of the file
 * \li [v=] \<version> -- a specific version of the file (if not specified, get
 *                   latest)
 * \li [b=] \<browser> -- a specific version for that browser
 * \li [e=] \<encoding> -- a specific encoding, in most cases a compression,
 *                         for a JavaScript/CSS file "minimize" is also
 *                         understood (i.e. min,gz or org,bz2); this can be
 *                         used to convert an image to another format
 * \li [d=] \<width>x<height> -- dimensions for an image
 *
 * \bug
 * The create_attachment() signal, just like the create_content() cannot
 * currently fail. Not so much because we cannot know whether the create
 * failed, but especially because otherwise you are likely to have dangling
 * data in the Cassandra database.
 *
 * \param[in,out] file  The file to save in the Cassandra database. It is an
 *                      in,out parameter because the exact filename used to
 *                      save the file in the content table is saved in this
 *                      object.
 * \param[in] branch_number  The branch used to save the attachment.
 * \param[in] locale  The language & country to use for this file.
 *
 * \return true if other plugins are to receive the signal too, the function
 *         generally returns false if the attachment cannot be created or
 *         already exists
 */
bool content::create_attachment_impl(attachment_file & file, snap_version::version_number_t branch_number, QString const & locale)
{
    // quick check for security reasons so we can avoid unwanted uploads
    // (note that we already had the check for size and similar "problems")
    //
    permission_flag secure;
    check_attachment_security(file, secure, true);
    if(!secure.allowed())
    {
        SNAP_LOG_ERROR("attachment not created because it is viewed as insecure; reference: \"")(file.get_attachment_cpath())("\".");
        return false;
    }

    // TODO: uploading compressed files is a problem if we are to match the
    //       proper MD5 of the file; we will want to check and decompress
    //       files so we only save the decompressed version MD5 and not the
    //       compressed MD5 (otherwise we end up with TWO files.)

    // verify that the row specified by file::get_cpath() exists
    //
    libdbproxy::table::pointer_t content_table(get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const parent_key(site_key + file.get_parent_cpath());
    if(!content_table->exists(parent_key))
    {
        // the parent row does not even exist yet...
        //
        SNAP_LOG_ERROR("user attempted to create an attachment in page \"")(parent_key)("\" that does not exist.");
        return false;
    }

    // create the path to the new attachment itself
    // first get the basename
    //
    snap_child::post_file_t const & post_file(file.get_file());
    QString attachment_filename(post_file.get_basename());

    // make sure that the parent of the attachment is not final
    //
    if(is_final(parent_key))
    {
        // the user was trying to add content under a final leaf
        f_snap->die(
                snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                "Final Parent",
                QString("The attachment \"%1\" cannot be added under \"%2\" since it is marked as final.")
                            .arg(attachment_filename)
                            .arg(parent_key),
                "The parent row does not allow for further children.");
        NOTREACHED();
    }

    snap_version::quick_find_version_in_source fv;
    QString revision; // there is no default
    QString extension;

    // if JavaScript or CSS, add the version to the filename before
    // going forward (unless the version is already there, of course)
    bool const is_js(file.get_parent_cpath().startsWith("js/"));
    bool const is_css(file.get_parent_cpath().startsWith("css/"));
    if(is_js)
    {
        extension = snap_version::find_extension(attachment_filename, js_extensions);
        if(extension.isEmpty())
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN, "Invalid Extension",
                    QString("The attachment \"%1\" cannot be added under \"%2\" as it does not represent JavaScript code.")
                                .arg(attachment_filename).arg(parent_key),
                    "The filename does not have a .js extension.");
            NOTREACHED();
        }
    }
    else if(is_css)
    {
        extension = snap_version::find_extension(attachment_filename, css_extensions);
        if(extension.isEmpty())
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN, "Invalid Extension",
                    QString("The attachment \"%1\" cannot be added under \"%2\" as it does not represent CSS data.")
                                .arg(attachment_filename).arg(parent_key),
                    "The filename does not have a .css extension.");
            NOTREACHED();
        }
    }
    if(is_js || is_css)
    {
        // TODO: In this case, really, we probably should only accept
        //       filenames without anything specified although the version
        //       is fine if it matches what is defined in the file...
        //       However, if the name includes .min. (minimized) then we've
        //       got a problem because the non-minimized version would not
        //       match properly. This being said, a version that is
        //       pre-minimized can be uploaded as long as the .org. is not
        //       used to see a non-minimized version.

        if(!fv.find_version(post_file.get_data().data(), post_file.get_size()))
        {
            f_snap->die(
                    snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                    "Invalid File",
                    QString("The attachment \"%1\" does not include a valid C-like comment at the start."
                            " The comment must at least include a <a href=\"See "
                            "http://snapwebsites.org/implementation/feature-requirements/attachments-core\">Version field</a>.")
                                    .arg(attachment_filename),
                    "The content of this file is not valid for a JavaScript or CSS file (version required).");
            NOTREACHED();
        }

        // get the filename without the extension
        //
        QString const fn(attachment_filename.left(attachment_filename.length() - extension.length()));
        if(fn.contains("_"))
        {
            // WARNING: the following code says ".js" and js_filename even
            //          though all of that also works for ".css" files.
            //
            // if there is a "_" then we have a file such as
            //
            //   <name>_<version>.js
            // or
            //   <name>_<version>_<browser>.js
            //
            snap_version::versioned_filename js_filename(extension);
            if(!js_filename.set_filename(attachment_filename))
            {
                f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN, "Invalid Filename",
                        "The attachment \"" + attachment_filename + "\" has an invalid name and must be rejected. " + js_filename.get_error(),
                        "The name is not considered valid for a versioned file.");
                NOTREACHED();
            }
            if(fv.get_version_string() != js_filename.get_version_string())
            {
                f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN, "Versions Mismatch",
                        QString("The attachment \"%1\" filename version (%2) is not the same as the version inside the file (%3).")
                            .arg(attachment_filename)
                            .arg(js_filename.get_version_string())
                            .arg(fv.get_version_string()),
                        "The version in the filename is not equal to the one defined in the file.");
                NOTREACHED();
            }
            // TODO verify the browser defined in the filename
            //      against Browsers field found in the file

            // remove the version and browser information from the filename
            attachment_filename = js_filename.get_name() + extension;

            if(fv.get_name().isEmpty())
            {
                // no name field, use the filename
                fv.set_name(js_filename.get_name());
            }
        }
        else
        {
            // in this case the name is just <name> and must match
            //
            //    [a-z][-a-z0-9]*[a-z0-9]
            //
            // TBD: I removed the namespace, it does not look like we
            //      should support filename such as info::name.js and
            //      now we have a separate function to check the basic
            //      filename so I could remove the namespace support here
            //
            QString name_string(fn);
            QString errmsg;
            if(!snap_version::validate_basic_name(name_string, errmsg))
            {
                // unacceptable filename
                f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN, "Invalid Filename",
                        QString("The attachment \"%1\" has an invalid name and must be rejected. %2").arg(attachment_filename).arg(errmsg),
                        "The name is not considered valid for a versioned file.");
                NOTREACHED();
            }

            if(fv.get_name().isEmpty())
            {
                // no name field, use the filename
                fv.set_name(fn);
            }
        }

        // the filename is now just <name> (in case it had a version and/or
        // browser indication on entry.)

        // ignore the input branch number, instead retrieve first version
        // number of the file as the branch number...
        branch_number = fv.get_branch();
        revision = fv.get_version_string();
#ifdef DEBUG
        if(revision.isEmpty() || snap_version::SPECIAL_VERSION_UNDEFINED == branch_number)
        {
            // we already checked for errors while parsing the file so we
            // should never reach here if the version is empty in the file
            throw snap_logic_exception("the version of a JavaScript or CSS file just cannot be empty here");
        }
#endif

        // in the attachment, save the filename with the version so that way
        // it is easier to see which is which there
    }
    else
    {
        // for other attachments, there could be a language specified as
        // in .en.jpg. In that case we want to get the filename without
        // the language and mark that file as "en"

        // TODO: actually implement the language extraction capability
    }

    // path in the content table, the attachment_filename is the simple
    // name without version, language, or encoding
    path_info_t attachment_ipath;
    //attachment_ipath.set_owner(...); -- this is not additional so keep the default (content)
    attachment_ipath.set_path(QString("%1/%2").arg(file.get_parent_cpath()).arg(attachment_filename));
    if(!revision.isEmpty())
    {
        // in this case the revision becomes a string with more than one
        // number and the branch is the first number (this is for js/css
        // files only at this point.)
        attachment_ipath.force_extended_revision(revision, attachment_filename);
    }

    // save the path to the attachment so the caller knows exactly where it
    // is (if required by that code.)
    file.set_attachment_cpath(attachment_ipath.get_cpath());

#ifdef DEBUG
//SNAP_LOG_DEBUG("attaching ")(file.get_file().get_filename())(", attachment_key = ")(attachment_ipath.get_key());
#endif

    // compute the MD5 sum of the file
    // TBD should we forbid the saving of empty files?
    unsigned char md[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<unsigned char const *>(post_file.get_data().data()), post_file.get_size(), md);
    QByteArray const md5(reinterpret_cast<char const *>(md), sizeof(md));

    // check whether the file already exists in the database
    libdbproxy::table::pointer_t files_table(get_files_table());
    bool file_exists(files_table->exists(md5));
    if(!file_exists)
    {
        // the file does not exist yet, add it
        //
        // 1. create the row with the file data, the compression used,
        //    and size; also add it to the list of new cells
        files_table->getRow(md5)->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_DATA))->setValue(post_file.get_data());
        signed char const new_file(1);
        files_table->getRow(get_name(name_t::SNAP_NAME_CONTENT_FILES_NEW))->getCell(md5)->setValue(new_file);

        libdbproxy::row::pointer_t file_row(files_table->getRow(md5));

        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SIZE))->setValue(static_cast<int32_t>(post_file.get_size()));

        // Note we save the following mainly for completness because it is
        // not really usable (i.e. two people who are to upload the same file
        // with the same filename, the same original MIME type, the same
        // creation/modification dates... close to impossible!)
        //
        // 2. link back to the row where the file is saved in the content table
        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_FILENAME))->setValue(attachment_filename);

        // 3. save the computed MIME type
        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_MIME_TYPE))->setValue(post_file.get_mime_type());

        // 4. save the original MIME type
        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE))->setValue(post_file.get_original_mime_type());

        // 5. save the creation date if available (i.e. if not zero)
        if(post_file.get_creation_time() != 0)
        {
            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_CREATION_TIME))->setValue(static_cast<int64_t>(post_file.get_creation_time()));
        }

        // 6. save the modification date if available (i.e. if not zero)
        if(post_file.get_modification_time() != 0)
        {
            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME))->setValue(static_cast<int64_t>(post_file.get_modification_time()));
        }

        // 7. save the date when the file was uploaded
        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_CREATED))->setValue(f_snap->get_start_date());

        // 8. save the date when the file was last updated
        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_UPDATED))->setValue(f_snap->get_start_date());

        // 9. if the file is an image save the width & height
        int32_t width(post_file.get_image_width());
        int32_t height(post_file.get_image_height());
        if(width > 0 && height > 0)
        {
            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH))->setValue(width);
            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT))->setValue(height);
        }

        // 10. save the description
        // At this point we do not have that available, we could use the
        // comment/description from the file if there is such, but those
        // are often "broken" (i.e. version of the camera used...)

        // TODO should we also save a SHA1 of the files so people downloading
        //      can be given the SHA1 even if the file is saved compressed?

        // 11. Some additional fields
        signed char sflag(CONTENT_SECURE_UNDEFINED);
        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE))->setValue(sflag);
        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK))->setValue(static_cast<int64_t>(0));
        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURITY_REASON))->setValue(QString());

        // 12. save dependencies
        {
            // dependencies will always be the same for all websites so we
            // save them here too
            dependency_list_t const& deps(file.get_dependencies());
            QMap<QString, bool> found;
            int const max_size(deps.size());
            for(int i(0); i < max_size; ++i)
            {
                snap_version::dependency d;
                if(!d.set_dependency(deps[i]))
                {
                    // simply invalid...
                    SNAP_LOG_ERROR("Dependency \"")(deps[i])("\" is not valid (")(d.get_error())("). We cannot add it to the database. Note: the content plugin does not support <dependency> tags with comma separated dependencies. Instead create multiple tags.");
                }
                else
                {
                    QString const dependency_name(d.get_name());
                    QString full_name;
                    if(d.get_namespace().isEmpty())
                    {
                        full_name = dependency_name;
                    }
                    else
                    {
                        full_name = QString("%1::%2").arg(d.get_namespace()).arg(dependency_name);
                    }
                    if(found.contains(full_name))
                    {
                        // not unique
                        SNAP_LOG_ERROR("Dependency \"")(deps[i])("\" was specified more than once. We cannot safely add the same dependency (same name) more than once. Please merge both definitions or delete one of them.");
                    }
                    else
                    {
                        // save the canonicalized version of the dependency in the database
                        found[full_name] = true;
                        file_row->getCell(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_CONTENT_FILES_DEPENDENCY)).arg(full_name))->setValue(d.get_dependency_string());
                    }
                }
            }
        }
    }
// for test purposes to check a file over and over again
//{
//signed char const new_file(1);
//files_table->getRow(get_name(name_t::SNAP_NAME_CONTENT_FILES_NEW))->getCell(md5)->setValue(new_file);
//}

    // make a full reference back to the attachment (which may not yet
    // exist at this point, we do that next)
    QString ref_cell_name;
    if(is_css || is_js)
    {
        // CSS and JavaScript filenames are forced to include the version
        // and we generally want to use the minified version (I am not too
        // sure how to handle that one right now though.)
        //
        ref_cell_name = QString("%1::%2%3/%4_%5.min.%6")
                            .arg(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE))
                            .arg(site_key)
                            .arg(file.get_parent_cpath())
                            .arg(fv.get_name())
                            .arg(fv.get_version_string())
                            .arg(is_css ? "css" : "js");

        // TODO: also include the browser? I'm not too sure how we can
        //       handle this one correct here because it will depend on
        //       the browser the end user has and not a static information
        //       (i.e. fv has a get_browsers(), PLURAL...)

        {
            // verify that we do not already have a reference
            // if we do, make sure it is one to one equivalent to what we
            // just generated
            //
            auto references_column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
            references_column_predicate->setCount(10);
            references_column_predicate->setIndex(); // behave like an index
            QString const start_ref(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE)).arg(site_key));
            references_column_predicate->setStartCellKey(start_ref);
            references_column_predicate->setEndCellKey(start_ref + libdbproxy::cell_predicate::last_char);

            files_table->getRow(md5)->clearCache();
            files_table->getRow(md5)->readCells(references_column_predicate);
            libdbproxy::cells const ref_cells(files_table->getRow(md5)->getCells());
            if(!ref_cells.isEmpty())
            {
                if(ref_cells.size() > 1)
                {
                    throw snap_logic_exception(QString("JavaScript or CSS file \"%1\" has more than one reference to this website...").arg(post_file.get_filename()));
                }
                libdbproxy::cell::pointer_t ref_cell(*ref_cells.begin());
                if(ref_cell->columnName() != ref_cell_name)
                {
                    // this could be an error, but we can just refresh the
                    // wrong reference with the new correct one instead
                    // (i.e. existing files that used the old scheme are
                    // automatically updated that way)
                    SNAP_LOG_WARNING("JavaScript or CSS file \"")
                            (post_file.get_filename())
                            ("\" has an existing reference \"")
                            (ref_cell->columnName())
                            ("\" which is not equal to the expected string \"")
                            (ref_cell_name)
                            ("\"...");
                    files_table->getRow(md5)->dropCell(ref_cell->columnName());
                }
            }
        }
    }
    else
    {
        ref_cell_name = QString("%1::%2")
                            .arg(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE))
                            .arg(attachment_ipath.get_key());
    }
    signed char const ref(1);
    files_table->getRow(md5)->getCell(ref_cell_name)->setValue(ref);

    QByteArray attachment_ref;
    attachment_ref.append(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE));
    attachment_ref.append("::");
    attachment_ref.append(md5); // binary md5

    // check whether the row exists before we create it
    bool const content_row_exists(content_table->exists(attachment_ipath.get_key()));

    // this may be a new content row, that is, it may still be empty so we
    // have to test several things before we can call create_content()...

    libdbproxy::table::pointer_t branch_table(get_branch_table());
    libdbproxy::table::pointer_t revision_table(get_revision_table());

    bool remove_old_revisions(false);

    // if the revision is still empty then we are dealing with a file
    // which is neither a JavaScript nor a CSS file
    if(revision.isEmpty())
    {
        // TODO: allow editing of any branch, not just the working
        //       branch... (when using "?branch=123"...)
        snap_version::version_number_t revision_number(snap_version::SPECIAL_VERSION_UNDEFINED);

        if(file_exists
        && snap_version::SPECIAL_VERSION_UNDEFINED != branch_number
        && snap_version::SPECIAL_VERSION_INVALID != branch_number)
        {
            attachment_ipath.force_branch(branch_number);

            // the file already exists, it could very well be that the
            // file had an existing revision in this attachment row so
            // search for all existing revisions (need a better way to
            // instantly find those!)
            //QString const attachment_ref(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE)).arg(md5));
            file_exists = branch_table->exists(attachment_ipath.get_branch_key())
                       && branch_table->getRow(attachment_ipath.get_branch_key())->exists(attachment_ref);
            if(file_exists)
            {
                // the reference row exists!
                file_exists = true; // avoid generation of a new revision!
                revision_number = branch_table->getRow(attachment_ipath.get_branch_key())->getCell(attachment_ref)->getValue().int64Value();
                attachment_ipath.force_revision(revision_number);
                //content_table->getRow(attachment_ipath.get_key())->getCell(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT_REVISION_CONTROL_CURRENT_WORKING_VERSION))->setValue(revision);
            }
        }

        if(!file_exists)
        {
            if(snap_version::SPECIAL_VERSION_UNDEFINED == branch_number
            || snap_version::SPECIAL_VERSION_INVALID == branch_number)
            {
                branch_number = get_current_branch(attachment_ipath.get_key(), true);
            }
            attachment_ipath.force_branch(branch_number);

            // validity check; although the code would fail a few lines
            // later, by failing here we can better explain what the
            // problem is to the programmer
            //
            snap_version::version_number_t const old_branch_number(get_current_branch(attachment_ipath.get_key(), true));
            if(old_branch_number != snap_version::SPECIAL_VERSION_INVALID
            && old_branch_number != snap_version::SPECIAL_VERSION_UNDEFINED
            && old_branch_number != branch_number)
            {
                // the page exists, but not that branch so create it now
                //
                copy_branch(attachment_ipath.get_key(), old_branch_number, branch_number);
                revision_number = snap_version::SPECIAL_VERSION_FIRST_REVISION;
            }
            else
            {
                if(snap_version::SPECIAL_VERSION_UNDEFINED == branch_number)
                {
                    // this should nearly never (if ever) happen
                    branch_number = get_new_branch(attachment_ipath.get_key(), locale);
                    set_branch_key(attachment_ipath.get_key(), branch_number, true);
                    // new branches automatically get a revision of zero (0)
                    revision_number = snap_version::SPECIAL_VERSION_FIRST_REVISION;
                }
                else
                {
                    revision_number = get_new_revision(attachment_ipath.get_key(), branch_number, locale, true);

                    // only when we create a new revision do we need to
                    // possibly remove an old one
                    //
                    remove_old_revisions = true;
                }
            }

            attachment_ipath.force_revision(revision_number);
        }

        if(snap_version::SPECIAL_VERSION_UNDEFINED == branch_number
        || snap_version::SPECIAL_VERSION_UNDEFINED == revision_number)
        {
            throw snap_logic_exception(QString("the branch (%1) and/or revision (%2) numbers are still undefined").arg(branch_number).arg(revision_number));
        }

        set_branch(attachment_ipath.get_key(), branch_number, true);
        set_branch(attachment_ipath.get_key(), branch_number, false);
        set_branch_key(attachment_ipath.get_key(), branch_number, true);
        set_branch_key(attachment_ipath.get_key(), branch_number, false);

        // TODO: this call is probably wrong, that is, it works and shows the
        //       last working version but the user may want to keep a previous
        //       revision visible at this point...
        set_current_revision(attachment_ipath.get_key(), branch_number, revision_number, locale, false);
        set_current_revision(attachment_ipath.get_key(), branch_number, revision_number, locale, true);
        set_revision_key(attachment_ipath.get_key(), branch_number, revision_number, locale, true);
        set_revision_key(attachment_ipath.get_key(), branch_number, revision_number, locale, false);

        // back reference for quick search
        branch_table->getRow(attachment_ipath.get_branch_key())->getCell(attachment_ref)->setValue(static_cast<int64_t>(revision_number));

        revision = QString("%1.%2").arg(branch_number).arg(revision_number);
    }
    else
    {
        // for JavaScript and CSS files we have it simple for now but
        // this is probably somewhat wrong... (remember that for JS/CSS files
        // we do not generate a revision number, we use the file version
        // instead.)
        //

        // if the branch number is new, we want to copy the old one to
        // the new one to start somewhere
        //
        snap_version::version_number_t const old_branch_number(get_current_branch(attachment_ipath.get_key(), true));
        if(old_branch_number != snap_version::SPECIAL_VERSION_INVALID
        && old_branch_number != snap_version::SPECIAL_VERSION_UNDEFINED
        && old_branch_number != branch_number)
        {
            // the page exists, but not that branch so create it now
            //
            copy_branch(attachment_ipath.get_key(), old_branch_number, branch_number);
        }

        set_branch(attachment_ipath.get_key(), branch_number, true);
        set_branch(attachment_ipath.get_key(), branch_number, false);
        set_branch_key(attachment_ipath.get_key(), branch_number, true);
        set_branch_key(attachment_ipath.get_key(), branch_number, false);
        set_revision_key(attachment_ipath.get_key(), branch_number, revision, locale, true);
        set_revision_key(attachment_ipath.get_key(), branch_number, revision, locale, false);

        // TODO: add set_current_revision()/set_revision_key()/... to save
        //       that info (only the revision here may be multiple numbers)
    }

    // this name is "content::attachment::<plugin owner>::<field name>::path" (unique)
    //           or "content::attachment::<plugin owner>::<field name>::path::<server name>_<unique number>" (multiple)
    QString const name(file.get_name());
    libdbproxy::row::pointer_t parent_row(content_table->getRow(parent_key));

    libdbproxy::row::pointer_t content_attachment_row(content_table->getRow(attachment_ipath.get_key()));
    //libdbproxy::row::pointer_t branch_attachment_row(branch_table->getRow(attachment_ipath.get_branch_key()));
    libdbproxy::row::pointer_t revision_attachment_row(revision_table->getRow(attachment_ipath.get_revision_key()));

    // We depend on the JavaScript plugin so we have to do some of its
    // work here...
    if(is_js || is_css)
    {
        // JavaScripts and CSS files get added to a list so their
        // dependencies can be found "instantaneously".
        //snap_version::versioned_filename js_filename(".js");
        //js_filename.set_filename(attachment_filename);
        // the name is formatted to allow us to quickly find the files
        // we are interested in; for that we put the name first, then the
        // browser, and finally the version which is saved as integers
        snap_version::name::vector_t browsers(fv.get_browsers());
        int const bmax(browsers.size());
        bool const all(bmax == 1 && browsers[0].get_name() == "all");
        for(int i(0); i < bmax; ++i)
        {
            QByteArray jskey;
            jskey.append(fv.get_name());
            jskey.append('_');
            jskey.append(browsers[i].get_name());
            jskey.append('_');
            snap_version::version_numbers_vector_t const & version(fv.get_version());
            int const vmax(version.size());
            for(int v(0); v < vmax; ++v)
            {
                libdbproxy::appendUInt32Value(jskey, version[v]);
            }
            files_table->getRow(is_css
                    ? get_name(name_t::SNAP_NAME_CONTENT_FILES_CSS)
                    : get_name(name_t::SNAP_NAME_CONTENT_FILES_JAVASCRIPTS))
                            ->getCell(jskey)->setValue(md5);
            if(!all)
            {
                // TODO: need to parse the script for this specific browser
            }
        }
    }

    // if the field exists and that attachment is unique (i.e. supports only
    // one single file), then we want to delete the existing page unless
    // the user uploaded a file with the exact same filename
    if(content_row_exists)
    {
        // if multiple it can already exist, we just created a new unique number
        if(!file.get_multiple())
        {
            // it exists, check the filename first
            if(parent_row->exists(name))
            {
                // get the filename (attachment key)
                QString const old_attachment_key(parent_row->getCell(name)->getValue().stringValue());
                if(!old_attachment_key.isEmpty() && old_attachment_key != attachment_ipath.get_key())
                {
                    // that is not the same filename, trash the old one
                    //
                    SNAP_LOG_INFO("deleting now unused attachment \"")(old_attachment_key)("\" replacing with \"")(attachment_ipath.get_key())("\".");
                    path_info_t old_attachment_ipath;
                    old_attachment_ipath.set_path(old_attachment_key);
                    trash_page(old_attachment_ipath);

                    // TBD if I am correct, the md5 reference was already dropped
                    //     in the next if() blocks...
                    //
                    // TODO: we most certainly need to remove all the
                    //       references found in the branch table whenever
                    //       we replace/delete a file; right now that
                    //       just cumulates which is fine because I do not
                    //       think I use them really; although it could
                    //       be that I properly remove the reference in
                    //       the files table and not in the branch table...
                }
            }
        }

        if(revision_attachment_row->exists(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT)))
        {
            // the MD5 is saved in there, get it and compare
            libdbproxy::value const existing_ref(revision_attachment_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT))->getValue());
            if(existing_ref.size() == 16)
            {
                if(existing_ref.binaryValue() == md5)
                {
                    // this is the exact same file, do nearly nothing
                    // (i.e. the file may already exist but the path
                    //       may not be there anymore)
                    parent_row->getCell(name)->setValue(attachment_ipath.get_key());

                    path_info_t::status_t status(attachment_ipath.get_status());
                    if(status.get_state() == path_info_t::status_t::state_t::DELETED
                    || status.get_state() == path_info_t::status_t::state_t::MOVED)
                    {
                        // restore to a NORMAL page
                        //
                        // TODO: we may need to force a new branch so the user
                        //       would not see the old revisions (unless he
                        //       is an administrator)
                        //
                        SNAP_LOG_WARNING("Re-instating (i.e. \"Undeleting\") page \"")(attachment_ipath.get_key())("\" as we received a create_attachment() request on a deleted page.");
                        status.reset_state(path_info_t::status_t::state_t::NORMAL);
                        attachment_ipath.set_status(status);
                    }

                    modified_content(attachment_ipath);

                    // TBD -- should it be true here to let the other plugins
                    //        do their own work?
                    return false;
                }

                // not the same file, we've got to remove the reference
                // from the existing file since it's going to be moved
                // to a new file (i.e. the current md5 points to a
                // different file)
                //
                // TODO: nothing should just be dropped in our system,
                //       instead it should be moved to some form of
                //       trashcan; in this case we'd use a new name
                //       for the reference although if the whole row
                //       is to be "dropped" (see below) then we should
                //       not even have to drop this cell at all because
                //       it will remain there, only under a different
                //       name...
                files_table->getRow(existing_ref.binaryValue())->dropCell(attachment_ipath.get_cpath());
            }
        }

        // it is not there yet, so go on...
        //
        // TODO: we want to check all the attachments and see if any
        //       one of them is the same file (i.e. user uploading the
        //       same file twice with two different file names...)

        files_table->getRow(md5)->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_UPDATED))->setValue(f_snap->get_start_date());
    }

    // yes that path may already exists, no worries since the create_content()
    // function checks that and returns quickly if it does exist
    create_content(attachment_ipath, file.get_attachment_owner(), file.get_attachment_type());

    // if it is already filename it won't hurt too much to set it again
    parent_row->getCell(name)->setValue(attachment_ipath.get_key());

    // mark all attachments as final (i.e. cannot create children below an attachment)
    signed char const final_page(1);
    content_attachment_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FINAL))->setValue(final_page);

    // in this case 'post' represents the filename as sent by the
    // user, the binary data is in the corresponding file
    revision_attachment_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT_FILENAME))->setValue(attachment_filename);

    // save the file reference
    revision_attachment_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT))->setValue(md5);

    // save the MIME type (this is the one returned by the magic library)
    revision_attachment_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT_MIME_TYPE))->setValue(post_file.get_mime_type());

    // the date when it was created
    int64_t const start_date(f_snap->get_start_date());
    revision_attachment_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);

    // XXX we could also save the modification and creation dates, but the
    //     likelihood that these exist is so small that I'll skip at this
    //     time; we do save them in the files table

    // TODO: create an event for this last part because it requires JavaScript
    //       or CSS support which is not part of the base content plugin.

    // some files are generated by backends such as the feed plugin and
    // the xmlsitemap plugin; those files have no value once replaced;
    // therefore here we have a way to remove older revisions and the
    // corresponding file to make sure we do not just fill up the database
    // with totally useless data (i.e. data that would never be reused
    // later.)
    //
    // TODO: the xmlsitemap plugin needs to create all the new sitemap###.xml
    //       files and then switch the branch & reivision to that new set of
    //       files; this is not yet available in this function; any other
    //       plugin that creates a group of files would have to do the same
    //       thing (i.e. create all the files, then change the current
    //       revision to the new set); for such, the revisions to be
    //       destroyed need to be at least +2 from the new revision (i.e.
    //       we have to keep the current revision until the new one is
    //       fully available); also, the '###' of the sitemap should
    //       include the revision number so that way a system can continue
    //       to load the previous revision (i.e. use the ?revision=123 on
    //       the URL of sitemap###.xml)
    //
    if(remove_old_revisions)
    {
        // we have to remove some revisions only if the number of
        // revisions is limited
        int64_t const revision_limits(file.get_revision_limit());
        if(revision_limits > 0)
        {
            // save the revision limits so a backend could remove old revisions
            // automatically if we find some remnants...
            //
            // TODO: write said backend which should run about once a month
            //
            content_attachment_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_REVISION_LIMITS))->setValue(revision_limits);

            snap_version::version_number_t const revision_number(attachment_ipath.get_revision());
            if(revision_number >= static_cast<snap_version::version_number_t>(revision_limits))
            {
                int64_t const current_revision(get_current_revision(attachment_ipath.get_key(), branch_number, locale, false));
                int64_t const working_revision(get_current_revision(attachment_ipath.get_key(), branch_number, locale, true));

                // we have a +1 because we first do -1 in the while() loop
                snap_version::version_number_t rev(revision_number - static_cast<snap_version::version_number_t>(revision_limits) + 1);
                while(rev > 0)
                {
                    --rev;

                    // we cannot delete the current or working revisions,
                    // these are considered locked by the page
                    //
                    if(rev == current_revision
                    || rev == working_revision)
                    {
                        continue;
                    }

                    // calculate the revision key
                    QString const revision_key(generate_revision_key(
                                attachment_ipath.get_key(),
                                branch_number,
                                rev,
                                locale));

                    // check whether that revision exists, if not, then we
                    // assume we are done (if there is a gap in the list
                    // of revision, we will miss deleting older ones...
                    // for that we may want to have a backend that captures
                    // such problems but here we try to be relatively fast.)
                    //
                    // TODO: there is a "bug" in Cassandra and when I check
                    //       whether a row exists, we often get true if the
                    //       row was deleted "recently" (I'm not too sure
                    //       how recently, though); here we assume that
                    //       the revision deletion does not happen that
                    //       often and thus the following returns false
                    //
                    if(!revision_table->exists(revision_key))
                    {
                        break;
                    }

//SNAP_LOG_TRACE("*** DELETE OLD REVISION WITH KEY: ")(revision_key);

                    // okay, it looks like that revision still exists so
                    // get rid of it
                    //
                    destroy_revision(revision_key);
                }
            }
        }
    }

    return true;
}


/** \brief Check whether a page is marked as final.
 *
 * A page is marked final with the field named "content::final" set to 1.
 * Attachments are always marked final because you cannot create a sub-page
 * under an attachment.
 *
 * \param[in] key  The full key to the page in the content table.
 *
 * \return true if the page at 'key' is marked as being final.
 */
bool content::is_final(QString const& key)
{
    libdbproxy::table::pointer_t content_table(get_content_table());
    if(content_table->exists(key))
    {
        libdbproxy::row::pointer_t parent_row(content_table->getRow(key));
        if(parent_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FINAL)))
        {
            libdbproxy::value final_value(parent_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FINAL))->getValue());
            if(!final_value.nullValue())
            {
                if(final_value.signedCharValue())
                {
                    // it is final...
                    return true;
                }
            }
        }
    }

    return false;
}


/** \brief Load an attachment previously saved with create_attachment().
 *
 * The function checks that the attachment exists and is in good condition
 * and if so, loads it in the specified file parameter.
 *
 * \param[in] key  The key to the attachment to load.
 * \param[in,out] file  The file will be loaded in this structure.
 * \param[in] load_data  Whether the data should be loaded (true by default.)
 *
 * \return true if the attachment was successfully loaded.
 */
bool content::load_attachment(QString const & key, attachment_file & file, bool load_data)
{
    path_info_t ipath;
    ipath.set_path(key);

    // for CSS and JS files, the filename includes the version and .min.
    // which is not in the standard path, we have to remove those here
    snap_string_list segments(ipath.get_segments());
    if(segments.size() >= 3
    && (segments[0] == "css" || segments[0] == "js"))
    {
        snap_string_list const name(segments.last().split("_"));
        // TODO: later we may have a browser name in the filename?
        if(name.size() == 2)
        {
            segments.pop_back();
            segments << (name[0] + "." + segments[0]);
            ipath.set_path(segments.join("/"));
        }
    }

    libdbproxy::table::pointer_t content_table(get_content_table());
    if(!content_table->exists(ipath.get_key()))
    {
        // the row does not even exist yet...
        return false;
    }

    // TODO: select the WORKING_VERSION if the user is logged in and can
    //       edit this attachment
    //
    libdbproxy::table::pointer_t revision_table(get_revision_table());
    libdbproxy::row::pointer_t revision_attachment_row(revision_table->getRow(ipath.get_revision_key()));
    libdbproxy::value md5_value(revision_attachment_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT))->getValue());

    libdbproxy::table::pointer_t files_table(get_files_table());
    if(!files_table->exists(md5_value.binaryValue()))
    {
        // file not available?!
        return false;
    }
    libdbproxy::row::pointer_t file_row(files_table->getRow(md5_value.binaryValue()));

    if(!file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_DATA)))
    {
        // no data available
        return false;
    }

    file.set_attachment_cpath(ipath.get_cpath());
    path_info_t parent;
    ipath.get_parent(parent);
    file.set_parent_cpath(parent.get_cpath());

    if(load_data)
    {
        file.set_file_data(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_DATA))->getValue().binaryValue());

        // TODO if compressed, we may have (want) to decompress here?
    }
    else
    {
        // since we are not loading the data, we want to get some additional
        // information on the side: the verified MIME type and the file size
        if(file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_MIME_TYPE)))
        {
            // This one gets set automatically when we set the data so we only
            // load it if the data is not getting loaded
            file.set_file_mime_type(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_MIME_TYPE))->getValue().stringValue());
        }
        if(file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_SIZE)))
        {
            // since we're not loading the data, we get the size parameter
            // like this (later we may want to always do that once we save
            // files compressed in the database!)
            file.set_file_size(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SIZE))->getValue().int32Value());
        }
    }

    if(file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_FILENAME)))
    {
        file.set_file_filename(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_FILENAME))->getValue().stringValue());
    }
    if(file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE)))
    {
        file.set_file_original_mime_type(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE))->getValue().stringValue());
    }
    if(file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_CREATION_TIME)))
    {
        file.set_file_creation_time(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_CREATION_TIME))->getValue().int64Value());
    }
    if(file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME)))
    {
        file.set_file_creation_time(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME))->getValue().int64Value());
    }
    if(file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_CREATED)))
    {
        file.set_creation_time(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_CREATED))->getValue().int64Value());
    }
    if(file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_UPDATED)))
    {
        file.set_update_time(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_UPDATED))->getValue().int64Value());
    }
    if(file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH)))
    {
        file.set_file_image_width(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH))->getValue().int32Value());
    }
    if(file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT)))
    {
        file.set_file_image_height(file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT))->getValue().int32Value());
    }

    return true;
}


/** \brief Tell the system that data was updated.
 *
 * This signal should be called any time you modify something in a page.
 *
 * This very function takes care of updating the content::modified and
 * content:updated as required:
 *
 * \li content::modified -- if anything changes in a page, this date
 *                          is changed; in other words, any time this
 *                          function is called, this date is set to
 *                          the current date
 *
 * \li content::updated -- if the content gets updated then this date
 *                         is expected to change; "content" here means
 *                         the title, body, or "any" important content
 *                         that is shown to the user (i.e. a small
 *                         change in a field that is not displayed or
 *                         is not directly considered content as part of
 *                         the main body of the page should not change
 *                         this date)
 *
 * This signal also gives other modules a chance to update their own
 * data (i.e. the sitemap.xml needs to update this page information.)
 *
 * Since the other plugins may make use of your plugin changes, you have
 * to call this signal last.
 *
 * \note
 * The function returns false and generates a warning (in your log) in the
 * event the process cannot find the specified path.
 *
 * \param[in,out] ipath  The path to the page being udpated.
 *
 * \return true if the event should be propagated.
 */
bool content::modified_content_impl(path_info_t & ipath)
{
    if(f_snap->is_ready())
    {
        return false;
    }

    int64_t const start_date(f_snap->get_start_date());

    {
        // although we could use the CREATED of the last revision, we would need
        // special handling to know which one that is (because the last is not
        // always the current revision); also if data in the branch changes,
        // we get here too and that would have nothing to do with the last revision
        //
        libdbproxy::table::pointer_t content_table(get_content_table());
        QString const key(ipath.get_key());
        if(!content_table->exists(key))
        {
            // the row does not exist?!
            SNAP_LOG_WARNING("Page \"")(key)("\" does not exist. We cannot do anything about it being modified.");
            return false;
        }
        libdbproxy::row::pointer_t row(content_table->getRow(key));
        row->getCell(QString(get_name(name_t::SNAP_NAME_CONTENT_MODIFIED)))->setValue(start_date);
    }

    {
        libdbproxy::table::pointer_t branch_table(get_branch_table());
        QString const branch_key(ipath.get_branch_key());
        if(!branch_table->exists(branch_key))
        {
            // the row does not exist?!
            SNAP_LOG_WARNING("Page \"")(branch_key)("\" does not exist. We cannot do anything about it being modified.");
            return false;
        }
        libdbproxy::row::pointer_t row(branch_table->getRow(branch_key));
        row->getCell(QString(get_name(name_t::SNAP_NAME_CONTENT_MODIFIED)))->setValue(start_date);
    }

    return true;
}


/** \brief Retreive a content page parameter.
 *
 * This function reads a column from the content of the page using the
 * content key as defined by the canonicalization process. The function
 * cannot be called before the content::on_path_execute() function is
 * called and the key properly initialized.
 *
 * The table is opened once and remains opened so calling this function
 * many times is not a problem. Also the libQtCassandra library caches
 * all the data. Reading the same field multiple times is not a concern
 * at all.
 *
 * If the value is undefined, the result is a null value.
 *
 * \note
 * The path should be canonicalized before the call although we call
 * the remove_slashes() function on it cleanup starting and ending
 * slashes (because the URI object returns paths such as "/login" and
 * the get_content_parameter() requires just "login" to work right.)
 *
 * \param[in,out] ipath  The canonicalized path being managed.
 * \param[in] param_name  The name of the parameter to retrieve.
 * \param[in] revision  Which row we are to access for the required parameter.
 *
 * \return The content of the row as a Cassandra value.
 */
libdbproxy::value content::get_content_parameter(path_info_t & ipath, QString const & param_name, param_revision_t revision)
{
    switch(revision)
    {
    case param_revision_t::PARAM_REVISION_GLOBAL:
        {
            libdbproxy::table::pointer_t content_table(get_content_table());

            if(!content_table->exists(ipath.get_key())
            || !content_table->getRow(ipath.get_key())->exists(param_name))
            {
                // an empty value is considered to be a null value
                libdbproxy::value value;
                return value;
            }

            return content_table->getRow(ipath.get_key())->getCell(param_name)->getValue();
        }

    case param_revision_t::PARAM_REVISION_BRANCH:
        {
            libdbproxy::table::pointer_t branch_table(get_branch_table());

            if(!branch_table->exists(ipath.get_branch_key())
            || !branch_table->getRow(ipath.get_branch_key())->exists(param_name))
            {
                // an empty value is considered to be a null value
                libdbproxy::value value;
                return value;
            }

            return branch_table->getRow(ipath.get_branch_key())->getCell(param_name)->getValue();
        }

    case param_revision_t::PARAM_REVISION_REVISION:
        {
            libdbproxy::table::pointer_t revision_table(get_revision_table());

            if(!revision_table->exists(ipath.get_revision_key())
            || !revision_table->getRow(ipath.get_revision_key())->exists(param_name))
            {
                // an empty value is considered to be a null value
                libdbproxy::value value;
                return value;
            }

            return revision_table->getRow(ipath.get_revision_key())->getCell(param_name)->getValue();
        }

    default:
        throw snap_logic_exception("invalid PARAM_REVISION_... parameter to get_content_parameter().");

    }
    NOTREACHED();
}

/** \brief Prepare a set of content to add to the database.
 *
 * In most cases, plugins call this function in one of their do_update()
 * functions to add their content.xml file to the database.
 *
 * This function expects a plugin name as input to add the
 * corresponding content.xml file of that plugin. The data is search in
 * the resources (it is expected to be added there by the plugin).
 * The resource path is built as follow:
 *
 * \code
 * ":/plugins/" + plugin_name + "/content.xml"
 * \endcode
 *
 * The content is not immediately added to the database because
 * of dependency issues. At the time all the content is added
 * using this function, the order in which it is added is not
 * generally proper (i.e. the taxonomy "/types" may be
 * added after the content "/types/taxonomy/system/content-types"
 * which would then fail.)
 *
 * The content plugin saves this data when it receives the
 * save_content signal.
 *
 * To dynamically add content (opposed to adding information
 * from an XML file) you want to call the add_param() and
 * add_link() functions as required.
 *
 * \param[in] plugin_name  The name of the plugin loading this data.
 *
 * \sa on_save_content()
 * \sa add_param()
 * \sa add_link()
 */
void content::add_xml(QString const & plugin_name)
{
    if(!plugins::verify_plugin_name(plugin_name))
    {
        // invalid plugin name
        throw content_exception_invalid_content_xml("add_xml() called with an invalid plugin name: \"" + plugin_name + "\"");
    }
    QString const filename(":/plugins/" + plugin_name + "/content.xml");
    QFile xml_content(filename);
    if(!xml_content.open(QFile::ReadOnly))
    {
        // file not found
        throw content_exception_invalid_content_xml("add_xml() cannot open file: \"" + filename + "\"");
    }
    QDomDocument dom;
    if(!dom.setContent(&xml_content, false))
    {
        // invalid XML
        throw content_exception_invalid_content_xml("add_xml() cannot read the XML of content file: \"" + filename + "\"");
    }
    add_xml_document(dom, plugin_name);
}


/** \brief Add data to the database using a DOM.
 *
 * This function is called by the add_xml() function after a DOM was loaded.
 * It can be called by other functions which load content XML data from
 * a place other than the resources.
 *
 * \note
 * As an example, the layout plugin will call this function if it finds
 * a content.xml file in its list of files.
 *
 * \param[in] dom  The DOM to add to the content system.
 * \param[in] plugin_name  The name of the plugin loading this data.
 */
void content::add_xml_document(QDomDocument & dom, QString const & plugin_name)
{
    QDomNodeList content_nodes(dom.elementsByTagName(get_name(name_t::SNAP_NAME_CONTENT_TAG)));
    int const max_nodes(content_nodes.size());
    for(int i(0); i < max_nodes; ++i)
    {
        QDomNode content_node(content_nodes.at(i));
        if(!content_node.isElement())
        {
            // we are only interested in elements
            continue;
        }
        QDomElement content_element(content_node.toElement());
        if(content_element.isNull())
        {
            // somehow this is not an element
            continue;
        }

        // <content path="..." moved-from="..." owner="...">...</content>

        QString owner(content_element.attribute("owner"));
        if(owner.isEmpty())
        {
            owner = plugin_name;
        }

        QString path(content_element.attribute("path"));
        if(path.isEmpty())
        {
            throw content_exception_invalid_content_xml("all <content> tags supplied to add_xml_document() must include a valid \"path\" attribute");
        }
        f_snap->canonicalize_path(path);
        QString const key(f_snap->get_site_key_with_slash() + path);

        // in case the page was moved...
        QString moved_from(content_element.attribute("moved-from"));
        f_snap->canonicalize_path(moved_from);

        // create a new entry for the database
        add_content(key, moved_from, owner);

        QDomNodeList children(content_element.childNodes());
        bool found_content_type(false);
        bool found_prevent_delete(false);
        int const cmax(children.size());
        for(int c(0); c < cmax; ++c)
        {
            // grab <param> and <link> tags
            QDomNode child(children.at(c));
            if(!child.isElement())
            {
                // we are only interested by elements
                continue;
            }
            QDomElement element(child.toElement());
            if(element.isNull())
            {
                // somehow this is not really an element?!
                continue;
            }

            // <param name=... overwrite=... force-namespace=...> data </param>
            QString const tag_name(element.tagName());
            bool const remove_param(tag_name == "remove-param");
            if(tag_name == "param"
            || remove_param)
            {
                QString const param_name(element.attribute("name"));
                if(param_name.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <param> tags supplied to add_xml() must include a valid \"name\" attribute");
                }

                // 1) prepare the buffer
                // the parameter value can include HTML (should be in a [CDATA[...]] in that case)
                QString buffer;
                QTextStream data(&buffer);
                // we have to save all the element children because
                // saving the element itself would save the <param ...> tag
                // also if the whole is a <![CDATA[...]]> entry, remove it
                // (but keep sub-<![CDATA[...]]> if any.)
                QDomNodeList values(element.childNodes());
                int lmax(values.size());
                if(lmax == 1)
                {
                    QDomNode n(values.at(0));
                    if(n.isCDATASection())
                    {
                        QDomCDATASection raw_data(n.toCDATASection());
                        data << raw_data.data();
                    }
                    else
                    {
                        // not a CDATA section, save as is
                        n.save(data, 0);
                    }
                }
                else
                {
                    // save all the children
                    for(int l(0); l < lmax; ++l)
                    {
                        values.at(l).save(data, 0);
                    }
                }

                // 2) prepare the name
                QString fullname;
                // It seems to me that if the developer included any namespace
                // then it was meant to be defined that way
                //if(param_name.left(plugin_name.length() + 2) == plugin_name + "::")
                if(param_name.contains("::")) // already includes a namespace
                {
                    // plugin namespace already defined
                    fullname = param_name;
                }
                else
                {
                    // plugin namespace not defined
                    if(element.attribute("force-namespace") == "no")
                    {
                        // but developer said no namespace needed (?!)
                        fullname = param_name;
                    }
                    else
                    {
                        // this is the default!
                        fullname = plugin_name + "::" + param_name;
                    }
                }

                if(fullname == get_name(name_t::SNAP_NAME_CONTENT_PREVENT_DELETE))
                {
                    found_prevent_delete = true;
                }

                param_revision_t revision_type(param_revision_t::PARAM_REVISION_BRANCH);
                QString const revision_name(element.attribute("revision", "branch"));
                if(revision_name == "global")
                {
                    revision_type = param_revision_t::PARAM_REVISION_GLOBAL;
                }
                else if(revision_name == "revision")
                {
                    revision_type = param_revision_t::PARAM_REVISION_REVISION;
                }
                else if(revision_name != "branch")
                {
                    throw content_exception_invalid_content_xml(QString("<param> tag used an invalid \"revision\" attribute (%1); we expected \"global\", \"branch\", or \"revision\".").arg(revision_name));
                }

                QString locale(element.attribute("lang", "en"));
                QString country;
                f_snap->verify_locale(locale, country, true);
                if(!country.isEmpty())
                {
                    // stick the country back in the locale if defined
                    // (but this way it is canonicalized)
                    //
                    locale += '_';
                    locale += country;
                }

                QString const priority_str(element.attribute("priority", "0"));
                bool ok(false);
                param_priority_t const priority(priority_str.toLongLong(&ok, 10));
                if(!ok)
                {
                    throw content_exception_invalid_content_xml(QString("<param> attribute \"priority\" is not a valid number \"%1\".").arg(priority_str));
                }

                // add the resulting parameter
                add_param(key, fullname, revision_type, locale, buffer, priority, remove_param);

                // if we are to remove that parameter, we do not need the
                // overwrite and type info
                //
                if(!remove_param)
                {
                    // check whether we allow overwrites
                    //
                    if(element.attribute("overwrite") == "yes")
                    {
                        set_param_overwrite(key, fullname, true);
                    }

                    // check whether a data type was defined
                    //
                    QString const type(element.attribute("type"));
                    if(!type.isEmpty())
                    {
                        param_type_t param_type;
                        if(type == "string")
                        {
                            param_type = param_type_t::PARAM_TYPE_STRING;
                        }
                        else if(type == "float"
                             || type == "float32")
                        {
                            param_type = param_type_t::PARAM_TYPE_FLOAT32;
                        }
                        else if(type == "double"
                             || type == "float64")
                        {
                            param_type = param_type_t::PARAM_TYPE_FLOAT64;
                        }
                        else if(type == "int8")
                        {
                            param_type = param_type_t::PARAM_TYPE_INT8;
                        }
                        else if(type == "int32")
                        {
                            param_type = param_type_t::PARAM_TYPE_INT32;
                        }
                        else if(type == "int64")
                        {
                            param_type = param_type_t::PARAM_TYPE_INT64;
                        }
                        else
                        {
                            throw content_exception_invalid_content_xml(QString("unknown type in <param type=\"%1\"> tags").arg(type));
                        }
                        set_param_type(key, fullname, param_type);
                    }
                }
            }
            // <link name=... to=... [mode="1/*:1/*"]> destination path </link>
            else if(tag_name == "link")
            {
                QString link_name(element.attribute("name"));
                if(link_name.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <link> tags supplied to add_xml() must include a valid \"name\" attribute");
                }
                if(link_name == plugin_name)
                {
                    throw content_exception_invalid_content_xml(QString("the \"name\" attribute of a <link> tag cannot be set to the plugin name (%1)").arg(plugin_name));
                }
                if(!link_name.contains("::"))
                {
                    // force the owner in the link name
                    link_name = QString("%1::%2").arg(plugin_name).arg(link_name);
                }
                if(link_name == get_name(name_t::SNAP_NAME_CONTENT_PAGE_TYPE))
                {
                    found_content_type = true;
                }
                QString link_to(element.attribute("to"));
                if(link_to.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <link> tags supplied to add_xml() must include a valid \"to\" attribute");
                }
                if(link_to == plugin_name)
                {
                    throw content_exception_invalid_content_xml(QString("the \"to\" attribute of a <link> tag cannot be set to the plugin name (%1)").arg(plugin_name));
                }
                if(!link_to.contains("::"))
                {
                    // force the owner in the link name
                    link_to = QString("%1::%2").arg(plugin_name).arg(link_to);
                }
                bool source_unique(true);
                bool destination_unique(true);
                QString mode(element.attribute("mode"));
                if(!mode.isEmpty() && mode != "1:1")
                {
                    if(mode == "1:*")
                    {
                        destination_unique = false;
                    }
                    else if(mode == "*:1")
                    {
                        source_unique = false;
                    }
                    else if(mode == "*:*")
                    {
                        destination_unique = false;
                        source_unique = false;
                    }
                    else
                    {
                        throw content_exception_invalid_content_xml("<link> tags mode attribute must be one of \"1:1\", \"1:*\", \"*:1\", or \"*:*\"");
                    }
                }
                snap_version::version_number_t branch_source(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
                snap_version::version_number_t branch_destination(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
                QString branches(element.attribute("branches"));
                if(!branches.isEmpty() && branches != "0:0")
                {
                    if(branches == "*" || branches == "*:*")
                    {
                        branch_source = snap_version::SPECIAL_VERSION_ALL;
                        branch_destination = snap_version::SPECIAL_VERSION_ALL;
                    }
                    else
                    {
                        snap_string_list b(branches.split(':'));
                        if(b.size() != 2)
                        {
                            throw content_exception_invalid_content_xml("<remove-link> tags 'branches' attribute must be one of \"*\", \"*:*\", \"#:#\", where # represents a number or is \"system\" (default is 0:0)");
                        }
                        if(b[0] == "system")
                        {
                            branch_source = snap_version::SPECIAL_VERSION_SYSTEM_BRANCH;
                        }
                        else
                        {
                            bool ok(false);
                            branch_source = static_cast<snap_version::basic_version_number_t>(b[0].toInt(&ok, 10));
                            if(!ok)
                            {
                                throw content_exception_invalid_content_xml("<remove-link> tags 'branches' attribute must be one of \"*\", \"*:*\", \"#:#\", where # represents a number or is \"system\" (default is 0:0), invalid number before ':'.");
                            }
                        }
                        if(b[1] == "system")
                        {
                            branch_destination = snap_version::SPECIAL_VERSION_SYSTEM_BRANCH;
                        }
                        else
                        {
                            bool ok(false);
                            branch_destination = static_cast<snap_version::basic_version_number_t>(b[1].toInt(&ok, 10));
                            if(!ok)
                            {
                                throw content_exception_invalid_content_xml("<remove-link> tags 'branches' attribute must be one of \"*\", \"*:*\", \"#:#\", where # represents a number or is \"system\" (default is 0:0), invalid number after ':'.");
                            }
                        }
                    }
                }
                // the destination URL is defined in the <link> content
                QString destination_path(element.text());
                f_snap->canonicalize_path(destination_path);
                QString const destination_key(f_snap->get_site_key_with_slash() + destination_path);
                links::link_info source(link_name, source_unique, key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
                links::link_info destination(link_to, destination_unique, destination_key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
                add_link(key, source, destination, branch_source, branch_destination, false);
            }
            // <remove-link name=... to=... [mode="1/*:1/*"]> destination path </link>
            else if(tag_name == "remove-link")
            {
                // just like a link, only we will end up removing that link
                // instead of adding it
                QString link_name(element.attribute("name"));
                if(link_name.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <remove-link> tags supplied to add_xml() must include a valid \"name\" attribute");
                }
                if(link_name == plugin_name)
                {
                    throw content_exception_invalid_content_xml(QString("the \"name\" attribute of a <remove-link> tag cannot be set to the plugin name (%1)").arg(plugin_name));
                }
                if(!link_name.contains("::"))
                {
                    // force the owner in the link name
                    link_name = QString("%1::%2").arg(plugin_name).arg(link_name);
                }
                if(link_name == get_name(name_t::SNAP_NAME_CONTENT_PAGE_TYPE))
                {
                    found_content_type = true;
                }
                QString link_to(element.attribute("to"));
                if(link_to.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <remove-link> tags supplied to add_xml() must include a valid \"to\" attribute");
                }
                if(link_to == plugin_name)
                {
                    throw content_exception_invalid_content_xml(QString("the \"to\" attribute of a <remove-link> tag cannot be set to the plugin name (%1)").arg(plugin_name));
                }
                if(!link_to.contains("::"))
                {
                    // force the owner in the link name
                    link_to = QString("%1::%2").arg(plugin_name).arg(link_to);
                }
                bool source_unique(true);
                bool destination_unique(true);
                QString mode(element.attribute("mode"));
                if(!mode.isEmpty() && mode != "1:1")
                {
                    if(mode == "1:*")
                    {
                        destination_unique = false;
                    }
                    else if(mode == "*:1")
                    {
                        source_unique = false;
                    }
                    else if(mode == "*:*")
                    {
                        destination_unique = false;
                        source_unique = false;
                    }
                    else
                    {
                        throw content_exception_invalid_content_xml("<remove-link> tags mode attribute must be one of \"1:1\", \"1:*\", \"*:1\", or \"*:*\"");
                    }
                }
                snap_version::version_number_t branch_source(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
                snap_version::version_number_t branch_destination(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
                QString branches(element.attribute("branches"));
                if(!branches.isEmpty() && branches != "0:0")
                {
                    if(branches == "*" || branches == "*:*")
                    {
                        branch_source = snap_version::SPECIAL_VERSION_ALL;
                        branch_destination = snap_version::SPECIAL_VERSION_ALL;
                    }
                    else
                    {
                        snap_string_list b(branches.split(':'));
                        if(b.size() != 2)
                        {
                            throw content_exception_invalid_content_xml("<remove-link> tags 'branches' attribute must be one of \"*\", \"*:*\", \"#:#\", where # represents a number or is \"system\" (default is 0:0)");
                        }
                        if(b[0] == "system")
                        {
                            branch_source = snap_version::SPECIAL_VERSION_SYSTEM_BRANCH;
                        }
                        else
                        {
                            bool ok(false);
                            branch_source = static_cast<snap_version::basic_version_number_t>(b[0].toInt(&ok, 10));
                            if(!ok)
                            {
                                throw content_exception_invalid_content_xml("<remove-link> tags 'branches' attribute must be one of \"*\", \"*:*\", \"#:#\", where # represents a number or is \"system\" (default is 0:0), invalid number before ':'.");
                            }
                        }
                        if(b[1] == "system")
                        {
                            branch_destination = snap_version::SPECIAL_VERSION_SYSTEM_BRANCH;
                        }
                        else
                        {
                            bool ok(false);
                            branch_destination = static_cast<snap_version::basic_version_number_t>(b[1].toInt(&ok, 10));
                            if(!ok)
                            {
                                throw content_exception_invalid_content_xml("<remove-link> tags 'branches' attribute must be one of \"*\", \"*:*\", \"#:#\", where # represents a number or is \"system\" (default is 0:0), invalid number after ':'.");
                            }
                        }
                    }
                }
                // the destination URL is defined in the <link> content
                QString destination_path(element.text());
                f_snap->canonicalize_path(destination_path);
                QString const destination_key(f_snap->get_site_key_with_slash() + destination_path);
                links::link_info source(link_name, source_unique, key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
                links::link_info destination(link_to, destination_unique, destination_key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
                add_link(key, source, destination, branch_source, branch_destination, true);
            }
            // <attachment name=... type=... [owner=...]> resource path to file </link>
            else if(tag_name == "attachment")
            {
                content_attachment ca;

                // the owner is optional, it defaults to "content"
                // TODO: verify that "content" is correct, and that we should
                //       not instead use the plugin name (owner of this page)
                ca.f_owner = element.attribute("owner");
                if(ca.f_owner.isEmpty())
                {
                    // the output plugin is the default owner
                    ca.f_owner = get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT_PLUGIN);
                }
                ca.f_field_name = element.attribute("name");
                if(ca.f_field_name.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <attachment> tags supplied to add_xml() must include a valid \"name\" attribute");
                }
                ca.f_type = element.attribute("type");
                if(ca.f_type.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <attachment> tags supplied to add_xml() must include a valid \"type\" attribute");
                }

                // XXX Should we prevent filenames that do not represent
                //     a resource? If not a resource, changes that it is not
                //     accessible to the server are high unless the file was
                //     installed in a shared location (/usr/share/snapwebsites/...)
                QDomElement path_element(child.firstChildElement("path"));
                if(path_element.isNull())
                {
                    throw content_exception_invalid_content_xml("all <attachment> tags supplied to add_xml() must include a valid <paht> child tag");
                }
                ca.f_filename = path_element.text();

                QDomElement mime_type_element(child.firstChildElement("mime-type"));
                if(!mime_type_element.isNull())
                {
                    ca.f_mime_type = mime_type_element.text();
                }

                // there can be any number of dependencies
                // syntax is defined in the JavaScript plugin, something
                // like Debian "Depend" field:
                //
                //   <name> ( '(' (<version> <operator>)* <version> ')' )?
                //
                QDomElement dependency_element(child.firstChildElement("dependency"));
                while(!dependency_element.isNull())
                {
                    ca.f_dependencies.push_back(dependency_element.text());
                    dependency_element = dependency_element.nextSiblingElement("dependency");
                }

                ca.f_path = path;

                add_attachment(key, ca);
            }
        }
        if(!found_content_type)
        {
            QString const link_name(get_name(name_t::SNAP_NAME_CONTENT_PAGE_TYPE));
            QString const link_to(get_name(name_t::SNAP_NAME_CONTENT_PAGE));
            bool const source_unique(true);
            bool const destination_unique(false);
            QString destination_path;
            if(path.startsWith("layouts/"))
            {
                // make sure that this is the root of that layout and
                // not an attachment or sub-page
                //
                QString const base(path.mid(8));
                int const pos(base.indexOf('/'));
                if(pos < 0)
                {
                    destination_path = "types/taxonomy/system/content-types/layout-page";
                }
            }
            if(destination_path.isEmpty())
            {
                if(path.startsWith("admin/"))
                {
                    destination_path = "types/taxonomy/system/content-types/administration-page";
                }
                else
                {
                    destination_path = "types/taxonomy/system/content-types/system-page";
                }
            }
            QString const destination_key(f_snap->get_site_key_with_slash() + destination_path);
            links::link_info source(link_name, source_unique, key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
            links::link_info destination(link_to, destination_unique, destination_key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
            add_link(key, source, destination, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, false);
        }
        if(!found_prevent_delete)
        {
            // add the "content::prevent_delete" to 1 on all that do not
            // set it to another value (1 byte value)
            //
            // TBD: should the priority be something else than PARAM_DEFAULT_PRIORITY (0)?
            //
            add_param(key, get_name(name_t::SNAP_NAME_CONTENT_PREVENT_DELETE), param_revision_t::PARAM_REVISION_GLOBAL, "en", "1", PARAM_DEFAULT_PRIORITY, false);
            set_param_overwrite(key, get_name(name_t::SNAP_NAME_CONTENT_PREVENT_DELETE), true); // always overwrite
            set_param_type(key, get_name(name_t::SNAP_NAME_CONTENT_PREVENT_DELETE), param_type_t::PARAM_TYPE_INT8);
        }
    }
}


/** \brief Prepare to add content to the database.
 *
 * This function creates a new block of data to be added to the database.
 * Each time one wants to add content to the database, one must call
 * this function first. At this time the plugin_owner cannot be changed.
 * If that happens (i.e. two plugins trying to create the same piece of
 * content) then the system raises an exception.
 *
 * The \p moved_from_path can be used if you move your data from one
 * location to another. This will force the creation of a redirect
 * on the former page (the page pointed by the moved_from_path). However,
 * it will not copy anything from the former page. In most cases, this is
 * used to redirect users from your old settings to the new settings
 * because you renamed the page to better fit your plugin.
 *
 * \exception content_exception_content_already_defined
 * This exception is raised if the block already exists and the owner
 * of the existing block doesn't match the \p plugin_owner parameter.
 *
 * \param[in] path  The path of the content being added.
 * \param[in] moved_from_path  The path where the content was before.
 * \param[in] plugin_owner  The name of the plugin managing this content.
 */
void content::add_content(QString const & path, QString const & moved_from_path, QString const & plugin_owner)
{
    if(!plugins::verify_plugin_name(plugin_owner))
    {
        // invalid plugin name
        throw content_exception_invalid_name("install_content() called with an invalid plugin name: \"" + plugin_owner + "\"");
    }

    content_block_map_t::iterator b(f_blocks.find(path));
    if(b != f_blocks.end())
    {
        if(b->f_owner != plugin_owner)
        {
            // cannot change owner!?
            throw content_exception_content_already_defined("adding block \"" + path + "\" with owner \"" + b->f_owner + "\" cannot be changed to \"" + plugin_owner + "\"");
        }

        // it already exists, we are all good

        // TBD: should we yell if the paths both exist and
        //      are not equal?
        if(b->f_moved_from.isEmpty())
        {
            b->f_moved_from = moved_from_path;
        }
    }
    else
    {
        // create the new block
        content_block_t block;
        block.f_path = path;
        block.f_owner = plugin_owner;
        block.f_moved_from = moved_from_path;
        f_blocks.insert(path, block);
    }

    f_snap->new_content();
}


/** \brief Add a parameter to the content to be saved in the database.
 *
 * This function is used to add a parameter to the database.
 * A parameter is composed of a name and a block of data that may be of any
 * type (HTML, XML, picture, etc.)
 *
 * Other parameters can be attached to parameters using set_param_...()
 * functions, however, the add_param() function must be called first to
 * create the parameter.
 *
 * Note that the data added in this way is NOT saved in the database until
 * the save_content signal is sent.
 *
 * \warning
 * This function does NOT save the data immediately (if called after the
 * update, then it is saved after the execute() call returns!) Instead
 * the function prepares the data so it can be saved later. This is useful
 * if you expect many changes and dependencies may not all be available at
 * the time you add the content but will be at a later time. If you already
 * have all the data, you may otherwise directly call the Cassandra function
 * to add the data to the content table.
 *
 * \bug
 * At this time the data of a parameter is silently overwritten if this
 * function is called multiple times with the same path and name.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised when this funtion is called before the
 * add_content() is called (i.e. the block of data referenced by
 * \p path is not defined yet.)
 *
 * \exception content_exception_unexpected_revision_type
 * This exception is raised if the \p revision_type parameter is not
 * equal to the revision_type that was used to create this page.
 *
 * \param[in] path  The path of this parameter (i.e. /types/taxonomy)
 * \param[in] name  The name of this parameter (i.e. "Website Taxonomy")
 * \param[in] revision_type  The type of revision for this parameter (i.e. global, branch, revision)
 * \param[in] locale  The locale (\<language>_\<country>) for this data.
 * \param[in] data  The data of this parameter.
 * \param[in] priority  The priority for this parameter.
 *
 * \sa add_param()
 * \sa add_link()
 * \sa on_save_content()
 */
void content::add_param(
          QString const & path
        , QString const & name
        , param_revision_t revision_type
        , QString const & locale
        , QString const & data
        , param_priority_t const priority
        , bool remove)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" was found");
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        content_param param;
        param.f_name = name;
        param.f_data[locale] = data;
        param.f_revision_type = revision_type;
        param.f_priority = priority;
        param.f_remove = remove;
        b->f_params.insert(name, param);
    }
    else
    {
        // revision types cannot change between entries
        // (duplicates happen often when you have multiple languages)
        if(p->f_revision_type != revision_type)
        {
            throw content_exception_unexpected_revision_type(QString("the revision type cannot be different between locales; got %1 the first time and now %2")
                        .arg(static_cast<snap_version::basic_version_number_t>(p->f_revision_type))
                        .arg(static_cast<snap_version::basic_version_number_t>(revision_type)));
        }

        if(priority >= p->f_priority)
        {
            // replace the data
            //
            // TBD: should we generate an error because if defined by several
            //      different plugins then we cannot ensure which one is going
            //      to make it to the database! At the same time, we cannot
            //      know whether we are overwriting a default value.
            //
            p->f_data[locale] = data;
            p->f_priority = priority; // in case it is larger, save every time
            p->f_remove = remove;
        }
    }
}


/** \brief Set the overwrite flag to a specific parameter.
 *
 * The parameter must first be added with the add_param() function.
 * By default this is set to false as defined in the DTD of the
 * content XML format. This means if the attribute is not defined
 * then there is no need to call this function.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised if the path or the name parameters do
 * not match any block or parameter in that block.
 *
 * \param[in] path  The path of this parameter.
 * \param[in] name  The name of the parameter to modify.
 * \param[in] overwrite  The new value of the overwrite flag.
 *
 * \sa add_param()
 */
void content::set_param_overwrite(QString const & path, QString const & name, bool overwrite)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" found");
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        throw content_exception_parameter_not_defined("no param with name \"" + path + "\" found in block \"" + path + "\"");
    }

    p->f_overwrite = overwrite;
}


/** \brief Set the type to a specific value.
 *
 * The parameter must first be added with the add_param() function.
 * By default the type of a parameter is "string". However, some
 * parameters are integers and this function can be used to specify
 * such. Note that it is important to understand that if you change
 * the type in the content.xml then when reading the data you'll have
 * to use the correct type.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised if the path or the name parameters do
 * not match any block or parameter in that block.
 *
 * \param[in] path  The path of this parameter.
 * \param[in] name  The name of the parameter to modify.
 * \param[in] param_type  The new type for this parameter.
 *
 * \sa add_param()
 */
void content::set_param_type(QString const& path, QString const& name, param_type_t param_type)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" found");
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        throw content_exception_parameter_not_defined("no param with name \"" + path + "\" found in block \"" + path + "\"");
    }

    p->f_type = param_type;
}


/** \brief Add a link to the specified content.
 *
 * This function links the specified content (defined by path) to the
 * specified destination.
 *
 * The source parameter defines the name of the link, the path (has to
 * be the same as path) and whether the link is unique.
 *
 * The path must already represent a block as defined by the add_content()
 * function call otherwise the function raises an exception.
 *
 * Note that the link is not searched. If it is already defined in
 * the array of links, it will simply be written twice to the
 * database.
 *
 * \warning
 * This function does NOT save the data immediately (if called after the
 * update, then it is saved after the execute() call returns!) Instead
 * the function prepares the data so it can be saved later. This is useful
 * if you expect many changes and dependencies may not all be available at
 * the time you add the content but will be at a later time. If you already
 * have all the data, you may otherwise directly call the
 * links::create_link() function.
 *
 * \exception content_exception_parameter_not_defined
 * The add_content() function was not called prior to this call.
 *
 * \param[in] path  The path where the link is added (source URI, site key + path.)
 * \param[in] source  The link definition of the source.
 * \param[in] destination  The link definition of the destination.
 * \param[in] branch_source  The branch to modify, may be set to SPECIAL_VERSION_ALL.
 * \param[in] branch_destination  The branch to modify, may be set to SPECIAL_VERSION_ALL.
 * \param[in] remove  The link we actually be removed and not added if true.
 *
 * \sa add_content()
 * \sa add_attachment()
 * \sa add_xml()
 * \sa add_param()
 * \sa create_link()
 */
void content::add_link(
        QString const & path,
        links::link_info const & source,
        links::link_info const & destination,
        snap_version::version_number_t branch_source,
        snap_version::version_number_t branch_destination,
        bool const remove)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined(QString("no block with path \"%1\" found").arg(path));
    }

    content_link link;
    link.f_source = source;
    link.f_destination = destination;
    link.f_branch_source = branch_source;
    link.f_branch_destination = branch_destination;
    if(remove)
    {
        b->f_remove_links.push_back(link);
    }
    else
    {
        b->f_links.push_back(link);
    }
}


/** \brief Add an attachment to the list of data to add on initialization.
 *
 * This function is used by the add_xml() function to add an attachment
 * to the database once the content and links were all created.
 *
 * Note that the \p attachment parameter does not include the actual data.
 * That data is to be loaded when the on_save_content() signal is sent.
 * This is important to avoid using a huge amount of memory on setup.
 *
 * \warning
 * To add an attachment from your plugin, make sure to call
 * create_attachment() instead. The add_attachment() is a sub-function of
 * the add_xml() feature. It will work on initialization, it is likely to
 * fail if called from your plugin.
 *
 * \param[in] path  The path (key) to the parent of the attachment.
 * \param[in] ca  The attachment information.
 *
 * \sa add_xml()
 * \sa add_link()
 * \sa add_content()
 * \sa add_param()
 * \sa on_save_content()
 * \sa create_attachment()
 */
void content::add_attachment(QString const& path, content_attachment const& ca)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" found");
    }

    b->f_attachments.push_back(ca);
}


/** \brief Signal received when the system request that we save content.
 *
 * This function is called by the snap_child() after the update if any one of
 * the plugins requested content to be saved to the database (in most cases
 * from their content.xml file, although it could be created dynamically.)
 *
 * It may be called again after the execute() if anything more was saved
 * while processing the page.
 */
void content::on_save_content()
{
    // anything to save?
    if(f_blocks.isEmpty())
    {
        return;
    }

    class restore_flag_t
    {
    public:
        restore_flag_t(bool & flag)
            : f_flag(flag)
        {
            f_flag = true;
        }

        ~restore_flag_t()
        {
            f_flag = false;
        }

    private:
        bool &  f_flag;
    };

    restore_flag_t raii_updating(f_updating);

    QString const primary_owner(get_name(name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER));
    QString const site_key(f_snap->get_site_key_with_slash());

    // lock the entire website (this does not prevent others from accessing
    // the site, however, it prevents them from upgrading the database at the
    // same time... note that this is one lock per website)
    snap_lock lock(QString("%1#updating").arg(site_key));

    libdbproxy::table::pointer_t content_table(get_content_table());
    libdbproxy::table::pointer_t branch_table(get_branch_table());
    libdbproxy::table::pointer_t revision_table(get_revision_table());
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        // now do the actual save
        // connect this entry to the corresponding plugin
        // (unless that field is already defined!)
        path_info_t ipath;
        ipath.set_path(d->f_path);

        // for top level directories, send a trace() in case we are
        // initializing on a remote machine, it may be slow enough to
        // make sense to present such
        QString const cpath(ipath.get_cpath());
        snap_string_list const & segments(ipath.get_segments());

        // TODO: we should now be able to remove that test and show all
        //       the pages being created because we have a backend only
        //       system and thus making things a tad bit slower should
        //       not be a concern; I want to have the time to test that
        //       theory before removing that if(), though
        //
        if(segments.size() < 3)
        {
            f_snap->trace(QString("Saving \"%1\".\n").arg(ipath.get_key()));
        }

        // make sure we have a parent page (pages are sorted in the
        // blocks so a parent always appears first and thus it gets
        // created first, otherwise it is missing)
        //
        if(!cpath.isEmpty())   // ignore the root
        {
            path_info_t parent_ipath;
            ipath.get_parent(parent_ipath);
            if(!content_table->exists(parent_ipath.get_key())
            || !content_table->getRow(parent_ipath.get_key())->exists(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))
            {
                // we do not allow a parent to got missing, the programmer
                // has to fix his deal here!
                //
                throw content_exception_invalid_content_xml(QString("on_save_content(): Page \"%1\" is missing its parent page \"%2\".")
                        .arg(cpath)
                        .arg(parent_ipath.get_cpath()));
            }
        }

        path_info_t::status_t status(ipath.get_status());
        if(status.is_error())
        {
            if(status.get_error() == path_info_t::status_t::error_t::UNDEFINED)
            {
                // by saving the primary owner, we mark a page as be in
                // the CREATE state already
                //
                //status.reset_state(path_info_t::status_t::state_t::CREATE);
                //ipath.set_status(status);

                // we only set the primary owner on creation, which means
                // a plugin can take over the ownership of a page and we
                // do not reset that ownership on updates
                content_table->getRow(d->f_path)->getCell(primary_owner)->setValue(d->f_owner);
            }
            else
            {
                throw snap_logic_exception(QString("somehow on_save_content() stumble on an erroneous status %1 (%2)").arg(static_cast<int>(status.get_error())).arg(d->f_path));
            }
        }
        // we do not have a transition state anymore... (it was not tested
        // anyway, at some point we may want to have a form of lock instead?)
        //
        //else
        //{
        //    status.set_working(path_info_t::status_t::working_t::UPDATING);
        //    ipath.set_status(status);
        //}

        // make sure we have our different basic content dates setup
        int64_t const start_date(f_snap->get_start_date());
        if(content_table->getRow(d->f_path)->getCell(QString(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))->getValue().nullValue())
        {
            // do not overwrite the created date
            content_table->getRow(d->f_path)->getCell(QString(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))->setValue(start_date);
        }

        // TODO: fix the locale... actually the revision for English is
        //       the default and maybe we do not have to create the revision
        //       field? At the same time, we could call this function with
        //       all the locales defined in the parameters.
        //
        //       Note:
        //       The first reason for adding this initialization is in link
        //       with a problem I had and that problem is now resolved. This
        //       does not mean it should not be done, however, the revision
        //       is problematic because it needs to be incremented each time
        //       we do an update when at this point it will not be. (Although
        //       it seems to work fine at this point...) -- this is not
        //       correct: the branch MUST be set to SYSTEM (0) for all data
        //       added by content.xml. Also the branch does not include the
        //       locale so I do not see why I mentioned that. Maybe I had
        //       the locale there at the time.
        //
        initialize_branch(d->f_path);

        // TODO: add support to specify the "revision owner" of the parameter
        QString const branch_key(QString("%1#%2").arg(d->f_path).arg(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH)));

        // do not overwrite the created date
        if(branch_table->getRow(branch_key)->getCell(QString(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))->getValue().nullValue())
        {
            branch_table->getRow(branch_key)->getCell(QString(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))->setValue(start_date);
        }
        // always overwrite the modified date
        branch_table->getRow(branch_key)->getCell(QString(get_name(name_t::SNAP_NAME_CONTENT_MODIFIED)))->setValue(start_date);

        // save the parameters (i.e. cells of data defined by the developer)
        std::map<QString, bool> use_new_revision;
        for(content_params_t::iterator p(d->f_params.begin());
                p != d->f_params.end(); ++p)
        {
            // make sure no parameter is defined as content::primary_owner
            // because we are 100% in control of that one!
            // (we may want to add more as time passes)
            //
            if(p->f_name == primary_owner)
            {
                throw content_exception_invalid_content_xml("content::on_save_content() cannot accept a parameter named \"content::primary_owner\" as it is reserved");
            }

            // in order to overwrite values (parameters) from a different
            // plugin, one can give each field a priority
            //
            QString const priority_field_name(QString("%1::%2")
                                .arg(get_name(name_t::SNAP_NAME_CONTENT_FIELD_PRIORITY))
                                .arg(p->f_name));
            param_priority_t const priority(content_table->getRow(d->f_path)->getCell(priority_field_name)->getValue().safeUInt64Value());
            if(p->f_priority < priority)
            {
                // ignore entries with smaller priorities, they were
                // supplanted by another plugin
                //
                // IMPORTANT NOTE: this prevents translations to go through
                //                 so the other plugin(s) must provide all
                //                 the translations if necessary
                //
                continue;
            }
            if(p->f_priority > priority)
            {
                // the new priority is larger than the currently saved
                // priority, save the largest one
                //
                // note: this means we never save 0, which would not be
                //       useful and would really add tons of useless fields
                //       to the database
                //
                content_table->getRow(d->f_path)->getCell(priority_field_name)->setValue(p->f_priority);
            }

            for(QMap<QString, QString>::const_iterator data(p->f_data.begin());
                    data != p->f_data.end(); ++data)
            {
                QString const locale(data.key());

                // define the key and table affected
                libdbproxy::table::pointer_t param_table;
                QString row_key;
                switch(p->f_revision_type)
                {
                case param_revision_t::PARAM_REVISION_GLOBAL:
                    // in the content table
                    param_table = content_table;
                    row_key = d->f_path;
                    break;

                case param_revision_t::PARAM_REVISION_BRANCH:
                    // path + "#0" in the branch table
                    param_table = branch_table;
                    row_key = branch_key;
                    break;

                case param_revision_t::PARAM_REVISION_REVISION:
                    // path + "#xx/0.<revision>" in the revision table
                    param_table = revision_table;
                    bool const create_revision(use_new_revision.find(locale) == use_new_revision.end());
                    if(!create_revision)
                    {
                        row_key = get_revision_key(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, locale, false);
                    }
                    // else row_key.clear(); -- I think it is faster to test the flag again
                    if(create_revision || row_key.isEmpty())
                    {
                        // the revision does not exist yet, create it
                        snap_version::version_number_t revision_number(get_new_revision(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, locale, true));
                        set_current_revision(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, revision_number, locale, false);
                        set_current_revision(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, revision_number, locale, true);
                        set_revision_key(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, revision_number, locale, false);
                        row_key = set_revision_key(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, revision_number, locale, true);
                        use_new_revision[locale] = false;

                        // mark when the row was created
                        //
                        if(!p->f_remove)
                        {
                            revision_table->getRow(row_key)->getCell(get_name(name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
                        }
                    }
                    break;

                }

                // we just saved the content::primary_owner so the row exists now
                //if(content_table->exists(d->f_path)) ...

                // unless the developer said to overwrite the data, skip
                // the save if the data alerady exists
                //
                // Note: we could also use exist() instead of nullValue()?
                //       (which means that "" would not be viewed as a null)
                //
                if(p->f_remove)
                {
                    // make sure the cell does not exist
                    //
                    param_table->getRow(row_key)->dropCell(p->f_name);

                    // TBD: if this was the last cell, the "content::created"
                    //      should also be removed... that would also include
                    //      the "content::prevent_delete"
                }
                else if(p->f_overwrite
                || param_table->getRow(row_key)->getCell(p->f_name)->getValue().nullValue())
                {
                    bool ok(true);
                    switch(p->f_type)
                    {
                    case param_type_t::PARAM_TYPE_STRING:
                        param_table->getRow(row_key)->getCell(p->f_name)->setValue(*data);
                        break;

                    case param_type_t::PARAM_TYPE_FLOAT32:
                        {
                            float const v(data->toFloat(&ok));
                            param_table->getRow(row_key)->getCell(p->f_name)->setValue(v);
                        }
                        break;

                    case param_type_t::PARAM_TYPE_FLOAT64:
                        {
                            double const v(data->toDouble(&ok));
                            param_table->getRow(row_key)->getCell(p->f_name)->setValue(v);
                        }
                        break;

                    case param_type_t::PARAM_TYPE_INT8:
                        {
                            int const v(data->toInt(&ok));
                            ok = ok && v >= -128 && v <= 127; // verify overflows
                            param_table->getRow(row_key)->getCell(p->f_name)->setValue(static_cast<signed char>(v));
                        }
                        break;

                    case param_type_t::PARAM_TYPE_INT32:
                        param_table->getRow(row_key)->getCell(p->f_name)->setValue(static_cast<int32_t>(data->toInt(&ok)));
                        break;

                    case param_type_t::PARAM_TYPE_INT64:
                        param_table->getRow(row_key)->getCell(p->f_name)->setValue(static_cast<int64_t>(data->toLongLong(&ok)));
                        break;

                    }
                    if(!ok)
                    {
                        throw content_exception_invalid_content_xml(QString("content::on_save_content() tried to convert %1 to a number and failed.").arg(*data));
                    }
                }
            }
        }

        // if we have a moved-from path then we want to check whether that
        // "old" page exists and if so marked it as moved to the new location;
        // if the old page does not exist, do nothing
        //
        // if the status of the old page is not NORMAL, also do nothing
        //
        if(!d->f_moved_from.isEmpty())
        {
            path_info_t moved_from_ipath;
            moved_from_ipath.set_path(d->f_moved_from);
            libdbproxy::row::pointer_t row(content_table->getRow(moved_from_ipath.get_key()));
            if(row->exists(primary_owner))
            {
                // it already exists, but it could have been deleted or moved before
                // in which case we need to resurrect the page back to NORMAL
                //
                // the editor allowing creating such a page should have asked the
                // end user first to know whether the page should indeed be
                // "undeleted".
                //
                path_info_t::status_t moved_status(moved_from_ipath.get_status());
                if(moved_status.get_state() == path_info_t::status_t::state_t::NORMAL)
                {
                    // change page to MOVED (i.e. the path plugin will then
                    // redirect the user automatically)
                    //
                    // TODO: here we probably need to force a new branch so the
                    //       user would not see the old revisions by default...
                    //
                    SNAP_LOG_WARNING("Marked page \"")(moved_from_ipath.get_key())("\" as we moved to page \"")(ipath.get_key())("\".");
                    moved_status.reset_state(path_info_t::status_t::state_t::MOVED);
                    moved_from_ipath.set_status(moved_status);

                    // link both pages together in this branch
                    {
                        // note: we do not need a specific revision when
                        //       creating a link, however, we do need a
                        //       specific branch so we create a new path
                        //       info with the right branch, but leave
                        //       the revision to whatever it is by default
                        bool source_unique(false);
                        char const * clone_name(get_name(name_t::SNAP_NAME_CONTENT_CLONE));
                        links::link_info link_source(clone_name, source_unique, moved_from_ipath.get_key(), moved_from_ipath.get_branch());

                        bool destination_unique(true);
                        char const * original_page_name(get_name(name_t::SNAP_NAME_CONTENT_ORIGINAL_PAGE));
                        links::link_info link_destination(original_page_name, destination_unique, ipath.get_key(), ipath.get_branch());

                        links::links::instance()->create_link(link_source, link_destination);
                    }
                }
            }
        }

        // link this entry to its parent automatically
        // first we need to remove the site key from the path
        //f_snap->trace("Generate missing parent links.\n");
        QString const path(d->f_path.mid(site_key.length()));
        snap_string_list parts(path.split('/', QString::SkipEmptyParts));
        if(parts.count() > 0)
        {
            QString src(parts.join("/"));
            src = site_key + src;
            parts.pop_back();
            QString dst(parts.join("/"));
            dst = site_key + dst;

            path_info_t dst_ipath;
            dst_ipath.set_path(dst);
            if(!content_table->exists(dst_ipath.get_key())
            || !content_table->getRow(dst_ipath.get_key())->exists(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))
            {
                // we do not allow a parent to got missing, the programmer
                // has to fix his deal here!
                //
                // NOTE: This should NEVER happens since we already checked
                //       this earlier in the loop.
                //
                throw content_exception_invalid_content_xml(QString("on_save_content(): Page \"%1\" is missing its parent page \"%2\" when attempting to create the parent/child link.")
                        .arg(src)
                        .arg(dst));
            }

            links::link_info source(get_name(name_t::SNAP_NAME_CONTENT_PARENT), true, src, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
            links::link_info destination(get_name(name_t::SNAP_NAME_CONTENT_CHILDREN), false, dst, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);

            // TODO: these rows generate errors because they are missing the
            //       branch and revision information generally expected; we
            //       want to create some data here so the page is "real"
            //       enough to be used (i.e. call create_content() ?)

// TODO only repeat if the parent did not exist, otherwise we assume the
//      parent created its own parent/children link already.
//SNAP_LOG_TRACE("parent/children [")(src)("]/[")(dst)("]");
            links::links::instance()->create_link(source, destination);
        }
    }

    // link the nodes together (on top of the parent/child links)
    // this is done as a second step so we are sure that all the source
    // and destination rows exist at the time we create the links
    //
    f_snap->trace("Generate links between various pages.\n");
    on_save_links(&content_block_t::f_links, true);

    // remove some links that the developer found as spurious...
    // you have to be careful with this one:
    // (1) it will ALWAYS re-remove that link...
    // (2) the link can be added then immediately removed
    //     since the remove is applied after the add
    //
    f_snap->trace("Remove links between various pages.\n");
    on_save_links(&content_block_t::f_remove_links, false);

    // attachments are pages too, only they require a valid parent to be
    // created and many require links to work (i.e. be assigned a type)
    // so we add them after the basic content and links
    f_snap->trace("Save attachments to database.\n");
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        for(content_attachments_t::iterator a(d->f_attachments.begin());
                a != d->f_attachments.end(); ++a)
        {
            attachment_file file(f_snap);

            // attachment specific fields
            file.set_multiple(false);
            file.set_parent_cpath(a->f_path);
            file.set_field_name(a->f_field_name);
            file.set_attachment_owner(a->f_owner);
            file.set_attachment_type(a->f_type);
            file.set_creation_time(f_snap->get_start_date());
            file.set_update_time(f_snap->get_start_date());
            file.set_dependencies(a->f_dependencies);

            // post file fields
            file.set_file_name(a->f_field_name);
            file.set_file_filename(a->f_filename);
            //file.set_file_data(data);
            // TBD should we have an original MIME type defined by the
            //     user?
            //file.set_file_original_mime_type(QString const& mime_type);
            file.set_file_creation_time(f_snap->get_start_date());
            file.set_file_modification_time(f_snap->get_start_date());
            ++f_file_index; // this is more of a random number here!
            file.set_file_index(f_file_index);

            snap_child::post_file_t f;
            f.set_filename(a->f_filename);
            if(!f_snap->load_file(f))
            {
                throw content_exception_io_error("content::on_save_content(): load_file(\"" + a->f_filename + "\") failed.");
            }
            file.set_file_data(f.get_data());

            // for images, also check the dimensions and if available
            // save them in there because that's useful for the <img>
            // tags (it is faster to load 8 bytes from Cassandra than
            // a whole attachment!)
            snap_image info;
            if(info.get_info(file.get_file().get_data()))
            {
                if(info.get_size() > 0)
                {
                    smart_snap_image_buffer_t buffer(info.get_buffer(0));
                    file.set_file_image_width(buffer->get_width());
                    file.set_file_image_height(buffer->get_height());
                    file.set_file_mime_type(buffer->get_mime_type());
                }
            }

            // user forces the MIME type (important for many files such as
            // JavaScript which otherwise come out with really funky types)
            if(!a->f_mime_type.isEmpty())
            {
                file.set_file_mime_type(a->f_mime_type);
            }

            // ready, create the attachment
            create_attachment(file, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");

            // here the data buffer gets freed!
        }
    }

    // allow other plugins to add their own stuff dynamically
    //
    // (this mechanism is working only comme-ci comme-ca since all
    // the other plugins should anyway have workable defaults; however,
    // once in a while, defaults are not enough; for example the shorturl
    // needs to generate a shorturl, there is no real default other than:
    // that page has no shorturl.)
    //
    // The page is already considered created so the
    // content::create_content_impl() function just returns true quickly.
    //
    f_snap->trace("Generate create_content() events to all the new pages so other plugins have a chance to do their job.\n");
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        //bool created(false);
        QString const path(d->f_path);
        if(path.startsWith(site_key))
        {
            path_info_t ipath;
            ipath.set_path(path);
            links::link_info info(get_name(name_t::SNAP_NAME_CONTENT_PAGE_TYPE), true, ipath.get_key(), ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
            links::link_info child_info;
            if(link_ctxt->next_link(child_info))
            {
                // should always be true because all pages have a type
                QString const type_key(child_info.key());
                int const pos(type_key.indexOf("/types/taxonomy/system/content-types/"));
                create_content(ipath, d->f_owner, type_key.mid(pos + 37));
                //created = true;
            }
        }
        // else -- if the path does not start with site_key we have a problem

        //if(!created) -- we cannot be 100% sure that create_content() worked as expected
        {
            // mark the page as ready for use if create_content() was
            // not called (it should always be, though)
            //
            path_info_t ipath;
            ipath.set_path(d->f_path);
            path_info_t::status_t status(ipath.get_status());
            if(status.get_state() == path_info_t::status_t::state_t::CREATE)
            {
                status.set_state(path_info_t::status_t::state_t::NORMAL);
                //status.set_working(path_info_t::status_t::working_t::NOT_WORKING);
                ipath.set_status(status);
            }
        }
    }

    // we are done with that set of data, release it from memory
    f_blocks.clear();

    // RAII takes care of this one now
    //f_updating = false;
}


/** \brief Create or delete links defined in the specified list.
 *
 * This function goes through the list of blocks and search for those that
 * include links to create (f_links) or remove (f_remove_links).
 *
 * The \p create flag, if true, means the link will be created. If false,
 * it will be deleted.
 *
 * This is an internal function called by the on_save_content() function
 * to create / remove links as required.
 *
 * The function knows how to handle the branch definition. If set to
 * SPECIAL_VERSION_ALL then all the branches are affected. This can
 * be very important if you wanted to forcibly remove an invalid
 * permission.
 *
 * \param[in] list  The offset of the list to work with.
 * \param[in] create  Whether to create or delete links.
 */
void content::on_save_links(content_block_links_offset_t list, bool const create)
{
    libdbproxy::table::pointer_t content_table(get_content_table());
    QString const last_branch_key(QString("%1::%2")
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));

    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        // f_blocks use QMap so we need to use (*d) to be able to
        // use the C++ offset
        for(content_links_t::iterator l(((*d).*list).begin());
                l != ((*d).*list).end(); ++l)
        {
            snap_version::version_number_t start_source(l->f_branch_source);
            snap_version::version_number_t end_source(start_source);
            snap_version::version_number_t start_destination(l->f_branch_destination);
            snap_version::version_number_t end_destination(start_destination);

            if(start_source == snap_version::SPECIAL_VERSION_ALL)
            {
                start_source = snap_version::SPECIAL_VERSION_MIN;

                // get the end from the database
                path_info_t ipath;
                ipath.set_path(l->f_source.key());
                end_source = content_table->getRow(ipath.get_key())->getCell(last_branch_key)->getValue().safeUInt32Value();
            }

            if(start_destination == snap_version::SPECIAL_VERSION_ALL)
            {
                start_destination = snap_version::SPECIAL_VERSION_MIN;

                // get the end from the database
                path_info_t ipath;
                ipath.set_path(l->f_destination.key());
                end_destination = content_table->getRow(ipath.get_key())->getCell(last_branch_key)->getValue().safeUInt32Value();
            }

            for(snap_version::version_number_t i(start_source); i <= end_source; ++i)
            {
                l->f_source.set_branch(i);
                for(snap_version::version_number_t j(start_destination); j <= end_destination; ++j)
                {
                    l->f_destination.set_branch(j);

                    // handle that specific set of branches
                    if(create)
                    {
                        links::links::instance()->create_link(l->f_source, l->f_destination);
                    }
                    else
                    {
                        links::links::instance()->delete_this_link(l->f_source, l->f_destination);
                    }
                }
            }
        }
    }
}




/** \fn void check_attachment_security(attachment_file const& file, permission_flag& secure, bool const fast)
 * \brief Check whether the attachment is considered secure.
 *
 * Before processing an attachment further we want to know whether it is
 * secure. This event allows different plugins to check the security of
 * each file.
 *
 * Once a process decides that a file is not secure, the secure flag is
 * false and it cannot be reset back to true.
 *
 * \param[in] file  The file being processed.
 * \param[in,out] secure  Whether the file is secure.
 * \param[in] fast  If true only perform fast checks (i.e. not the virus check).
 */


/** \brief Add a javascript to the page.
 *
 * This function adds a javascript and all of its dependencies to the page.
 * If the script was already added, either immediately or as a dependency
 * of another script, then nothing more happens.
 *
 * This function adds a reference to a file. To add an inline javascript
 * snippet, check out the add_inline_javascript() function instead.
 *
 * \param[in,out] doc  The XML document receiving the javascript.
 * \param[in] name  The name of the script.
 */
void content::add_javascript(QDomDocument doc, QString const & name)
{
    // TBD: it may make sense to move to the javascript plugin since it now
    //      can include the content plugin; the one advantage would be that
    //      the get_name() from the JavaScript plugin would then make use
    //      of the "local" name_t::SNAP_NAME_JAVASCRIPT_...
    //
    if(f_added_javascripts.contains(name))
    {
        // already added, we are done
        return;
    }
    f_added_javascripts[name] = true;

    libdbproxy::table::pointer_t files_table(get_files_table());
    if(!files_table->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_JAVASCRIPTS)))
    {
        // absolutely no JavaScripts available!
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "JavaScript Not Found",
                "JavaScript \"" + name + "\" could not be read for inclusion in your HTML page.",
                "A JavaScript was requested in the \"files\" table before it was inserted under /js/...");
        NOTREACHED();
    }
    libdbproxy::row::pointer_t javascript_row(files_table->getRow(get_name(name_t::SNAP_NAME_CONTENT_FILES_JAVASCRIPTS)));
    javascript_row->clearCache();

    // TODO: at this point I read all the entries with "name_..."
    //       we will want to first check with the user's browser and
    //       then check with "any"/"all" as the browser name if no
    //       specific script is found
    //
    //       Also the following loop does NOT handle dependencies in
    //       a full tree to determine what would be best; instead it
    //       makes uses of the latest and if a file does not match
    //       the whole process fails even if by not using the latest
    //       would have worked
    //
    auto column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
    column_predicate->setCount(10); // small because we generally really only are interested by the first 1 unless marked as insecure or not yet updated on that website
    column_predicate->setIndex(); // behave like an index
    column_predicate->setStartCellKey(name + "_"); // start/end keys not reversed since using CQL...
    column_predicate->setEndCellKey(name + "`");
    column_predicate->setReversed(); // read the last first
    for(;;)
    {
        javascript_row->readCells(column_predicate);
        libdbproxy::cells const cells(javascript_row->getCells());
        if(cells.isEmpty())
        {
            // no script found, error appears at the end of the function
            break;
        }
        // handle one batch
        //
        // WARNING: "cells" is a map so we want to walk it backward since
        //          maps are sorted "in the wrong direction" for a reverse
        //          read...
        //
        QMapIterator<QByteArray, libdbproxy::cell::pointer_t> c(cells);
        c.toBack();
        while(c.hasPrevious())
        {
            c.previous();

            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            libdbproxy::cell::pointer_t cell(c.value());
            libdbproxy::value const file_md5(cell->getValue());
            if(file_md5.size() != 16)
            {
                // cell is invalid?
                SNAP_LOG_ERROR("invalid JavaScript MD5 for \"")(name)("\", it is not exactly 16 bytes.");
                continue;
            }
            QByteArray const key(file_md5.binaryValue());
            if(!files_table->exists(key))
            {
                // file does not exist?!
                //
                // TODO: we probably want to report that problem to the
                //       administrator with some form of messaging.
                //
                SNAP_LOG_ERROR("JavaScript for \"")(name)("\" could not be found with its MD5 \"")(dbutils::key_to_string(key))("\".");
                continue;
            }
            libdbproxy::row::pointer_t row(files_table->getRow(key));
            if(!row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE)))
            {
                // secure field missing?! (file was probably deleted)
                SNAP_LOG_ERROR("file referenced as JavaScript \"")(name)("\" does not have a ")(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE))(" field.");
                continue;
            }
            libdbproxy::value const secure(row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE))->getValue());
            if(secure.nullValue())
            {
                // secure field missing?!
                SNAP_LOG_ERROR("file referenced as JavaScript \"")(name)("\" has an empty ")(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE))(" field.");
                continue;
            }
            signed char const sflag(secure.signedCharValue());
            if(sflag == CONTENT_SECURE_INSECURE)
            {
                // not secure
#ifdef DEBUG
                SNAP_LOG_DEBUG("JavaScript named \"")(name)("\" is marked as being insecure.");
#endif
                continue;
            }

            // we want to get the full URI to the script
            // (WARNING: the filename is only the name used for the very first
            //           upload the very first time that file is loaded and
            //           different websites may have used different filenames)
            //
            // TODO: allow for remote paths by checking a flag in the file
            //       saying "remote" (i.e. to use Google Store and alike)
            //
            auto references_column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
            references_column_predicate->setCount(1);
            references_column_predicate->setIndex(); // behave like an index
            QString const site_key(f_snap->get_site_key_with_slash());
            QString const start_ref(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE)).arg(site_key));
            references_column_predicate->setStartCellKey(start_ref);
            references_column_predicate->setEndCellKey(start_ref + libdbproxy::cell_predicate::last_char);

            row->clearCache();
            row->readCells(references_column_predicate);
            libdbproxy::cells const ref_cells(row->getCells());
            if(ref_cells.isEmpty())
            {
                // this is not an error, it happens that a website is not
                // 100% fully updated and when that happens, we get this
                // error; we continue and try to read the next (one before
                // last) file and see whether that one is satisfactory...
                // the process continues untill all the versions of
                // a file were checked
                SNAP_LOG_WARNING("file referenced as JavaScript \"")(name)("\" has no reference back to \"")(site_key)("\" (this happens if your website is not 100% up to date).");
                continue;
            }
            // the key of this cell is the path we want to use to the file
            libdbproxy::cell::pointer_t ref_cell(*ref_cells.begin());
            libdbproxy::value const ref_string(ref_cell->getValue());
            if(ref_string.nullValue()) // bool true cannot be empty
            {
                SNAP_LOG_ERROR("file referenced as JavaScript \"")(name)("\" has an invalid reference back to ")(site_key)(" (empty).");
                continue;
            }

            // file exists and is considered secure

            // we want to first add all dependencies since they need to
            // be included first, so there is another sub-loop for that
            // note that all of those must be loaded first but the order
            // we read them as does not matter
            row->clearCache();
            auto dependencies_column_predicate(std::make_shared<libdbproxy::cell_range_predicate>());
            dependencies_column_predicate->setCount(100);
            dependencies_column_predicate->setIndex(); // behave like an index
            QString const start_dep(QString("%1:").arg(get_name(name_t::SNAP_NAME_CONTENT_FILES_DEPENDENCY)));
            dependencies_column_predicate->setStartCellKey(start_dep + ":");
            dependencies_column_predicate->setEndCellKey(start_dep + ";");
            for(;;)
            {
                row->readCells(dependencies_column_predicate);
                libdbproxy::cells const dep_cells(row->getCells());
                if(dep_cells.isEmpty())
                {
                    break;
                }
                // handle one batch
                for(libdbproxy::cells::const_iterator dc(dep_cells.begin());
                        dc != dep_cells.end();
                        ++dc)
                {
                    // get the email from the database
                    // we expect empty values once in a while because a dropCell() is
                    // not exactly instantaneous in Cassandra
                    libdbproxy::cell::pointer_t dep_cell(*dc);
                    libdbproxy::value const dep_string(dep_cell->getValue());
                    if(!dep_string.nullValue())
                    {
                        snap_version::dependency dep;
                        if(dep.set_dependency(dep_string.stringValue()))
                        {
                            // TODO: add version and browser tests
                            QString const & dep_name(dep.get_name());
                            QString const & dep_namespace(dep.get_namespace());
                            if(dep_namespace == "css")
                            {
                                add_css(doc, dep_name);
                            }
                            else if(dep_namespace.isEmpty() || dep_namespace == "javascript")
                            {
                                add_javascript(doc, dep_name);
                            }
                            else
                            {
                                // note: since the case when dep_namespace is empty is already
                                //       managed, when we reach this line it is not empty
                                //
                                f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Invalid Dependency",
                                        QString("JavaScript dependency \"%1::%2\" has a non-supported namespace.").arg(dep_namespace).arg(name),
                                        QString("The namespace is expected to be \"javascripts\" (or empty,) or \"css\"."));
                                NOTREACHED();
                            }
                        }
                        // else TBD -- we checked when saving that darn string
                        //             so failures should not happen here
                    }
                    // else TBD -- error if empty? (should not happen...)
                }
            }

            // TBD: At this point we get a bare name, no version, no browser.
            //      This means the loader will pick the latest available
            //      version with the User Agent match. This may not always
            //      be desirable though.
//#ifdef DEBUG
//SNAP_LOG_TRACE("Adding JavaScript [")(name)("] [")(ref_cell->columnName().mid(start_ref.length() - 1))("]");
//#endif
            QDomNodeList metadata(doc.elementsByTagName("metadata"));
            QDomNode javascript_tag(metadata.at(0).firstChildElement("javascript"));
            if(javascript_tag.isNull())
            {
                javascript_tag = doc.createElement("javascript");
                metadata.at(0).appendChild(javascript_tag);
            }
            QDomElement script_tag(doc.createElement("script"));
            script_tag.setAttribute("src", ref_cell->columnName().mid(start_ref.length() - 1));
            script_tag.setAttribute("type", "text/javascript");
            script_tag.setAttribute("charset", "utf-8");
            javascript_tag.appendChild(script_tag);
            return; // we are done since we found our script and added it
        }
    }

    // If the installation of a script fails, then it will not appear
    // in the "javascripts" row... this usually means the JavaScript
    // header is not valid (i.e. missing the version, invalid dependency,
    // field syntax error, etc.)
    //
    f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "JavaScript Not Found",
            "JavaScript \"" + name + "\" was not found. Was it installed?",
            "The named JavaScript was not found in the \"javascripts\" row of the \"files\" table.");
    NOTREACHED();
}


/** \brief Add inline javascript code to the page.
 *
 * This function adds a javascript code snippet to the page.
 *
 * At this time there is nothing to prevents duplication, nor is there
 * any way to change the order in which such javascript snippets are
 * added to a page. In most cases, these should just and only be variables
 * such as:
 *
 * \code
 *      users_administrative_login_time_limit = 123;
 * \endcode
 *
 * This method to add javascript code snippet should only be used when
 * the values are nearly always changing between each call. Otherwise,
 * look into dynamically creating a javascript file and reference that
 * file instead (i.e. a snippet that only changes when you edit some
 * preferences must be saved in a file. The cookie_consent_silktide
 * plugin does that if you want to see an example of such.)
 *
 * To add a reference to a script, check the add_javascript() function
 * instead.
 *
 * \warning
 * All the code must be valid JavaScript code that ends with ';' or '}'
 * as required. This function does not end your code in any specific
 * way. If the ending ';' is missing, then the concatenation of multiple
 * JavaScript entries will fail.
 *
 * \param[in,out] doc  The XML document receiving the JavaScript.
 * \param[in] code  The actual script.
 */
void content::add_inline_javascript(QDomDocument doc, QString const & code)
{
    // TBD: it may make sense to move to the javascript plugin since it now
    //      can include the content plugin; the one advantage would be that
    //      the get_name() from the JavaScript plugin would then make use
    //      of the "local" name_t::SNAP_NAME_JAVASCRIPT_...
    //
    if(code.isEmpty())
    {
        // nothing to add, return immediately
        return;
    }

    // .../metadata
    QDomNodeList metadata(doc.elementsByTagName("metadata"));

    // .../metadata/inline-javascript
    QDomNode inline_javascript_tag(metadata.at(0).firstChildElement("inline-javascript"));
    if(inline_javascript_tag.isNull())
    {
        inline_javascript_tag = doc.createElement("inline-javascript");
        metadata.at(0).appendChild(inline_javascript_tag);
    }

    // .../metadata/inline-javascript/script
    QDomNode script_tag(inline_javascript_tag.firstChildElement("script"));
    if(script_tag.isNull())
    {
        QDomElement script_element(doc.createElement("script"));
        script_element.setAttribute("type", "text/javascript");
        script_element.setAttribute("charset", "utf-8");
        inline_javascript_tag.appendChild(script_element);
        script_tag = script_element;
    }

    QDomNode data(script_tag.firstChild());
    if(data.isNull())
    {
        data = doc.createTextNode(code);
        script_tag.appendChild(data);
    }
    else if(data.isText())
    {
        QDomText data_section(data.toText());
        data_section.insertData(data_section.length(), code);
    }
    else
    {
        // Not too sure that a die() is really appropriate here, but
        // we found a node of an unexpected type...
        //
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Inline JavaScript CDATA Section Not Found",
                "The metadata/inline-javascript/script included a child node which was not a CDATA section. We do not know how to proceed.",
                "This error should never happen unless someone messes around with the metadata tree and inserts nodes before the CDATA section.");
        NOTREACHED();
    }
}


/** \brief Add a CSS to the page.
 *
 * This function adds a CSS and all of its dependencies to the page.
 * If the CSS was already added, either immediately or as a dependency
 * of another CSS, then nothing more happens.
 *
 * \param[in,out] doc  The XML document receiving the CSS.
 * \param[in] name  The name of the script.
 */
void content::add_css(QDomDocument doc, QString const & name)
{
    if(f_added_css.contains(name))
    {
        // already added, we're done
        return;
    }
    f_added_css[name] = true;

    libdbproxy::table::pointer_t files_table(get_files_table());
    if(!files_table->exists("css"))
    {
        // absolutely no CSS available!
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "CSS Not Found",
                "CSS \"" + name + "\" could not be read for inclusion in your HTML page.",
                "A CSS was requested in the \"files\" table before it was inserted under /css/...");
        NOTREACHED();
    }
    libdbproxy::row::pointer_t css_row(files_table->getRow("css"));
    css_row->clearCache();

    // TODO: at this point I read all the entries with "name_..."
    //       we will want to first check with the user's browser and
    //       then check with "any" as the browser name if no specific
    //       file is found
    //
    //       Also the following loop does NOT handle dependencies in
    //       a full tree to determine what would be best; instead it
    //       makes uses of the latest and if a file does not match
    //       the whole process fails even if by not using the latest
    //       would have worked
    auto column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
    column_predicate->setCount(10); // small because we are really only interested by the first 1 unless marked as insecure
    column_predicate->setIndex(); // behave like an index
    column_predicate->setStartCellKey(name + "_"); // start/end keys not reversed since using CQL
    column_predicate->setEndCellKey(name + "`");
    column_predicate->setReversed(); // read the last first
    for(;;)
    {
        css_row->readCells(column_predicate);
        libdbproxy::cells const cells(css_row->getCells());
        if(cells.isEmpty())
        {
            break;
        }
        // handle one batch
        QMapIterator<QByteArray, libdbproxy::cell::pointer_t> c(cells);
        c.toBack();
        while(c.hasPrevious())
        {
            c.previous();

            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            libdbproxy::cell::pointer_t cell(c.value());
            libdbproxy::value const file_md5(cell->getValue());
            if(file_md5.nullValue())
            {
                // cell is invalid?
                SNAP_LOG_ERROR("invalid CSS MD5 for \"")(name)("\", it is empty");
                continue;
            }
            QByteArray const key(file_md5.binaryValue());
            if(!files_table->exists(key))
            {
                // file does not exist?!
                // TODO: we probably want to report that problem
                SNAP_LOG_ERROR("CSS for \"")(name)("\" could not be found with its MD5");
                continue;
            }
            libdbproxy::row::pointer_t row(files_table->getRow(key));
            if(!row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE)))
            {
                // secure field missing?! (file was probably deleted)
                SNAP_LOG_ERROR("file referenced as CSS \"")(name)("\" does not have a ")(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE))(" field");
                continue;
            }
            libdbproxy::value const secure(row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE))->getValue());
            if(secure.nullValue())
            {
                // secure field missing?!
                SNAP_LOG_ERROR("file referenced as CSS \"")(name)("\" has an empty ")(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE))(" field");
                continue;
            }
            signed char const sflag(secure.signedCharValue());
            if(sflag == CONTENT_SECURE_INSECURE)
            {
                // not secure
#ifdef DEBUG
                SNAP_LOG_DEBUG("CSS named \"")(name)("\" is marked as being insecure");
#endif
                continue;
            }

            // we want to get the full URI to the CSS file
            // (WARNING: the filename is only the name used for the very first
            //           upload the very first time that file is loaded and
            //           different websites may have used different filenames)
            //
            // TODO: allow for remote paths by checking a flag in the file
            //       saying "remote" (i.e. to use Google Store and alike)
            auto references_column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
            references_column_predicate->setCount(1);
            references_column_predicate->setIndex(); // behave like an index
            QString const site_key(f_snap->get_site_key_with_slash());
            QString const start_ref(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE)).arg(site_key));
            references_column_predicate->setStartCellKey(start_ref);
            references_column_predicate->setEndCellKey(start_ref + libdbproxy::cell_predicate::last_char);

            row->clearCache();
            row->readCells(references_column_predicate);
            libdbproxy::cells const ref_cells(row->getCells());
            if(ref_cells.isEmpty())
            {
                SNAP_LOG_ERROR("file referenced as CSS \"")(name)("\" has no reference back to ")(site_key);
                continue;
            }
            // the key of this cell is the path we want to use to the file
            libdbproxy::cell::pointer_t ref_cell(*ref_cells.begin());
            libdbproxy::value const ref_string(ref_cell->getValue());
            if(ref_string.nullValue()) // bool true cannot be empty
            {
                SNAP_LOG_ERROR("file referenced as CSS \"")(name)("\" has an invalid reference back to ")(site_key)(" (empty)");
                continue;
            }

            // file exists and is considered secure

            // we want to first add all dependencies since they need to
            // be included first, so there is another sub-loop for that
            // note that all of those must be loaded first but the order
            // we read them as does not matter
            row->clearCache();
            auto dependencies_column_predicate(std::make_shared<libdbproxy::cell_range_predicate>());
            dependencies_column_predicate->setCount(100);
            dependencies_column_predicate->setIndex(); // behave like an index
            QString const start_dep(QString("%1::").arg(get_name(name_t::SNAP_NAME_CONTENT_FILES_DEPENDENCY)));
            dependencies_column_predicate->setStartCellKey(start_dep);
            dependencies_column_predicate->setEndCellKey(start_dep + libdbproxy::cell_predicate::last_char);
            for(;;)
            {
                row->readCells(dependencies_column_predicate);
                libdbproxy::cells const dep_cells(row->getCells());
                if(dep_cells.isEmpty())
                {
                    break;
                }
                // handle one batch
                for(libdbproxy::cells::const_iterator dc(dep_cells.begin());
                        dc != dep_cells.end();
                        ++dc)
                {
                    // get the email from the database
                    // we expect empty values once in a while because a dropCell() is
                    // not exactly instantaneous in Cassandra
                    libdbproxy::cell::pointer_t dep_cell(*dc);
                    libdbproxy::value const dep_string(dep_cell->getValue());
                    if(!dep_string.nullValue())
                    {
                        snap_version::dependency dep;
                        if(dep.set_dependency(dep_string.stringValue()))
                        {
                            // TODO: add version and browser tests
                            QString const& dep_name(dep.get_name());
                            add_css(doc, dep_name);
                        }
                        // else TBD -- we checked when saving that darn string
                        //             so failures should not happen here
                    }
                    // else TBD -- error if empty? (should not happen...)
                }
            }

            // TBD: At this point we get a bare name, no version, no browser.
            //      This means the loader will pick the latest available
            //      version with the User Agent match. This may not always
            //      be desirable though.
//#ifdef DEBUG
//SNAP_LOG_TRACE() << "Adding CSS [" << name << "] [" << ref_cell->columnName().mid(start_ref.length() - 1) << "]";
//#endif
            QDomNodeList metadata(doc.elementsByTagName("metadata"));
            QDomNode css_tag(metadata.at(0).firstChildElement("css"));
            if(css_tag.isNull())
            {
                css_tag = doc.createElement("css");
                metadata.at(0).appendChild(css_tag);
            }
            QDomElement link_tag(doc.createElement("link"));
            link_tag.setAttribute("href", ref_cell->columnName().mid(start_ref.length() - 1));
            link_tag.setAttribute("type", "text/css");
            link_tag.setAttribute("rel", "stylesheet");
            css_tag.appendChild(link_tag);
            return; // we are done since we found our script and added it
        }
    }

    f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "CSS Not Found",
            "CSS \"" + name + "\" was not found. Was it installed?",
            "The named CSS was not found in the \"css\" row of the \"files\" table.");
    NOTREACHED();
}


/** \brief Check whether the created pages are from the content.xml
 *
 * While updating a website, many callbacks get called, such as
 * the on_modified_content(), and these may need to know whether
 * the update is from content.xml data or an end user creating
 * a page.
 *
 * This function returns true when the content module is creating
 * data from various content.xml files. Since the process locks
 * others out, it should be pretty safe.
 *
 * \return true if the content plugin is currently running updates
 *         from content.xml data.
 */
bool content::is_updating()
{
    return f_updating;
}


/** \brief Load an attachment.
 *
 * This function is used to load a file from an attachment. As additional
 * plugins are added additional protocols can be supported.
 *
 * The file information defaults are kept as is as much as possible. If
 * a plugin returns a file, though, it is advised that any information
 * available to the plugin be set in the file object.
 *
 * This load_file() function supports the attachment protocol (attachment:)
 * to load a file that was uploaded as an attachment. Note that this function
 * does NOT check permissions. For this reason, it is considered insecure
 * by default.
 *
 * The filename is expected to be the full URI to the attachment. If the
 * URI points to a page without an attachment (or a page that does not
 * even exist) then the function returns nothing.
 *
 * \note
 * If the found parameter is already true, then this function does nothing.
 *
 * \param[in,out] file  The file name and content.
 * \param[in,out] found  Whether the file was found.
 */
void content::on_load_file(snap_child::post_file_t& file, bool& found)
{
#ifdef DEBUG
    SNAP_LOG_TRACE("content::on_load_file(), filename=")(file.get_filename());
#endif
    if(!found)
    {
        QString filename(file.get_filename());
        if(filename.startsWith("attachment:"))     // Read an attachment file
        {
            // remove the protocol
            int i(11);
            for(; i < filename.length() && filename[i] == '/'; ++i);
            filename = filename.mid(i);
            path_info_t ipath;
            ipath.set_path(filename);
            libdbproxy::table::pointer_t content_table(get_content_table());
            if(content_table->exists(ipath.get_key())
            && content_table->getRow(ipath.get_key())->exists(get_name(name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER)))
            {
                // set the default filename, the load_attachment() is likely
                // going to set the filename as defined when uploading the
                // file (among other parameters)
                int const pos(filename.lastIndexOf('/'));
                file.set_filename(filename.mid(pos + 1));

                attachment_file f(f_snap);
                if(load_attachment(ipath.get_key(), f, true))
                {
                    file = f.get_file();
                    found = true;
                }
            }
        }
    }
}


/** \brief Check whether the cell can securily be used in a script.
 *
 * This signal is sent by the cell() function of snap_expr objects.
 * The plugin receiving the signal can check the table, row, and cell
 * names and mark that specific cell as secure. This will prevent the
 * script writer from accessing that specific cell.
 *
 * In case of the content plugin, this is used to protect all contents
 * in the secret table.
 *
 * The \p secure flag is used to mark the cell as secure. Simply call
 * the mark_as_secure() function to do so.
 *
 * \param[in] table  The table being accessed.
 * \param[in] accessible  Whether the cell is secure.
 *
 * \return This function returns true in case the signal needs to proceed.
 */
void content::on_table_is_accessible(QString const & table_name, server::accessible_flag_t & accessible)
{
    // all data in the secret table are considered secure
    // also check the lock table which really does not need to be public
    if(table_name == get_name(name_t::SNAP_NAME_CONTENT_TABLE)
    || table_name == get_name(name_t::SNAP_NAME_CONTENT_BRANCH_TABLE)
    || table_name == get_name(name_t::SNAP_NAME_CONTENT_REVISION_TABLE)
    || table_name == snap::get_name(snap::name_t::SNAP_NAME_SITES))
    {
        accessible.mark_as_accessible();
    }
    else if(table_name == get_name(name_t::SNAP_NAME_CONTENT_SECRET_TABLE)
    || table_name == get_name(name_t::SNAP_NAME_CONTENT_PROCESSING_TABLE)
    || table_name == get_name(name_t::SNAP_NAME_CONTENT_FILES_TABLE)
    || table_name == snap::get_name(snap::name_t::SNAP_NAME_DOMAINS)
    || table_name == snap::get_name(snap::name_t::SNAP_NAME_WEBSITES)
    || table_name == snap::get_name(snap::name_t::SNAP_NAME_SITES))
    {
        // this is very important for the secret table; this way any
        // other plugin cannot authorize a user to make that table
        // accessible
        accessible.mark_as_secure();
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
