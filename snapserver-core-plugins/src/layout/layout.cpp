// Snap Websites Server -- handle the theme/layout information
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

#include "layout.h"

#include "../filter/filter.h"
#include "../taxonomy/taxonomy.h"
#include "../path/path.h"

#include <snapwebsites/log.h>
#include <snapwebsites/qdomreceiver.h>
#include <snapwebsites/qhtmlserializer.h>
#include <snapwebsites/qxmlmessagehandler.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/qdomxpath.h>
//#include <snapwebsites/qdomnodemodel.h> -- at this point the DOM Node Model seems bogus.
#include <snapwebsites/snap_expr.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/xslt.h>

#include <iostream>
#include <fstream>

#include <QFile>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(layout, 1, 0)


/** \brief Get a fixed layout name.
 *
 * The layout plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_LAYOUT_BODY_XSL:
        return "body-parser.xsl";

    case name_t::SNAP_NAME_LAYOUT_BOX:
        return "layout::box";

    case name_t::SNAP_NAME_LAYOUT_BOXES:
        return "layout::boxes";

    case name_t::SNAP_NAME_LAYOUT_CONTENT_XML:
        return "content.xml";

    case name_t::SNAP_NAME_LAYOUT_LAYOUT:
        return "layout::layout";

    case name_t::SNAP_NAME_LAYOUT_LAYOUTS_PATH:
        return "layouts";

    case name_t::SNAP_NAME_LAYOUT_NAMESPACE:
        return "layout";

    case name_t::SNAP_NAME_LAYOUT_REFERENCE:
        return "layout::reference";

    case name_t::SNAP_NAME_LAYOUT_TABLE:
        return "layout";

    case name_t::SNAP_NAME_LAYOUT_THEME:
        return "layout::theme";

    case name_t::SNAP_NAME_LAYOUT_THEME_XSL:
        return "theme-parser.xsl";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_LAYOUT_...");

    }
    NOTREACHED();
}


/** \brief Initialize the layout plugin.
 *
 * This function is used to initialize the layout plugin object.
 */
layout::layout()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the layout plugin.
 *
 * Ensure the layout object is clean before it is gone.
 */
layout::~layout()
{
}


/** \brief Initialize the layout.
 *
 * This function terminates the initialization of the layout plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void layout::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(layout, "server", server, load_file, _1, _2);
    SNAP_LISTEN(layout, "server", server, improve_signature, _1, _2, _3);
    SNAP_LISTEN(layout, "content", content::content, copy_branch_cells, _1, _2, _3);
}


/** \brief Get a pointer to the layout plugin.
 *
 * This function returns an instance pointer to the layout plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the layout plugin.
 */
layout * layout::instance()
{
    return g_plugin_layout_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString layout::icon() const
{
    return "/images/snap/layout-logo-64x64.png";
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
QString layout::description() const
{
    return "Determine the layout for a given content and generate the output"
          " for that layout.";
}


/** \brief Return our dependencies
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString layout::dependencies() const
{
    return "|content|filter|links|path|server_access|taxonomy|";
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
 * \return The UTC Unix date of the last update of this plugin or a layout.
 */
int64_t layout::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    // first time, make sure the default theme is installed
    //
    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, content_update);

    // always call the do_layout_updates() function since it may be
    // that a layout was updated at a date different from any other
    //
    // i.e. you may upgrade layout A, run snapinstallwebsite,
    //      upgrade layout B, run snapinstallwebsite
    //
    // In that example, if B has a "last update" timestamp that's
    // smaller than A's "last update" timestamp, we cannot here
    // know that B has a smaller timestamp and thus we have to check
    // each entry and make sure they all get updated acconding to
    // their own "last update" timestamp.
    //
    // TBD: This may be too soon because the output and editor
    //      will add their own themes AFTER this call (i.e. they
    //      depend on us, not the other way around.)
    //
    //      We may instead need to have a form of signal to know
    //      that we need to do something.
    //
    //      That being said, the first time I do not think we need
    //      that because the install_layout() does the necessar on
    //      its own. (we do not need to install and update.)
    //
    do_layout_updates();

    SNAP_PLUGIN_UPDATE_EXIT();
}


int64_t layout::do_dynamic_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    // we cannot use a static data here since a layout can be updated at
    // any time and we already check that, at this point we have a list
    // of names that the next function can use
    //
    finish_install_layout();

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our layout references.
 *
 * This first initialization is used to initialize the default
 * layout.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void layout::content_update(int64_t const last_updated)
{
    NOTUSED(last_updated);

    install_layout("default");
}


/** \brief Update layouts as required.
 *
 * This function goes through the list of layouts that are installed
 * on this website. If some need to be updated, run the update code
 * as required against those.
 *
 * Whenever you update a layout file, all references are reset to zero.
 * This function searches such references and if zero, does the update
 * and then sets the reference to one.
 */
void layout::do_layout_updates()
{
    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    //libdbproxy::table::pointer_t layout_table(get_layout_table());

    // the user may have asked to install a layout too
    //
    QString const qs_install_layout(f_snap->get_server_parameter("qs_layout::install"));
    if(!qs_install_layout.isEmpty())
    {
        snap_uri const & uri(f_snap->get_uri());
        QString const install_layouts(uri.query_option(qs_install_layout));
        snap_string_list layout_names(install_layouts.split(","));
        for(auto const & ln : layout_names)
        {
            QString const layout_name(ln.trimmed());
            if(!layout_name.isEmpty())
            {
                install_layout(layout_name);
            }
        }
    }

    QString const site_key(f_snap->get_site_key_with_slash());
    QString const base_key(site_key + get_name(name_t::SNAP_NAME_LAYOUT_LAYOUTS_PATH) + "/");

    // verify that we have a valid layout-page type, without it we cannot
    // install the layout
    //
    content::path_info_t types_ipath;
    types_ipath.set_path("types/taxonomy/system/content-types/layout-page");
    if(!content_table->exists(types_ipath.get_key()))
    {
        // this is likely to happen on first initialization, which is fine
        // because the install_layout() gets called directory and we get
        // the wanted effect anyway
        //
        return;
    }

    // get the list of links to layouts, that's our list of layout for
    // this one website (i.e. we do not update all the layouts for one
    // website that may be using just one or two layouts...)
    //
    links::link_info info(content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE), false, types_ipath.get_key(), types_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info layout_info;
    while(link_ctxt->next_link(layout_info))
    {
        QString const layout_key(layout_info.key());
        if(layout_key.startsWith(base_key))
        {
            QString const name(layout_key.mid(base_key.length()));
            int const pos(name.indexOf('/'));
            if(pos < 0)
            {
                // 'name' is the name of a layout

                // define limit with the original last_updated because
                // the order in which we read the layouts has nothing to
                // do with the order in which they were last updated
                //
                // TODO: change the algorithm to use one last_updated time
                //       per layout (just like plugins, having a single
                //       time definition is actually bogus)
                //
                install_layout(name);
            }
        }
    }
}


/** \brief Initialize the layout table.
 *
 * This function creates the layout table if it doesn't exist yet. Otherwise
 * it simple retrieves it from Cassandra.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \return The shared pointer to the layout table.
 */
libdbproxy::table::pointer_t layout::get_layout_table()
{
    return f_snap->get_table(get_name(name_t::SNAP_NAME_LAYOUT_TABLE));
}


/** \brief Retrieve the name of a theme or layout.
 *
 * This function checks for the name of a theme or layout in the current object
 * or the specified type and its parent.
 *
 * \param[in,out] ipath  The path to the content to process.
 * \param[in] column_name  The name of the column to search (layout::theme
 *                         or layout::layout).
 * \param[in] use_qs_theme  Whether the ?theme=...&layout=... query string
 *                          parameters should be checked.
 *
 * \return The name of the layout, may be "default" if no other name was found.
 */
