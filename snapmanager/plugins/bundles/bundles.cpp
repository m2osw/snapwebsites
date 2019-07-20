// Snap Websites Server -- manage the bundles
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "bundles.h"


// snapmanager lib
//
#include "snapmanager/bundle.h"
#include "snapmanager/form.h"


// snapwebsites lib
//
#include <snapwebsites/file_content.h>
#include <snapwebsites/log.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>


// snapdev lib
//
#include <snapdev/join_strings.h>
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>
#include <snapdev/string_pathinfo.h>
#include <snapdev/tokenize_string.h>


// libaddr lib
//
#include <libaddr/addr_parser.h>
#include <libaddr/iface.h>


// Qt lib
//
#include <QFile>


// C lib
//
#include <sys/file.h>
#include <sys/sysinfo.h>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(bundles, 1, 0)


namespace
{


char const * g_configuration_fullname = "/etc/snapwebsites/snapwebsites.d/snapmanager.conf";


void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}

} // no name namespace



/** \brief Get a fixed bundles plugin name.
 *
 * The bundles plugin makes use of different fixed names. This function
 * ensures that you always get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_SNAPMANAGERCGI_BUNDLES_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_BUNDLES_...");

    }
    NOTREACHED();
}




/** \brief Initialize the bundles plugin.
 *
 * This function is used to initialize the bundles plugin object.
 */
bundles::bundles()
{
}


/** \brief Clean up the bundles plugin.
 *
 * Ensure the bundles object is clean before it is gone.
 */
bundles::~bundles()
{
}


/** \brief Get a pointer to the bundles plugin.
 *
 * This function returns an instance pointer to the bundles plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the bundles plugin.
 */
