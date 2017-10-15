// Snap Websites Server -- handle credit card data for other plugins
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

#include "epayment_creditcard.h"

#include "../messages/messages.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(epayment_creditcard, 1, 0)



/* \brief Get a fixed path name.
 *
 * The path plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_DEFAULT_COUNTRY:
        return "epayment_creditcard::default_country";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_GATEWAY:
        return "epayment_creditcard::gateway";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SETTINGS_PATH:
        return "admin/settings/epayment/credit-card";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_ADDRESS2:
        return "epayment_creditcard::show_address2";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_BUSINESS_NAME:
        return "epayment_creditcard::show_business_name";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_COUNTRY:
        return "epayment_creditcard::show_country";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_DELIVERY:
        return "epayment_creditcard::show_delivery";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_ONE_NAME:
        return "epayment_creditcard::show_one_name";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_PHONE:
        return "epayment_creditcard::show_phone";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_PROVINCE:
        return "epayment_creditcard::show_province";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_USER_ALLOWS_SAVING_TOKEN: // TODO: make this "magically" appear on the user's profiles
        return "epayment_creditcard::user_allows_saving_token";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_EPAYMENT_CREDITCARD_...");

    }
    NOTREACHED();
}


/** \brief Initialize the epayment_creditcard plugin.
 *
 * This function is used to initialize the epayment_creditcard plugin object.
 *
 * \todo
 * Add support for a list of countries and whether they support a postal
 * code since we currently make the zip code a mandatory field...
 * List of countries and whether they have a zip code:
 *
 * https://en.wikipedia.org/wiki/List_of_postal_codes
 *
 * \todo
 * Add support for currencies per country. We want to support currencies
 * so customers may not need to pay extra fees (i.e. that way we can
 * charge the card in their currency and they avoid the conversion...
 * but we have to have a way to know, at least more or less, the
 * change for that currency.)
 */
epayment_creditcard::epayment_creditcard()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the epayment_creditcard plugin.
 *
 * Ensure the epayment_creditcard object is clean before it is gone.
 */
epayment_creditcard::~epayment_creditcard()
{
}


/** \brief Get a pointer to the epayment_creditcard plugin.
 *
 * This function returns an instance pointer to the epayment_creditcard plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the epayment_creditcard plugin.
 */
