// Snap Websites Server -- manage taxonomy types
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

#include "taxonomy.h"

#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(taxonomy, 1, 0)



/** \brief Get a fixed taxnomy name.
 *
 * The taxnomy plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_TAXONOMY_NAME:
        return "taxonomy::name";

    case name_t::SNAP_NAME_TAXONOMY_NAMESPACE:
        return "taxonomy";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_TAXONOMY_...");

    }
    NOTREACHED();
}


/** \brief Initialize the taxonomy plugin.
 *
 * This function is used to initialize the taxonomy plugin object.
 */
taxonomy::taxonomy()
    //: f_snap(NULL) -- auto-init
{
}


/** \brief Clean up the taxonomy plugin.
 *
 * Ensure the taxonomy object is clean before it is gone.
 */
taxonomy::~taxonomy()
{
}


/** \brief Get a pointer to the taxonomy plugin.
 *
 * This function returns an instance pointer to the taxonomy plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the taxonomy plugin.
 */
taxonomy * taxonomy::instance()
{
    return g_plugin_taxonomy_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString taxonomy::icon() const
{
    return "/images/taxonomy/taxonomy-logo-64x64.png";
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
QString taxonomy::description() const
{
    return "This plugin manages the different types on your website."
        " Types include categories, tags, permissions, etc."
        " Some of these types are locked so the system continues to"
        " work, however, all can be edited by the user in some way.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString taxonomy::dependencies() const
{
    return "|content|";
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
int64_t taxonomy::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 1, 16, 23, 52, 0, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the taxonomy plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * This file, contrary to most content files, makes heavy use
 * of the overwrite flag to make sure that the basic system
 * types are and stay defined as expected.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void taxonomy::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Check whether dynamic updates are necessary.
 *
 * This function updates the database dynamically when we detect a
 * blunder, which requires fixing.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t taxonomy::do_dynamic_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 10, 9, 23, 27, 14, owner_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Fix the ownership of our old pages to "output".
 *
 * This function is a good example of how a dynamic update is performed.
 * The old ownership of the types pages were "taxonomy". Now we do not
 * want the taxonomy plugin to depend on the layout or output plugins
 * and thus we need all its pages to be handled by the output plugin
 * directly.
 *
 * This function checks each page and reassign the owner.
 *
 * The function can safely be re-run (although it is not expected to
 * be.)
 *
 * \param[in] variables_timestamp  The time when this function is being
 *            called (same as f_snap->start_date()).
 */
void taxonomy::owner_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());

    // we cannot include the output plugin from the taxonomy plugin...
    //QString const new_owner(output::output::instance()->get_plugin_name());
    QString const new_owner("output");

    snap_string_list paths;
    paths << "types";
    paths << "types/permissions/rights/administer/taxonomy";
    paths << "types/permissions/rights/administer/taxonomy/vocabulary";
    paths << "types/permissions/rights/administer/taxonomy/vocabulary/tag";
    paths << "types/permissions/rights/edit/taxonomy";
    paths << "types/permissions/rights/edit/taxonomy/vocabulary";
    paths << "types/permissions/rights/edit/taxonomy/vocabulary/tag";
    paths << "types/taxonomy";
    paths << "types/taxonomy/system";
    for(auto const & p : paths)
    {
        content::path_info_t taxonomy_ipath;
        taxonomy_ipath.set_path(p);
        if(content_table->exists(taxonomy_ipath.get_key()))
        {
            // the page still exists, change the owner
            content_table->getRow(taxonomy_ipath.get_key())->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER))->setValue(new_owner);
        }
    }
}


/** \brief Initialize the taxonomy plugin.
 *
 * This function terminates the initialization of the taxonomy plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void taxonomy::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(taxonomy, "content", content::content, copy_branch_cells, _1, _2, _3);
}


/** \brief Search for a field in a type tree.
 *
 * This function checks for the \p col_name field in the specified type
 * and up checking each parent up to and including the parent as specified
 * by the \p limit_name column name.
 *
 * If the limit is not found, then an error is generated because it should
 * always exist (i.e. at least a system type that the user cannot edit.)
 *
 * \param[in,out] ipath  The content where we start.
 * \param[in] taxonomy_name  The name of the link to the taxonomy to use for the search.
 * \param[in] col_name  The name of the column to search.
 * \param[in] limit_name  The title at which was stop the search.
 *
 * \return The value found in the Cassandra database.
 */
libdbproxy::value taxonomy::find_type_with(content::path_info_t & ipath, QString const & taxonomy_name, QString const & col_name, QString const & limit_name)
{
    libdbproxy::value const not_found;
    // get link taxonomy_name from ipath
    links::link_info type_info(taxonomy_name, true, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> type_ctxt(links::links::instance()->new_link_context(type_info));
    links::link_info link_type;
    if(!type_ctxt->next_link(link_type))
    {
        // this should not happen assuming the pages are properly defined
        return not_found;
    }
    QString type_key(link_type.key());
    if(type_key.isEmpty())
    {
        return not_found;
    }
    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    for(;;)
    {
        // TODO: determine whether the type should be checked in the branch instead of global area.
        content::path_info_t tpath;
        tpath.set_path(type_key);

        if(!content_table->exists(type_key))
        {
            // TODO: should this be an error instead? all the types should exist!
            return not_found;
        }
        libdbproxy::row::pointer_t row(content_table->getRow(type_key));

        // check for the key, if it exists we found what the user is
        // looking for!
        libdbproxy::value result(row->getCell(col_name)->getValue());
        if(!result.nullValue())
        {
            return result;
        }
        // have we reached the limit
        libdbproxy::value limit(row->getCell(QString(get_name(name_t::SNAP_NAME_TAXONOMY_NAME)))->getValue());
        if(!limit.nullValue() && limit.stringValue() == limit_name)
        {
            // we reached the limit and have not found a result
            return not_found;
        }
        // get the parent
        links::link_info info(content::get_name(content::name_t::SNAP_NAME_CONTENT_PARENT), true, tpath.get_key(), tpath.get_branch());
        QSharedPointer<links::link_context> ctxt(links::links::instance()->new_link_context(info));
        links::link_info link_info;
        if(!ctxt->next_link(link_info))
        {
            // this should never happen because we should always have a parent
            // up until limit_name is found
            return not_found;
        }
        type_key = link_info.key();
    }
    NOTREACHED();
}


// Using the output::on_path_execute() instead
//bool taxonomy::on_path_execute(content::path_info_t& ipath)
//{
//    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));
//
//    return true;
//}


// no need, output will take over
//void taxonomy::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
//{
//    // a type is just like a regular page
//    output::output::instance()->on_generate_main_content(ipath, page, body);
//}


void taxonomy::on_copy_branch_cells(libdbproxy::cells & source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    NOTUSED(destination_branch);

    content::content::copy_branch_cells_as_is(source_cells, destination_row, get_name(name_t::SNAP_NAME_TAXONOMY_NAMESPACE));
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