QString layout::get_layout(content::path_info_t & ipath, QString const & column_name, bool use_qs_theme)
{
    QString layout_name;

    // TODO: if the parameters are the same, then the same result is to
    //       be returned, we may want to look for a way to cache if it
    //       makes things faster (i.e. the layout and editor both call
    //       this function with the same parameters.)

    // TODO: We may actually want to first check the page theme, then the
    //       Query String user definition; although frankly that would not
    //       make sense; we definitively have a problem here because the
    //       theme of each item should probably follow the main theme and
    //       not change depending on the part being themed...

    // first check whether the user is trying to overwrite the layout
    //
    if(use_qs_theme && ipath.is_main_page())
    {
        QString const qs_layout(f_snap->get_server_parameter("qs_" + column_name));
        if(!qs_layout.isEmpty())
        {
            // although query_option("") works as expected by returning ""
            // we avoid the call to the get_uri() by testing early
            //
            snap_uri const & uri(f_snap->get_uri());
            layout_name = uri.query_option(qs_layout);
        }
    }

    if(layout_name.isEmpty())
    {
        // try the content itself since the user did not define a theme
        libdbproxy::value layout_value(content::content::instance()->get_content_table()->getRow(ipath.get_key())->getCell(column_name)->getValue());
        if(layout_value.nullValue())
        {
            // that very content does not define a layout, check its type(s)
            layout_value = taxonomy::taxonomy::instance()->find_type_with(
                ipath,
                content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE),
                column_name,
                content::get_name(content::name_t::SNAP_NAME_CONTENT_CONTENT_TYPES_NAME)
            );
            if(layout_value.nullValue())
            {
                // no layout, check the .conf
                layout_value = f_snap->get_server_parameter(column_name);
                if(layout_value.nullValue())
                {
                    // user did not define any layout, set the value to "default"
                    //
                    layout_value = QString("\"default\"");
                }
                else
                {
                    // the name coming from the server .conf file will
                    // not be JavaScript, only a name, change it so it
                    // is compatible with the rest of the code below
                    //
                    // TODO: make sure the name is not tainted.
                    //
                    layout_value.setStringValue(QString("\"%1\"").arg(layout_value.stringValue()));
                }
            }
        }

        // TODO: remove support for the ';' at the end of the line
        //       (or add support for multiple lines in snap_expr?)
        //

        QString layout_script(layout_value.stringValue());
        if(layout_script.endsWith(";"))
        {
            // get rid of the ending ";" (snap_expr does not support that)
            //
            layout_script = layout_script.mid(0, layout_script.length() - 1);
        }

        bool run_script(true);
        if(layout_script.startsWith("\"")
        && layout_script.endsWith("\"")
        && layout_script.length() >= 3)
        {
            run_script = false;
            QString const check_name(layout_script.mid(1, layout_script.length() - 2));
            QByteArray const utf8(check_name.toUtf8());
            for(char const *s(utf8.data()); *s != '\0'; ++s)
            {
                // verify allowed characters, if one is not allowed, run
                // the script, to make sure...
                //
                if((*s < 'a' || *s > 'z')
                && (*s < 'A' || *s > 'Z')
                && (*s < '0' || *s > '9')
                && *s != '-'
                && *s != '_')
                {
                    run_script = true;
                    break;
                }
            }
        }
//SNAP_LOG_INFO("Layout selection with [")(layout_script)("] run_script = ")(run_script)(" for ")(ipath.get_key());

        if(run_script)
        {
            snap_expr::expr e;
            if(e.compile(layout_script))
            {
                // TODO: we could serialize the program and save it in the
                //       cache table. That way we could avoid calling
                //       "compile()" each time.
                //
                snap_expr::variable_t::variable_map_t variables;
                snap_expr::variable_t var_path("path");
                var_path.set_value(ipath.get_cpath());
                variables["path"] = var_path;
                snap_expr::variable_t var_page("page");
                var_page.set_value(ipath.get_key());
                variables["page"] = var_page;
                snap_expr::variable_t var_column_name("column_name");
                var_column_name.set_value(column_name);
                variables["column_name"] = var_column_name;

                snap_expr::variable_t result;
                snap_expr::functions_t functions;
                e.execute(result, variables, functions);

                layout_name = result.get_string("result");
            }
            else
            {
                // let admins know there is a bug in their layout script
                //
                SNAP_LOG_ERROR("could not compile layout script: [")(layout_script)("]");
            }
        }
        else
        {
            // remove the quotes really quick, we avoid the whole script deal!
            //
            layout_name = layout_script.mid(1, layout_script.length() - 2);
        }

        // does it look like the script failed? if so get a default
        //
        if(layout_name.isEmpty())
        {
            // TODO: make sure the name is not tainted.
            //
            layout_name = f_snap->get_server_parameter(column_name);
        }
        if(layout_name.isEmpty())
        {
            layout_name = "default";
        }
    }
    else
    {
        // in this case we do not run any kind of script, the name has
        // to be specified as is
        QByteArray const utf8(layout_name.toUtf8());
        for(char const *s(utf8.data() + 1); *s != '\0'; ++s)
        {
            if((*s < 'a' || *s > 'z')
            && (*s < 'A' || *s > 'Z')
            && (*s < '0' || *s > '9')
            && *s != '-'
            && *s != '_')
            {
                // tainted layout/theme name
                f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                        "Layout Not Found",
                        QString("User specified layout \"%1\"").arg(layout_name),
                        "Found a tainted layout name, refusing it!");
                NOTREACHED();
            }
        }
    }

    return layout_name;
}


/** \brief Apply the layout to the content defined at \p cpath.
 *
 * This function defines a page content using the data as defined by \p cpath.
 *
 * First it looks for a JavaScript under the column key "layout::theme".
 * If such doesn't exist at cpath, then the function checks the \p cpath
 * content type link. If that type of content has no "layout::theme" then
 * the parent type is checked up to the "Content Types" type.
 *
 * The result is a new document with the data found at cpath and any
 * references as determine by the theme and layouts used by the process.
 * The type of the new document depends on the layout (it could be XHTML,
 * XML, PDF, text, SVG, etc.)
 *
 * You may use the create_body() function directly to gather all the data
 * to be used to create a page. The apply_theme() will then layout the
 * result in a page.
 *
 * \param[in,out] ipath  The canonicalized path of content to be laid out.
 * \param[in] body_plugin  The plugin that will generate the content of the page.
 *
 * \return The result is the output of the layout applied to the data in cpath.
 */
QString layout::apply_layout(content::path_info_t & ipath, layout_content * body_plugin)
{
    // Determine the name of the theme
    // (Note: we need that name to determine the body XSLT data)
    //
    QString theme_name;
    QString theme_xsl(define_layout(
                  ipath
                , get_name(name_t::SNAP_NAME_LAYOUT_THEME)
                , get_name(name_t::SNAP_NAME_LAYOUT_THEME_XSL)
                , ":/xsl/layout/default-theme-parser.xsl"
                , QString() // no theme name defined yet...
                , theme_name));

    // Get the body XSLT data
    //
    QString layout_name;
    QString body_xsl(define_layout(
                  ipath
                , get_name(name_t::SNAP_NAME_LAYOUT_LAYOUT)
                , get_name(name_t::SNAP_NAME_LAYOUT_BODY_XSL)
                , ":/xsl/layout/default-body-parser.xsl"
                , theme_name
                , layout_name));

    // Generate the body document now
    //
    QDomDocument doc(create_document(ipath, dynamic_cast<plugin *>(body_plugin)));
    create_body(doc, ipath, body_xsl, body_plugin, true, layout_name, theme_name);

    // Then apply the theme to the body document
    //
    // HTML5 DOCTYPE is just "html" as follow
    //
    return "<!DOCTYPE html>" + apply_theme(doc, theme_xsl, theme_name);
}


/** \brief Determine the layout XSL code and name.
 *
 * This function determines the layout XSL code and name given a content
 * info path.
 *
 * The \p name parameter defines the field to be used. By default it is
 * expected to be set to layout::layout or layout::theme, but other names
 * could be used. The default names come from name_t::SNAP_NAME_LAYOUT_LAYOUT and
 * name_t::SNAP_NAME_LAYOUT_THEME names.
 *
 * The \p key parameter is the name of the cell to load from the layout
 * table if the name parameter is something else than "default". Note that
 * the key can be overwritten if the name returns a theme name and a key
 * name separated by a slash. For example, we could have:
 *
 * \code
 * "bare/blog"
 * \endcode
 *
 * which could be used to display the blog page when the user visits one
 * of those pages. Note that this name must match one to one to what is
 * saved in the layout table (cell name to be loaded.) It cannot include
 * a colon.
 *
 * \param[in,out] ipath  The canonicalized path of content to be laid out.
 * \param[in] name  The name of the field to user to retrieve the layout name
 *                  from the database (expects layout::layout or layout::theme)
 * \param[in] key  The key of the cell to load the XSL from.
 * \param[in] default_filename  The name of a resource file representing
 *                              the default theme in case no theme was
 *                              already specified.
 * \param[in] theme_name  A QString holding the name of the theme.
 * \param[out] layout_name  A QString to hold the resulting layout name.
 *
 * \return The XSL code in a string.
 */
QString layout::define_layout(
              content::path_info_t & ipath
            , QString const & name
            , QString const & key
            , QString const & default_filename
            , QString const & theme_name
            , QString & layout_name)
{
    // result variable
    //
    QString xsl;

    // Retrieve the name of the layout for this path and column name
    //
    layout_name = get_layout(ipath, name, true);

//SNAP_LOG_TRACE("Got theme / layout name = [")(layout_name)("] (key=")(ipath.get_key())(", theme_name=")(theme_name)(")");

    // If layout_name is not default, attempt to obtain the selected
    // XSL file from the layout table.
    //
    if(layout_name != "default")
    {
        // the layout name may have two entries: "row/cell" so we check
        // that first and cut the name in half if required
        //
        snap_string_list const names(layout_name.split("/"));
        if(layout_name.isEmpty() || names.size() > 2)
        {
            // can be one or two workds, no more
            //
            f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Layout Unavailable",
                    "Somehow no website layout was accessible.",
                    QString("layout::define_layout() found more than one '/' in \"%1\".").arg(layout_name));
            NOTREACHED();
        }

        // The following two lines are really ugly:
        //   1. we may want to remove the support for the "<theme>/<layout>" syntax,
        //      it's not needed now that we clearly have a theme_name
        //   2. we may want to do it with if()'s instead of just '?:' expressions
        //
        // Note: when theme_name.isEmpty() is true, we are retrieving the theme name...
        //       and when false, we are retrieving the layout name
        //
        layout_name = names.size() >= 2 || theme_name.isEmpty() ? names[0] : theme_name;
        QString cell_name(names.size() >= 2 ? names[1] : (names[0] == theme_name || theme_name.isEmpty() ? key : names[0]));

        // quick verification of the cell_name, just in case
        //
        if(cell_name == "content"
        || cell_name == "content.xml"
        || cell_name == "style"
        || cell_name == "style.css"
        || cell_name == "."
        || cell_name == ".."
        || cell_name.contains(':'))
        {
            // this is just to try to avoid some security issues
            //
            f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Layout Unavailable",
                    QString("The name \"%1\" used as the layout cell is not acceptable.").arg(cell_name),
                    "layout::define_layout() found an illegal cell name.");
            NOTREACHED();
        }

        // most often we do not put the .xsl at the end of the name
        //
        if(!cell_name.endsWith(".xsl"))
        {
            cell_name += ".xsl";
        }

        // try to load the layout from the database (i.e. any theme can
        // thus overload any system/plugin form!), if not found
        // we will try the Qt resources and if that fails too
        // switch to the default layout instead
        //
        libdbproxy::table::pointer_t layout_table(get_layout_table());
        libdbproxy::value const layout_value(layout_table->getRow(layout_name)->getCell(cell_name)->getValue());
        if(layout_value.nullValue())
        {
            // no data found in the layout database
            // the XSLT data may be in Qt, so we check there,
            // but we still return the layout name as "default"
            // (which is possibly wrong but works with my current tests)
            //
            QByteArray data;
            QString const rc_name(QString(":/xsl/layout/%1").arg(cell_name));
            QFile rc_parser(rc_name);
            if(rc_parser.open(QIODevice::ReadOnly))
            {
                data = rc_parser.readAll();
            }
            if(!data.isEmpty())
            {
                xsl = QString::fromUtf8(data.data(), data.size());
            }
            else
            {
                // this warning will help at least me to debug a problem
                // with loading a layout
                //
                SNAP_LOG_WARNING("layout data named \"")(names.join("/"))("\" could not be loaded. We will be using the \"default\" layout instead.");

                // if we could not load any XSL, switch to the default theme
                //
                // (note: we do not need to test that in the else part below
                // since we already checked that layout_value was not empty)
                //
                layout_name = "default";
            }
        }
        else
        {
            xsl = layout_value.stringValue();
        }
    }

    // Fallback to the default theme if none was set properly above.
    //
    if(xsl.isEmpty() && layout_name == "default")
    {
        // Grab the default theme XSL from the Qt resources.
        //
        QFile file(default_filename);
        if(!file.open(QIODevice::ReadOnly))
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Layout Unavailable",
                    "Somehow no website layout was accessible, not even the internal default.",
                    "layout::define_layout() could not open \"" + default_filename + "\" resource file.");
            NOTREACHED();
        }
        QByteArray const data(file.readAll());
        if(data.size() == 0)
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Layout Unavailable",
                    "Somehow no website layout was accessible, not even the internal default.",
                    "layout::define_layout() could not read the \"" + default_filename + "\" resource file.");
            NOTREACHED();
        }
        xsl = QString::fromUtf8(data.data(), data.size());
    }

    // replace <xsl:include ...> with other XSTL files (should be done
    // by the parser, but Qt's parser does not support it yet)
    //
    replace_includes(xsl);

    return xsl;
}