epayment_creditcard * epayment_creditcard::instance()
{
    return g_plugin_epayment_creditcard_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString epayment_creditcard::settings_path() const
{
    return get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SETTINGS_PATH);
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString epayment_creditcard::icon() const
{
    return "/images/epayment/epayment-credit-card-logo-64x64.png";
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
QString epayment_creditcard::description() const
{
    return "Generate a credit card form that the end user is expected to"
          " fill in. This plugin is generally not installed by itself,"
          " instead it is marked as a dependency of a plugin that is"
          " capable of processing credit cards.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString epayment_creditcard::dependencies() const
{
    return "|date_widgets|editor|epayment|messages|path|permissions|users|";
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
int64_t epayment_creditcard::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 5, 6, 23, 33, 16, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update (in
 *                                 micro-seconds).
 */
void epayment_creditcard::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the epayment_creditcard.
 *
 * This function terminates the initialization of the epayment_creditcard plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void epayment_creditcard::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(epayment_creditcard, "server", server, process_post, _1);
    SNAP_LISTEN(epayment_creditcard, "editor", editor::editor, dynamic_editor_widget, _1, _2, _3);
    SNAP_LISTEN(epayment_creditcard, "editor", editor::editor, save_editor_fields, _1);
}


/** \brief Accept a POST to request information about the server.
 *
 * This function manages the data sent to the server by a client script.
 * In many cases, it is used to know whether something is true or false,
 * although the answer may be any valid text.
 *
 * The function verifies that the "editor_session" variable is set, if
 * not it ignores the POST sine another plugin may be the owner.
 *
 * \note
 * This function is a server signal generated by the snap_child
 * execute() function.
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void epayment_creditcard::on_process_post(QString const & uri_path)
{
    NOTUSED(uri_path);
}



/** \brief Dynamically tweak the credit card form.
 *
 * The settings allow the user to enter his credit card information.
 * Only that form includes parameters that may not be useful to all
 * website owners. The settings let the owner of a site turn off a
 * few fields and this function executes those orders.
 *
 * First the functions checks whether this is the credit card form
 * being handled, if not, it returns immediately.
 *
 * If it is the credit card form, it then reads the settings and
 * compeletely removes the second line of address, the province/state
 * field, and the country. At some point we will want to offer a way
 * for users to define the billing address.
 *
 * If a default country name was defined, it is also saved in that
 * field assuming the field is not being removed.
 */
void epayment_creditcard::on_dynamic_editor_widget(
        content::path_info_t & ipath,
        QString const & name,
        QDomDocument & editor_widgets)
{
    NOTUSED(name);

    // are we dealing with the epayment credit card form?
    //
    QDomElement root(editor_widgets.documentElement());
    if(root.isNull())
    {
        return;
    }
    QString const owner_name(root.attribute("owner"));
    if(owner_name != "epayment_creditcard")
    {
        return;
    }
    snap_uri const & main_uri(f_snap->get_uri());
    content::path_info_t main_ipath;
    main_ipath.set_path(main_uri.path());
    if(main_ipath.get_cpath() != ipath.get_cpath())
    {
        // this happens when generating lists and such
        return;
    }
    QString const form_id(root.attribute("id"));
    if(form_id == "creditcard_form")
    {
        setup_form(ipath, editor_widgets);
    }
    else if(form_id == "settings")
    {
        setup_settings(editor_widgets);
    }
}


void epayment_creditcard::setup_form(
        content::path_info_t & ipath,
        QDomDocument & editor_widgets)
{
    // read the settings
    //
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    content::path_info_t epayment_creditcard_settings_ipath;
    epayment_creditcard_settings_ipath.set_path(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SETTINGS_PATH));
    if(!content_table->exists(epayment_creditcard_settings_ipath.get_key())
    || !content_table->row(epayment_creditcard_settings_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        // the form by default is what we want if no settings were defined
        return;
    }
    QtCassandra::QCassandraRow::pointer_t settings_row(revision_table->row(epayment_creditcard_settings_ipath.get_revision_key()));

    // remove unwanted widgets if the administrator required so...
    //

    // delivery
    //
    {
        bool const show_delivery(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_DELIVERY))->value().safeSignedCharValue(0, 1) != 0);
        if(!show_delivery)
        {
            char const * delivery_fields[] =
            {
                "delivery_business_name",
                "delivery_attention",
                "delivery_address1",
                "delivery_address2",
                "delivery_city",
                "delivery_province",
                "delivery_postal_code",
                "delivery_country"
            };
            // forget all of those widgets
            for(size_t idx(0); idx < sizeof(delivery_fields) / sizeof(delivery_fields[0]); ++idx)
            {
                QDomXPath dom_xpath;
                dom_xpath.setXPath(QString("/editor-form/widget[@id='%1']").arg(delivery_fields[idx]));
                QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
                if(result.size() > 0
                && result[0].isElement())
                {
                    result[0].parentNode().removeChild(result[0]);
                }
            }
        }
    }

    // one name
    //
    {
        bool const show_one_name(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_ONE_NAME))->value().safeSignedCharValue(0, 1) != 0);
        if(show_one_name) // WARNING: here we test the flag INVERTED! (default is hide those fields)
        {
            // forget that widget
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/editor-form/widget[@id='billing_attention']");
            QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
            dom_xpath.setXPath("/editor-form/widget[@id='delivery_attention']");
            result = dom_xpath.apply(editor_widgets);
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
        }
    }

    // business name
    //
    {
        bool const show_business_name(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_BUSINESS_NAME))->value().safeSignedCharValue(0, 1) != 0);
        if(!show_business_name)
        {
            // forget that widget
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/editor-form/widget[@id='billing_business_name']");
            QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
            dom_xpath.setXPath("/editor-form/widget[@id='delivery_business_name']");
            result = dom_xpath.apply(editor_widgets);
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
        }
    }

    // address2
    //
    {
        bool const show_address2(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_ADDRESS2))->value().safeSignedCharValue(0, 1) != 0);
        if(!show_address2)
        {
            // forget that widget
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/editor-form/widget[@id='billing_address2']");
            QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
            dom_xpath.setXPath("/editor-form/widget[@id='delivery_address2']");
            result = dom_xpath.apply(editor_widgets);
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
        }
    }

    // country
    //
    {
        bool const show_country(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_COUNTRY))->value().safeSignedCharValue(0, 1) != 0);
        if(!show_country)
        {
            // forget that widget
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/editor-form/widget[@id='billing_country']");
            QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
            dom_xpath.setXPath("/editor-form/widget[@id='delivery_country']");
            result = dom_xpath.apply(editor_widgets);
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
        }
        else
        {
            // setup the default if there is one and we did not remove the
            // widget
            QString const default_country(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_DEFAULT_COUNTRY))->value().stringValue());
            if(!default_country.isEmpty())
            {
                QDomXPath dom_xpath;
                dom_xpath.setXPath("/editor-form/widget[@id='billing_country']");
                QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
                if(result.size() > 0
                && result[0].isElement())
                {
                    QDomElement default_value(editor_widgets.createElement("value"));
                    result[0].appendChild(default_value);
                    snap_dom::append_plain_text_to_node(default_value, default_country);
                }
                dom_xpath.setXPath("/editor-form/widget[@id='delivery_country']");
                result = dom_xpath.apply(editor_widgets);
                if(result.size() > 0
                && result[0].isElement())
                {
                    QDomElement default_value(editor_widgets.createElement("value"));
                    result[0].appendChild(default_value);
                    snap_dom::append_plain_text_to_node(default_value, default_country);
                }
            }
        }
    }

    // province
    //
    {
        bool const show_province(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_PROVINCE))->value().safeSignedCharValue(0, 1) != 0);
        if(!show_province)
        {
            // forget that widget
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/editor-form/widget[@id='billing_province']");
            QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
            dom_xpath.setXPath("/editor-form/widget[@id='delivery_province']");
            result = dom_xpath.apply(editor_widgets);
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
        }
    }

    // phone
    //
    {
        int const show_phone(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_PHONE))->value().safeSignedCharValue(0, 1));
        switch(show_phone)
        {
        case 0: // Hide phone number
        case 2: // Required phone number
            {
                // forget that widget
                QDomXPath dom_xpath;
                dom_xpath.setXPath("/editor-form/widget[@id='phone']");
                QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
                if(result.size() > 0
                && result[0].isElement())
                {
                    if(show_phone == 0)
                    {
                        result[0].parentNode().removeChild(result[0]);
                    }
                    else
                    {
                        QDomElement required_tag(editor_widgets.createElement("required"));
                        result[0].appendChild(required_tag);
                        snap_dom::append_plain_text_to_node(required_tag, "required");
                    }
                }
            }
            break;

        }
    }

    // gateway
    //
    {
        QString gateway;

        snap_uri const & main_uri(f_snap->get_uri());

        if(ipath.get_cpath() == "admin/settings/epayment/credit-card-test")
        {
            // for the test, force this plugin
            //
            gateway = "epayment_creditcard";

            if(main_uri.has_query_option("gateway"))
            {
                messages::messages * messages(messages::messages::instance());
                messages->set_warning(
                    "Specified Gateway Ignored",
                    "The ?gateway=... parameter is always ignored on the credit card test page.",
                    "For security reasons, we completely ignore the gateway=... parameter on this page."
                );
            }
        }
        else
        {
            // for any other form, make sure the user defined a gateway
            //
            if(main_uri.has_query_option("gateway"))
            {
                // "user" specified a gateway in the URI
                //
                gateway = main_uri.query_option("gateway");
            }
            else
            {
                // no gateway in the URI, try with the default
                //
                gateway = settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_GATEWAY))->value().stringValue();
            }

            if(gateway.isEmpty()
            || gateway == "no-default")
            {
                throw epayment_creditcard_exception_gateway_missing("the \"?gateway=<plugin-name>\" is mandatory when loading a credit card form and no default gateway is defined");
            }

            if(!plugins::exists(gateway))
            {
                throw epayment_creditcard_exception_gateway_missing(QString("could not find plugin \"%1\" to process credit card.").arg(gateway));
            }
        }

        {
            // save the name of the gateway in the form
            //
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/editor-form/widget[@id='gateway']");
            QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
            if(result.size() > 0
            && result[0].isElement())
            {
                QDomElement value(editor_widgets.createElement("value"));
                result[0].appendChild(value);
                snap_dom::append_plain_text_to_node(value, gateway);
            }
        }
    }

    // from URI
    //
    {
        QString from;

        snap_uri const & main_uri(f_snap->get_uri());
        if(main_uri.has_query_option("from"))
        {
            from = main_uri.query_option("from");
        }

        {
            // save the from URI in the corresponding widget if
            // defined, otherwise remove the widget
            //
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/editor-form/widget[@id='from']");
            QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
            if(result.size() > 0
            && result[0].isElement())
            {
                if(!from.isEmpty())
                {
                    QDomElement value(editor_widgets.createElement("value"));
                    result[0].appendChild(value);
                    snap_dom::append_plain_text_to_node(value, from);
                }
                else
                {
                    result[0].parentNode().removeChild(result[0]);
                }
            }
        }
    }
}


