// Snap Websites Server -- plugin selection
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

#include "info.h"

#include "../locale/snap_locale.h"
#include "../messages/messages.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/xslt.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(info)



void info::init_plugin_selection_editor_widgets(content::path_info_t & ipath, QString const & field_id, QDomElement & widget)
{
    NOTUSED(ipath);

    if(field_id == "plugin_path")
    {
        QString const plugins_paths( f_snap->get_server_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_PARAM_PLUGINS_PATH)) );
        QString const site_plugins(f_snap->get_server_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_PARAM_PLUGINS)));

        QDomDocument doc(widget.ownerDocument());
        QDomNode parent(widget.parentNode());

        {
            QDomElement value_tag(snap_dom::create_element(widget, "value"));
            snap_dom::insert_html_string_to_xml_doc(value_tag, plugins_paths);
        }

        // make sure we are properly localized
        locale::locale * locale_plugin(locale::locale::instance());
        locale_plugin->set_timezone();
        locale_plugin->set_locale();

        plugins::plugin_map_t const installed_plugins(plugins::get_plugin_list());

        // then we loop through the plugins and add one entry per plugin
        //
        // each entry has:
        //  . plugin name
        //  . plugin icon
        //  . plugin description
        //  . plugin help
        //  . a set of action buttons: install, remove, setup
        //
        // We also include a few more parameters that are automatically
        // available.
        //
        int count(0);
        snap_string_list const plugin_names(plugins::list_all(plugins_paths));
        for(auto const & name : plugin_names)
        {
            try
            {
                plugins::plugin_info const information(plugins_paths, name);

                // we also want to have the date when it was last updated
                // (as in the do_update() callbacks)
                //
                QString const core_last_updated(snap::get_name(snap::name_t::SNAP_NAME_CORE_LAST_UPDATED));
                QString const param_name(QString("%1::%2").arg(core_last_updated).arg(name));
                QtCassandra::QCassandraValue plugin_last_updated(f_snap->get_site_parameter(param_name));
                int64_t const last_updated(plugin_last_updated.safeInt64Value());

                QDomDocument xml;
                QDomElement root(xml.createElement("snap"));
                xml.appendChild(root);

                // /snap[@locked=locked]
                if(!site_plugins.isEmpty())
                {
                    // list of plugins is hard coded in snapserver.conf
                    root.setAttribute("locked", "locked");
                }

                // /snap/name/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "name"));
                    snap_dom::append_plain_text_to_node(value_tag, name);
                }

                // /snap/filename/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "filename"));
                    snap_dom::append_plain_text_to_node(value_tag, information.get_filename());
                }

                // /snap/last-modification/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "last-modification"));
                    snap_dom::append_integer_to_node(value_tag, information.get_last_modification());
                }

                // /snap/last-modification-date/...
                {
                    // format this date using the user locale
                    QDomElement value_tag(snap_dom::create_element(root, "last-modification-date"));
                    snap_dom::append_plain_text_to_node(value_tag, locale_plugin->format_date(information.get_last_modification() / 1000000LL) + " " + locale_plugin->format_time(information.get_last_modification() / 1000000LL));
                }

                // /snap/last-updated/...
                if(last_updated > 0)
                {
                    QDomElement value_tag(snap_dom::create_element(root, "last-updated-date"));
                    snap_dom::append_plain_text_to_node(value_tag, locale_plugin->format_date(last_updated / 1000000LL) + " " + locale_plugin->format_time(last_updated / 1000000LL));
                }

                // /snap/icon/...
                {
                    QString plugin_icon(information.get_icon());
                    content::path_info_t icon_ipath;
                    icon_ipath.set_path(plugin_icon);
                    content::content * content_plugin(content::content::instance());
                    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
                    if(!content_table->exists(icon_ipath.get_key()))
                    {
                        plugin_icon = "/images/snap/plugin-icon-64x64.png";
                    }
                    QDomElement value_tag(snap_dom::create_element(root, "icon"));
                    snap_dom::append_plain_text_to_node(value_tag, plugin_icon);
                }

                // /snap/description/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "description"));
                    snap_dom::append_plain_text_to_node(value_tag, information.get_description());
                }

                // /snap/help/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "help-uri"));
                    snap_dom::append_plain_text_to_node(value_tag, information.get_help_uri());
                }

                // /snap/dependencies/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "dependencies"));
                    snap_string_list deps(information.get_dependencies().split('|', QString::SkipEmptyParts));
                    snap_dom::append_plain_text_to_node(value_tag, deps.join(","));
                }

                // /snap/version-major/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "version-major"));
                    snap_dom::append_integer_to_node(value_tag, information.get_version_major());
                }

                // /snap/version-minor/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "version-minor"));
                    snap_dom::append_integer_to_node(value_tag, information.get_version_minor());
                }

                // /snap/installed/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "installed"));
                    snap_dom::append_plain_text_to_node(value_tag, installed_plugins.contains(name) ? "true" : "false");
                }

                // /snap/core-plugin/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "core-plugin"));
                    snap_dom::append_plain_text_to_node(value_tag, f_snap->is_core_plugin(name) ? "true" : "false");
                }

                // /snap/settings-path/...
                {
                    QDomElement value_tag(snap_dom::create_element(root, "settings-path"));

                    // the get_cpath() returns an absolute path without
                    // the introductory '/', the XSLT re-adds it. However,
                    // if the path was just "/" or "", then the resulting
                    // cpath is "" as expected by the XSLT tests.
                    //
                    content::path_info_t settings_ipath;
                    settings_ipath.set_path(information.get_settings_path());
                    snap_dom::append_plain_text_to_node(value_tag, settings_ipath.get_cpath());
                }

                QDomDocument output("output");

                // transform to a nice HTML output
                //
                // TBD: again, we should look into at least one indirection
                //      so a theme or at least a website can request for
                //      the use of a different XSLT file for these intermediate
                //      transformations so as to format things differently
                //
                xslt x;
                x.set_xsl_from_file("qrc://xsl/layout/plugin-selection.xsl");
                x.set_document(xml);
                x.evaluate_to_document(output);

                QDomNodeList output_tags(output.elementsByTagName("output"));