/** \brief Create the layout XML document
 *
 * This function creates the basic layout XML document which is composed
 * of a root, a header and a page. The following shows the tree that
 * you get:
 *
 * \code
 *   + snap (path=... owner=...)
 *     + head
 *       + metadata
 *     + page
 *       + body
 * \endcode
 *
 * The root element, which is named "snap", is given the ipath as the
 * path attribute, and the name of the plugin as the owner attribute.
 *
 * \param[in,out] ipath  The path to set in the snap tag.
 * \param[in] content_plugin  The plugin whose name is added to the snap tag.
 *
 * \return A DOM document with the basic layout tree.
 */
QDomDocument layout::create_document(content::path_info_t & ipath, plugin * content_plugin)
{
    // Initialize the XML document tree
    // More is done in the generate_header_content_impl() function
    QDomDocument doc;
    QDomElement root(doc.createElement("snap"));
    root.setAttribute("path", ipath.get_cpath());

    if(content_plugin != nullptr)
    {
        root.setAttribute("owner", content_plugin->get_plugin_name());
    }

    doc.appendChild(root);

    // snap/head/metadata
    QDomElement head(doc.createElement("head"));
    root.appendChild(head);
    QDomElement metadata(doc.createElement("metadata"));
    head.appendChild(metadata);

    // snap/page/body
    QDomElement page(doc.createElement("page"));
    root.appendChild(page);
    QDomElement body(doc.createElement("body"));
    page.appendChild(body);

    return doc;
}


/** \brief Create the body XML data.
 *
 * This function creates the entire XML data that will be used by the
 * theme XSLT parser. It first creates an XML document using the
 * different generate functions to create the header and page data,
 * then runs the body XSLT parser to format the specified content
 * in a valid HTML buffer (valid as in, valid HTML tags, as a whole
 * this is not a valid HTML document, only a block of content; in
 * particular, the result does not include the \<head> tag.)
 *
 * This function is often used to generate parts of the content such
 * as boxes on the side of the screen. It can also be used to create
 * content of a page from a template (i.e. the user profile is
 * created from the users/pages/profile template.) In many
 * cases, when the function is used in this way, only the title and
 * body are used. If a block is to generate something that should
 * appear in the header, then it should create it in the header of
 * the main page.
 *
 * \note
 * You may want to call the replace_includes() function on your XSLT
 * document before calling this function.
 *
 * \param[in,out] doc  The layout document being created.
 * \param[in,out] ipath  The path being dealt with.
 * \param[in] xsl  The XSL of this body layout.
 * \param[in] body_plugin  The plugin handling the content (body/title in general.)
 * \param[in] handle_boxes  Whether the boxes of this theme are to be handled.
 * \param[in] layout_name  The name of the layout (only necessary if handle_boxes is true.)
 * \param[in] theme_name  The name of the layout (only necessary if handle_boxes is true.)
 */
void layout::create_body(QDomDocument & doc
                       , content::path_info_t & ipath
                       , QString const & xsl
                       , layout_content * body_plugin
                       , bool const handle_boxes
                       , QString const & layout_name
                       , QString const & theme_name)
{
#ifdef DEBUG
SNAP_LOG_TRACE("layout::create_body() ... cpath = [")
              (ipath.get_cpath())
              ("] layout_name = [")
              (layout_name)
              ("] theme_name = [")
              (theme_name)
              ("]");

//SNAP_LOG_TRACE("The XSL of the layout body is: [")
//              (xsl)
//              ("]");
#endif

    // get the elements we are dealing with in this function
    QDomElement head(snap_dom::get_element(doc, "head"));
    QDomElement metadata(snap_dom::get_element(doc, "metadata"));
    QDomElement page(snap_dom::get_element(doc, "page"));
    QDomElement body(snap_dom::get_element(doc, "body"));

    metadata.setAttribute("layout-name", layout_name);
    metadata.setAttribute("theme-name", theme_name);

    // other plugins generate defaults
//std::cerr << "*** Generate header...\n" << doc.toString() << "-------------------------\n";
    generate_header_content(ipath, head, metadata);

    // concerned (owner) plugin generates content
//std::cerr << "*** Generate main content...\n";
    body_plugin->on_generate_main_content(ipath, page, body);
//std::cout << "Header + Main XML is [" << doc.toString(-1) << "]\n";

    // add boxes content
    // if the "boxes" entry does not exist yet then we can create it now
    // (i.e. we are creating a parent if the "boxes" element is not present;
    //       although we should not get called recursively, this makes things
    //       safer!)
    if(handle_boxes && page.firstChildElement("boxes").isNull())
    {
//std::cerr << "*** Generate boxes?!...\n";
        generate_boxes(ipath, theme_name, doc);
    }

    // other plugins are allowed to modify the content if so they wish
//std::cerr << "*** Generate page content...\n";
    generate_page_content(ipath, page, body);
//std::cout << "Prepared XML is [" << doc.toString(-1) << "]\n";

    // replace all tokens
    //
    // TODO: the filtering needs to be a lot more generic!
    //       plus the owner of the page should be able to select the
    //       filters he wants to apply against the page content
    //       (i.e. ultimately we want to have some sort of filter
    //       tagging capability)
    //
#ifdef _DEBUG
    //SNAP_LOG_DEBUG("*** Filter all of that...: [")(doc.toString())("]");
#endif
    filter::filter * filter_plugin(filter::filter::instance());
    filter_plugin->on_token_filter(ipath, doc);

    // XSLT parser may also request a pre-filtering
    //
    QString filtered_xsl(xsl);
    int const output_pos(filtered_xsl.indexOf("<output"));
    if(output_pos > 0)
    {
        for(QChar const * s(filtered_xsl.data() + output_pos + 7); !s->isNull(); ++s)
        {
            ushort c(s->unicode());
            if(c == '>')
            {
                // found end of tag, exit loop now
                break;
            }
            if(c == 'f')
            {
                // filter="token" or filter='token'
                if(s[1].unicode() == 'i'
                && s[2].unicode() == 'l'
                && s[3].unicode() == 't'
                && s[4].unicode() == 'e'
                && s[5].unicode() == 'r'
                && s[6].unicode() == '='
                && (s[7].unicode() == '"' || s[7].unicode() == '\'')
                && s[8].unicode() == 't'
                && s[9].unicode() == 'o'
                && s[10].unicode() == 'k'
                && s[11].unicode() == 'e'
                && s[12].unicode() == 'n'
                && (s[7].unicode() == s[13].unicode())) // closing must be the same as opening
                {
                    QDomDocument xsl_doc;
                    if(xsl_doc.setContent(filtered_xsl))
                    {
                        filter_plugin->on_token_filter(ipath, xsl_doc);
                        filtered_xsl = xsl_doc.toString(-1);
                    }
                    break;
                }
            }
        }
    }

//std::cerr << "*** Filtered content...\n";
    filtered_content(ipath, doc, filtered_xsl);

#ifdef _DEBUG
//SNAP_LOG_DEBUG("==== Generated XML is [")(doc.toString(-1))("]");
//SNAP_LOG_DEBUG("++++ fitered_xsl=")(filtered_xsl);
//std::cout << "Generated XSL is [" << xsl            << "]\n";
#endif

#if 0
QFile out("/tmp/doc.xml");
out.open(QIODevice::WriteOnly);
out.write(doc.toString(-1).toUtf8());
#endif

    // Somehow binding crashes everything at this point?! (Qt 4.8.1)
//std::cerr << "*** Generate output...\n";

    QDomDocument doc_output("output");

    xslt x;
    x.set_xsl(filtered_xsl);
    x.set_document(doc);
    x.evaluate_to_document(doc_output);

    extract_js_and_css(doc, doc_output);
    body.appendChild(doc.importNode(doc_output.documentElement(), true));
}


/** \brief Create the body of a page and return it as a string.
 *
 * This function, like apply_layout(), determines the name of the
 * layout to be used to parse the specified \p ipath page. Then
 * it generates the body in the existing document and returns it
 * as a string.
 *
 * The system may apply modifications to the header and other
 * parts of the document, but only if it was not already in
 * the source document. (i.e. it does not overwrite anything.)
 *
 * \note
 * You may want to call the replace_includes() function on your XSLT
 * document before calling this function.
 *
 * \param[in,out] doc  The layout document being created.
 * \param[in,out] ipath  The path being dealt with.
 * \param[in] body_plugin  The plugin handling the content (body/title in general.)
 *
 * \return The resulting body as a string.
 */