void epayment_creditcard::setup_settings(QDomDocument & editor_widgets)
{
    QDomXPath dom_xpath;
    dom_xpath.setXPath("/editor-form/widget[@id='gateway']/preset");
    QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
    if(result.size() > 0
    && result[0].isElement())
    {
        // retrieve the list of gateways and display them in the settings.
        //
        QMap<QString, QString> gateways;
        plugins::plugin_vector_t const & plugin_list(plugins::get_plugin_vector());
        size_t const max_plugins(plugin_list.size());
        for(size_t idx(0); idx < max_plugins; ++idx)
        {
            epayment_creditcard_gateway_t * gateway(dynamic_cast<epayment_creditcard_gateway_t *>(plugin_list[idx]));
            if(gateway != nullptr)
            {
                epayment_gateway_features_t gateway_info(plugin_list[idx]->get_plugin_name());
                gateway->gateway_features(gateway_info);

                // save in temporary map so it gets sorted alphabetically
                // (assuming all names are English it will be properly
                // sorted...)
                //
                // TBD: should we not add the "epayment_creditcard" test
                //      gateway here (since it really do absolutely nothing
                //      and thus you cannot even test what happens if an
                //      invoice gets paid...)
                //
                gateways[gateway_info.get_name()] = gateway_info.get_gateway();
            }
        }

        // now that we have a complete list, generate the <item> entries
        //
        for(QMap<QString, QString>::const_iterator it(gateways.begin());
                                                   it != gateways.end();
                                                   ++it)
        {
            QDomElement item(editor_widgets.createElement("item"));
            item.setAttribute("value", it.value());
            snap_dom::append_plain_text_to_node(item, it.key());
            result[0].appendChild(item);
        }
    }
}


