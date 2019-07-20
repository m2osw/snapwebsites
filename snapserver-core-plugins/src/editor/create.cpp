// Snap Websites Server -- all the user content and much of the system content
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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


/** \file
 * \brief The implementation of the content create page.
 *
 * This file contains the create_page_t class implementation and the
 * corresponding create_page() function in the attachment class.
 *
 * \note
 * This function was added to the attachment because it wants to access
 * the attachment, layout, theme, content, links... and therefore it can't
 * just be in the content plugin.
 */


// self
//
#include "editor.h"


// other plugins
//
#include "../list/list.h"
#include "../permissions/permissions.h"


// last include
//
#include <snapdev/poison.h>



namespace snap
{
namespace editor
{




create_page_t::create_page_t()
{
    // by default use the content plugin's name
    //
    f_owner = content::content::instance()->get_plugin_name();
}


void create_page_t::set_path(QString const & path)
{
    f_path = path;
}


void create_page_t::set_type(QString const & type)
{
    f_type = type;
}


void create_page_t::set_owner(QString const & owner)
{
    f_owner = owner;
}


void create_page_t::set_locale(QString const & locale)
{
    f_locale = locale;
}


void create_page_t::set_global_field(QString const & field_name, libdbproxy::value const & value)
{
    f_global_fields[field_name] = value;
}


void create_page_t::set_global_field(QString const & field_name, QString const & value)
{
    set_global_field(field_name, libdbproxy::value(value));
}


void create_page_t::set_global_field(QString const & field_name, std::string const & value)
{
    set_global_field(field_name, libdbproxy::value(QString::fromUtf8(value.c_str())));
}


void create_page_t::set_global_field(QString const & field_name, char const * value)
{
    set_global_field(field_name, libdbproxy::value(QString::fromUtf8(value)));
}


void create_page_t::set_quoted_global_field(QString const & field_name, QString const & value)
{
    set_global_field(field_name, libdbproxy::value(QString("\"%1\"").arg(value)));
}


void create_page_t::set_quoted_global_field(QString const & field_name, std::string const & value)
{
    set_global_field(field_name, libdbproxy::value(QString("\"%1\"").arg(QString::fromUtf8(value.c_str()))));
}


void create_page_t::set_quoted_global_field(QString const & field_name, char const * value)
{
    set_global_field(field_name, libdbproxy::value(QString("\"%1\"").arg(QString::fromUtf8(value))));
}


void create_page_t::set_layout_layout(QString const & value)
{
    set_quoted_global_field(layout::get_name(layout::name_t::SNAP_NAME_LAYOUT_LAYOUT), value);
}


void create_page_t::set_layout_theme(QString const & value)
{
    set_quoted_global_field(layout::get_name(layout::name_t::SNAP_NAME_LAYOUT_THEME), value);
}


void create_page_t::set_editor_layout(QString const & value)
{
    set_quoted_global_field(get_name(name_t::SNAP_NAME_EDITOR_LAYOUT), value);
}


void create_page_t::set_dynamic_path(signed char dynamic)
{
    set_global_field(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_DYNAMIC), dynamic);
}


void create_page_t::set_branch_field(QString const & field_name, libdbproxy::value const & value)
{
    f_branch_fields[field_name] = value;
}


void create_page_t::set_branch_field(QString const & field_name, QString const & value)
{
    set_revision_field(field_name, libdbproxy::value(value));
}


void create_page_t::set_branch_field(QString const & field_name, std::string const & value)
{
    set_revision_field(field_name, libdbproxy::value(QString::fromUtf8(value.c_str())));
}


void create_page_t::set_branch_field(QString const & field_name, char const * value)
{
    set_revision_field(field_name, libdbproxy::value(QString::fromUtf8(value)));
}


void create_page_t::set_list_test_script(QString const & value)
{
    set_branch_field(list::get_name(list::name_t::SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT), libdbproxy::value(value));
}


void create_page_t::set_list_key_script(QString const & value)
{
    set_branch_field(list::get_name(list::name_t::SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT), libdbproxy::value(value));
}


void create_page_t::set_list_selector(QString const & value)
{
    set_branch_field(list::get_name(list::name_t::SNAP_NAME_LIST_SELECTOR), libdbproxy::value(value));
}


void create_page_t::set_revision_field(QString const & field_name, libdbproxy::value const & value)
{
    f_revision_fields[field_name] = value;
}


void create_page_t::set_revision_field(QString const & field_name, QString const & value)
{
    set_revision_field(field_name, libdbproxy::value(value));
}


void create_page_t::set_revision_field(QString const & field_name, std::string const & value)
{
    set_revision_field(field_name, libdbproxy::value(QString::fromUtf8(value.c_str())));
}


void create_page_t::set_revision_field(QString const & field_name, char const * value)
{
    set_revision_field(field_name, libdbproxy::value(QString::fromUtf8(value)));
}


void create_page_t::set_title(QString const & title)
{
    set_revision_field(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE), title);
}