QString layout::create_body_string(
                         QDomDocument & doc
                       , content::path_info_t & ipath
                       , layout_content * body_plugin)
{
    // Determine the name of the theme
    // (Note: we need that name to determine the body XSLT data)
    // (Note: here we do not need the theme XSLT data so we ignore it)
    //
    QString theme_name;
    NOTUSED(define_layout(
                  ipath
                , get_name(name_t::SNAP_NAME_LAYOUT_THEME)
                , get_name(name_t::SNAP_NAME_LAYOUT_THEME_XSL)
                , ":/xsl/layout/default-theme-parser.xsl"
                , QString() // no theme name defined yet...
                , theme_name));

    // Get the body XSLT data
    //
    QString layout_name;
    QString filtered_xsl(define_layout(
                  ipath
                , get_name(name_t::SNAP_NAME_LAYOUT_LAYOUT)
                , get_name(name_t::SNAP_NAME_LAYOUT_BODY_XSL)
                , ":/xsl/layout/default-body-parser.xsl"
                , theme_name
                , layout_name));

    // Generate the body document now
    //
    QDomDocument page_doc(create_document(ipath, dynamic_cast<plugin *>(body_plugin)));

    // the following is the same as the create_body() function without
    // the boxes and using different documents
    //
#ifdef DEBUG
SNAP_LOG_TRACE("layout::create_body_string() ... cpath = [")
              (ipath.get_cpath())
              ("] layout_name = [")
              (layout_name)
              ("] unused theme_name = [")
              (theme_name)
              ("]");

//SNAP_LOG_TRACE("The XSL of the layout body is: [")
//              (xsl)
//              ("]");
#endif

    // get the elements we are dealing with in this function
    QDomElement head(snap_dom::get_element(page_doc, "head"));
    QDomElement metadata(snap_dom::get_element(page_doc, "metadata"));
    QDomElement page(snap_dom::get_element(page_doc, "page"));
    QDomElement body(snap_dom::get_element(page_doc, "body"));

    metadata.setAttribute("layout-name", layout_name);
    metadata.setAttribute("theme-name", theme_name);

    // other plugins generate defaults
//std::cerr << "*** Generate header...\n" << page_doc.toString() << "-------------------------\n";
    generate_header_content(ipath, head, metadata);

    // concerned (owner) plugin generates content
//std::cerr << "*** Generate main content...\n";
    body_plugin->on_generate_main_content(ipath, page, body);
//std::cout << "Header + Main XML is [" << page_doc.toString(-1) << "]\n";

    // no boxes for this one, boxes should appear only once and be handled
    // by the main layout and not the layout that will handle a standalone
    // page (although I'm not too sure whether that is correct right now,
    // I am sure that we do not want the boxes!)

    // other plugins are allowed to modify the content if so they wish
//std::cerr << "*** Generate page content...\n";
    generate_page_content(ipath, page, body);
//std::cout << "Prepared XML is [" << page_doc.toString(-1) << "]\n";

    // replace all tokens
    //
    // Note that we are in create_body_string() which is expected to
    // be called through a filter already and since the filtering
    // is "recursive" (whatever gets added to the output gets itself
    // parsed) we should not have to filter at this level. However,
    // the page_doc variable is very different and thus the filtering
    // for this very page is going to be different from filtering
    // using the parent QDomDocument variable. As a side effect, this
    // allows us to have a standalone function (i.e. it can be called
    // from other places than just the filter implementing the
    // "content::page" token.)
    //
    // TODO: the filtering needs to be a lot more generic!
    //       plus the owner of the page should be able to select the
    //       filters he wants to apply against the page content
    //       (i.e. ultimately we want to have some sort of filter
    //       tagging capability)
    //
//SNAP_LOG_WARNING("*** Filter all of that...: [")(page_doc.toString())("]");
    filter::filter * filter_plugin(filter::filter::instance());
    filter_plugin->on_token_filter(ipath, page_doc);

    // XSLT parser may also request a pre-filtering
    //
    int const output_pos(filtered_xsl.indexOf("<output"));
    if(output_pos > 0)
    {
        for(QChar const * s(filtered_xsl.data() + output_pos + 7); !s->isNull(); ++s)
        {
            ushort c(s->unicode());
            if(c == '>')
            {
                // found end of tag, exit loop now
                break;
            }
            if(c == 'f')
            {
                // filter="token" or filter='token'
                if(s[1].unicode() == 'i'
                && s[2].unicode() == 'l'
                && s[3].unicode() == 't'
                && s[4].unicode() == 'e'
                && s[5].unicode() == 'r'
                && s[6].unicode() == '='
                && (s[7].unicode() == '"' || s[7].unicode() == '\'')
                && s[8].unicode() == 't'
                && s[9].unicode() == 'o'
                && s[10].unicode() == 'k'
                && s[11].unicode() == 'e'
                && s[12].unicode() == 'n'
                && (s[7].unicode() == s[13].unicode())) // closing must be the same as opening
                {
                    QDomDocument xsl_doc;
                    if(xsl_doc.setContent(filtered_xsl))
                    {
                        filter_plugin->on_token_filter(ipath, xsl_doc);
                        filtered_xsl = xsl_doc.toString(-1);
                    }
                    break;
                }
            }
        }
    }

//std::cerr << "*** Filtered content...\n";
    // XXX: although we filtered, I'm not totally sure we want to run this
    //      one here--the ipath is different from the caller's so it could
    //      have side effects we would not otherwise get in the parent's
    //      page filtered_content() call.
    //
    filtered_content(ipath, page_doc, filtered_xsl);

#if 0
std::cout << "Generated XML is [" << page_doc.toString(-1) << "]\n";
std::cout << "Generated XSL is [" << xsl            << "]\n";
#endif

#if 0
QFile out("/tmp/page_doc.xml");
out.open(QIODevice::WriteOnly);
out.write(page_doc.toString(-1).toUtf8());
#endif

    // Somehow binding crashes everything at this point?! (Qt 4.8.1)
//std::cerr << "*** Generate output...\n";

    QDomDocument doc_output("output");

    xslt x;
    x.set_xsl(filtered_xsl);
    x.set_document(page_doc);
    x.evaluate_to_document(doc_output);

    extract_js_and_css(doc, doc_output);
    //body.appendChild(doc.importNode(doc_output.documentElement(), true));

    return snap_dom::xml_children_to_string(doc_output.documentElement());
}


/** \brief Extract any JavaScript and CSS references.
 *
 * When running the XSLT parser the user may want to add layout specific
 * scripts by adding tags as follow:
 *
 * \code
 * <javascript name="/path/of/js"/>
 * <css name="/path/of/css"/>
 * \endcode
 *
 * This will place those definitions in the HTML \<head\> tag and ensure that
 * their dependencies also get included (which is probably the most important
 * part of the mechanism.)
 *
 * The function removes the definitions from the \p doc_output document.
 *
 * \param[in,out] doc  Main document where the JavaScript and CSS are added.
 * \param[in,out] doc_output  The document where the defines are taken from.
 */
void layout::extract_js_and_css(QDomDocument & doc, QDomDocument & doc_output)
{
    content::content * content_plugin(content::content::instance());

    // javascripts can be added in any order because we have
    // proper dependencies thus they automatically get sorted
    // exactly as required (assuming the programmers know what
    // they are doing....)
    QDomNodeList all_js(doc_output.elementsByTagName("javascript"));
    int js_idx(all_js.size());
    while(js_idx > 0)
    {
        --js_idx;
        QDomNode node(all_js.at(js_idx));
        QDomElement js(node.toElement());
        if(!js.isNull())
        {
            QString const name(js.attribute("name"));
            content_plugin->add_javascript(doc, name);

            // done with that node, remove it
            QDomNode parent(node.parentNode());
            parent.removeChild(node);
        }
    }

    // At this pointer the CSS are not properly defined with
    // dependencies (although I think they should just like
    // their JavaScript counter part.) So we have to add
    // them in the order they were defined in
    QDomNodeList all_css(doc_output.elementsByTagName("css"));
    while(all_css.size())
    {
        QDomNode node(all_css.at(0));
        QDomElement css(node.toElement());
        if(!css.isNull())
        {
            QString const name(css.attribute("name"));
            content_plugin->add_css(doc, name);

            // done with that node, remove it
            QDomNode parent(node.parentNode());
            parent.removeChild(node);
        }
    }
}


/** \brief Generate a list of boxes.
 *
 * This function handles the page boxes of a theme. This is generally only
 * used for main pages. When creating a body, you may specify whether you
 * want to also generate the boxes for that body.
 *
 * The function retrieves the boxes found in that theme and goes through
 * the list and generates all the boxes that are accessible by the user.
 *
 * The list of boxes to display is taken from the page, the type of the
 * page, or the layout (NOTE: the page and type are not yet implemented.)
 * The name of the cell used to retrieve the layout boxes is simple:
 * "layout::boxes". Note that these definitions are not cumulative. The
 * first list of boxes we find is the one that gets used. Thus, the user
 * can specialize the list of boxes to use on a per page or per type basis.
 *
 * The path used to find the layout list of boxes is:
 *
 * \code
 * layouts/<layout name>
 * \endcode
 *
 * The boxes are defined inside the layout and are found by their name.
 * The name of a box is limited to what is acceptable in a path (i.e.
 * [-_a-z0-9]+). For example, a box named left would appear as:
 *
 * \code
 * layouts/<layout name>/left
 * \endcode
 *
 * \param[in,out] ipath  The path being dealt with.
 * \param[in] doc  The document we're working on.
 * \param[in] layout_name  The name of the layout being worked on.
 */
void layout::generate_boxes(content::path_info_t & ipath, QString const & layout_name, QDomDocument doc)
{
    // the list of boxes is defined in the database under (GLOBAL)
    //    layouts/<layout_name>[layout::boxes]
    // as one row name per box; for example, the left box would appears as:
    //    layouts/<layout_name>/left
    QDomElement boxes(doc.createElement("boxes"));

    QDomNodeList all_pages(doc.elementsByTagName("page"));
    if(all_pages.isEmpty())
    {
        // this should never happen because we do explicitly create this
        // <page> tag before calling this function
        throw snap_logic_exception("layout::generate_boxes() <page> tag not found in the body DOM");
    }
    QDomElement page(all_pages.at(0).toElement());
    if(page.isNull())
    {
        // we just got a tag, this is really impossible!?
        throw snap_logic_exception("layout::generate_boxes() <page> tag not a DOM Element???");
    }
    page.appendChild(boxes);

    // Search for a list of boxes:
    //
    //   . Under "/snap/head/metadata/boxes" of the XML document
    //   . Under current page branch[layout::boxes]
    //   . Under the current page type (and parents) branch[layout::boxes]
    //   . Under the theme path branch[layout::boxes]
    //
    content::path_info_t boxes_ipath;
    boxes_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_LAYOUT_LAYOUTS_PATH)).arg(layout_name));

    // get the page type
    //
    // TODO: we probably want to also add a specificy tag for boxes
    //       (i.e. a page_boxes link to a tree that defines boxes)
    //
    links::link_info type_info(content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE), true, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> type_ctxt(links::links::instance()->new_link_context(type_info));
    links::link_info link_type;
    QString type_key;
    if(type_ctxt->next_link(link_type))
    {
        type_key = link_type.key();
    }
    content::path_info_t type_ipath;
    if(!type_key.isEmpty())
    {
        type_ipath.set_path(type_key);
    }