/** \brief Gather the credit card information and generate a specific signal.
 *
 * This function is called whenever the credit card form is saved by
 * the editor processes. It does three main things:
 *
 * \li Validation
 *
 * First it makes sure that the data was validated without errors. If there
 * was an error, the credit card data received should not be processed sine
 * it is likely not going to work.
 *
 * \li Reformat Data
 *
 * Second the function reads all the available data and saves it in a
 * structure making it a lot easier for further processing to take place.
 *
 * For example, it is much easier to receive a "struct tm" for the
 * expiration date than having to go the post environment and get
 * the partial date used in the credit card form.
 *
 * \li Signal Processing Gateways
 *
 * Third the function calls a gateway callback. The form will include the
 * name of a gateway, which for us is the name of a plugin. This function
 * searches for that plugin and expects it to have the
 * epayment_creditcard_gateway_t interface implemented.
 *
 * \param[in,out] save_info  The editor form being saved.
 */
void epayment_creditcard::on_save_editor_fields(editor::save_info_t & save_info)
{
    // on errors, forget it immediately, whatever form this is
    //
    if(save_info.has_errors())
    {
        return;
    }

    // are we dealing with the epayment credit card form?
    //
    QDomElement root(save_info.editor_widgets().documentElement());
    if(root.isNull())
    {
        // no widgets?!
        return;
    }
    QString const owner_name(root.attribute("owner"));
    if(owner_name != "epayment_creditcard")
    {
        // not the expected owner
        return;
    }
    QString const form_id(root.attribute("id"));
    if(form_id != "creditcard_form")
    {
        // not the expected form
        return;
    }

    // get the settings ready
    //
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    content::path_info_t epayment_creditcard_settings_ipath;
    epayment_creditcard_settings_ipath.set_path(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SETTINGS_PATH));
    if(!content_table->exists(epayment_creditcard_settings_ipath.get_key())
    || !content_table->row(epayment_creditcard_settings_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        // the form by default is what we want if no settings were defined
        return;
    }
    QtCassandra::QCassandraRow::pointer_t settings_row(revision_table->row(epayment_creditcard_settings_ipath.get_revision_key()));

    // retrieve the data and save it in a epayment_creditcard_info_t object
    //
    epayment_creditcard_info_t creditcard_info;

    // information about credit card itself
    //
    editor::editor * editor_plugin(editor::editor::instance());
    creditcard_info.set_user_name(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("user_name"))));

    // remove spaces and dashes from card number
    QString card_number(snap_dom::unescape(f_snap->postenv("card_number")));
    card_number = card_number.replace(" ", "").replace("-", "");
    creditcard_info.set_creditcard_number(editor_plugin->clean_post_value("line-edit", card_number));

    creditcard_info.set_security_code(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("security_code"))));

    // small processing on the expiration date
    QString const expiration_date(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("expiration_date"))));
    snap_string_list const expiration_date_parts(expiration_date.split('/'));
    if(expiration_date_parts.size() < 2)
    {
        SNAP_LOG_FATAL("could not save the epayment_creditcard data because the expiration date is invalid.");

        messages::messages * messages(messages::messages::instance());
        messages->set_error(
            "Invalid Expiration Date",
            // WARNING: DO NOT INCLUDE THE EXPIRATION DATE, it is not supposed
            //          to be saved anywhere unless properly encrypted
            "We could not process your payment, the expiration date is invalid.",
            "The expiration date is expected to be 'MM/YY', the '/' is missing.",
            false
        );

        return;
    }
    creditcard_info.set_expiration_date_month(expiration_date_parts[1]);
    creditcard_info.set_expiration_date_year(expiration_date_parts[0]);

    // billing address
    //
    creditcard_info.set_billing_business_name(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("billing_business_name"))));
    creditcard_info.set_billing_attention(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("billing_attention"))));
    creditcard_info.set_billing_address1(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("billing_address1"))));
    // address2 may be hidden in which case it ends up empty, which is fine
    creditcard_info.set_billing_address2(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("billing_address2"))));
    creditcard_info.set_billing_city(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("billing_city"))));
    // province may be hidden in which case it ends up empty, which is fine
    creditcard_info.set_billing_province(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("billing_province"))));
    creditcard_info.set_billing_postal_code(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("billing_postal_code"))));

    // country may be hidden and have a default instead
    //
    {
        bool const show_country(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_COUNTRY))->value().safeSignedCharValue(0, 1) != 0);
        if(!show_country)
        {
            // user could not enter a country, administrator may have
            // a default though...
            //
            creditcard_info.set_billing_country(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_DEFAULT_COUNTRY))->value().stringValue());
        }
        else
        {
            creditcard_info.set_billing_country(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("billing_country"))));
        }
    }

    // delivery address
    //
    creditcard_info.set_delivery_business_name(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("delivery_business_name"))));
    creditcard_info.set_delivery_attention(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("delivery_attention"))));
    creditcard_info.set_delivery_address1(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("delivery_address1"))));
    // address2 may be hidden in which case it ends up empty, which is fine
    creditcard_info.set_delivery_address2(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("delivery_address2"))));
    creditcard_info.set_delivery_city(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("delivery_city"))));
    // province may be hidden in which case it ends up empty, which is fine
    creditcard_info.set_delivery_province(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("delivery_province"))));
    creditcard_info.set_delivery_postal_code(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("delivery_postal_code"))));

    // country may be hidden and have a default instead
    //
    {
        bool const show_country(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_COUNTRY))->value().safeSignedCharValue(0, 1) != 0);
        if(!show_country)
        {
            // user could not enter a country, administrator may have
            // a default though...
            //
            // TBD: should we check whether the delivery address should be
            //      added and if not avoid this call?
            //
            creditcard_info.set_delivery_country(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_DEFAULT_COUNTRY))->value().stringValue());
        }
        else
        {
            creditcard_info.set_delivery_country(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("delivery_country"))));
        }
    }

    // other fields
    //
    creditcard_info.set_phone(editor_plugin->clean_post_value("line-edit", snap_dom::unescape(f_snap->postenv("phone"))));

    // the data is ready, search for the gateway (a plugin)
    //
    QString const gateway(f_snap->postenv("gateway"));
    plugins::plugin * gateway_plugin(plugins::get_plugin(gateway));
    if(gateway_plugin == nullptr)
    {
        // this should not happen since it was tested in the generation of
        // the form, but an administrator may have turned off that gateway
        // in between...
        //
        throw epayment_creditcard_exception_gateway_missing(QString("could not find plugin \"%1\" to process credit card.").arg(gateway));
    }
    epayment_creditcard_gateway_t * gateway_processor(dynamic_cast<epayment_creditcard_gateway_t *>(gateway_plugin));
    if(gateway_processor == nullptr)
    {
        // this can definitely happen since a hacker could specify the
        // name of a different plugin before returning the form; it
        // should not matter though (it is safe) as long as we make
        // sure that the gateway_processor pointer is not null...
        //
        throw epayment_creditcard_exception_gateway_missing(QString("plugin \"%1\" is not capable of processing credit cards.").arg(gateway));
    }

    // we are on
    //
    SNAP_LOG_INFO("Processing a credit card with \"")(gateway)("\".");
    // also log the name of the person, but only in the secure logs
    SNAP_LOG_INFO(logging::log_security_t::LOG_SECURITY_SECURE)
            ("Processing \"")(creditcard_info.get_user_name())("\"'s credit card with \"")(gateway)("\".");

    // This actually processes the data (i.e. sends the credit card
    // information to the bank's gateway and return with PAID or FAILED.)
    // As far as the epayment_creditcard plugin is concerned, the result
    // of the processing are ignored here.
    //
    if(gateway_processor->process_creditcard(creditcard_info, save_info))
    {
        // redirect the user to the Thank You page
        //
        // TODO: look into redirecting to the correct page, i.e.
        //       o Thank You for Your Payment, or
        //       o Thank You for Your Subscription
        //
        server_access::server_access * server_access_plugin(server_access::server_access::instance());
        server_access_plugin->ajax_redirect(creditcard_info.get_subscription()
                            ? "/epayment/thank-you-subscription"
                            : "/epayment/thank-you",
                    "_top");
    }
}