//SNAP_LOG_WARNING("got output with [")(output.toString())("] -> ")(output_tags.size());
                if(output_tags.size() == 1)
                {
                    ++count;

                    QString const id(QString("plugin%1").arg(count));
                    QDomElement plugin_widget(doc.createElement("widget"));
                    plugin_widget.setAttribute("id", id);
                    plugin_widget.setAttribute("type", "custom");
                    plugin_widget.setAttribute("path", QString("plugin/selection/%1").arg(id));
                    plugin_widget.setAttribute("auto-save", "no");
                    parent.appendChild(plugin_widget);
                    QDomElement plugin_value(doc.createElement("value"));
                    //snap::snap_dom::insert_html_string_to_xml_doc(plugin_value, plugin_xml);
                    snap_dom::insert_node_to_xml_doc(plugin_value, output_tags.at(0));
                    plugin_widget.appendChild(plugin_value);
                }
            }
            catch(plugins::plugin_exception const & e)
            {
                // ignore invalid entries...
                SNAP_LOG_TRACE("could not load plugin named \"")(name)("\". Error: ")(e.what());
            }
        }
    }
}


/** \brief Execute the "install" or "remove" of a plugin.
 *
 * This function checks whether it can install or remove the specified
 * plugin and if so, apply the function.
 *
 * The function refuse to do any work if the list of plugins comes
 * from the "plugins" variable in the snapserver.conf file because
 * in that case the list of plugins defined in the database is ignored.
 *
 * \note
 * The path is /admin/plugin/install/\<plugin-name> to install a new
 * plugin, and /admin/plugin/remove/\<plugin-name> to remove it.
 *
 * \param[in] ipath  The path to the admin/plugin with the function and
 *                   name of the plugin appened to it.
 */