bundles * bundles::instance()
{
    return g_plugin_bundles_factory.instance();
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
QString bundles::description() const
{
    return "Manage the bundles installations.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString bundles::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in snapmanager.cgi and snapmanagerdaemon plugins.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t bundles::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize bundles.
 *
 * This function terminates the initialization of the bundles plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void bundles::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN  ( bundles, "server", snap_manager::manager, retrieve_status,          _1     );
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void bundles::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    {
        std::vector<std::string> const & bundle_uri(f_snap->get_bundle_uri());
        snap_manager::status_t const bundle(
                      bundle_uri.empty()
                            ? snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            : snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "bundle_uri"
                    , QString::fromUtf8(snap::join_strings(bundle_uri, ",").c_str()));
        server_status.set_field(bundle);
    }

    // if an upgrade is required, avoid offering users a way to install
    // something (this test is not rock solid, but we have another "instant"
    // test in the installer anyway, still that way we will avoid many
    // installation errors.)
    //
    if(f_snap->count_packages_that_can_be_updated(true).isEmpty())
    {
        retrieve_bundles_status(server_status);
    }
}


void bundles::retrieve_bundles_status(snap_manager::server_status & server_status)
{
    // TODO: make sure that the type of lock we use on the /var/lib/dpkg/lock
    //       file is indeed the one apt-get and Co. are using; note that the
    //       file does not get deleted between accesses
    //
    // if the lock created by dpkg and apt-get is in place, then do
    // nothing; note obviously that this is not a very good test since
    // we test the flag once and then go in a loop that's going to be
    // rather slow and a process may lock the database at that point
    //
    raii_fd_t const lock_fd(open("/var/lib/dpkg/lock", O_RDONLY | O_CLOEXEC, 0));
    if(lock_fd)
    {
        // the lock file exists, attempt a lock
        //
        if(::flock(lock_fd.get(), LOCK_EX) != 0)
        {
            return;
        }

        // the lock file gets inactived here
        //
        // (TBD: should we keep the lock active while running the next loop?)
    }

    // get the bundles
    //
    snap_manager::bundle::vector_t bundle_list(f_snap->load_bundles());
    for(auto & b : bundle_list)
    {
        std::string description;

        snap_manager::bundle::bundle_status_t const status(b->get_bundle_status());

        // no need for further work if this is hidden anyway
        //
        if(status == snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_HIDE)
        {
            continue;
        }

#ifdef _DEBUG
        // show the exact status in debug mode
        //
        description += "<p>DEBUG: Status = ";
        description += static_cast<char>(status);
        description += " (";
        description += std::to_string(static_cast<int>(status));
        description += ")</p>";
#endif

        // if in conflict, show that at the top
        //
        if(status == snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_IN_CONFLICT)
        {
            description += "<p class=\"in-conflict\">This bundle is in conflict with one or more other bundles:</p>";

            // TODO: when in conflict, you may be in conflict because of
            //       a prereq; i.e. your list of f_conflicts_bundles may
            //       be empty; at this point we don't have that case, but
            //       that's something we'd have to fix one day
            //
            description += "<ul>";
            bool found_conflicts(false);
            auto const & conflicts(b->get_conflicts_bundles());
            for(auto const & c : conflicts)
            {
                auto const & l(c.lock());
                if(l != nullptr)
                {
                    found_conflicts = true;
                    description += "<li><a href=\"#bundles::"
                                 + l->get_name()
                                 + "\">"
                                 + l->get_name()
                                 + "</a></li>";
                }
            }

            if(!found_conflicts)
            {
                description += "<li>This bundle was marked as being in conflict but no conflicting bundles were found.</li>";
            }
            description += "</ul>";
        }

        description += b->get_description();

        // go through each package to generate the list of packages
        // in our bundle description
        //
        auto const & packages(b->get_packages());
        if(packages.empty())
        {
            description += "<ul><li>No package names or versions for this bundle.</li></ul>";
        }
        else
        {
            description += "<ul>";
            for(auto p : packages)
            {
                auto const package(b->get_package(p));
                if(package->is_installed())
                {
                    description += "<li class='installed-package'>";
                    description += p;
                    description += " (";
                    description += package->get_version();
                    description += ")</li>";
                }
                else
                {
                    std::string const package_status(package->get_status());
                    description += "<li class='uninstalled-package'>";
                    description += p;
                    description += " (";
                    description += package_status.empty() ? "unknown" : package_status;
                    description += ")</li>";
                }
            }
            description += "</ul>";
        }

        snap_manager::status_t::state_t state(snap_manager::status_t::state_t::STATUS_STATE_INFO);
        switch(status)
        {
        case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_UNKNOWN:
            state = snap_manager::status_t::state_t::STATUS_STATE_ERROR;
            description += "<p>Bundle status could not be determined properly (UNKNOWN)</p>";
            break;

        case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_ERROR:
            state = snap_manager::status_t::state_t::STATUS_STATE_ERROR;
            description += "<p>Bundle status is in error, in most cases this means"
                           " the bundle XML file is invalid or a command/script"
                           " returned an unexpected error</p>";
            break;

        case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_HIDE:
            // if HIDE we do not even come here, we just skip the entire block
            break;

        case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_INSTALLED:
            description += "<p>This bundle is installed.</p>";
            break;

        case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_LOCKED:
            description += "<p>This bundle is installed and can't be uninstalled because another bundle depends on it:</p>";

            {
                bool found_lockers(false);
                auto const & locked(b->get_locked_by_bundles());
                for(auto const & p : locked)
                {
                    auto const & l(p.lock());
                    if(l != nullptr)
                    {
                        if(!found_lockers)
                        {
                            found_lockers = true;
                            description += "<ul>";
                        }
                        description += "<li><a href=\"#bundles::"
                                     + l->get_name()
                                     + "\">"
                                     + l->get_name()
                                     + "</a></li>";
                    }
                }

                if(found_lockers)
                {
                    description += "</ul>";
                }
            }
            break;

        case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_NOT_INSTALLED:
            description += "<p>This bundle is not currently installed.</p>";
            if(b->get_expected())
            {
                state = snap_manager::status_t::state_t::STATUS_STATE_WARNING;
                description += "<p><strong>We strongly suggest that you install this bundle on your system.</strong></p>";
            }
            break;

        case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_PREREQ_MISSING:
            description += "<p>This bundle can't be installed because pre-requisites are missing:</p>";

            {
                bool found_prereq(false);
                auto const & prereq(b->get_prereq_bundles());
                for(auto const & p : prereq)
                {
                    auto const & l(p.lock());
                    if(l != nullptr)
                    {
                        if(!found_prereq)
                        {
                            found_prereq = true;
                            description += "<ul>";
                        }
                        description += "<li><a href=\"#bundles::"
                                     + l->get_name()
                                     + "\">"
                                     + l->get_name()
                                     + "</a></li>";
                    }
                }

                if(found_prereq)
                {
                    description += "</ul>";
                }
            }
            break;

        case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_IN_CONFLICT:
            // already done at the top of the loop
            break;

        }

        {
            bool found_suggestions(false);
            auto const & suggestions(b->get_suggestions_bundles());
            for(auto const & s : suggestions)
            {
                auto const & l(s.lock());
                if(l != nullptr)
                {
                    if(!found_suggestions)
                    {
                        found_suggestions = true;
                        description += "<p>The following are bundles we suggest you install along this bundle:</p><ul>";
                    }
                    description += "<li><a href=\"#bundles::"
                                 + l->get_name()
                                 + "\">"
                                 + l->get_name()
                                 + "</a></li>";
                }
            }

            if(found_suggestions)
            {
                description += "</ul>";
            }
        }

        // we do not have to check the hide flag because we do not
        // execute this block when hidden (see at the top of the block)
        //
        snap_manager::status_t const package_status(
                    state,
                    get_plugin_name(),
                    QString::fromUtf8(b->get_name().c_str()),
                    QString::fromUtf8(description.c_str()));
        server_status.set_field(package_status);
    }
}



/** \brief Transform a value to HTML for display.
 *
 * This function expects the name of a field and its value. It then adds
 * the necessary HTML to the specified element to display that value.
 *
 * If the value is editable, then the function creates a form with the
 * necessary information (hidden fields) to save the data as required
 * by that field (i.e. update a .conf/.xml file, create a new file,
 * remove a file, etc.)
 *
 * \param[in] server_status  The map of statuses.
 * \param[in] s  The field being worked on.
 *
 * \return true if we handled this field.
 */
bool bundles::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "bundle_uri")
    {
        // the list of URIs from which we can download software bundles;
        // this should not be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "List of URIs to Directories of Bundles"
                        , s.get_field_name()
                        , s.get_value()
                        , QString("This is a list of comma separated URIs specifying the location of Directory Bundles. Usually, this is just one URI.")
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    // any other field must be a bundle definition
    //
    snap_manager::bundle::vector_t bundle_list(f_snap->load_bundles());

    auto it(std::find_if(
              bundle_list.begin()
            , bundle_list.end()
            , [&s](auto b)
            {
                return b->get_name() == s.get_field_name().toUtf8().data();
            }));
    if(it == bundle_list.end())
    {
        // we've got a pretty bug problem here?!
        //
        SNAP_LOG_ERROR("could not find your bundle inthe existing list of bundles we just loaded.");
        return false; // in effect we did not manage that field...
    }

    snap_manager::bundle::bundle_status_t const status((*it)->get_bundle_status());

    snap_manager::form::button_t buttons(snap_manager::form::FORM_BUTTON_NONE);

    switch(status)
    {
    case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_INSTALLED:
    //case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_LOCKED: -- first implement the batch then allow this one
        buttons |= snap_manager::form::FORM_BUTTON_UNINSTALL;
        break;

    case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_NOT_INSTALLED:
    //case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_PREREQ_MISSING: -- first implement the batch then allow this one
        buttons |= snap_manager::form::FORM_BUTTON_INSTALL;
        break;

    default:
        // no buttons
        break;

    }

    // offer the end user to install (not yet installed) or
    // uninstall (already installed) the bundle
    //
    snap_manager::form f(
              get_plugin_name()
            , s.get_field_name()
            , buttons);
    snap_manager::widget_description::pointer_t description_field(std::make_shared<snap_manager::widget_description>(
                      "Bundle Details"
                    , s.get_field_name()
                    , s.get_value()
                ));
    f.add_widget(description_field);

    // also add the fields, but only if necessary (i.e. if the bundle is
    // not yet installed and we have an INSTALL button, otherwise it's
    // really not useful)
    //
    switch(status)
    {
    case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_NOT_INSTALLED:
    //case snap_manager::bundle::bundle_status_t::BUNDLE_STATUS_PREREQ_MISSING: -- first implement the batch then allow this one
        for(auto const & fld : (*it)->get_fields())
        {
            if(fld->get_type() == "select")
            {
                snap_manager::widget_select::pointer_t install_field(std::make_shared<snap_manager::widget_select>(
                                  QString::fromUtf8(fld->get_label().c_str())
                                , QString("bundle_install_field::%1").arg(QString::fromUtf8(fld->get_name().c_str()))
                                , snap_string_list(fld->get_options())
                                , QString::fromUtf8(fld->get_initial_value().c_str())
                                , QString::fromUtf8(fld->get_description().c_str())
                            ));
                f.add_widget(install_field);
            }
            else
            {
                snap_manager::widget_input::pointer_t install_field(std::make_shared<snap_manager::widget_input>(
                                  QString::fromUtf8(fld->get_label().c_str())
                                , QString("bundle_install_field::%1").arg(QString::fromUtf8(fld->get_name().c_str()))
                                , QString::fromUtf8(fld->get_initial_value().c_str())
                                , QString::fromUtf8(fld->get_description().c_str())
                            ));
                f.add_widget(install_field);
            }
        }
        break;

    default:
        // in all other cases you don't need fields because you cannot
        // install this bundle (probably because it is already installed
        // or because it is in conflict with another bundle)
        break;

    }

    f.generate(parent, uri);

    return true;
}