//SNAP_LOG_TRACE("get layout boxes list from this page ")(ipath.get_key())
//                                          (" or type ")(type_ipath.get_key())
//                                    (" or boxes_path ")(boxes_ipath.get_key());

    content::field_search::search_result_t box_names;
    FIELD_SEARCH
        (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)

        // /snap/head/metadata/boxes
        (content::field_search::command_t::COMMAND_ELEMENT, doc)
        (content::field_search::command_t::COMMAND_PATH_ELEMENT, "/snap/head/metadata/boxes")
        // if boxes exist in doc then that is our result
        (content::field_search::command_t::COMMAND_IF_ELEMENT_NULL, 1)
            (content::field_search::command_t::COMMAND_ELEMENT_TEXT)
            (content::field_search::command_t::COMMAND_RESULT, box_names)
            (content::field_search::command_t::COMMAND_GOTO, 100)

        // no boxes in source document
        (content::field_search::command_t::COMMAND_LABEL, 1)

        // check in this specific page for a layout::boxes field
        (content::field_search::command_t::COMMAND_PATH_INFO_BRANCH, ipath)
        (content::field_search::command_t::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_LAYOUT_BOXES))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_IF_FOUND, 100)

        // check in the type or any parents
        (content::field_search::command_t::COMMAND_PATH_INFO_BRANCH, type_ipath)
        (content::field_search::command_t::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_LAYOUT_BOXES))
        (content::field_search::command_t::COMMAND_PARENTS, content::get_name(content::name_t::SNAP_NAME_CONTENT_CONTENT_TYPES_NAME))
        (content::field_search::command_t::COMMAND_IF_FOUND, 100)

        // check in the boxes path for a layout::boxes field
        (content::field_search::command_t::COMMAND_PATH_INFO_BRANCH, boxes_ipath)
        (content::field_search::command_t::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_LAYOUT_BOXES))
        (content::field_search::command_t::COMMAND_SELF)

        (content::field_search::command_t::COMMAND_LABEL, 100)
        (content::field_search::command_t::COMMAND_RESULT, box_names)

        // retrieve names of all the boxes
        ;

    int const max_names(box_names.size());
    if(max_names != 0)
    {
        if(max_names != 1)
        {
            throw snap_logic_exception("layout::generate_boxes(): expected zero or one entry from a COMMAND_SELF / COMMAND_ELEMENT_TEXT");
        }
        // an empty list is represented by a period because "" cannot be
        // properly saved in the database!
        QString box_list(box_names[0].stringValue());

        if(!box_list.isEmpty() && box_list != ".")
        {
            snap_string_list names(box_list.split(","));
            QVector<QDomElement> dom_boxes;
            int const max_boxes(names.size());
            for(int i(0); i < max_boxes; ++i)
            {
                names[i] = names[i].trimmed();
                QDomElement box(doc.createElement(names[i]));
                boxes.appendChild(box);
                dom_boxes.push_back(box); // will be the same offset as names[...]
            }
#ifdef DEBUG
            if(dom_boxes.size() != max_boxes)
            {
                throw snap_logic_exception("somehow the 'DOM boxes' and 'names' vectors do not have the same size.");
            }
#endif
            quiet_error_callback box_error_callback(f_snap, true); // TODO: set log parameter to false once we are happy about the results

            for(int i(0); i < max_boxes; ++i)
            {
                content::path_info_t ichild;
                ichild.set_path(QString("%1/%2/%3").arg(get_name(name_t::SNAP_NAME_LAYOUT_LAYOUTS_PATH)).arg(layout_name).arg(names[i]));
                // links cannot be read if the version is undefined;
                // the version is undefined if the theme has no boxes at all
                snap_version::version_number_t branch(ichild.get_branch());
                if(snap_version::SPECIAL_VERSION_UNDEFINED != branch)
                {
                    links::link_info info(content::get_name(content::name_t::SNAP_NAME_CONTENT_CHILDREN), false, ichild.get_key(), ichild.get_branch());
                    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                    links::link_info child_info;
                    while(link_ctxt->next_link(child_info))
                    {
                        box_error_callback.clear_error();
                        content::path_info_t box_ipath;
                        box_ipath.set_path(child_info.key());
                        box_ipath.set_parameter("action", "view"); // we are always only viewing those boxes from here
SNAP_LOG_TRACE("box_ipath key = ")(box_ipath.get_key())(", branch_key=")(box_ipath.get_branch_key());
                        plugin * box_plugin(path::path::instance()->get_plugin(box_ipath, box_error_callback));
                        if(!box_error_callback.has_error() && box_plugin)
                        {
                            layout_boxes * lb(dynamic_cast<layout_boxes *>(box_plugin));
                            if(lb != nullptr)
                            {
                                // put each box in a filter tag because we have to
                                // specify a different owner and path for each
                                //
                                QDomElement filter_box(doc.createElement("filter"));
                                filter_box.setAttribute("path", box_ipath.get_cpath()); // not the full key
                                filter_box.setAttribute("owner", box_plugin->get_plugin_name());
                                dom_boxes[i].appendChild(filter_box);
SNAP_LOG_TRACE("handle box for ")(box_plugin->get_plugin_name())(" with owner \"")(box_plugin->get_plugin_name())("\"");

                                // Unfortunately running the full header content
                                // signal would overwrite the main data... not good!
                                //QDomElement head(snap_dom::get_element(doc, "head"));
                                //QDomElement metadata(snap_dom::get_element(doc, "metadata"));
                                //generate_header_content(ipath, head, metadata, "");

                                lb->on_generate_boxes_content(ipath, box_ipath, page, filter_box);

                                // Unfortunately running the full page content
                                // signal would overwrite the main data... not good!
                                //QDomElement page(snap_dom::get_element(doc, "page"));
                                //QDomElement body(snap_dom::get_element(doc, "body"));
                                //generate_page_content(ipath, page, body, "");
                            }
                            else
                            {
                                // if this happens a plugin offers a box but not
                                // the handler
                                f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                                        "Plugin Missing",
                                        "Plugin \"" + box_plugin->get_plugin_name() + "\" does not know how to handle a box assigned to it.",
                                        "layout::generate_boxes() the plugin does not derive from layout::layout_boxes.");
                                NOTREACHED();
                            }
                        }
                    }
                }
            }
        }
    }
}


/** \brief Apply the theme on an XML document.
 *
 * This function applies the theme to an XML document representing a
 * page. This should only be used against blocks that are themed
 * and final pages.
 *
 * Whenever you create a body from a template, then you should not call
 * this function since it would otherwise pre-theme your result. Instead
 * you'd want to save the title and body elements of the \p doc XML
 * document.
 *
 * \param[in,out] doc  The XML document to theme.
 * \param[in] xsl  The XSLT data to use to apply the theme.
 * \param[in] theme_name  The name of the theme used to generate the output.
 *
 * \return The XML document themed in the form of a string.
 */
QString layout::apply_theme(QDomDocument doc, QString const & xsl, QString const & theme_name)
{
    QDomElement metadata(snap_dom::get_element(doc, "metadata"));
    metadata.setAttribute("theme-name", theme_name);

    QString doc_str(doc.toString(-1));
    {
        QDomXPath xpath;
        xpath.setXPath("/snap/page/body/content");
        QDomXPath::node_vector_t content_tag(xpath.apply(doc));
        if(!content_tag.empty())
        {
            content_tag[0].parentNode().removeChild(content_tag[0]);
            //doc_str = doc.toString(-1);
        }
    }

    //if(doc_str.length() > 1024 * 500)
    //{
    //    QDomXPath::node_vector_t output_tag;
    //    QDomXPath xpath;
    //    xpath.setXPath("/snap/page/body/output");
    //    output_tag = xpath.apply(doc);
    //    if(!output_tag.empty())
    //    {
    //        output_tag[0].parentNode().removeChild(output_tag[0]);
    //        doc_str = doc.toString(-1);
    //    }
    //}

    xslt x;
    x.set_xsl(xsl);
    x.set_document(doc);
    return x.evaluate_to_string();
}


/** \brief Search the XSLT document and replace include/import tags.
 *
 * This function searches the XSLT document for tags that look like
 * \<xsl:include ...> and \<xsl:import ...>.
 *
 * \todo
 * At this point the xsl:import is not really properly supported because
 * the documentation imposes a definition priority which we're not
 * imposing. (i.e. any definition in the main document remains the one
 * in place even after an xsl:import of the same definition.) It would
 * probably be possible to support that feature, but at this point we
 * simply recommand that you only use xsl:include at the top of your XSLT
 * documents.
 *
 * \todo
 * To avoid transforming the document to a DOM, we do the parsing "manually".
 * This means the XML may be completely wrong. Especially, the include
 * and import tags could be in a sub-tag which would be considered wrong.
 * We expect, at some point, to have a valid XSLT lint parser which will
 * verify the files at compile time. That means the following code can
 * already be considered valid.
 *
 * \todo
 * This is a TBD: at this point the function generates an error log on
 * invalid input data. Since we expect the files to be correct (as mentioned
 * in another todo) we should never get errors here. Because of that I
 * think that just and only an error log is enough here. Otherwise we may
 * want to have them as messages instead.
 *
 * Source: http://www.w3.org/TR/xslt#section-Combining-Stylesheets
 */
