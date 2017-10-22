// Snap Websites Server -- form handling
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

#include "form.h"

#include "../content/content.h"
#include "../messages/messages.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomreceiver.h>
#include <snapwebsites/qdomxpath.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/qxmlmessagehandler.h>
#include <snapwebsites/xslt.h>

#include <QFile>
#include <QFileInfo>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(form, 1, 0)


namespace
{

/** \brief The next unique tab index.
 *
 * Any form on the screen needs to make use of unique tab indices. This
 * value represents the current index. You get it with the current_tab_id()
 * function. Once done with it, you call the used_tab_id() function to
 * mark those identifiers as used.
 */
int64_t g_tabindex_base = 0;

}


/** \brief Get an HTML form from an XML document.
 *
 * The form plugin makes use of a set of XSTL templates to generate HTML
 * forms from simple XML documents.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t const name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_FORM_FORM:
        return "form::form";

    case name_t::SNAP_NAME_FORM_NAMESPACE:
        return "form";

    case name_t::SNAP_NAME_FORM_PATH:
        return "form::path";

    case name_t::SNAP_NAME_FORM_RESOURCE:
        return "form::resource";

    case name_t::SNAP_NAME_FORM_SETTINGS:
        return "form::settings";

    case name_t::SNAP_NAME_FORM_SOURCE:
        return "form::source";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_FORM_...");

    }
    NOTREACHED();
}

/** \brief Initialize the form plugin.
 *
 * This function initializes the form plugin.
 */
form::form()
    //: f_snap(nullptr) -- auto-init
    //, f_form_initialized(false) -- auto-init
    //: f_form_elements("form-xslt")
{
}

/** \brief Destroy the form plugin.
 *
 * This function cleans up the form plugin.
 */
form::~form()
{
}


/** \brief Get a pointer to the form plugin.
 *
 * This function returns an instance pointer to the form plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the form plugin.
 */
