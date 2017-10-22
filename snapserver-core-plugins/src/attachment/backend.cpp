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
 * \brief The implementation of the attachment plugin class backend parts.
 *
 * This file contains the implementation of the various attachment backend
 * functions of the attachment plugin.
 */

#include "attachment.h"

#include <snapwebsites/plugins.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>

#include <QFile>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(attachment)


/** \brief Register various attachment backend actions.
 *
 * This function registers this plugin as supporting the following
 * actions:
 *
 * \li extractfile -- from a path to a file in the content table, extract
 *                    the corresponding file; the path must represent an
 *                    attachment; parameters are FILE_URL that specifies
 *                    the URI to find the file in the database and
 *                    FILENAME to specify the filename to use to save the
 *                    file on disk; you may also specify the FIELD_NAME
 *                    if the file has several representations and you want
 *                    to access one which is not the default
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void attachment::on_register_backend_action(server::backend_action_set & actions)
{
    actions.add_action(get_name(name_t::SNAP_NAME_ATTACHMENT_ACTION_EXTRACTFILE), this);
}


/** \brief Process various backend tasks.
 *
 * Content backend processes are descrobed in the
 * on_register_backend_action() function.
 *
 * \param[in] action  The action to work on.
 */
void attachment::on_backend_action(QString const & action)
{
    if(action == get_name(name_t::SNAP_NAME_ATTACHMENT_ACTION_EXTRACTFILE))
    {
        backend_action_extract_file();
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        throw snap_logic_exception(QString("content.cpp:on_backend_action(): content::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


/** \brief Load a file from the database and save it to a file.
 *
 * This function is used to extract a file from the database and save
 * it to a file on your computer.
 *
 * This allows administrators to retrieve files from a database to test
 * them with tools such as anti-virus, loaders, graphic tools, etc.
 */
void attachment::backend_action_extract_file()
{
    content::path_info_t ipath;
    ipath.set_path(f_snap->get_server_parameter("FILE_URL"));

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());

    // main page exists
    if(!content_table->exists(ipath.get_key())
    || !content_table->getRow(ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        SNAP_LOG_ERROR("Page for attachment \"")(ipath.get_key())("\" does not exist.");
        return;
    }

    // TODO: should we check whether the page is owned by the attachment plugin?
    //       the fact is a derived plugin could put its own name there...



    // retrieve MD5
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
    libdbproxy::value const attachment_key(revision_table->getRow(ipath.get_revision_key())->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))->getValue());
    if(attachment_key.nullValue())
    {
        SNAP_LOG_ERROR("Attachment MD5 number in page \"")(ipath.get_key())("\" is not defined.");
        return;
    }

    // optionally, user can specify the name of the field to load
    char const * files_data(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA));
    QString field_name(f_snap->get_server_parameter("FIELD_NAME"));
    if(field_name.isEmpty())
    {
        field_name = files_data;
    }
    QString const starts_with(QString("%1::").arg(files_data));
    if(field_name != files_data
    && !field_name.startsWith(starts_with))
    {
        SNAP_LOG_ERROR("field name \"")(field_name)("\" is not an acceptable field name for a file data field.");
        return;
    }

    libdbproxy::table::pointer_t files_table(content_plugin->get_files_table());
    if(!files_table->exists(attachment_key.binaryValue())
    || !files_table->getRow(attachment_key.binaryValue())->exists(field_name))
    {
        // somehow the file data is not available
        SNAP_LOG_ERROR("Attachment \"")(ipath.get_key())("\" was not found.");
        return;
    }

    libdbproxy::value data(files_table->getRow(attachment_key.binaryValue())->getCell(field_name)->getValue());

    QString const filename(f_snap->get_server_parameter("FILENAME"));
    QFile out(filename);
    if(!out.open(QIODevice::WriteOnly))
    {
        SNAP_LOG_ERROR("file \"")(filename)("\" could not be opened for writing.");
        return;
    }
    out.write(data.binaryValue());
}




SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