void layout::replace_includes(QString & xsl)
{
    // use a sub-function so we can apply the xsl:include and xsl:import
    // with the exact same code instead of copy & paste.
    class replace_t
    {
    public:
        static void replace(snap_child * snap, QString const & tag, QString & xsl)
        {
            // the xsl:include is recursive, what gets included may itself
            // include some more sub-data
            int const len(tag.length());
            for(int start(xsl.indexOf(tag)); start >= 0; start = xsl.indexOf(tag, start))
            {
                // get the end position of the tag
                int const end(xsl.indexOf(">", start + len));
                if(end < 0)
                {
                    SNAP_LOG_ERROR("an ")(tag)(" .../> tag is missing the '>' (byte position: ")(start)(")");
                    break;
                }
                QString attributes(xsl.mid(start + len, end - start - len));
                int const href_start(attributes.indexOf("href="));
                if(href_start < 0 || href_start + 7 >= attributes.length())
                {
                    SNAP_LOG_ERROR(tag)(" tag missing a valid href=... attribute (")(attributes)(")");
                    break;
                }
                ushort const quote(attributes[href_start + 5].unicode());
                if(quote != '\'' && quote != '"') // href value is note quoted?! (not valid XML)
                {
                    SNAP_LOG_ERROR("the href=... attribute of an ")(tag)(" .../> does not seem to be quoted as expected in XML (")(attributes)(")");
                    break;
                }
                int const href_end(attributes.indexOf(quote, href_start + 6));
                if(href_end < 0)
                {
                    SNAP_LOG_ERROR("the href=... attribute of an ")(tag)(" .../> does not seem to end with a similar quote as expected in XML (")(attributes)(")");
                    break;
                }
                QString uri(attributes.mid(href_start + 6, href_end - href_start - 6));
                if(!uri.endsWith(".xsl"))
                {
                    uri += ".xsl";
                }
                if(!uri.contains(':')
                && !uri.contains('/'))
                {
                    uri = QString(":/xsl/layout/%1").arg(uri);
                }

                // load the file in memory
                //
                snap_child::post_file_t file;
                file.set_filename(uri);
                if(!snap->load_file(file))
                {
                    SNAP_LOG_ERROR("xsl tag ")(tag)(" href=\"")(uri)("\" .../> did not reference a known file (file could not be loaded).");
                    // the include string below will be empty
                }
                QString include(QString::fromUtf8(file.get_data(), file.get_size()));

                // grab the content within the <xsl:stylesheet> root tag
                //
                int const open_stylesheet_start(include.indexOf("<xsl:stylesheet"));
                int const open_stylesheet_end(include.indexOf(">", open_stylesheet_start + 15));
                int const close_stylesheet_start(include.lastIndexOf("</xsl:stylesheet"));
                include = include.mid(open_stylesheet_end + 1, close_stylesheet_start - open_stylesheet_end - 1);

                // replace the <xsl:include ...> tag
                //
                xsl.replace(start, end - start + 1, include);
            }
        }
    };
    replace_t::replace(f_snap, "<xsl:include", xsl);
    replace_t::replace(f_snap, "<xsl:import", xsl);
//SNAP_LOG_TRACE() << "include [" << xsl << "]";
}


/** \brief Install a layout.
 *
 * This function installs a layout. The function first checks whether the
 * layout was already installed. If so, it runs the content.xml only if
 * the layout was updated.
 *
 * \param[in] layout_name  The name of the layout to install.
 */
void layout::install_layout(QString const & layout_name)
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t layout_table(get_layout_table());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());

    libdbproxy::value last_updated_value;
    if(layout_name == "default")
    {
        // the default theme does not get a new date and time without us
        // having to read, parse, analyze the XML date, so instead we use
        // the date and time when this file gets compiled
        //
        time_t const last_update_of_default_theme(SNAP_UNIX_TIMESTAMP(UTC_YEAR, UTC_MONTH, UTC_DAY, UTC_HOUR, UTC_MINUTE, UTC_SECOND));
        last_updated_value.setInt64Value(last_update_of_default_theme * 1000000LL);
    }
    else
    {
        last_updated_value = layout_table->getRow(layout_name)->getCell(snap::get_name(snap::name_t::SNAP_NAME_CORE_LAST_UPDATED))->getValue();

        if(last_updated_value.size() != sizeof(int64_t))
        {
            // this is a rather bad error, i.e. we do not know when that
            // layout was last updated?! the snaplayout tool does write
            // that information, but if you program your own thing, then
            // it could go missing
            //
            SNAP_LOG_ERROR("layout::install_layout(): the ")
                          (snap::get_name(snap::name_t::SNAP_NAME_CORE_LAST_UPDATED))
                          (" field is not defined for layout ")
                          (layout_name)
                          (".");

            // force a default using "now"
            //
            // TBD: this may need to be a different value (i.e. maybe
            //      2012/1/1 00:00:00)
            //
            int64_t const start_date(f_snap->get_start_date());
            last_updated_value.setInt64Value(start_date);
            layout_table->getRow(layout_name)->getCell(snap::get_name(snap::name_t::SNAP_NAME_CORE_LAST_UPDATED))->setValue(last_updated_value);
        }
    }

    // here the last_updated_value must be correct
    //
    if(last_updated_value.size() != sizeof(int64_t))
    {
        throw snap_logic_exception("layout::install_layout(): somehow last_updated_value is not exactly sizeof(int64_t).");
    }

    // installing a layout can be complex so knowing which one breaks an
    // update can be really useful
    //
    /*SNAP_LOG_DEBUG("layout::install_layout(): layout name \"")
                  (layout_name)
                  ("\" last updated on ")
                  (last_updated_value.safeInt64Value());*/

    // determine the path to this layout
    //
    content::path_info_t layout_ipath;
    layout_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_LAYOUT_LAYOUTS_PATH)).arg(layout_name));

    // Define the name of the field to be used to record the last time
    // the layout was updated
    //
    QString const layout_last_update_field_name(QString("%1::layout::%2")
                                .arg(snap::get_name(snap::name_t::SNAP_NAME_CORE_LAST_UPDATED))
                                .arg(layout_name));

    // if the layout is already installed (has_branch() returns true) then
    // then check when the last update was applied
    //
    if(layout_ipath.has_branch()
    && branch_table->exists(layout_ipath.get_branch_key())
    && branch_table->getRow(layout_ipath.get_branch_key())->exists(get_name(name_t::SNAP_NAME_LAYOUT_BOXES)))
    {
        // the layout is already installed
        //

        // retrieve the timestamp of the last update for this layout
        //
        int64_t const last_install(f_snap->get_site_parameter(layout_last_update_field_name).safeInt64Value());

        // get the timestamp from the layout
        //
        int64_t const last_update(last_updated_value.safeInt64Value());

        // compare whether the layout was updated more recently
        //
        if(last_update <= last_install)
        {
            // we are good already
            //
            // last update of the website is older or the same as
            // the last installation we have done of it on this
            // website
            //
            return;
        }
    }

    // make sure the layout is considered valid if it exists
    //
    if(content_table->exists(layout_ipath.get_key()))
    {
        content::path_info_t::status_t const status(layout_ipath.get_status());
        if(status.get_state() != content::path_info_t::status_t::state_t::NORMAL)
        {
            // layout page is not marked as being "normal", we cannot work
            // on it while the content plugin is also working on it
            //
            return;
        }
    }

    // save the new updated date to the database
    //
    // Saving it now somewhat replicates what we do in the child process
    // (really it would happen even if the previous return gets executed!)
    // because if anything else fails, we do not want to try it again until
    // a new update is available (otherwise the website gets hosed until
    // the fixing update gets installed...)
    //
    f_snap->set_site_parameter(layout_last_update_field_name, last_updated_value);

    // this layout is missing or needs updates
    //
    QString xml_content;
    if(layout_name == "default")
    {
        // the default theme is in our resources instead of the database
        // so it can work even if we were to not be able to read that info
        // from the database (although at this time we are bound to having
        // a database connection in any event)
        //
        QFile file(":/xml/layout/content.xml");
        if(!file.open(QIODevice::ReadOnly))
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Layout Unavailable",
                    "Could not read content.xml from the resources.",
                    "layout::install_layout() could not open content.xml resource file.");
            NOTREACHED();
        }
        QByteArray data(file.readAll());
        xml_content = QString::fromUtf8(data.data(), data.size());
    }
    else
    {
        if(!layout_table->getRow(layout_name)->exists(get_name(name_t::SNAP_NAME_LAYOUT_CONTENT_XML)))
        {
            // that should probably apply to the body and theme names
            //
            SNAP_LOG_ERROR("Could not read \"")
                          (layout_name)
                          ("/")
                          (get_name(name_t::SNAP_NAME_LAYOUT_CONTENT_XML))
                          ("\" from the layout table while updating layouts, error is ignored now so your plugin can fix it.");
            return;
        }
        xml_content = layout_table->getRow(layout_name)->getCell(get_name(name_t::SNAP_NAME_LAYOUT_CONTENT_XML))->getValue().stringValue();
    }

    // transform the XML data to a DOM
    //
    QDomDocument dom;
    if(!dom.setContent(xml_content, false))
    {
        f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                "Layout Unavailable",
                QString("Layout \"%1\" content.xml file could not be loaded.").arg(layout_name),
                "layout::install_layout() could not load the content.xml file from the layout table.");
        NOTREACHED();
    }

    // add the XML document to the installation data
    //
    // IMPORTANT NOTE: We use the "output" plugin as the default owner of
    //                 all layout data because we expect the "output"
    //                 plugin to display any page added by a layout (there
    //                 should be nothing of much interest to a hacker, etc.
    //                 in a layout so this should always be fine.)
    //
    content_plugin->add_xml_document(dom, content::get_name(content::name_t::SNAP_NAME_CONTENT_OUTPUT_PLUGIN));

    // memorize which layout we updated so we can finalize the installation
    // in our finish_install_layout() function
    //
    f_initialized_layout.push_back(layout_name);
}


void layout::finish_install_layout()
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t layout_table(get_layout_table());
    libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());

    for(auto const & layout_name : f_initialized_layout)
    {
        content::path_info_t layout_ipath;
        layout_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_LAYOUT_LAYOUTS_PATH)).arg(layout_name));

        // after an update of the content.xml file we expect the layout::boxes
        // field to be defined
        //
        if(!branch_table->getRow(layout_ipath.get_branch_key())->exists(get_name(name_t::SNAP_NAME_LAYOUT_BOXES)))
        {
            SNAP_LOG_ERROR("Could not read \"")(layout_ipath.get_branch_key())(".")
                    (get_name(name_t::SNAP_NAME_LAYOUT_BOXES))
                    ("\" from the layout, error is ignored now so your plugin can fix it.");
        }

        // create a reference back to us from the layout that way we know
        // who uses what
        //
        // note that at this point we do not yet have a way to remove
        // those references
        //
        QString const reference(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_LAYOUT_REFERENCE)).arg(layout_ipath.get_key()));
        int64_t const start_date(f_snap->get_start_date());
        libdbproxy::value value;
        value.setInt64Value(start_date);
        layout_table->getRow(layout_name)->getCell(reference)->setValue(value);
    }

    // just in case, clear the list because we do not want to re-run that
    // code over and over again (this function should never be called more
    // than once, though.)
    //
    f_initialized_layout.clear();
}