/** \brief Define the test gateway.
 *
 * This function is a callback that is used by the system whenever it
 * wants to offer a specific gateway to process credit cards.
 *
 * \param[in,out] gateway_info  The structure that takes the various gateway
 *                              details.
 */
void epayment_creditcard::gateway_features(epayment_gateway_features_t & gateway_info)
{
    gateway_info.set_name("Credit Card Test Gateway");
}


/** \brief Test a credit card processing.
 *
 * This function is used to test the credit card processing mechanism.
 * The function just logs a message to let you know that it worked.
 *
 * \param[in,out] creditcard_info  The credit card information.
 * \param[in,out] save_info  The information from the editor form.
 */
bool epayment_creditcard::process_creditcard(epayment_creditcard_info_t & creditcard_info, editor::save_info_t & save_info)
{
    NOTUSED(creditcard_info);
    NOTUSED(save_info);

    SNAP_LOG_INFO("epayment_creditcard::process_creditcard() called.");

#ifdef _DEBUG
// For debug purposes, you may check all the values with the following
std::cerr << "cc user_name [" << creditcard_info.get_user_name() << "]\n";
std::cerr << "cc number [" << creditcard_info.get_creditcard_number() << "]\n";
std::cerr << "cc security_code [" << creditcard_info.get_security_code() << "]\n";
std::cerr << "cc expiration_date_month [" << creditcard_info.get_expiration_date_month() << "]\n";
std::cerr << "cc expiration_date_year [" << creditcard_info.get_expiration_date_year() << "]\n";

std::cerr << "cc billing_business_name [" << creditcard_info.get_billing_business_name() << "]\n";
std::cerr << "cc billing_attention [" << creditcard_info.get_billing_attention() << "]\n";
std::cerr << "cc billing_address1 [" << creditcard_info.get_billing_address1() << "]\n";
std::cerr << "cc billing_address2 [" << creditcard_info.get_billing_address2() << "]\n";
std::cerr << "cc billing_city [" << creditcard_info.get_billing_city() << "]\n";
std::cerr << "cc billing_province [" << creditcard_info.get_billing_province() << "]\n";
std::cerr << "cc billing_postal_code [" << creditcard_info.get_billing_postal_code() << "]\n";
std::cerr << "cc billing_country [" << creditcard_info.get_billing_country() << "]\n";

std::cerr << "cc delivery_business_name [" << creditcard_info.get_delivery_business_name() << "]\n";
std::cerr << "cc delivery_attention [" << creditcard_info.get_delivery_attention() << "]\n";
std::cerr << "cc delivery_address1 [" << creditcard_info.get_delivery_address1() << "]\n";
std::cerr << "cc delivery_address2 [" << creditcard_info.get_delivery_address2() << "]\n";
std::cerr << "cc delivery_city [" << creditcard_info.get_delivery_city() << "]\n";
std::cerr << "cc delivery_province [" << creditcard_info.get_delivery_province() << "]\n";
std::cerr << "cc delivery_postal_code [" << creditcard_info.get_delivery_postal_code() << "]\n";
std::cerr << "cc delivery_country [" << creditcard_info.get_delivery_country() << "]\n";

std::cerr << "cc phone [" << creditcard_info.get_phone() << "]\n";
#endif

    return true;
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