form * form::instance()
{
    return g_plugin_form_factory.instance();
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
QString form::description() const
{
    return "The form plugin is used to generate forms from simple XML"
        " documents. This plugin uses an XSLT template to process"
        " the the XML data. This plugin is a required backend plugin.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString form::dependencies() const
{
    return "|content|filter|messages|sessions|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last
 *                          updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t form::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 9, 19, 2, 9, 8, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the form plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void form::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Bootstrap the form.
 *
 * This function adds the events the form plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void form::bootstrap(::snap::snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(form, "server", server, process_post, _1);
    SNAP_LISTEN(form, "content", content::content, copy_branch_cells, _1, _2, _3);
    SNAP_LISTEN(form, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(form, "layout", layout::layout, filtered_content, _1, _2, _3);
}


/** \brief Transform an XML form into an HTML document.
 *
 * Transforms the specified XML document using a set of XSTL templates
 * into an HTML document.
 *
 * Each time this function is called a new form identifier is generated.
 * It is important to understand that this means the same XML form will
 * output different HTML data every time this function is called.
 *
 * \param[in] info  The session information used to register this form
 * \param[in] xml_form  The XML document (a form) to transform to HTML.
 *
 * \return The resulting HTML document.
 */
QDomDocument form::form_to_html(sessions::sessions::session_info & info, QDomDocument & xml_form)
{
    static int64_t g_unique_id(0);

    QDomDocument doc_output("body");
    if(!f_form_initialized)
    {
        QFile file(":/xsl/form/core-form.xsl");
        if(!file.open(QIODevice::ReadOnly))
        {
            SNAP_LOG_FATAL("form::form_to_html() could not open core-form.xsl resource file.");
            return doc_output;
        }
        // WARNING WARNING WARNING
        // Do not turn on the namespaces because otherwise it gets all messed
        // up by the toString() below (it is to wonder how messed up it must be
        // in memory.)
        if(!f_form_elements.setContent(&file, false))
        {
            SNAP_LOG_FATAL("form::form_to_html() could not parse core-form.xsl resource file.");
            return doc_output;
        }
        QDomNode p(f_form_elements.firstChild());
        while(!p.isElement())
        {
            // the first node may not be an element since we have comments
            if(p.isNull())
            {
                // well... nothing found?
                SNAP_LOG_FATAL("form::form_to_html() could not find the first element.");
                return doc_output;
            }
            p = p.nextSibling();
        }
        QDomElement stylesheet(p.toElement());
        if(stylesheet.tagName() != "xsl:stylesheet")
        {
            // we only can handle stylesheets
            SNAP_LOG_FATAL("form::form_to_html() the first element is not a stylesheet.");
            return doc_output;
        }
        f_form_stylesheet = stylesheet;

        // give other plugins a chance to add their own widgets to the XSTL
        // (this is used to extend the capability of Snap! forms)
        form_element(this);
        f_form_elements_string = f_form_elements.toString(-1);
//printf("form [%s]\n", f_form_elements_string.toUtf8().data());
        f_form_initialized = true;
    }

    //content::permission_flag can_edit;
    //path::path::instance()->access_allowed(
    //        users::users::instance()->get_user_path(),
    //        ipath,
    //        "edit",
    //        permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED),
    //        result);

    // IMPORTANT NOTE:
    // Forms are nearly NOT modified, although we have to allow plugins to
    // setup the form "default" values (i.e. if you saved a text entry
    // with the word "foo", when you come back to that form you probably
    // want the word "foo" in there, unless the field is a secret field
    // or the form is not to be saved, i.e. a search like form.)
    //
    // Note that this update should only change the <value> tags, NOT the
    // form itself (add/remove widgets). Form modifications are only allowed
    // after the load of the form in the tweak_form() signal.
    //
    // Note that we cannot really avoid filling the form here even if the
    // POST even just filled the last post information. The main reason is
    // that certain fields (mainly the File based fields at this time) need
    // to be set here if optional. (Also at this time a POST does not
    // properly save attachments if the form generates an error!)
    auto_fill_form(xml_form);

    ++g_unique_id;

    xslt x;
    x.add_variable("_form_session", QVariant(sessions::sessions::instance()->create_session(info)));
    x.add_variable("action",        QVariant(info.get_page_path()));
    x.add_variable("unique_id",     QVariant(QString("%1").arg(g_unique_id)));
    x.add_variable("tabindex_base", QVariant(current_tab_id()));
    //x.add_variable("can_edit",      QVariant(QString(can_edit.allowed() ? "yes" : "")));
    x.set_xsl(f_form_elements_string);
    x.set_document(xml_form);
    x.evaluate_to_document(doc_output);

    // the count includes all the widgets even those that do not make
    // use of the tab index so we will get some gaps, but that is a very
    // small price to pay for this cool feature
    used_tab_id(xml_form.elementsByTagName("widget").size());

    return doc_output;
}


/** \brief Automatically fill the form.
 *
 * This function automatically fills the fields of a form with data
 * from the database. Forms that accept defaults should have those
 * defaults predefined in the XML data.
 *
 * For example, the favicon plugin offers the user to define whether
 * only the main favicon file should be used. If so a flag is set to
 * true. That value can be changed by going to the settings at:
 *
 * \code
 * .../admin/settings/favicon
 * \endcode
 *
 * The form is defined in the plugin and it has a corresponding entry
 * in the favicon content.xml. That entry could include a line like
 * this to create the default parameter:
 *
 * \code
 * <param name="favicon::sitewide" type="int8">1</param>
 * \endcode
 *
 * Only "ultra"-dynamic defaults are to be added through the
 * fill_form_widget() signal. Actually, it is very unlikely that any form
 * will use that signal since in most cases the defaults and the last
 * saved values come from the database.
 *
 * \warning
 * This function is NOT called if the form was just POSTed. This is because
 * the default values for all the fields are taken from what the user just
 * sent us and not the default or what's in the database.
 *
 * \param[in] xml_form  The form that's being saved.
 */
void form::auto_fill_form(QDomDocument xml_form)
{
    // Get the root element
    QDomElement snap_form(xml_form.documentElement());

    // retrieve the cpath of the form (i.e. where the form is to be posted.)
    QString const cpath(snap_form.attribute("path"));

    // make sure that row exists
    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const key(site_key + cpath);
    if(!content_table->exists(key))
    {
        // the row does not exist yet... the form should not even be
        // in auto-save mode!?
        return;
    }
    libdbproxy::row::pointer_t row(content_table->getRow(key));

    // if we have an auto-save, then we can auto-load too
    // otherwise only let the user plugin take care of the auto-fill
    QString const auto_save_str(snap_form.attribute("auto-save"));
    bool const auto_save(!auto_save_str.isEmpty());

    QString const owner(snap_form.attribute("owner"));

    QDomNodeList widgets(xml_form.elementsByTagName("widget"));
    int const count(widgets.length());
    for(int i(0); i < count; ++i)
    {
        QDomNode w(widgets.item(i));
        if(!w.isElement())
        {
            throw form_exception_invalid_form_xml("elementsByTagName() returned a node that is not an element");
        }
        QDomElement widget(w.toElement());

        // secrets are never sent back to the client!
        // (i.e. these are passwords and such)
        QString const secret(widget.attribute("secret"));
        if(secret == "secret")
        {
            continue;
        }

        // retrieve the name and type once; use the name to retrieve the
        // value from the database
        QString const widget_name(widget.attribute("id"));
        if(widget_name.isEmpty())
        {
            throw form_exception_invalid_form_xml("All widgets must have an id with its HTML variable form name");
        }

        // only widgets that are marked for auto-save can be auto-filled
        // (note: although the auto-save could also be done inside the
        //        fill_form_widget() signal, it is done here so we can
        //        optimize on this very test)
        if(auto_save)
        {
            QString const auto_save_type(widget.attribute("auto-save"));
            if(!auto_save_type.isEmpty())
            {
                // check whether that cell exists
                QString name(owner + "::" + widget_name);
                if(auto_save_type == "binary")
                {
                    // only the path is saved in the parent for attachments
                    // and that path represents an attachment
                    name = QString(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))
                            + "::" + name + "::"
                            + content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT_PATH_END);
                }
                if(row->exists(name))
                {
                    QString const widget_type(widget.attribute("type"));
                    if(widget_type.isEmpty())
                    {
                        throw form_exception_invalid_form_xml("All auto-save widgets must have a type with its HTML variable form name");
                    }
                    libdbproxy::value const value(row->getCell(name)->getValue());

                    QString widget_value;

                    // the auto-save attribute is set to the type of the data
                    if(auto_save_type == "int8")
                    {
                        int const v(value.signedCharValue());
                        if(widget_type == "checkbox")
                        {
                            if(v == 0)
                            {
                                widget_value = "off";
                            }
                            else
                            {
                                widget_value = "on";
                            }
                        }
                        else
                        {
                            widget_value = QString("%1").arg(v);
                        }
                    }
                    else if(auto_save_type == "binary")
                    {
                        // this is an attachment
                        // we just create a link to it
                        if(widget_type == "image")
                        {
                            // in this case we can simply show the image
                            content::attachment_file attachment(f_snap);
                            if(content::content::instance()->load_attachment(value.stringValue(), attachment, false))
                            {
                                snap_child::post_file_t const& file(attachment.get_file());
                                int width(file.get_image_width());
                                int height(file.get_image_height());
                                QString path(value.stringValue());
                                if(path.startsWith(site_key))
                                {
                                    // keep the start '/' but remove the domain
                                    path = path.mid(site_key.length() - 1);
                                }
                                widget_value = "<img src=\"" + path + "\"";
                                if(width != 0 && height != 0)
                                {
                                    widget_value += QString(" width=\"%1\" height=\"%2\"").arg(width).arg(height);
                                }
                                widget_value += "/>";
                            }
                        }
                        else
                        {
                            widget_value = "<a href=\"" + value.stringValue() + "\">view attachment</a>";
                        }
                    }
                    else if(auto_save_type == "string")
                    {
                        // this is somewhat viewed as the default, but the
                        // type must still be valid and set to "string"
                        widget_value = value.stringValue();
                    }
                    // else -- undefined? -- should probably err here?
                    if(!widget_value.isEmpty())
                    {
//printf("fill [%s] with [%s]\n", name.toUtf8().data(), widget_value.toUtf8().data());
                        fill_value(widget, widget_value);
                    }
                }
            }
        }

        fill_form_widget(this, owner, cpath, xml_form, widget, widget_name);
    }
}


/** \brief Define the default value dynamically.
 *
 * This function can be called to setup the default value of a widget
 * when it is determined dynamically (which should be really rare).
 *
 * Note that this function can also be used to change the text of a
 * button. For example, a Save button could become Overwrite the
 * next time the user saves that form.
 *
 * \param[in] widget  The widget receiving the value.
 * \param[in] value  The value to assign to the widget as the default.
 */
void form::fill_value(QDomElement widget, QString const& value)
{
    // create the tag only if it doesn't already exist
    QDomElement value_tag(widget.firstChildElement("value"));
    if(value_tag.isNull())
    {
        value_tag = widget.ownerDocument().createElement("value");
        widget.appendChild(value_tag);
    }
    else
    {
        // remove any old value
        while(value_tag.hasChildNodes())
        {
            value_tag.removeChild(value_tag.lastChild());
        }
    }

    snap_dom::insert_html_string_to_xml_doc(value_tag, value);
}


/** \fn void form::fill_form_widget(form *f, QString const& owner, QString const& cpath, QDomDocument xml_form, QDomElement widget, QString const& id)
 * \brief Request plugins to fill in form widgets.
 *
 * This function offers widgets to fill out form widgets. This is used
 * to dynamically fill forms. It should rarely be necessary to do so
 * because in most cases the data is defined using the auto-fill form
 * features.
 *
 * Your plugin should end up calling the set_default_value() function
 * as follow if a dynamic fill value exists:
 *
 * \code
 *   f->fill_value(width, "this is the default value");
 * \endcode
 *
 * \param[in] f  A pointer to the form plugin
 * \param[in] owner  The form owner.
 * \param[in] cpath  The path of the form.
 * \param[in] xml_form  The XML form being filled.
 * \param[in] widget  The widget being worked on.
 * \param[in] id  The identifier (name) of the widget (id=... attribute).
 */


/** \fn void form::form_element(form *f)
 * \brief Default implementation of the form element event.
 *
 * This function loads the default XSTL that transform Core defined
 * elements to HTML. Other plugins can add their own widgets.
 * However, they are not expected to hijack the core widgets (i.e.
 * their widget should be given a type after their module such as
 * "beautifier::dropdown".)
 *
 * If the function fails, the XSLT document will not be valid.
 *
 * \param[in,out] f  A pointer to the form plugin
 */


/** \brief Add the templates and parameters defined in 'add'.
 *
 * This function merges the elements defined by 'add' to the
 * elements defined in f_form_elements. The merge is done here
 * for two reasons:
 *
 * 1. We can control what happens;
 *
 * 2. We avoid having to write complicated code all over the place.
 *
 * This function only does the merge. We'll want to have a backend process
 * that checks whether all those additions work properly and do not
 * have different plugins overwrite each others data.
 *
 * \param[in] add  The document to merge to the f_form_elements document.
 */
void form::add_form_elements(QDomDocument& add)
{
    QDomNode p(add.firstChild());
    while(!p.isElement())
    {
        // this can happen if we have comments
        if(p.isNull())
        {
            // well... nothing found?
            return;
        }
        p = p.nextSibling();
    }
    QDomElement stylesheet(p.toElement());
    if(stylesheet.tagName() != "stylesheet")
    {
        // we only can handle stylesheets
        return;
    }
    p = stylesheet.firstChild();
    while(!p.isNull())
    {
        if(p.isElement())
        {
            QDomElement e(p.toElement());
            QString name(e.tagName());
            if(name == "param" || name == "template")
            {
                f_form_stylesheet.appendChild(e);
            }
        }
        p = p.nextSibling();
    }
}


/** \brief Add the templates and parameters of the specified XSL file.
 *
 * This function is a helper function that reads a file and adds it to
 * the f_form_elements.
 *
 * \param[in] filename  The name of the XSL file to read and add to f_form_elements.
 *
 * \sa add_form_elements(QDomDocument& add);
 */
void form::add_form_elements(QString& filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        SNAP_LOG_FATAL("form::add_form_elements() could not open core-form.xsl resource file.");
        return;
    }
    QDomDocument add;
    if(!add.setContent(&file, true))
    {
        SNAP_LOG_FATAL("form::add_form_elements() could not parse core-form.xsl resource file.");
        return;
    }
    add_form_elements(add);
}


/** \brief Load an XML form.
 *
 * This function loads an XML form and pre-processes it through any plugin
 * that wants to make changes.
 *
 * The function supports two types of paths:
 *
 * \li Resource paths that start with ":/" or "qrc:/".
 * \li "Cassandra" paths which start with "http://" or "https://".
 *
 * \param[in,out] ipath  The canonicalized path to page managing this form.
 * \param[in] source  The path to the form to be loaded.
 * \param[out] error  The error while loading the form.
 *
 * \return The document (may be empty if the load fails)
 */
QDomDocument const form::load_form(content::path_info_t & ipath, QString const & source, QString & error)
{
    // forms are cached as static variables so calling the function more
    // than once for the exact same form simply returns the exact same
    // document
    //
    // NOTE: Yes! We know. This means the forms are NOT ever freed from
    //       memory. Remember that we are in a child process which will
    //       die within 1 second or less. So it's fine!
    struct doc_t
    {
        doc_t()
            : f_doc("form")
            //, f_error("") -- auto-init
        {
        }

        QDomDocument    f_doc;
        QString         f_error;
    };
    static QMap<QString, doc_t> g_cached_form;

    // 1. canonicalize the source path
    QString csource(source);
    bool const qrc(csource.startsWith("qrc:/"));
    if(qrc)
    {
        // remove "qrc" because the QFile does not recognize it
        csource.remove(0, 3);
    }

    // 2. check whether the form is already available
    if(g_cached_form.contains(csource))
    {
        error = g_cached_form[csource].f_error;
        return g_cached_form[csource].f_doc;
    }

    // 3. load the form
    if(qrc || csource.startsWith(":/"))
    {
        // 3.1 from the executable resources
        QFile file(csource);
        if(!file.open(QIODevice::ReadOnly))
        {
            g_cached_form[csource].f_error = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> Resource \""
                                                + source + "\" could not be opened.</span>";
            SNAP_LOG_ERROR("form::load_form() could not open \"" + csource + "\" resource file.");
            return g_cached_form[csource].f_doc;
        }
        if(!g_cached_form[csource].f_doc.setContent(&file, true))
        {
            g_cached_form[csource].f_error = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> Resource \""
                                                + source + "\" could not be parsed as valid XML.</span>";
            SNAP_LOG_ERROR("form::load_form() could not parse \"" + csource + "\" resource file.");
            return g_cached_form[csource].f_doc;
        }
    }
    else if(csource.startsWith("http://") || csource.startsWith("https://"))
    {
        // 3.2 from Cassandra
        libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
        // TODO: make use of content::path_info_t
        if(!content_table->exists(csource))
        {
            g_cached_form[csource].f_error = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> Form \""
                                                + source + "\" could not be loaded from the database.</span>";
            SNAP_LOG_ERROR("form::load_form() could not load \"" + csource + "\" from the database.");
            return g_cached_form[csource].f_doc;
        }
        libdbproxy::row::pointer_t row(content_table->getRow(csource));
        if(!row->exists(get_name(name_t::SNAP_NAME_FORM_FORM)))
        {
            g_cached_form[csource].f_error = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> No form defined at \""
                                                + source + "\".</span>";
            SNAP_LOG_ERROR("form::load_form() could not find a form at \"" + csource + "\".");
            return g_cached_form[csource].f_doc;
        }
        libdbproxy::value form_xml(row->getCell(get_name(name_t::SNAP_NAME_FORM_FORM))->getValue());
        if(!g_cached_form[csource].f_doc.setContent(form_xml.binaryValue(), true))
        {
            g_cached_form[csource].f_error = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> Form \""
                                                + source + "\" could not be parsed as valid XML.</span>";
            SNAP_LOG_ERROR("form::load_form() could not parse \"" + csource + "\" form.");
            return g_cached_form[csource].f_doc;
        }
    }
    else
    {
        // ???
        // we cannot load from a file because otherwise it would be necessary
        // to have that file on all computers in use and we cannot really
        // guarantee that...
        //
        // SECURITY CONSIDERATION
        // Also we need a function that allows us to verify that the csource
        // sought is 100% secure (i.e. not a file such as /etc/passwd)
        g_cached_form[csource].f_error = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> Form \""
                                            + source + "\" could not be loaded (direct files not supported yet).</span>";
        SNAP_LOG_ERROR("form::load_form() prevented loading \"" + csource + "\" file from disk for security reasons.");
        return g_cached_form[csource].f_doc;
    }

    // 4. save the page path and source path in the document
    g_cached_form[csource].f_doc.documentElement().setAttribute("path", ipath.get_cpath());
    g_cached_form[csource].f_doc.documentElement().setAttribute("src", csource);

    // 5. broadcast the fact that this form was loaded
    //
    // form was loaded, give other plugins means to modify it
    tweak_form(this, ipath, g_cached_form[csource].f_doc);

    // 6. return to caller
    return g_cached_form[csource].f_doc;
}


/** \fn void form::tweak_form(form *f, content::path_info_t& ipath, QDomDocument form_doc)
 * \brief Allow any plugin to tweak a form.
 *
 * Just before the load_form() returns, the form plugin broadcasts this
 * signal so other plugins have a chance to tweak the form.
 *
 * \todo
 * We will need to add some kind of support so plugins that need to be called
 * at the time we receive the post are.
 *
 * \param[in] f  The form plugin pointer.
 * \param[in,out] ipath  The path of the page this form is attached to.
 * \param[in,out] form_doc  The XML form in a DOM.
 */


/** \typedef form::auto_save_types_t
 * \brief Type used to save the type of the widget data for auto-save.
 *
 * This type defines the type of each widget content defined in
 * the auto-save attribute of the corresponding widget. This
 * allows us to also avoid to save all the widgets, even though that
 * we could not handle.
 *
 * The widget id (the name in the f_post) and the type as defined in the
 * XML form.
 */


/** \brief Analyze the URL and process the POST data accordingly.
 *
 * This function searches for the plugin that generated the form this
 * POST is about. The form session is used to determine the data and
 * the path is used to verify that it does indeed correspond to that
 * plugin.
 *
 * The plugin is expected to handle the results although the form
 * processing includes all the necessary code to verify the post
 * contents (minimum, maximum, filters, required fields, etc.)
 *
 * The HTML used in the form includes an input that has a form
 * session variable with what is required for us to access the
 * session table. The following is the line used in the form-core.xml
 * file:
 *
 * \code
 * <input id="_form_session" name="_form_session" type="hidden" value="{$_form_session}"/>
 * \endcode
 *
 * Therefore, we expect the POST variables to always include a
 * "_form_session" entry and that entry to represent a session
 * in the sessions table that has not expired yet.
 *
 * \note
 * This function is a server signal generated by the snap_child
 * execute() function.
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void form::on_process_post(QString const & uri_path)
{
    QString const form_session(f_snap->postenv("_form_session"));
    if(form_session.isEmpty())
    {
        // if the form_session variable does not exist, do not consider this
        // POST as a Form POST; it could be an Editor POST or another plugin
        return;
    }

    messages::messages *messages(messages::messages::instance());

    content::path_info_t ipath;
    ipath.set_path(uri_path);
    ipath.set_main_page(true);
    QString const cpath(ipath.get_cpath());

    // First we verify the session information
    // <input id="_form_session" name="_form_session" type="hidden" value="{$_form_session}"/>
    sessions::sessions::session_info info;
    sessions::sessions::instance()->load_session(form_session, info);
    switch(info.get_session_type())
    {
    case sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID:
        // unless we get this value we've got a problem with the session itself
        break;

    case sessions::sessions::session_info::session_info_type_t::SESSION_INFO_MISSING:
        f_snap->die(snap_child::http_code_t::HTTP_CODE_GONE,
                    "Form Session Gone",
                    "It looks like you attempted to submit a form without first loading it.",
                    "User sent a form with a form session identifier that is not available.");
        NOTREACHED();
        return;

    case sessions::sessions::session_info::session_info_type_t::SESSION_INFO_OUT_OF_DATE:
        messages->set_http_error(snap_child::http_code_t::HTTP_CODE_GONE,
                    "Form Timeout",
                    "Sorry! You sent this request back to Snap! way too late. It timed out. Please re-enter your information and re-submit.",
                    "User did not click the submit button soon enough, the server session timed out.",
                    true);
        return;

    case sessions::sessions::session_info::session_info_type_t::SESSION_INFO_USED_UP:
        messages->set_http_error(snap_child::http_code_t::HTTP_CODE_CONFLICT,
                    "Form Already Submitted",
                    "This form was already processed. If you clicked Reload, this error is expected.",
                    "The user submitted the same form more than once.",
                    true);
        return;

    default:
        throw snap_logic_exception(QString("form.cpp:form::on_process_post(): load_session() returned an unexpected SESSION_INFO_... value (%1) in form::on_process_post()")
                    .arg(static_cast<int>(info.get_session_type())));

    }

    // verify that one of the paths is valid
    // and that the user agent did not change
    if((info.get_page_path() != cpath && info.get_object_path() != cpath)
    || info.get_user_agent() != f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)))
    {
        // the path or user agent was tempered with?
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_ACCEPTABLE,
                "Not Acceptable",
                "The request you sent does not seem to correspond to the form it was defined for.",
                QString("User POSTed a request against form \"%1\" with an incompatible page (%2) or object (%2) path or his user agent changed.")
                        .arg(cpath)
                        .arg(info.get_page_path())
                        .arg(info.get_object_path()));
        NOTREACHED();
    }

    // get the owner of this form (plugin name)
    QString const & owner(info.get_plugin_owner());
    plugin * const p(plugins::get_plugin(owner));
    if(p == nullptr)
    {
        // we got a problem, that plugin does not exist?!
        // (it could happen assuming someone is removing plugins just before someone else submits a form)
        f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                "Forbidden",
                "The request you just sent is not attached to a currently supported plugin. The plugin may have been uninstalled after you loaded the form.",
                "Somehow the user posted a form that has a plugin name which is not currently loaded by this website.");
        NOTREACHED();
    }
    form_post * const fp(dynamic_cast<form_post *>(p));

    QDomDocument xml_form;

    // define the name of the form (usually ":/xml/<owner>/settings-form.xml")
    QString const source(get_source(owner, ipath));
    if(source.isEmpty())
    {
        // the programmer forgot to derive from form_post?!
        throw snap_logic_exception(QString("form.cpp:form::on_process_post(): could not find a valid source for a form in \"%1\".").arg(cpath));
    }

    // now load the form
    QString error;
    xml_form = load_form(ipath, source, error);
    if(xml_form.isNull())
    {
        // programmer mispelled the path?
        throw form_exception_invalid_form_xml(QString("form.cpp:form::on_process_post(): path \"%1\" does not correspond to a valid XML form (tried with \"%2\")")
                    .arg(cpath)
                    .arg(source));
    }

    // clearly mark that this form has post values (i.e. do not
    // update the form with the default data saved in the database)
    QDomElement root(xml_form.documentElement());
    root.setAttribute("post", "post");

    auto_save_types_t auto_save_type;
    QDomNodeList widgets(xml_form.elementsByTagName("widget"));
    int const count(widgets.length());
    QString previous_widget_name("first widget");
    for(int i(0); i < count; ++i)
    {
        // TODO properly record the use of each and every single widget
        //      returned by the user (i.e. the POST variables) because
        //      if some funky entries appear, we have to reject the
        //      whole thing!
        QDomNode w(widgets.item(i));
        if(!w.isElement())
        {
            throw form_exception_invalid_form_xml("form.cpp:form::on_process_post(): elementsByTagName() returned a node that is not an element");
        }
        QDomElement widget(w.toElement());

        // retrieve the name and type once; use the name to retrieve the post
        // value: QString value(f_snap->postenv(widget_name));
        QDomNamedNodeMap attributes(widget.attributes());
        QDomNode id(attributes.namedItem("id"));
        QString const widget_name(id.nodeValue());
        if(widget_name.isEmpty())
        {
            // be precise so programmers can easily find their mistake
            throw form_exception_invalid_form_xml(QString("form.cpp:form::on_process_post(): All widgets must have an \"id\" attribute, missing for widget \"%1\" (index = %2)")
                    .arg(previous_widget_name)
                    .arg(i + 1));
        }

        // verify that the name is considered valid as per us...
        // (this should be a verification step at compile time!)
        int const name_length(widget_name.length());
        for(int j(0); j < name_length; ++j)
        {
            ushort c(widget_name.at(j).unicode());
            if((c < '0' || c > '9' || j == 0)
            && (c < 'a' || c > 'z')
            && (c < 'A' || c > 'Z')
            && c != '_')
            {
                // note that we actually limit identifiers to A-Z characters
                // whereas JavaScript really accepts any letters... it also
                // supports '$' in identifiers
                throw form_exception_invalid_form_xml(QString("form.cpp:form::on_process_post(): Widget names (\"id\" attribute) must be valid JavaScript identifiers. \"%2\" is not considered valid.")
                        .arg(widget_name));
            }
        }

        // get the widget type
        QDomNode type(attributes.namedItem("type"));
        QString const widget_type(type.nodeValue());
        if(widget_type.isEmpty())
        {
            // be precise so programmers can easily find their mistake
            throw form_exception_invalid_form_xml(QString("form.cpp:form::on_process_post(): All widgets must have a \"type\" attribute, missing for widget \"%1\" (index = %2)")
                    .arg(previous_widget_name)
                    .arg(i + 1));
        }

        QDomNode secret(attributes.namedItem("secret"));
        bool const is_secret(!secret.isNull() && secret.nodeValue() == "secret");

        QDomNode auto_save_attr(attributes.namedItem("auto-save"));
        if(!auto_save_attr.isNull())
        {
            auto_save_type[widget_name] = auto_save_attr.nodeValue();
        }

        // if the form was submitted, we have postenv() values
        // which we want to save in the <post> tag of the widget then
        // the widget can decide whether to use the <post> data or the
        // default <value> data (although we do not hand the value back
        // if the widget is marked as secret.)
        QString post(f_snap->postenv(widget_name));
        // XXX this is not correct, we should offer the widget owner
        //     a way to handle special cases like this! (at this point,
        //     though, we want to discourage the user of standard HTML
        //     forms anyway...)
        if(post.isEmpty() && widget_type == "checkbox")
        {
            post = "off";
        }
        // TODO: I will keep the "!post.isEmpty()" test for now, this it
        //       seems wrong because if the user posted an empty variable
        //       then we want to send back an empty variable and not some
        //       "random" default
        if(!is_secret && !post.isEmpty() && widget_type != "image" && widget_type != "file")
        {
            QDomElement post_tag(xml_form.createElement("post"));
            widget.appendChild(post_tag);
            // TBD should post be HTML instead of just text here?
            //     (especially for a textarea widget)
            QDomText post_value(xml_form.createTextNode(post));
            post_tag.appendChild(post_value);
        }

        // now validate using a signal so any plugin can take over
        // the validation process
        sessions::sessions::session_info::session_info_type_t const save_session_type(info.get_session_type());
        // pretend that everything is fine so far...
        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID);
        int const errcnt(messages->get_error_count());
        int const warncnt(messages->get_warning_count());
        validate_post_for_widget(ipath, info, widget, widget_name, widget_type, is_secret);
        if(info.get_session_type() != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID)
        {
            // it was not valid so mark the widgets as errorneous (i.e. so we
            // can display it with an error message)
            if(messages->get_error_count() == errcnt
            && messages->get_warning_count() == warncnt)
            {
                // the pluing marked that it found an error but did not
                // generate an actual error, do so here with a generic
                // error message
                messages->set_error(
                    "Invalid Form Content",
                    QString("\"%1\" is not valid for \"%2\".")
                            .arg(html_64max(post, is_secret))
                            .arg(widget_name),
                    "unspecified error for widget",
                    false
                ).set_widget_name(widget_name);
            }
            messages::messages::message const & msg(messages->get_last_message());

            // Add the following to the widget so we can display the
            // widget as having an error and show the error on request
            //
            // <error idref="messages_message_<id>">
            //   <title>$title</title>
            //   <message>$message</message>
            // </error>

            QDomElement err_tag(xml_form.createElement("error"));
            err_tag.setAttribute("idref", QString("messages_message_%1").arg(msg.get_id()));
            widget.appendChild(err_tag);
            QDomElement title_tag(xml_form.createElement("title"));
            err_tag.appendChild(title_tag);
            QDomText title_text(xml_form.createTextNode(msg.get_title()));
            title_tag.appendChild(title_text);
            QDomElement message_tag(xml_form.createElement("message"));
            err_tag.appendChild(message_tag);
            QDomText message_text(xml_form.createTextNode(msg.get_body()));
            message_tag.appendChild(message_text);
        }
        else
        {
            // restore the last type
            info.set_session_type(save_session_type);

            // TODO support for attachment so they do not just disappear on
            //      errors is required here; i.e. we need a way to be able
            //      to save all the valid attachments in a temporary place
            //      and then "move" them to their final location once the
            //      form validates properly
        }

        previous_widget_name = widget_name;
    }
    // if the previous loop found 1 or more errors, return now
    // (i.e. we do not want to process the data any further in this case)
    if(info.get_session_type() != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID)
    {
        return;
    }

    // data looks good, let the plugin process it
    QDomElement snap_form(xml_form.documentElement());
    QString const auto_save_str(snap_form.attribute("auto-save"));
    if(!auto_save_str.isEmpty())
    {
        // in this case the form plugin just saves the data as is in the page
        auto_save_form(owner, ipath, auto_save_type, xml_form);

        // after the auto-save, we also call the plugin on_process_form_post()
        // if available and in case the plugin wants to do a little more
        // work (i.e. mark something else out of date...)
    }
    else
    {
        if(fp == nullptr)
        {
            // the programmer forgot to derive from form_post?!
            throw snap_logic_exception(QString("form.cpp:form::on_process_post(): you cannot use plugin \"%1\" as dynamically saving forms without also deriving it from form_post")
                                            .arg(owner));
        }
    }
    if(fp != nullptr)
    {
        // user specified a special page to handle this form?
        QString const processor(root.attribute("processor", ""));
        if(!processor.isEmpty())
        {
            // request the processing of the form to be done as if the
            // page was "processor"; we go as far as pretending that
            // this processing page is the main page!
            content::path_info_t processor_ipath;
            processor_ipath.set_path(processor);
            processor_ipath.set_main_page(true);
            fp->on_process_form_post(processor_ipath, info);
        }
        else
        {
            fp->on_process_form_post(ipath, info);
        }

        // forms do not support AJAX, use the editor for that!
        // this also means a POST will be sent to a box if the
        // box has a form and the user POST's that form... so
        // here we want to at least try to fix the problem
        //
        // (i.e. if we're still here, then the form did not
        // generate a redirect... so there must have been an
        // error because a redirect after this place seems
        // unlikely)
        if(cpath.startsWith("layouts/"))
        {
            snap_string_list const segments(cpath.split("/"));
            // TBD: I am not totally sure that boxes will always have exactly
            //      4 segments, but this is the case at this point
            if(segments.size() == 4)
            {
                // this is a box, try to send the user back to where he
                // came from, which means he loses the POST data... Ooops!
                QString const referrer(f_snap->snapenv("HTTP_REFERER"));
                f_snap->page_redirect(referrer,
                    snap_child::http_code_t::HTTP_CODE_SEE_OTHER,
                    "Sending you back to the page you are coming from.",
                    "We are trying to send the user back where he came from because the place we are in now is the box with the submitted form...");
                NOTREACHED();
            }
        }
    }
}


/** \brief Automatically save the form.
 *
 * This function automatically saves the form data in the database.
 * This is particularly useful for any form that is not dynamic.
 * (Dynamic forms will be supported at a later time.)
 *
 * \param[in] owner  The name of the plugin that owns this form.
 * \param[in,out] ipath  The page where the data is to be saved.
 * \param[in] auto_save_type  The type of this cell in the database.
 * \param[in] xml_form  The form that's being saved.
 */
void form::auto_save_form(QString const& owner, content::path_info_t& ipath, auto_save_types_t const& auto_save_type, QDomDocument xml_form)
{
    content::content *content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    //QString const site_key(f_snap->get_site_key_with_slash());
    QString const key(ipath.get_key());
    if(!content_table->exists(key))
    {
        // the row does not exist yet... the form should not even be
        // in auto-save mode!?
        return;
    }
    libdbproxy::row::pointer_t row(content_table->getRow(key));

    for(snap_child::environment_map_t::const_iterator it(auto_save_type.begin());
            it != auto_save_type.end();
            ++it)
    {
        // the key is the widget identifier
        QString id(it.key());

        // retrieve the value from the post variable
        QString const post(f_snap->postenv(id));

        QString const name(owner + "::" + id);

        QString type(*it);
        if(type == "int8")
        {
            if(post == "on")
            {
                row->getCell(name)->setValue((static_cast<signed char>(1)));
            }
            else if(post == "off")
            {
                row->getCell(name)->setValue((static_cast<signed char>(0)));
            }
            else
            {
                row->getCell(name)->setValue((static_cast<signed char>(post.toInt())));
            }
        }
        else if(type == "binary")
        {
            // make sure the user uploaded an actual file!
            if(f_snap->postfile_exists(id))
            {
                // by default the owner is the same as the form owner
                QString attachment_owner(owner);

                // by default the type of an attachment is set to
                // "private attachment", it can be changed by the widget
                // attachment tag and any module that wants to do so while
                // generating the new page, of course
                QString attachment_type("attachment/private");

                // by default attachments are unique (load one file)
                bool multiple(false);

                // retrieve the attachment tag and get additional parameters
                QDomXPath dom_xpath;
                dom_xpath.setXPath(QString("/snap-form//widget[@id=\"%1\"]/attachment").arg(id));
                QDomXPath::node_vector_t result(dom_xpath.apply(xml_form));
                if(result.size() > 0 && result[0].isElement())
                {
                    // retrieve the parameters from the tag attributes
                    QDomElement attachment_tag(result[0].toElement());
                    QString value;

                    // overwrite default owner
                    value = attachment_tag.attribute("owner");
                    if(!value.isEmpty())
                    {
                        attachment_owner = value;
                    }

                    // overwrite default type
                    value = attachment_tag.attribute("type");
                    if(!value.isEmpty())
                    {
                        attachment_type = type;
                    }

                    // accept multiple attachments
                    value = attachment_tag.attribute("multiple");
                    if(value == "multiple")
                    {
                        multiple = true;
                    }
                }

                // save the file in the database
                content::attachment_file attachment(f_snap, f_snap->postfile(id));
                attachment.set_multiple(multiple);
                attachment.set_parent_cpath(ipath.get_cpath());
                attachment.set_field_name(id);
                // TODO: It is likely that this is wrong because it is likely
                //       something like output when it should be attachment
                attachment.set_attachment_owner(attachment_owner);
                attachment.set_attachment_type(attachment_type);
                // TODO: define the locale in some ways...
                //       (use ipath.get_locale()?)
                snap_version::version_number_t const branch(content_plugin->get_current_user_branch(ipath.get_key(), "", true));
                content_plugin->create_attachment(attachment, branch, "");
            }
        }
        else if(type == "string")
        {
            // a simple string
            row->getCell(name)->setValue(post);
        }
        // else -- "undefined"
    }

    // let the world know that we modified this page
    // this is not an update since the form itself was not modified
    content_plugin->modified_content(ipath);
}


/** \brief Ensure that messages don't display extremely large values.
 *
 * This function truncates a value to at most 64 characters (+3 period
 * for the elipsis.)
 *
 * This function is only to truncate plain text. HTML should use the
 * html_64max() function instead.
 *
 * \todo
 * Move to the filter plugin.
 *
 * \param[in] text  The text string to clip to 64 characters.
 * \param[in] is_secret  If true then change text with asterisks.
 *
 * \return The clipped value as required.
 */
QString form::text_64max(QString const& text, bool const is_secret)
{
    if(is_secret && !text.isEmpty())
    {
        return "******";
    }

    if(text.length() > 64)
    {
        return text.mid(0, 64) + "...";
    }
    return text;
}


/** \brief Shorten the specified HTML to 64 characters.
 *
 * \todo
 * Move to the filter plugin.
 *
 * \param[in] html  The HTML string to reduce to a maximum of 64 characters.
 * \param[in] is_secret  If true then change text with asterisks.
 *
 * \return The shorten HTML, although still 100% valid HTML.
 */
QString form::html_64max(QString const & html, bool const is_secret)
{
    if(is_secret)
    {
        return "******";
    }

    if(html.indexOf('<') == -1)
    {
        // only text, make it easy on us
        // (also a lot faster)
        return text_64max(html, is_secret);
    }

    // TODO: go through the tree and keep data as long
    //       as the text is more than 64 characters
    //       and we have more than 100 tags (or so...)
    return html;
}


/** \brief Count the number of lines in a text string.
 *
 * This function goes through a text string and counts the number of
 * new line characters.
 *
 * \todo
 * Should we have a flag to know whether empty lines should be counted?
 * Should we count the last line if it doesn't end with a new line
 * character?
 *
 * \param[in] text  The text to count lines in.
 *
 * \return The number of new line characters (or \\r\\n) found in \p text.
 */
int form::count_text_lines(QString const& text)
{
    int lines(0);

    for(char *s(text.toUtf8().data()); *s != '\0'; ++s)
    {
        if(*s == '\r')
        {
            ++lines;
            if(s[1] == '\r')
            {
                // \r\n <=> one line
                ++s;
            }
        }
        else if(*s == '\n')
        {
            ++lines;
        }
    }

    return lines;
}


/** \brief Count the number of lines in an HTML buffer.
 *
 * This function goes through the HTML in \p html and counts the number
 * paragraphs.
 *
 * \param[in] html  The html to count lines in.
 *
 * \return The number of paragraphs in the HTML buffer.
 */
int form::count_html_lines(QString const& html)
{
    int lines(0);

    QDomDocument doc;
    doc.setContent(html);
    QDomElement parent(doc.documentElement());
    
    // go through all the children elements
    for(QDomElement child(parent.firstChildElement()); !child.isNull(); child = child.nextSiblingElement())
    {
        QString name(child.nodeName());
        if(name == "p" || name == "div")
        {
            // <p> and <div> are considered paragraphs
            // (TBD: should we count the number of <p> inside a <div>)
            ++lines;
        }
    }

    return lines;
}


/** \brief Start a widget validation.
 *
 * This function prepares the validation of the specified widget by
 * applying common core validations.
 *
 * The \p info parameter is used for the result. If something is wrong,
 * then the type of the session is changed from SESSION_INFO_VALID to
 * one of the SESSION_INFO_... that represent an error, in most cases we
 * use SESSION_INFO_INCOMPATIBLE.
 *
 * \param[in] cpath  The path where the form is defined
 * \param[in,out] info  The information linked with this form (loaded from the session)
 * \param[in] widget  The widget being tested
 * \param[in] widget_name  The name of the widget (i.e. the id="..." attribute value)
 * \param[in] widget_type  The type of the widget (i.e. the type="..." attribute value)
 * \param[in] is_secret  If true, the field is considered a secret field (i.e. a password.)
 *
 * \return Always return true so other plugins have a chance to validate too.
 */
bool form::validate_post_for_widget_impl(content::path_info_t & ipath, sessions::sessions::session_info & info,
                                         QDomElement const & widget, QString const & widget_name,
                                         QString const & widget_type, bool const is_secret)
{
    messages::messages * messages(messages::messages::instance());

    // get the value we are going to validate
    QString const value(f_snap->postenv(widget_name));
    bool has_minimum(false);

    QDomElement sizes(widget.firstChildElement("sizes"));
    if(!sizes.isNull())
    {
        QDomElement min_element(sizes.firstChildElement("min"));
        if(!min_element.isNull())
        {
            has_minimum = true;
            QString const m(min_element.text());
            if(widget_type == "image")
            {
                int width, height;
                if(!parse_width_height(m, width, height))
                {
                    // invalid width 'x' height
                    messages->set_error(
                        "Invalid Sizes",
                        "minimum size \"" + html_64max(m, false) + "\" is not a valid \"width 'x' height \" definition for image widget " + widget_name + ".",
                        "incorrect sizes for " + widget_name,
                        false
                    ).set_widget_name(widget_name);
                    // TODO add another type of error for setup ("programmer") data?
                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                }
                else if(f_snap->postfile_exists(widget_name))
                {
                    snap_child::post_file_t const& image(f_snap->postfile(widget_name));
                    int image_width(image.get_image_width());
                    int image_height(image.get_image_height());
                    if(width == 0 || height == 0)
                    {
                        messages->set_error(
                            "Incompatible Image File",
                            "The image \"" + widget_name + "\" was not recognized as a supported image file format.",
                            "the system did not recognize the image as such (width/height are not valid), cannot verify the minimum size",
                            false
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                    else if(image_width < width || image_height < height)
                    {
                        messages->set_error(
                            "Image Too Small",
                            QString("The image \"" + widget_name + "\" you uploaded is too small (your image is %1x%2, the minimum required is %3x%4).").arg(image_width).arg(image_height).arg(width).arg(height),
                            "the user uploaded an image that is too small",
                            false
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                }
            }
            else
            {
                bool ok;
                int const l(m.toInt(&ok));
                if(!ok)
                {
                    throw form_exception_invalid_form_xml("the minimum size \"" + m + "\" must be a valid decimal integer");
                }
                if(value.length() < l)
                {
                    // length too small
                    QString const label(widget.firstChildElement("label").text());
                    messages->set_error(
                        "Length Too Small",
                        "\"" + html_64max(value, is_secret) + "\" is too small in \"" + label + "\". The widget requires at least " + m + " characters.",
                        "not enough characters in " + widget_name + " error",
                        false
                    ).set_widget_name(widget_name);
                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                }
            }
        }
        QDomElement max_element(sizes.firstChildElement("max"));
        if(!max_element.isNull())
        {
            QString const m(max_element.text());
            if(widget_type == "image")
            {
                int width, height;
                if(!parse_width_height(m, width, height))
                {
                    // invalid width 'x' height
                    messages->set_error(
                        "Invalid Sizes",
                        "maximum size \"" + html_64max(m, false) + "\" is not a valid \"width 'x' height \" definition for this image widget.",
                        "incorrect sizes for " + widget_name,
                        false
                    ).set_widget_name(widget_name);
                    // TODO add another type of error for setup ("programmer") data?
                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                }
                else if(f_snap->postfile_exists(widget_name))
                {
                    snap_child::post_file_t const& image(f_snap->postfile(widget_name));
                    int image_width(image.get_image_width());
                    int image_height(image.get_image_height());
                    if(width == 0 || height == 0)
                    {
                        // TODO avoid error a 2nd time if done in minimum case
                        messages->set_error(
                            "Incompatible Image File",
                            "The image \"" + widget_name + "\" was not recognized as a supported image file format.",
                            "the system did not recognize the image as such (width/height are not valid), cannot verify the minimum size",
                            false
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                    else if(image_width > width || image_height > height)
                    {
                        messages->set_error(
                            "Image Too Large",
                            QString("The image \"" + widget_name + "\" you uploaded is too large (your image is %1x%2, the maximum allowed is %3x%4).").arg(image_width).arg(image_height).arg(width).arg(height),
                            "the user uploaded an image that is too large",
                            false
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                }
            }
            else
            {
                bool ok;
                int const l(m.toInt(&ok));
                if(!ok)
                {
                    throw form_exception_invalid_form_xml("the maximum size \"" + m + "\" must be a valid decimal integer");
                }
                if(value.length() > l)
                {
                    // length too large
                    QString const label(widget.firstChildElement("label").text());
                    messages->set_error(
                        "Length Too Long",
                        "\"" + html_64max(value, is_secret) + "\" is too long in \"" + label + "\". The widget requires at most " + m + " characters.",
                        "too many characters " + widget_name + " error",
                        false
                    ).set_widget_name(widget_name);
                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                }
            }
        }
        QDomElement lines(sizes.firstChildElement("lines"));
        if(!lines.isNull())
        {
            QString const m(lines.text());
            bool ok;
            int const l(m.toInt(&ok));
            if(!ok)
            {
                throw form_exception_invalid_form_xml("the number of lines \"" + m + "\" must be a valid decimal integer");
            }
            if(widget_type == "text-edit")
            {
                if(count_text_lines(value) < l)
                {
                    // not enough lines (text)
                    messages->set_error(
                        "Not Enough Lines",
                        "\"" + html_64max(value, is_secret) + "\" is too long in \"" + widget_name + "\". The widget requires at least " + m + " lines.",
                        "not enough lines",
                        false
                    ).set_widget_name(widget_name);
                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                }
            }
            else if(widget_type == "html-edit")
            {
                if(count_html_lines(value) < l)
                {
                    // not enough lines (HTML)
                    QString const label(widget.firstChildElement("label").text());
                    messages->set_error(
                        "Not Enough Lines",
                        "\"" + html_64max(value, is_secret) + "\" is too long in \"" + label + "\". The widget requires at least " + m + " lines.",
                        "not enough lines in " + widget_name,
                        false
                    ).set_widget_name(widget_name);
                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                }
            }
        }
    }

    // check whether the field is required, in case of a checkbox required
    // means that the user selects the checkbox ("on")
    if(widget_type == "line-edit"
    || widget_type == "password"
    || widget_type == "checkbox"
    || widget_type == "file"
    || widget_type == "image")
    {
        QDomElement required(widget.firstChildElement("required"));
        if(!required.isNull())
        {
            QString const required_text(required.text());
            if(required_text == "required")
            {
                if(widget_type == "file"
                || widget_type == "image")
                {
                    if(!f_snap->postfile_exists(widget_name)) // TBD <- this test is not logical if widget_type cannot be a FILE type...
                    {
                        QDomElement root(widget.ownerDocument().documentElement());
                        QString const name(QString("%1::%2::%3::%4")
                                .arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))
                                .arg(root.attribute("owner"))
                                .arg(widget_name)
                                .arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT_PATH_END)));
                        libdbproxy::value cassandra_value(content::content::instance()->get_content_parameter(ipath, name, content::content::param_revision_t::PARAM_REVISION_GLOBAL));
                        if(cassandra_value.nullValue())
                        {
                            // not defined!
                            QString const label(widget.firstChildElement("label").text());
                            messages->set_error(
                                    "Value is Invalid",
                                    QString("\"%1\" is a required field.").arg(label),
                                    QString("no data entered by user in widget \"%1\"").arg(widget_name),
                                    false
                                ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                    }
                }
                else
                {
                    // not an additional error if the minimum error was
                    // already generated
                    if(!has_minimum && value.isEmpty())
                    {
                        QString const label(widget.firstChildElement("label").text());
                        messages->set_error(
                                "Value is Invalid",
                                "\"" + label + "\" is a required field.",
                                "no data entered in widget \"" + widget_name + "\" by user",
                                false
                            ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                }
            }
        }
    }

    // check whether the widget has a "duplicate-of" attribute, if so
    // then it must be equal to that other widget's value
    QString duplicate_of(widget.attribute("duplicate-of"));
    if(!duplicate_of.isEmpty())
    {
        // What we need is the name of the widget so we can get its
        // current value and the duplicate-of attribute is just that!
        QString duplicate_value(f_snap->postenv(duplicate_of));
        if(duplicate_value != value)
        {
            QString dup_label(duplicate_of);
            QDomXPath dom_xpath;
            dom_xpath.setXPath(QString("/snap-form//widget[@id=\"%1\"]/@id").arg(duplicate_of));
            QDomXPath::node_vector_t result(dom_xpath.apply(widget));
            if(result.size() > 0 && result[0].isElement())
            {
                // we found the widget, display its label instead
                dup_label = result[0].toElement().text();
            }
            QString const label(widget.firstChildElement("label").text());
            messages->set_error(
              "Value is Invalid",
              "\"" + label + "\" must be an exact copy of \"" + dup_label + "\". Please try again.",
              "confirmation widget \"" + widget_name + "\" is not equal to the original \"" + duplicate_of + "\" (i.e. most likely a password confirmation)",
              false
            ).set_widget_name(widget_name);
            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
        }
    }

    QDomElement filters(widget.firstChildElement("filters"));
    if(!filters.isNull())
    {
        QDomElement regex_tag(filters.firstChildElement("regex"));
        if(!regex_tag.isNull())
        {
            QString re;

            QDomNamedNodeMap attributes(regex_tag.attributes());
            QDomNode name(attributes.namedItem("name"));
            if(!name.isNull())
            {
                QString regex_name(name.nodeValue());
                if(!regex_name.isEmpty())
                {
                    switch(regex_name[0].unicode())
                    {
                    case 'd':
                        if(regex_name == "decimal")
                        {
                            re = "^[0-9]+(?:\\.[0-9]+)?$";
                        }
                        break;

                    case 'e':
                        if(regex_name == "email")
                        {
                            // TODO: replace the email test with libtld
                            // For emails we accept anything except local emails:
                            //     <name>@[<sub-domain>.]<domain>.<tld>
                            re = "/^[a-z0-9_\\-\\.\\+\\^!#\\$%&*+\\/\\=\\?\\`\\|\\{\\}~\\']+@(?:[a-z0-9]|[a-z0-9][a-z0-9\\-]*[a-z0-9])+\\.(?:(?:[a-z0-9]|[a-z0-9][a-z0-9\\-]*[a-z0-9])\\.?)+$/i";
                        }
                        break;

                    case 'f':
                        if(regex_name == "float")
                        {
                            re = "^[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?$";
                        }
                        break;

                    case 'i':
                        if(regex_name == "integer")
                        {
                            re = "^[0-9]+$";
                        }
                        break;

                    }
                    // TBD: offer other plugins to support their own regex?
                }
                // else -- should empty be ignored? TBD
                if(re.isEmpty())
                {
                    // TBD: this can be a problem if we remove a plugin that
                    //      adds some regexes (although right now we don't
                    //      have such a signal...)
                    throw form_exception_invalid_form_xml("the regular expression named \"" + regex_name + "\" is not supported.");
                }
                // Note:
                // We don't test whether there is some text here to avoid
                // wasting time; we could have such a test in a tool of
                // ours used to verify that the form is well defined.
            }
            else
            {
                re = regex_tag.text();
            }

            Qt::CaseSensitivity cs(Qt::CaseSensitive);
            if(!re.isEmpty() && re[0] == '/')
            {
                re = re.mid(1);
            }
            int p(re.lastIndexOf('/'));
            if(p >= 0)
            {
                QString flags(re.mid(p + 1));
                re = re.mid(0, p);
                for(char *s(flags.toUtf8().data()); *s != '\0'; ++s)
                {
                    switch(*s)
                    {
                    case 'i':
                        cs = Qt::CaseInsensitive;
                        break;

                    default:
                        {
                        char buf[2];
                        buf[0] = *s;
                        buf[1] = '\0';
                        throw form_exception_invalid_form_xml("\"" + QString::fromLatin1(buf) + "\" is not a supported regex flag");
                        }

                    }
                }
            }
            QRegExp reg_expr(re, cs, QRegExp::RegExp2);
            if(!reg_expr.isValid())
            {
                throw form_exception_invalid_form_xml("\"" + re + "\" regular expression is invalid.");
            }
            if(reg_expr.indexIn(value) == -1)
            {
                QString const label(widget.firstChildElement("label").text());
                messages->set_error(
                    "Invalid Value",
                    "\"" + html_64max(value, is_secret) + "\" is not valid for \"" + label + "\".",
                    "the value did not match the filter regular expression of " + widget_name,
                    false
                ).set_widget_name(widget_name);
                info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
            }
        }

        if(!value.isEmpty())
        {
            QDomElement extensions_tag(filters.firstChildElement("extensions"));
            if(!extensions_tag.isNull())
            {
                QString extensions(extensions_tag.text());
                snap_string_list ext_list(extensions.split(",", QString::SkipEmptyParts));
                int const max_strings(ext_list.size());
                QFileInfo const file_info(value);
                QString const file_ext(file_info.suffix());
                int i;
                for(i = 0; i < max_strings; ++i)
                {
                    QString ext(ext_list[i].trimmed());
                    if(ext.isEmpty())
                    {
                        // skip empty entries (this can happen if the trimmed()
                        // call removed all spaces and it was only spaces!)
                        continue;
                    }
                    if(file_ext == ext)
                    {
                        break;
                    }
                    ext_list[i] = ext; // save the trimmed version back for errors
                }
                // if all extensions were checked and none accepted, error
                if(i >= max_strings)
                {
                    QString const label(widget.firstChildElement("label").text());
                    messages->set_error(
                        "Filename Extension is Invalid",
                        "\"" + value + "\" must end with one of \"" + ext_list.join(", ") + "\" in \"" + label + "\". Please try again.",
                        "widget " + widget_name + " included a filename with an invalid extension",
                        false
                    ).set_widget_name(widget_name);
                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                }
            }
        }
    }

    // Note:
    // We always return true because errors generated here are first but
    // complimentary errors may be generated by other plugins
    return true;
}


/** \brief Parse dimensions (width by height).
 *
 * This function parses a width and a height. Both numbers must be in
 * decimal. The numbers are separated by the letter 'x'. There can be
 * spaces before and after each number. No other characters are
 * allowed. Dimensions cannot be negative.
 *
 * If only one decimal number appears (and no 'x') then the height is
 * set to the width and the function returns true.
 *
 * There are some examples:
 *
 * \code
 *    16x16     equivalent to    16
 *    800x600
 *    45 x 95
 * \endcode
 *
 * Note: The separator is the 'x' character (0x78) eventually in uppercase
 * (0x58), not the multiply character (0x97).
 *
 * \return true if the dimensions were properly parsed, false otherwise.
 */
bool form::parse_width_height(QString const& size, int& width, int& height)
{
    width = 0;
    height = 0;

    QChar const *s(size.data());
    while(isspace(s->unicode()))
    {
        ++s;
    }
    if(s->unicode() < '0' || s->unicode() > '9')
    {
        return false;
    }
    while(s->unicode() >= '0' && s->unicode() <= '9')
    {
        width = width * 10 + s->unicode() - '0';
        ++s;
    }
    while(isspace(s->unicode()))
    {
        ++s;
    }
    if(s->unicode() == '\0')
    {
        height = width;
        return true;
    }
    if(s->unicode() != 'x' && s->unicode() != 'X')
    {
        return false;
    }
    do
    {
        ++s;
    }
    while(isspace(s->unicode()));
    if(s->unicode() < '0' || s->unicode() > '9')
    {
        return false;
    }
    while(s->unicode() >= '0' && s->unicode() <= '9')
    {
        height = height * 10 + s->unicode() - '0';
        ++s;
    }
    while(isspace(s->unicode()))
    {
        ++s;
    }
    return s->unicode() == '\0';
}


/** \brief Retrieve the next tab identifier.
 *
 * Tab identifiers have to be unique for the entire page if we want them to
 * work properly. For this reason we have a function that can be called
 * from any plugin to retrieve the next identifier.
 *
 * \return The next unique tab identifier.
 */
int form::current_tab_id()
{
    return g_tabindex_base;
}


/** \brief Add to the used tab identifier.
 *
 * This function is used to add the number of widgets a form used so one
 * can use the next identifier for the next form on the same page.
 */
void form::used_tab_id(int used)
{
    g_tabindex_base += used;
}


/** \brief Replace a [form::...] token with a form.
 *
 * This function replaces the form tokens with their value.
 *
 * The supported tokens are:
 *
 * \li [form::path(path="\<website path>")]
 * \li [form::resource(path="\<resource path>")]
 * \li [form::settings]
 *
 * \param[in,out] ipath  The path to the page being worked on.
 * \param[in,out] xml  The XML document used with the layout.
 * \param[in,out] token  The token object, with the token name and optional parameters.
 */
void form::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(xml);

    // a form::... token?
    if(!token.is_namespace("form::"))
    {
        return;
    }

    QString const plugin_owner(ipath.get_parameter("token_owner"));
    QString const site_key(f_snap->get_site_key_with_slash());
    QDomDocument form_doc;
    QString source;

    bool const resource(token.is_token(get_name(name_t::SNAP_NAME_FORM_RESOURCE)));
    bool const settings(token.is_token(get_name(name_t::SNAP_NAME_FORM_SETTINGS)));
    if(resource || settings)
    {
        if(resource)
        {
            // form::resource expects one parameter
            if(!token.verify_args(1, 1))
            {
                return;
            }
            filter::filter::parameter_t param(token.get_arg("path", 0, filter::filter::token_t::TOK_STRING));
            if(token.f_error)
            {
                // we're done
                return;
            }
            source = param.f_value;
            if(!source.isEmpty())
            {
                // define the full path to the form
                source = ":/xml/" + plugin_owner + "/" + source + ".xml";
            }
        }
        else //if(settings) -- if not resource, then settings is true
        {
            // form::settings does not take any parameter
            source = ":/xml/" + plugin_owner + "/settings-form.xml";
        }
    }
    else if(token.is_token(get_name(name_t::SNAP_NAME_FORM_SOURCE)))
    {
        // path to form comes from the database
        source = get_source(plugin_owner, ipath);
    }
    else if(token.is_token(get_name(name_t::SNAP_NAME_FORM_PATH)))
    {
        if(token.verify_args(1, 1))
        {
            // form::path takes one parameter
            filter::filter::parameter_t param(token.get_arg("path", 0, filter::filter::token_t::TOK_STRING));
            source = param.f_value;
            if(!source.isEmpty())
            {
                bool const includes_site_key(source.startsWith(site_key));
                if(source[0] != '/' && !includes_site_key)
                {
                    // in case the user passed a relative path, change it
                    // to a full URI using the page URI saved in the XML
                    // document
                    //
                    // WARNING: DO NOT MOVE THE CANONALIZATION BEFORE THE IF()
                    //          it removes the '/' at the start!
                    f_snap->canonicalize_path(source);
                    source = ipath.get_key() + "/" + source;
                }
                else if(!includes_site_key)
                {
                    // in this case the user did not enter the site key
                    // although the path is already absolute; note that
                    // here we prevent cross domain URIs too (i.e. another
                    // Snap! website on the same system cannot be accessed
                    // in this way.)
                    //
                    // WARNING: DO NOT MOVE THE CANONALIZATION BEFORE THE IF()
                    //          it removes the '/' at the start!
                    f_snap->canonicalize_path(source);
                    source = site_key + source;
                }
            }
        }
    }
    else
    {
        // no token found, return as is so the [...] remains as is
        // in the source
        return;
    }

    if(source.isEmpty())
    {
        token.f_error = true;
        token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">error:</span> Could not determine a valid resource path.</span>";
        SNAP_LOG_ERROR("form::on_replace_token() could not determine a valid resource path (empty) for token \"")(token.f_name)("\" and owner \"")(plugin_owner)("\".");
        return;
    }

    // verify that we had a valid plugin owner (we just cannot be processing
    // anything without such.)
    if(plugin_owner.isEmpty())
    {
        token.f_error = true;
        token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">error:</span> Resource \"" + source + "\" could not determine the plugin owner.</span>";
        SNAP_LOG_ERROR("form::on_replace_token() could not determine the plugin owner for \"" + source + "\" resource file.");
        return;
    }

    // if we get here and source is not empty then a form was loaded
    // successfully
    // 0. load the form from resources or Cassandra
    QString error;
    form_doc = load_form(ipath, source, error);
    if(!error.isEmpty())
    {
        token.f_error = true;
        token.f_replacement = error;
        return;
    }
    QDomElement snap_form(form_doc.documentElement());

    // 1. Initialize session
    //
    // initialize the session information
    sessions::sessions::session_info info;
    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_FORM);
    //info.set_object_path(); -- form sessions are based on paths only, no objects
    info.set_user_agent(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));

    // 2. Get session identifier and optionally the type
    //
    // retrieve the identifier for this session so one can verify when
    // processing the POST; we default to 1 for all those plugins that
    // only have one form; the session information also may starts with
    // "user/" or "form/" or "secure/" and those define the level of
    // complexity for the session identifier; the default is "form/"
    QString session_id_str(snap_form.attribute("session_id", "1"));
    if(session_id_str.startsWith("form/"))
    {
        // this is the default, we still need to remove the prefix
        session_id_str = session_id_str.mid(5);
    }
    else if(session_id_str.startsWith("user/"))
    {
        session_id_str = session_id_str.mid(5);
        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_USER);
    }
    else if(session_id_str.startsWith("secure/"))
    {
        session_id_str = session_id_str.mid(7);
        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_SECURE);
    }
    bool ok(false);
    info.set_session_id(session_id_str.toInt(&ok));
    if(!ok)
    {
        token.f_error = true;
        token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> Session identifier \"" + session_id_str + "\" is not a valid decimal number.</span>";
        SNAP_LOG_ERROR("form::on_replace_token() could not parse \"" + session_id_str + "\" as a session identifier.");
        return;
    }

    // 3. Get form auto-reset
    //
    // The auto-reset timeout defaults to 1h (in minutes)
    //
    // Note: we don't need this parameter for the session, we probably
    // could remove the code although this verifies that if the
    // auto-reset is defined that it is correct
    int auto_reset_timeout(60);
    QDomElement auto_reset(form_doc.firstChildElement("auto-reset"));
    if(!auto_reset.isNull())
    {
        QString const minutes(auto_reset.attribute("minutes"));
        if(!minutes.isEmpty())
        {
            auto_reset_timeout = minutes.toInt(&ok);
            if(!ok)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> Session auto-reset minutes attribute (" + minutes + ") is not a valid decimal number.</span>";
                SNAP_LOG_ERROR("form::on_replace_token() could not parse \"" + minutes + "\" as a auto-reset timeout in minutes.");
                return;
            }
        }
    }

    // 4. Get form timeout
    //
    // The timeout defaults to 8h (in minutes)
    int timeout(8 * 60);
    QDomElement timeout_tag(form_doc.firstChildElement("timeout"));
    if(!timeout_tag.isNull())
    {
        QString const minutes(timeout_tag.attribute("minutes"));
        if(!minutes.isEmpty())
        {
            timeout = minutes.toInt(&ok);
            if(!ok)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> Session timeout minutes attribute (" + minutes + ") is not a valid decimal number.</span>";
                SNAP_LOG_ERROR("form::on_replace_token() could not parse \"" + minutes + "\" as a timeout in minutes.");
                return;
            }
        }
    }
    info.set_time_to_live(timeout * 60);  // time to live is in seconds, timeout is in minutes

    // 5. Define the owner of the form
    //
    // In many cases the owner is not defined in the form in which
    // case it comes from the page; however, if defined in the form
    // then it is viewed as authoritative
    QString const owner(snap_form.attribute("owner", plugin_owner));
    // if(owner != plugin_owner) -- what would that test mean?
    // I think the owner in a form is authoritative, period.
    info.set_plugin_owner(owner);

    // 6. Define the path of the form from the XML document
    //
    // If not empty then it was already defined in the previous step
    // (when retrieving the form:: from the page)
    info.set_page_path(ipath);

    // 7. Run the XSLT against the form and save the result
    //
    QDomDocument result(form_to_html(info, form_doc));
    token.f_replacement = result.toString(-1);
//printf("form is [%s] (%d), %d\n", token.f_replacement.toUtf8().data(), static_cast<int>(token.f_error), static_cast<int>(token.f_found));
}


/** \brief Retrieve the path to the form of the specified page.
 *
 * This function retrieves the path to the form specified in this page.
 * It is possible to include this form in the body using the [form::source]
 * token.
 *
 * This is particularly useful to automatically retrieve forms for
 * settings so the user does not even have to derive from the form_post
 * interface.
 *
 * \param[in] owner  The name of the plugin owner.
 * \param[in,out] ipath  The page of which we want to take the default form.
 *
 * \return The path to the form.
 */
QString form::get_source(QString const & owner, content::path_info_t & ipath)
{
    libdbproxy::table::pointer_t branch_table(content::content::instance()->get_branch_table());
    if(!branch_table->exists(ipath.get_branch_key()))
    {
        return QString();
    }
    libdbproxy::row::pointer_t row(branch_table->getRow(ipath.get_branch_key()));
    if(!row->exists(get_name(name_t::SNAP_NAME_FORM_SOURCE)))
    {
        return QString();
    }

    QString source(row->getCell(get_name(name_t::SNAP_NAME_FORM_SOURCE))->getValue().stringValue());
    if(source.isEmpty())
    {
        // if empty it is not valid
        return source;
    }

    if(source == "settings")
    {
        // assume the default settings form filename
        source = ":/xml/" + owner + "/settings-form.xml";
    }
    else if(!source.contains("/"))
    {
        // assume the default resource filename
        source = ":/xml/" + owner + "/" + source + ".xml";
    }

    SNAP_LOG_TRACE("Form source filename: \"")(source)("\"");

    return source;
}


/** \brief Setup for forms.
 *
 * The forms make use of some JavaScript code and this function is used to
 * insert it if the page includes one or more forms.
 *
 * \param[in] ipath  The path being managed.
 * \param[in,out] doc  The document that was generated.
 * \param[in] xsl  The XSLT document that is about to be used to transform
 *                 the body (still as a string).
 */
void form::on_filtered_content(content::path_info_t & ipath, QDomDocument & doc, QString const & xsl)
{
    NOTUSED(ipath);
    NOTUSED(xsl);

    if(f_form_initialized)
    {
        content::content::instance()->add_javascript(doc, "form");
    }
}


void form::on_copy_branch_cells(libdbproxy::cells& source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    NOTUSED(destination_branch);

    content::content::copy_branch_cells_as_is(source_cells, destination_row, get_name(name_t::SNAP_NAME_FORM_NAMESPACE));
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