/** \brief Generate the header of the content.
 *
 * This function generates the main content header information. Other
 * plugins will also receive the event and are invited to add their
 * own information to any header as required by their implementation.
 *
 * Remember that this is not exactly the HTML header, it's the XML
 * header that will be parsed through the theme XSLT file.
 *
 * This function is also often used to setup HTTP headers early on.
 * For example the robots.txt plugin sets up the X-Robots header with
 * a call to the snap_child object:
 *
 * \code
 * f_snap->set_header("X-Robots", f_robots_cache);
 * \endcode
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 *
 * \return true if the signal should go on to all the other plugins.
 */
bool layout::generate_header_content_impl(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(header);

    content::path_info_t main_ipath;
    main_ipath.set_path(f_snap->get_uri().path());

    int const p(ipath.get_cpath().lastIndexOf('/'));
    QString const base(f_snap->get_site_key_with_slash() + (p == -1 ? "" : ipath.get_cpath().left(p)));

    QString const qs_action(f_snap->get_server_parameter("qs_action"));
    snap_uri const & uri(f_snap->get_uri());
    QString const action(uri.query_option(qs_action));

    // the canonical URI may point to another website (i.e. if we are on a
    // test system, then all canonical URIs should point to the original)
    //
    // WARNING: we cannot use path_info_t to canonicalize this string
    //          since the domain is not going to be the same
    //
    QString canonical_domain(f_snap->get_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_CANONICAL_DOMAIN)).stringValue().trimmed());
    if(canonical_domain.isEmpty())
    {
        // the port is tricky in this case, i.e. the destination may require
        // a specific port but the port of the test website may be different
        // in all likelyhood, though, we do not want a port
        //
        canonical_domain = uri.get_website_uri();
    }
    // snap_uri will canonicalize the URI for us
    //
    snap_uri const canonical_uri(canonical_domain
                               + "/"
                               + main_ipath.get_cpath());

    http_link canonical_link(f_snap, canonical_uri.get_uri().toUtf8().data(), "canonical");
    f_snap->add_http_link(canonical_link);

    QString site_name(f_snap->get_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_NAME)).stringValue().trimmed());
    QString site_short_name(f_snap->get_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_SHORT_NAME)).stringValue().trimmed());
    QString site_long_name(f_snap->get_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_LONG_NAME)).stringValue().trimmed());

    if(site_name.isEmpty())
    {
        if(!site_long_name.isEmpty())
        {
            site_name = site_long_name;
        }
        else if(!site_short_name.isEmpty())
        {
            site_name = site_short_name;
        }
        else
        {
            site_name = "Your Website Name";
        }
    }

    FIELD_SEARCH
        (content::field_search::command_t::COMMAND_ELEMENT, metadata)
        (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)

        // snap/head/metadata/desc[@type="version"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, SNAPWEBSITES_VERSION_STRING)
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=version]/data")

        // snap/head/metadata/desc[@type="website_uri"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, f_snap->get_site_key())
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=website_uri]/data")

        // snap/head/metadata/desc[@type="base_uri"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, base)
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=base_uri]/data")

        // snap/head/metadata/desc[@type="page_uri"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, main_ipath.get_key())
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=page_uri]/data")

        // snap/head/metadata/desc[@type="canonical_uri"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, canonical_uri.get_uri())
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=canonical_uri]/data")

        // snap/head/metadata/desc[@type="real_uri"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, ipath.get_key())
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=real_uri]/data")

        // snap/head/metadata/desc[@type="name"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, site_name)
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=name]/data")
        // snap/head/metadata/desc[@type="name"]/short-data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE_OR_NULL, site_short_name)
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=name]/short-data")
        // snap/head/metadata/desc[@type="name"]/long-data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE_OR_NULL, site_long_name)
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=name]/long-data")

        // snap/head/metadata/desc[@type="email"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE_OR_NULL, f_snap->get_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_ADMINISTRATOR_EMAIL)))
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=email]/data")

        // snap/head/metadata/desc[@type="remote_ip"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)))
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=remote_ip]/data")

        // snap/head/metadata/desc[@type="action"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, action)
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=action]/data")

        // generate!
        ;

//SNAP_LOG_TRACE() << "layout stuff [" << header.ownerDocument().toString(-1) << "]";
    return true;
}


/** \fn void layout::generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
 * \brief Generate the page main content.
 *
 * This signal generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * areas may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the <page> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the intermediate and final output.
 *
 * \param[in] ipath  The path being managed.
 * \param[in] page_content  The main content of the page.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */


/** \fn void layout::filtered_content(content::path_info_t & ipath, QDomDocument & doc, QString const & xsl)
 * \brief Signals that the content in the DOM document was filtered.
 *
 * This signal is called once all the filters for that page were applied
 * on the DOM document.
 *
 * This allows your plugin to do further processing of the content after
 * all the filters were applied.
 *
 * \param[in] ipath  The path being managed.
 * \param[in,out] doc  The document that was just generated.
 * \param[in] xsl  The XSLT document that is about to be used to transform
 *                 the body (still as a string).
 */


/** \brief Load a file.
 *
 * This function is used to load a file. As additional plugins are added
 * additional protocols can be supported.
 *
 * The file information defaults are kept as is as much as possible. If
 * a plugin returns a file, though, it is advised that any information
 * available to the plugin be set in the file object.
 *
 * This function loads files that have a name starting with the layout
 * protocol (layout:).
 *
 * \param[in,out] file  The file name and content.
 * \param[in,out] found  Whether the file was found.
 */
void layout::on_load_file(snap_child::post_file_t & file, bool & found)
{
#ifdef DEBUG
    SNAP_LOG_TRACE("layout::on_load_file(), filename=")(file.get_filename());
#endif
    if(!found)
    {
        QString filename(file.get_filename());
        if(filename.startsWith("layout:"))     // Read a layout file
        {
            // remove the protocol
            int i(7);
            for(; i < filename.length() && filename[i] == '/'; ++i);
            filename = filename.mid(i);
            snap_string_list const parts(filename.split('/'));
            if(parts.size() != 2)
            {
                // wrong number of parts...
                SNAP_LOG_ERROR("layout load_file() called with an invalid path: \"")(filename)("\"");
                return;
            }
            libdbproxy::table::pointer_t layout_table(get_layout_table());
            QString column_name(parts[1]);
            if(layout_table->exists(parts[0])
            && layout_table->getRow(parts[0])->exists(QString(column_name)))
            {
                libdbproxy::value layout_value(layout_table->getRow(parts[0])->getCell(QString(column_name))->getValue());

                file.set_filename(filename);
                file.set_data(layout_value.binaryValue());
                found = true;
            }
            else
            {
                // if "column_name" does not exist, we try again with
                // the ".xsl" extension
                //
                // TODO: this is to be backward compatible, all filenames
                //       should have an extension specified so we do not
                //       take a chance like this...
                //
                column_name += ".xsl";
                if(layout_table->exists(parts[0])
                && layout_table->getRow(parts[0])->exists(QString(column_name)))
                {
                    libdbproxy::value layout_value(layout_table->getRow(parts[0])->getCell(QString(column_name))->getValue());

                    file.set_filename(filename);
                    file.set_data(layout_value.binaryValue());
                    found = true;
                }
            }
        }
    }
}


/** \brief Add a layout from a set of resource files.
 *
 * This function is used to create a layout in the layout table using a
 * set of resource files:
 *
 * \code
 * :/xsl/layout/%1-body-parser.xsl        body
 * :/xsl/layout/%1-theme-parser.xsl       theme
 * :/xsl/layout/%1-content.xml            content
 * \endcode
 *
 * The update date is set to start_date().
 *
 * \warning
 * This function can only be called from your do_update() function or
 * things will break. The finalization will automatically be handled
 * as required.
 *
 * \param[in] name  The name of the layout. In most cases it will be the
 *                  same as the name of your plugin.
 */
bool layout::add_layout_from_resources_impl(QString const & name)
{
    libdbproxy::table::pointer_t layout_table(layout::layout::instance()->get_layout_table());

    {
        QString const body(QString(":/xsl/layout/%1-body-parser.xsl").arg(name));
        QFile file(body);
        if(!file.open(QIODevice::ReadOnly))
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Body Layout Unavailable",
                    QString("Could not read \"%1\" from the Qt resources.").arg(body),
                    "layout::add_layout_from_resources_impl() could not open resource file for a body file.");
            NOTREACHED();
        }
        QByteArray data(file.readAll());
        layout_table->getRow(name)->getCell(get_name(name_t::SNAP_NAME_LAYOUT_BODY_XSL))->setValue(data);
    }

    {
        QString const theme(QString(":/xsl/layout/%1-theme-parser.xsl").arg(name));
        QFile file(theme);
        if(!file.open(QIODevice::ReadOnly))
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Theme Layout Unavailable",
                    QString("Could not read \"%1\" from the Qt resources.").arg(theme),
                    "layout::add_layout_from_resources_impl() could not open resource file for a theme file.");
            NOTREACHED();
        }
        QByteArray data(file.readAll());
        layout_table->getRow(name)->getCell(get_name(name_t::SNAP_NAME_LAYOUT_THEME_XSL))->setValue(data);
    }

    {
        QString const content(QString(":/xml/layout/%1-content.xml").arg(name));
        QFile file(content);
        if(!file.open(QIODevice::ReadOnly))
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Sendmail Theme Content Unavailable",
                    QString("Could not read \"%1\" from the Qt resources.").arg(content),
                    "layout::add_layout_from_resources_impl() could not open resource file for a content.xml file.");
            NOTREACHED();
        }
        QByteArray data(file.readAll());
        layout_table->getRow(name)->getCell(get_name(name_t::SNAP_NAME_LAYOUT_CONTENT_XML))->setValue(data);
    }

    return true;
}