bool info::plugin_selection_on_path_execute(content::path_info_t & ipath)
{
    QString const cpath(ipath.get_cpath());
    if(!cpath.startsWith(QString("%1/").arg(get_name(name_t::SNAP_NAME_INFO_PLUGIN_SELECTION))))
    {
        return false;
    }

    server_access::server_access * server_access_plugin(server_access::server_access::instance());

    // forced by .conf?
    QString site_plugins(f_snap->get_server_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_PARAM_PLUGINS)));
    if(!site_plugins.isEmpty())
    {
        // list cannot be changed, so do not give hopes to the website administrator
        messages::messages::instance()->set_error(
            "Plugin Installation Fixed",
            QString("You cannot change your plugin installation via the website interface because the list is hard coded in snapserver.conf where you used the plugins=... variable."),
            "info::plugin_selection_on_path_execute(): the list of plugins is locked by the snapserver configuration.",
            false
        );
        server_access_plugin->create_ajax_result(ipath, false);
    }
    else
    {
        snap_string_list plugin_list;
        QtCassandra::QCassandraValue plugins(f_snap->get_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_PLUGINS)));
        site_plugins = plugins.stringValue();
        if(site_plugins.isEmpty())
        {
            site_plugins = f_snap->get_server_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_PARAM_DEFAULT_PLUGINS));
        }
        if(!site_plugins.isEmpty())
        {
            plugin_list = site_plugins.split(',');

            // this list may come from the snapserver.conf file so we
            // want to clean it up
            for(int i(0); i < plugin_list.length(); ++i)
            {
                plugin_list[i] = plugin_list[i].trimmed();
                if(plugin_list.at(i).isEmpty())
                {
                    // remove parts that the trimmed() rendered empty
                    plugin_list.removeAt(i);
                    --i;
                }
            }
        }

        QString const function(cpath.mid(strlen(get_name(name_t::SNAP_NAME_INFO_PLUGIN_SELECTION)) + 1));
        if(function.startsWith("install/"))
        {
            // first make sure the name is valid and indeed represents a
            // plugin that we can install
            //
            QString const plugin_name(function.mid(8));
            QString const plugins_paths( f_snap->get_server_parameter( snap::get_name(snap::name_t::SNAP_NAME_CORE_PARAM_PLUGINS_PATH) ) );
            snap_string_list const paths( plugins_paths.split(':') );
            QString const filename(plugins::find_plugin_filename(paths, plugin_name));
            if(filename.isEmpty())
            {
                messages::messages::instance()->set_error(
                    "Plugin Not Found",
                    QString("Could not install plugin \"%1\" since it does not look like it exists.").arg(plugin_name),
                    "info::plugin_selection_on_path_execute(): the name of the plugin was incorrect.",
                    false
                );
                server_access_plugin->create_ajax_result(ipath, false);
            }
            else if(!plugin_list.contains(plugin_name))
            {
                if(install_plugin(plugin_list, plugin_name))
                {
                    site_plugins = plugin_list.join(",");
                    plugins.setStringValue(site_plugins);
                    f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_PLUGINS), plugins);
                    QString const installed("installed");
                    server_access_plugin->create_ajax_result(ipath, true);
                    server_access_plugin->ajax_append_data("plugin_selection", installed.toUtf8());
                    server_access_plugin->ajax_append_data("installed_plugins", site_plugins.toUtf8());
                }
                else
                {
                    messages::messages::instance()->set_error(
                        "Plugin Dependencies Missing",
                        QString("One or more dependencies of plugin \"%1\" is missing.").arg(plugin_name),
                        "info::plugin_selection_on_path_execute(): the plugin could not be installed because one or more dependency is missing.",
                        false
                    );
                    server_access_plugin->create_ajax_result(ipath, false);
                }
            }
            else
            {
                messages::messages::instance()->set_warning(
                    "Plugin Already Installed",
                    QString("Plugin \"%1\" is already installed.").arg(plugin_name),
                    "info::plugin_selection_on_path_execute(): the plugin is already installed so we should not have gotten this event."
                );
                server_access_plugin->create_ajax_result(ipath, false);
            }
        }
        else if(function.startsWith("remove/"))
        {
            // here we do not check the validity of the name from the file system
            // if the name is not in the list of plugins, we do nothing anyway
            QString const plugin_name(function.mid(7));
            if(plugin_list.contains(plugin_name))
            {
                if(!f_snap->is_core_plugin(plugin_name))
                {
                    if(uninstall_plugin(plugin_list, plugin_name))
                    {
                        site_plugins = plugin_list.join(",");
                        plugins.setStringValue(site_plugins);
                        f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_PLUGINS), plugins);
                        QString const removed("removed");
                        server_access_plugin->create_ajax_result(ipath, true);
                        server_access_plugin->ajax_append_data("plugin_selection", removed.toUtf8());
                        server_access_plugin->ajax_append_data("installed_plugins", site_plugins.toUtf8());
                    }
                    else
                    {
                        messages::messages::instance()->set_error(
                            "Plugin Dependencies Missing",
                            QString("One or more dependencies of plugin \"%1\" is missing.").arg(plugin_name),
                            "info::plugin_selection_on_path_execute(): the plugin could not be installed because one or more dependency is missing.",
                            false
                        );
                        server_access_plugin->create_ajax_result(ipath, false);
                    }
                }
                else
                {
                    messages::messages::instance()->set_error(
                        "Core Plugin Removal is Forbidden",
                        QString("It is not possible to remove plugin \"%1\" since it is a core plugin.").arg(plugin_name),
                        "info::plugin_selection_on_path_execute(): a core plugin cannot be removed at all.",
                        false
                    );
                    server_access_plugin->create_ajax_result(ipath, false);
                }
            }
            else
            {
                messages::messages::instance()->set_warning(
                    "Plugin Not Found",
                    QString("Could not remove plugin \"%1\" since it does not look like it was installed.").arg(plugin_name),
                    "info::plugin_selection_on_path_execute(): the plugin could not be found in the list of installed plugins."
                );
                server_access_plugin->create_ajax_result(ipath, false);
            }
        }
        else
        {
            //SNAP_LOG_ERROR("invalid access to page \"")(cpath)("\". Plugin name missing?");

            messages::messages::instance()->set_error(
                "Plugin Not Found",
                "Invalid access to this Snap! website.",
                "info::on_path_execute(): the path does not match one of the expected paths (.../install/... or .../remove/...).",
                false
            );
            server_access_plugin->create_ajax_result(ipath, false);
        }
    }

    // create AJAX response
    server_access_plugin->ajax_output();

    return true;
}