void create_page_t::set_body(QString const & body)
{
    set_revision_field(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY), body);
}


/** \brief Create a page with additional info.
 *
 * Many times we want to create a page including many fields, more than the
 * few defaults offered by the create_content() event.
 *
 * This function is used to handle that situation. This allows for the
 * creation of a complete page with any number of fields. This is
 * particularly useful since some fields may be created in the content
 * table, others in the branch table and also some in the revision table.
 *
 * The function returns the path to the newly created page.
 *
 * The parameters are all defined in the create_page_t structure.
 *
 * \note
 * Although this function can be called multiple times against the same
 * page, the results may not end up being 100% as expected.
 *
 * \todo
 * Add support for attachments.
 *
 * \todo
 * Transform the function to work with the tree table once it becomes
 * available.
 *
 * \param[in] page  The new page various details and fields.
 *
 * \return Path to the newly created page.
 */
content::path_info_t editor::create_page(create_page_t const & page)
{
    content::content * content_plugin(content::content::instance());

    // use the journal so if the creation fails, all the entries get
    // removed
    //
    content::journal_list * journal(content_plugin->get_journal_list());

    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());

    // get ready to create a page at 'f_path'
    //
    content::path_info_t ipath;
    ipath.set_path(page.f_path);

    // check whether it exists, if so, do not try to re-created it
    //
    bool const new_page = !content_table->exists(ipath.get_key())
                       || !content_table->getRow(ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED));

    if(new_page)
    {
        // page is brand new, create it
        //
        ipath.force_branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);
        ipath.force_revision(snap_version::SPECIAL_VERSION_FIRST_REVISION);
        ipath.force_locale(page.f_locale);
        journal->add_page_url(page.f_path);
        content_plugin->create_content(ipath, page.f_owner, page.f_type);
    }

    // editor layout if one was defined
    //
    if(!page.f_global_fields.empty())
    {
        libdbproxy::row::pointer_t global_row(content_table->getRow(ipath.get_key()));
        for(auto const & it : page.f_global_fields)
        {
            global_row->getCell(it.first)
                      ->setValue(it.second);
        }
    }

    // revision if some data was defined
    //
    if(new_page
    && !page.f_revision_fields.empty())
    {
        libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
        libdbproxy::row::pointer_t revision_row(revision_table->getRow(ipath.get_revision_key()));

        // add a content::name_t::SNAP_NAME_CONTENT_CREATED field automatically
        //
        int64_t const start_date(f_snap->get_start_date());
        revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))
                    ->setValue(start_date);

        // add the other fields
        //
        for(auto const & it : page.f_revision_fields)
        {
            revision_row->getCell(it.first)
                         ->setValue(it.second);
        }
    }

    // branch layout if one was defined
    //
    if(!page.f_branch_fields.empty())
    {
        bool has_list(false);
        libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
        libdbproxy::row::pointer_t branch_row(branch_table->getRow(ipath.get_branch_key()));
        for(auto const & it : page.f_branch_fields)
        {
            if(it.first == list::get_name(list::name_t::SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT)
            || it.first == list::get_name(list::name_t::SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT)
            || it.first == list::get_name(list::name_t::SNAP_NAME_LIST_SELECTOR))
            {
                has_list = true;
            }

            branch_row->getCell(it.first)
                      ->setValue(it.second);
        }

        // mark this page as a list if we just added some list parameters
        // to the branch
        //
        if(has_list)
        {
            content::path_info_t list_type_ipath;
            list_type_ipath.set_path(list::get_name(list::name_t::SNAP_NAME_LIST_TAXONOMY_PATH));
    
            QString const link_name(list::get_name(list::name_t::SNAP_NAME_LIST_TYPE));
            bool const source_unique(true);
            bool const destination_unique(false);
            links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
            links::link_info destination(link_name, destination_unique, list_type_ipath.get_key(), list_type_ipath.get_branch());
            links::links::instance()->create_link(source, destination);
        }
    }

    //if(!page.f_layout_layout.isEmpty()
    //|| !page.f_layout_theme.isEmpty()
    //|| !page.f_editor_layout.isEmpty()
    //|| page.f_dynamic_path != 0)
    //{

    //    if(!page.f_layout_layout.isEmpty())
    //    {
    //        global_row->getCell(layout::get_name(layout::name_t::SNAP_NAME_LAYOUT_LAYOUT))->setValue(QString("\"%1\"").arg(page.f_layout_layout));
    //    }
    //    if(!page.f_layout_theme.isEmpty())
    //    {
    //        global_row->getCell(layout::get_name(layout::name_t::SNAP_NAME_LAYOUT_THEME))->setValue(QString("\"%1\"").arg(page.f_layout_theme));
    //    }
    //    if(!page.f_editor_layout.isEmpty())
    //    {
    //        global_row->getCell(editor::get_name(editor::name_t::SNAP_NAME_EDITOR_LAYOUT))->setValue(QString("\"%1\"").arg(page.f_editor_layout));
    //    }

    //    if(page.f_dynamic_path != 0)
    //    {
    //        signed char const dynamic(page.f_dynamic_path);
    //        global_row->getCell(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_DYNAMIC))->setValue(dynamic);
    //    }
    //}

    // save the title, description, and creation date
    //
    //if(new_page)
    //{
    //    libdbproxy::table::pointer_t revision_table(get_revision_table());
    //    libdbproxy::row::pointer_t revision_row(revision_table->getRow(ipath.get_revision_key()));
    //    int64_t const start_date(f_snap->get_start_date());
    //    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
    //    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(page.f_title);
    //    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue(page.f_body);
    //}

    // setup the list parameters
    //
    //if(!page.f_list_test_script.isEmpty())
    //{
    //    libdbproxy::table::pointer_t branch_table(get_branch_table());
    //    libdbproxy::row::pointer_t branch_row(branch_table->getRow(ipath.get_branch_key()));
    //
    //    // setup the page to generate a list of items
    //    //
    //    branch_row->getCell(list::get_name(list::name_t::SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT))->setValue(page.f_list_test_script);
    //
    //    // setup the script generating the keys which has the effect
    //    // of sorting the items in the list
    //    //
    //    branch_row->getCell(list::get_name(list::name_t::SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT))->setValue(page.f_list_key_script);
    //
    //    // list selector
    //    //
    //    branch_row->getCell(list::get_name(list::name_t::SNAP_NAME_LIST_SELECTOR))->setValue(page.f_list_selector);
    //
    //    // mark it as a list
    //    {
    //        content::path_info_t list_type_ipath;
    //        list_type_ipath.set_path(list::get_name(list::name_t::SNAP_NAME_LIST_TAXONOMY_PATH));
    //
    //        QString const link_name(list::get_name(list::name_t::SNAP_NAME_LIST_TYPE));
    //        bool const source_unique(true);
    //        bool const destination_unique(false);
    //        links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
    //        links::link_info destination(link_name, destination_unique, list_type_ipath.get_key(), list_type_ipath.get_branch());
    //        links::links::instance()->create_link(source, destination);
    //    }
    //}

    journal->done();

    return ipath;
}


} // namespace editor
} // namespace snap
// vim: ts=4 sw=4 et