/** \brief Save 'new_value' in field 'field_name'.
 *
 * This function saves 'new_value' in 'field_name'.
 *
 * \param[in] button_name  The name of the button the user clicked.
 * \param[in] field_name  The name of the field to update.
 * \param[in] new_value  The new value to save in that field.
 * \param[in] old_or_installation_value  The old value, just in case
 *            (usually ignored,) or the installation values (only
 *            for the bundles plugin that manages bundles.)
 *
 * \return true if the new_value was applied and the affected_services
 *         should be sent their RELOADCONFIG message.
 */
bool bundles::apply_setting(QString const & button_name
                          , QString const & field_name
                          , QString const & new_value
                          , QString const & old_or_installation_value
                          , std::set<QString> & affected_services)
{
    // restore defaults?
    //
    bool const use_default_value(button_name == "restore_default");

    if(field_name == "bundle_uri")
    {
        // if a failure happens, we do not create the last update time
        // file, that means we will retry to read the bundles each time;
        // so deleting that file is like requesting an immediate reload
        // of the bundles
        //
        QString const reset_filename(QString("%1/bundles.reset").arg(f_snap->get_bundles_path()));
        QFile reset_file(reset_filename);
        if(!reset_file.open(QIODevice::WriteOnly))
        {
            SNAP_LOG_WARNING("failed to create the \"")
                            (reset_filename)
                            ("\", changes to the bundles URI may not show up as expected.");
        }

        affected_services.insert("snapmanagerdaemon");

        QString value(new_value);
        if(use_default_value)
        {
            value = "http://bundles.snapwebsites.info/";
        }

        // TODO: the path to the snapmanager.conf is hard coded, it needs to
        //       use the path of the file used to load the .conf in the
        //       first place (I'm just not too sure how to get that right
        //       now, probably from the "--config" parameter, but how do
        //       we do that for each service?) -- I may be able to use
        //       the snap::config interface to get to it?
        //
        NOTUSED(f_snap->replace_configuration_value(g_configuration_fullname, field_name, value));
        return true;
    }

    // installation is a special case in the "bundles" plugin only (or at least
    // it should most certainly only be specific to this plugin.)
    //
    bool const install(button_name == "install");
    if(install
    || button_name == "uninstall")
    {
        //if(!field_name.startsWith("bundle::"))
        //{
        //    SNAP_LOG_ERROR("install or uninstall with field_name \"")
        //                  (field_name)
        //                  ("\" is invalid, we expected a name starting with \"bundle::\".");
        //    return false;
        //}
        QByteArray values(old_or_installation_value.toUtf8());
        NOTUSED(f_snap->installer(field_name, install ? "install" : "purge", values.data(), affected_services));
        return true;
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