bool info::install_plugin(snap_string_list & plugin_list, QString const & plugin_name)
{
    // accept but ignore request to install the main "server" plugin
    // and any other core plugin since they are not required in the
    // sites/core::plugins field
    //
    if(f_snap->is_core_plugin(plugin_name))
    {
        return true;
    }

    plugin_list << plugin_name;

    QString const plugins_paths( f_snap->get_server_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_PARAM_PLUGINS_PATH)) );
    try
    {
        plugins::plugin_info const information(plugins_paths, plugin_name);
        snap_string_list deps(information.get_dependencies().split('|', QString::SkipEmptyParts));
        for(auto const & d : deps)
        {
            if(!plugin_list.contains(d))
            {
                if(!install_plugin(plugin_list, d))
                {
                    // on false we ignore the entire result...
                    return false;
                }
            }
        }
    }
    catch(plugins::plugin_exception const & e)
    {
        // if there is one we cannot read, we cannot really go on, can we?
        return false;
    }

    return true;
}


bool info::uninstall_plugin(snap_string_list & plugin_list, QString const & plugin_name)
{
    int const pos(plugin_list.indexOf(plugin_name));
    plugin_list.removeAt(pos);

    QString const plugins_paths( f_snap->get_server_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_PARAM_PLUGINS_PATH)) );
    snap_string_list const plugin_names(plugin_list);
    try
    {
        for(auto const & n : plugin_names)
        {
            plugins::plugin * p(plugins::get_plugin(n));
            snap_string_list const deps(p->dependencies().split('|', QString::SkipEmptyParts));
            for(auto const & d : deps)
            {
                if(d == plugin_name)
                {
                    if(!uninstall_plugin(plugin_list, n))
                    {
                        return false;
                    }
                }
            }
        }
    }
    catch(plugins::plugin_exception const & e)
    {
        // if there is one we cannot read, we cannot really go on, can we?
        return false;
    }

    return true;
}



SNAP_PLUGIN_END()
// vim: ts=4 sw=4 et