/** \brief Helper function to install a theme from resources.
 *
 * This function is called after other plugins had a chance to tweak a
 * few things in this theme. For example, the editor plugin may add
 * the editor XSL file if present in the source.
 *
 * It also finalize the installation by calling the install_layout()
 * function.
 *
 * \param[in] layout_name  The layout name.
 */
void layout::add_layout_from_resources_done(QString const & layout_name)
{
    libdbproxy::table::pointer_t layout_table(layout::layout::instance()->get_layout_table());

    // simulate a "last updated" date?
    //
    // TBD: is this required? At this time we generate an error if it does
    //      not get defined and use 'get_start_date()' as the default anyway...
    //      (see install_layout() for details)
    //
    //      I am wondering whether this should not be set at all?
    //
    int64_t const updated(f_snap->get_start_date());
    layout_table->getRow(layout_name)->getCell(snap::get_name(snap::name_t::SNAP_NAME_CORE_LAST_UPDATED))->setValue(updated);

    install_layout(layout_name);
}


void layout::on_copy_branch_cells(libdbproxy::cells & source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    NOTUSED(destination_branch);

    content::content::copy_branch_cells_as_is(source_cells, destination_row, get_name(name_t::SNAP_NAME_LAYOUT_NAMESPACE));
}


bool layout::on_improve_signature(QString const & path, QDomDocument doc, QDomElement & signature_tag)
{
    NOTUSED(path);
    NOTUSED(signature_tag);

    QDomElement head;
    QDomElement root(doc.documentElement());
    if(snap_dom::get_tag("head", root, head, false))
    {
        QDomElement generator(doc.createElement("link"));
        generator.setAttribute("rel", "bookmark");
        generator.setAttribute("type", "text/html");
        // TODO: translate
        generator.setAttribute("title", "Generator");
        generator.setAttribute("href", "http://snapwebsites.org/");
        head.appendChild(generator);

        QDomElement top(doc.createElement("link"));
        top.setAttribute("rel", "top");
        top.setAttribute("type", "text/html");
        // TODO: translate
        top.setAttribute("title", "Index");
        top.setAttribute("href", f_snap->get_site_key());
        head.appendChild(top);

        QDomElement meta_tag(doc.createElement("meta"));
        meta_tag.setAttribute("name", "generator");
        meta_tag.setAttribute("content", "Snap! Websites");
        head.appendChild(meta_tag);
    }

    return true;
}



// This was to test, at this point we don't offer anything in the layout itself
//int layout::js_property_count() const
//{
//    return 1;
//}
//
//QVariant layout::js_property_get(const QString& name) const
//{
//    if(name == "name")
//    {
//        return "default_layout";
//    }
//    return QVariant();
//}
//
//QString layout::js_property_name(int index) const
//{
//    return "name";
//}
//
//QVariant layout::js_property_get(int index) const
//{
//    if(index == 0)
//    {
//        return "default_layout";
//    }
//    return QVariant();
//}


/* sample XML file for a default Snap! website home page --
<!DOCTYPE snap>
<snap>
 <head path="" owner="content">
  <metadata>
   <desc type="website_uri">
    <data>http://csnap.m2osw.com/</data>
   </desc>
   <desc type="base_uri">
    <data>http://csnap.m2osw.com/</data>
   </desc>
   <desc type="page_uri">
    <data>http://csnap.m2osw.com/</data>
   </desc>
   <desc type="name">
    <data>Website Name</data>
   </desc>
   <desc type="remote_ip">
    <data>162.226.130.121</data>
   </desc>
   <desc type="shorturl">
    <data>http://csnap.m2osw.com/s/4</data>
   </desc>
  </metadata>
 </head>
 <page>
  <body>
   <titles>
    <title>Home Page</title>
   </titles>
   <content>
    <p>Welcome to your new Snap! C++ website.</p>
    <p>
     <a href="/login">Log In Now!</a>
    </p>
   </content>
   <created>2014-01-09</created>
   <modified>2014-01-09</modified>
   <updated>2014-01-09</updated>
   <image>
    <shortcut width="16" height="16" type="image/x-icon" href="http://csnap.m2osw.com/favicon.ico"/>
   </image>
   <bookmarks>
    <link title="Search" rel="search" type="text/html" href="http://csnap.m2osw.com/search"/>
   </bookmarks>
  </body>
  <boxes>
   <left>
    <filter path="layouts/bare/left/login" owner="users_ui">
     <titles>
      <title>User Login</title>
     </titles>
     <content>
      <p>The login box is showing!</p>
      <div class="form-wrapper">
       <div class="snap-form">
        <form onkeypress="javascript:if((event.which&amp;&amp;event.which==13)||(event.keyCode&amp;&amp;event.keyCode==13))fire_event(login_34,'click');" method="post" accept-charset="utf-8" id="form_34" autocomplete="off">
         <input type="hidden" value=" " id="form__iehack" name="form__iehack"/>
         <input type="hidden" value="3673b0558e8ad92c" id="_form_session" name="_form_session"/>
         <div class="form-item fieldset">
          <fieldset class="" id="log_info_34">
           <legend title="Enter your log in information below then click the Log In button." accesskey="l">Log In Form</legend>
           <div class="field-set-content">
            <div class="form-help fieldset-help" style="display: none;">This form allows you to log in your Snap! website. Enter your log in name and password and then click on Log In to get a log in session.</div>
            <div class="form-item line-edit ">
             <label title="Enter your email address to log in your Snap! Website account." class="line-edit-label" for="email_34">
              <span class="line-edit-label-span">Email:</span>
              <span class="form-item-required">*</span>
              <input title="Enter your email address to log in your Snap! Website account." class=" line-edit-input " alt="Enter the email address you used to register with Snap! All the Snap! Websites run by Made to Order Software Corp. allow you to use the same log in credentials." size="20" maxlength="60" accesskey="e" type="text" id="email_34" name="email" tabindex="1"/>
             </label>
             <div class="form-help line-edit-help" style="display: none;">Enter the email address you used to register with Snap! All the Snap! Websites run by Made to Order Software Corp. allow you to use the same log in credentials.</div>
            </div>
            <div class="form-item password ">
             <label title="Enter your password, if you forgot your password, just the link below to request a change." class="password-label" for="password_34">
              <span class="password-label-span">Password:</span>
              <span class="form-item-required">*</span>
              <input title="Enter your password, if you forgot your password, just the link below to request a change." class="password-input " alt="Enter the password you used while registering with Snap! Your password is the same for all the Snap! Websites run by Made to Order Software Corp." size="25" maxlength="256" accesskey="p" type="password" id="password_34" name="password" tabindex="2"/>
             </label>
             <div class="form-help password-help" style="display: none;">Enter the password you used while registering with Snap! Your password is the same for all the Snap! Websites run by Made to Order Software Corp.</div>
            </div>
            <div class="form-item link">
             <a title="Forgot your password? Click on this link to request Snap! to send you a link to change it with a new one." class="link " accesskey="f" href="/forgot-password" id="forgot_password_34" tabindex="6">Forgot Password</a>
             <div class="form-help link-help" style="display: none;">You use so many websites with an account... and each one has to have a different password! So it can be easy to forget the password for a given website. We store passwords in a one way encryption mechanism (i.e. we cannot decrypt it) so if you forget it, we can only offer you to replace it. This is done using the form this link sends you to.</div>
            </div>
            <div class="form-item link">
             <a title="No account yet? Register your own Snap! account now." class="link " accesskey="u" href="/register" id="register_34" tabindex="5">Register</a>
             <div class="form-help link-help" style="display: none;">To log in a Snap! account, you first have to register an account. Click on this link if you don't already have an account. If you are not sure, you can always try the <strong>Forgot Password</strong> link. It will tell you whether we know your email address.</div>
            </div>
            <div class="form-item checkbox">
             <label title="Select this checkbox to let your browser record a long time cookie. This way you can come back to your Snap! account(s) without having to log back in everytime." class="checkbox-label" for="remember_34">
              <input title="Select this checkbox to let your browser record a long time cookie. This way you can come back to your Snap! account(s) without having to log back in everytime." class="checkbox-input remember-me-checkbox" alt="By checking this box you agree to have Snap! save a full session cookie which let you come back to your website over and over again. By not selecting the checkbox, you still get a cookie, but it will only last 2 hours unless your use your website constantly." accesskey="m" type="checkbox" checked="checked" id="remember_34" name="remember" tabindex="3"/>
              <script type="text/javascript">remember_34.checked="checked";</script>
              <span class="checkbox-label-span">Remember Me</span>
             </label>
             <div class="form-help checkbox-help" style="display: none;">By checking this box you agree to have Snap! save a full session cookie which let you come back to your website over and over again. By not selecting the checkbox, you still get a cookie, but it will only last 2 hours unless your use your website constantly.</div>
            </div>
            <div class="form-item submit">
             <input title="Well... we may want to rename this one if we use it as the alternate text of widgets..." class="submit-input my-button-class" alt="Long description that goes in the help box, for example." size="25" accesskey="s" type="submit" value="Log In" disabled="disabled" id="login_34" name="login" tabindex="4"/>
             <div class="form-help submit-help" style="display: none;">Long description that goes in the help box, for example.</div>
            </div>
            <div class="form-item link">
             <a title="Click here to return to the home page" class="link my-cancel-class" accesskey="c" href="/" id="cancel_34" tabindex="7">Cancel</a>
             <div class="form-help link-help" style="display: none;">Long description that goes in the help box explaining why you'd want to click Cancel.</div>
            </div>
           </div>
          </fieldset>
         </div>
         <script type="text/javascript">email_34.focus();email_34.select();</script>
         <script type="text/javascript">function auto_reset_34(){form_34.reset();}window.setInterval(auto_reset_34,1.8E6);</script>
         <script type="text/javascript">
              function fire_event(element, event_type)
              {
                if(element.fireEvent)
                {
                  element.fireEvent('on' + event_type);
                }
                else
                {
                  var event = document.createEvent('Events');
                  event.initEvent(event_type, true, false);
                  element.dispatchEvent(event);
                }
              }
            </script>
        </form>
        <script type="text/javascript">login_34.disabled="";</script>
       </div>
      </div>
     </content>
    </filter>
   </left>
  </boxes>
 </page>
</snap>
*/

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
