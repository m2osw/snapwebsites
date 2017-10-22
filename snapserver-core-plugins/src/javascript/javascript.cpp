// Snap Websites Server -- JavaScript plugin to run scripts on the server side
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

// This language is based on the ECMA definitions although we added some of
// our own operators and functionality to make it easier/faster to use in
// our environment.
//
// This plugin let you compile a JavaScript text file (string) in some form
// of byte code that works with a Forth like stack. This plugin also includes
// an interpreter of that byte code so you can run the scripts.
//
// Like in a Browser, at this point this JavaScript does not allow you to
// read and/or write to a file. It has access to the database though, with
// limits.

#include "javascript.h"

#include <snapwebsites/plugins.h>
#include <snapwebsites/snap_version.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/log.h>

#include <QScriptEngine>
#include <QScriptProgram>
#include <QScriptClass>
#include <QScriptClassPropertyIterator>
#include <QSharedPointer>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(javascript, 1, 0)


/*
 * At this time we're using the Qt implementation which we assume will work
 * well enough as a JavaScript interpreter. However, this introduce a slowness
 * in that we cannot save the compiled byte code of a program. This is an
 * annoyance because we'd want to just load the byte code from the database
 * to immediately execute that. This would make things a lot faster especially
 * when each time you run you have to recompile many scripts!
 */




/** \brief Get a fixed javascript name.
 *
 * The javascript plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name) {
    case name_t::SNAP_NAME_JAVASCRIPT_MINIMIZED:
        return "javascript::minimized";

    case name_t::SNAP_NAME_JAVASCRIPT_MINIMIZED_COMPRESSED:
        return "javascript::minimized::compressed";

    //case name_t::SNAP_NAME_JAVASCRIPT_ROW: -- use SNAP_NAME_CONTENT_FILES_JAVASCRIPTS instead
    //    return "javascripts";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_JAVASCRIPT_...");

    }
    NOTREACHED();
}








/** \brief Initialize the javascript plugin.
 *
 * This function is used to initialize the javascript plugin object.
 */
javascript::javascript()
    //: f_snap(nullptr) -- auto-init
    //, f_dynamic_plugins() -- auto-init
{
}


/** \brief Clean up the javascript plugin.
 *
 * Ensure the javascript object is clean before it is gone.
 */
javascript::~javascript()
{
}


/** \brief Get a pointer to the javascript plugin.
 *
 * This function returns an instance pointer to the javascript plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the javascript plugin.
 */
javascript * javascript::instance()
{
    return g_plugin_javascript_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString javascript::icon() const
{
    return "/images/snap/javascript-logo-64x64.png";
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
QString javascript::description() const
{
    return "Offer server side JavaScript support for different plugins."
            " This implementation makes use of the QScript extension.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString javascript::dependencies() const
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
int64_t javascript::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    //SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, content_update); -- content depends on JavaScript so we cannot do a content update here

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize the javascript.
 *
 * This function terminates the initialization of the javascript plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void javascript::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(javascript, "content", content::content, check_attachment_security, _1, _2, _3);
    SNAP_LISTEN(javascript, "content", content::content, process_attachment, _1, _2);
}


/** \brief Add plugin p as a dynamic plugin.
 *
 * This function registers the specified plugin (p) as one of the
 * dynamic plugin that want to know if the user attempts to
 * access data from that plugin.
 *
 * \param[in] p  The dynamic plugin to register.
 */
void javascript::register_dynamic_plugin(javascript_dynamic_plugin * p)
{
    f_dynamic_plugins.push_back(p);
}


/** \brief Dynamic plugin object iterator.
 *
 * This class is used to iterate through the members of a dynamic plugin.
 */
class javascript_dynamic_plugin_iterator : public QScriptClassPropertyIterator
{
public:
    javascript_dynamic_plugin_iterator(javascript * js, QScriptEngine * engine, QScriptValue const & object_value, javascript_dynamic_plugin * plugin)
        : QScriptClassPropertyIterator::QScriptClassPropertyIterator(object_value)
        , f_javascript(js)
        , f_engine(engine)
        //, f_pos(-1) -- auto-init
        , f_object(object_value)
        , f_plugin(plugin)
    {
    }

    //virtual QScriptValue::PropertyFlags flags() const;

    virtual bool hasNext() const
    {
        return f_pos + 1 < f_plugin->js_property_count();
    }

    virtual bool hasPrevious() const
    {
        return f_pos > 0;
    }

    virtual uint id() const
    {
        return f_pos;
    }

    virtual QScriptString name() const
    {
        if(f_pos < 0 || f_pos >= f_plugin->js_property_count())
        {
            throw std::runtime_error("querying the name of the iterator object when the iterator pointer is out of scope");
        }
        return f_engine->toStringHandle(f_plugin->js_property_name(f_pos));
    }

    virtual void next()
    {
        if(f_pos < f_plugin->js_property_count()) {
            ++f_pos;
        }
    }

    virtual void previous()
    {
        if(f_pos > -1) {
            --f_pos;
        }
    }

    virtual void toBack()
    {
        // right after the last position
        f_pos = f_plugin->js_property_count();
    }

    virtual void toFront()
    {
        // right before the first position
        f_pos = -1;
    }

    QScriptValue object() const
    {
        return f_object;
    }

private:
    javascript *                f_javascript = nullptr;
    QScriptEngine *             f_engine = nullptr;
    int32_t                     f_pos = -1;
    QScriptValue                f_object;
    javascript_dynamic_plugin * f_plugin = nullptr;
};


/** \brief Implement our own script class so we can dynamically get plugins values.
 *
 * This function is used to read data from the database based on the name
 * of the plugin and the name of the parameter that the user is interested
 * in. The JavaScript syntax looks like this:
 *
 * \code
 *        var n = plugins.layout.name;
 * \endcode
 *
 * In this case the layout plugin is queried for its parameter "name".
 */
class dynamic_plugin_class : public QScriptClass
{
public:
    dynamic_plugin_class(javascript * js, QScriptEngine * script_engine, javascript_dynamic_plugin * plugin)
        : QScriptClass(script_engine),
          f_javascript(js),
          f_plugin(plugin)
    {
//printf("Yo! got a dpc %p\n", reinterpret_cast<void*>(this));
    }

//~dynamic_plugin_class()
//{
//printf("dynamic_plugin_class destroyed... %p\n", reinterpret_cast<void*>(this));
//}

    // we don't currently support extensions
    //virtual bool supportsExtension(Extension extension) const { return false; }
    //virtual QVariant extension(Extension extension, const QVariant& argument = QVariant());

    virtual QString name() const
    {
        plugins::plugin * p(dynamic_cast<plugins::plugin *>(f_plugin));
        if(!p)
        {
            throw std::runtime_error("plugin pointer is null (dynamic_plugin_class::name)");
        }
        return p->get_plugin_name();
    }

    virtual QScriptClassPropertyIterator * newIterator(QScriptValue const & object)
    {
        return new javascript_dynamic_plugin_iterator(f_javascript, engine(), object, f_plugin);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    virtual QScriptValue property(QScriptValue const& object, QScriptString const& object_name, uint id)
    {
        QScriptValue result(f_plugin->js_property_get(object_name).toString());
        return result;
    }

    virtual QScriptValue::PropertyFlags propertyFlags(QScriptValue const& object, QScriptString const& property_name, uint id)
    {
        // at some point we may want to allow read/write/delete...
        return QScriptValue::ReadOnly | QScriptValue::Undeletable | QScriptValue::KeepExistingFlags;
    }
#pragma GCC diagnostic pop

    virtual QScriptValue prototype() const
    {
        QScriptValue result;
        return result;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    virtual QueryFlags queryProperty(QScriptValue const& object, QScriptString const& property_name, QueryFlags flags, uint * id)
    {
        return QScriptClass::HandlesReadAccess;
    }

    virtual void setProperty(QScriptValue& object, QScriptString const& property_name, uint id, QScriptValue const& value)
    {
//SNAP_LOG_TRACE() << "setProperty() called... not implemented yet\n";
        throw std::runtime_error("setProperty() not implemented yet");
    }
#pragma GCC diagnostic pop

private:
    javascript *                f_javascript = nullptr;
    javascript_dynamic_plugin * f_plugin = nullptr;
};


/** \brief Plugins object iterator.
 *
 * This class is used to iterate through the list of plugins.
 */
class javascript_plugins_iterator : public QScriptClassPropertyIterator
{
public:
    javascript_plugins_iterator(javascript * js, QScriptEngine * engine, QScriptValue const & object_value)
        : QScriptClassPropertyIterator::QScriptClassPropertyIterator(object_value)
        , f_javascript(js)
        , f_engine(engine)
        //, f_pos(-1) -- auto-init
        , f_object(object_value)
    {
    }

    //virtual QScriptValue::PropertyFlags flags() const;

    virtual bool hasNext() const
    {
        return f_pos + 1 < f_javascript->f_dynamic_plugins.size();
    }

    virtual bool hasPrevious() const
    {
        return f_pos > 0;
    }

    virtual uint id() const
    {
        return f_pos;
    }

    virtual QScriptString name() const
    {
        if(f_pos < 0 || f_pos >= f_javascript->f_dynamic_plugins.size())
        {
            throw std::runtime_error("querying the name of the iterator object when the iterator pointer is out of scope");
        }
        plugins::plugin *p(dynamic_cast<plugins::plugin *>(f_javascript->f_dynamic_plugins[f_pos]));
        if(!p)
        {
            throw std::runtime_error("javascript.cpp:javascript_plugins_iterator::name: plugin pointer is null");
        }
        return f_engine->toStringHandle(p->get_plugin_name());
    }

    virtual void next()
    {
        if(f_pos < f_javascript->f_dynamic_plugins.size())
        {
            ++f_pos;
        }
    }

    virtual void previous()
    {
        if(f_pos > -1)
        {
            --f_pos;
        }
    }

    virtual void toBack()
    {
        // right after the last position
        f_pos = f_javascript->f_dynamic_plugins.size();
    }

    virtual void toFront()
    {
        // right before the first position
        f_pos = -1;
    }

    QScriptValue object() const
    {
        return f_object;
    }

private:
    javascript *                f_javascript = nullptr;
    QScriptEngine *             f_engine = nullptr;
    int32_t                     f_pos = -1;
    QScriptValue                f_object;
};


/** \brief Implement our own script class so we can dynamically get plugins values.
 *
 * This function is used to read data from the database based on the name
 * of the plugin and the name of the parameter that the user is interested
 * in. The JavaScript syntax looks like this:
 *
 * \code
 *        var n = plugins.layout.name;
 * \endcode
 *
 * In this case the layout plugin is queried for its parameter "name".
 */
class plugins_class : public QScriptClass
{
public:
    plugins_class(javascript * js, QScriptEngine * script_engine)
        : QScriptClass(script_engine)
        , f_javascript(js)
    {
    }

    // we do not currently support extensions
    //virtual bool supportsExtension(Extension extension) const { return false; }
    //virtual QVariant extension(Extension extension, const QVariant& argument = QVariant());

    virtual QString name() const
    {
        return "plugins";
    }

    virtual QScriptClassPropertyIterator * newIterator(QScriptValue const & object)
    {
        return new javascript_plugins_iterator(f_javascript, engine(), object);
    }

    virtual QScriptValue property(QScriptValue const & object, QScriptString const & object_name, uint id)
    {
        QString const temp_name(object_name);
        if(f_dynamic_plugins.contains(temp_name))
        {
            QScriptValue const plugin_object(engine()->newObject(f_dynamic_plugins[temp_name].data()));
            return plugin_object;
        }
        int const max_plugins(f_javascript->f_dynamic_plugins.size());
        for(int i(0); i < max_plugins; ++i)
        {
            plugins::plugin * p(dynamic_cast<plugins::plugin *>(f_javascript->f_dynamic_plugins[i]));
            if(!p)
            {
                throw std::runtime_error("plugin pointer is null (plugins_class::property)");
            }
            if(p->get_plugin_name() == temp_name)
            {
                QSharedPointer<dynamic_plugin_class> plugin(new dynamic_plugin_class(f_javascript, engine(), f_javascript->f_dynamic_plugins[i]));
                f_dynamic_plugins[temp_name] = plugin;
                QScriptValue plugin_object(engine()->newObject(plugin.data()));
                return plugin_object;
            }
        }
        // otherwise return whatever the default is
        return QScriptClass::property(object, object_name, id);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    virtual QScriptValue::PropertyFlags propertyFlags(QScriptValue const & object, QScriptString const & property_name, uint id)
    {
        // at some point we may want to allow read/write/delete...
        return QScriptValue::ReadOnly | QScriptValue::Undeletable | QScriptValue::KeepExistingFlags;
    }
#pragma GCC diagnostic pop

    virtual QScriptValue prototype() const
    {
        QScriptValue result;
        return result;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    virtual QueryFlags queryProperty(QScriptValue const & object, QScriptString const & property_name, QueryFlags flags, uint * id)
    {
        return QScriptClass::HandlesReadAccess;
    }

    virtual void setProperty(QScriptValue& object, const QScriptString& property_name, uint id, const QScriptValue& value)
    {
//SNAP_LOG_TRACE() << "setProperty() called... not implemented yet\n";
        throw std::runtime_error("setProperty() not implemented yet");
    }
#pragma GCC diagnostic pop

private:
    QMap<QString, QSharedPointer<dynamic_plugin_class> >    f_dynamic_plugins;
    javascript *                            f_javascript = nullptr;
};


/** \brief Use this function to run a script and get the result.
 *
 * This function compiles and run the specified script and then
 * return the result.
 *
 * Note that at this time we expect that all the server side code
 * is generated by the server and thus is 100% safe to run. This
 * includes the return value is under control by the different
 * plugins using this function.
 *
 * \param[in] script  The JavaScript code to evaluate.
 *
 * \return The result in a QVariant.
 */
QVariant javascript::evaluate_script(QString const & script)
{
//SNAP_LOG_TRACE() << "evaluating JS [" << script << "]\n";
    QScriptProgram program(script);
    QScriptEngine engine;
    plugins_class plugins(this, &engine);
    QScriptValue plugins_object(engine.newObject(&plugins));
    engine.globalObject().setProperty("plugins", plugins_object);
//SNAP_LOG_TRACE("object name = [")(plugins_object.scriptClass()->name())("] (")(plugins_object.isObject())(")\n");
    QScriptValue value(engine.evaluate(program));
    QVariant const variant(value.toVariant());
    if(value.isError())
    {
        // this happens if the script is not correct and it cannot be executed
        SNAP_LOG_ERROR("javascript: value says there is an error in \"")(script)("\"!");
    }
    if(engine.hasUncaughtException())
    {
        QScriptValue e(engine.uncaughtException());
        SNAP_LOG_ERROR("javascript: result = ")(engine.hasUncaughtException())(", e = ")(e.isError())(", s = \"")(e.toString())("\"");
    }
    return variant;
}


/** \brief Process new JavaScripts.
 *
 * As users upload new JavaScripts to the server, we want to have them
 * pre-minimized and compressed to serve them as fast as possible.
 *
 * \warning
 * The JavaScript plugin cannot depend on the content plugin (because
 * the layout depends on JavaScript and content depends on layout)
 *
 * \param[in] file_row  The row where the file is saved in \p files_table.
 * \param[in] file  The file to be processed.
 */
void javascript::on_process_attachment(libdbproxy::row::pointer_t file_row, content::attachment_file const & file)
{
    // TODO: got to finish the as2js compiler...
    NOTUSED(file_row);
    NOTUSED(file);
}


/** \brief Verify filename on upload.
 *
 * If uploading a file under /js/... then we prevent "invalid" filenames.
 * We force users to have a Debian compatible filename as in:
 *
 * \code
 * <name>_<version>.js
 * \endcode
 *
 * Any other filename is refused.
 *
 * The \<name> is composed of lower case letters (a-z) and digits (0-9)
 * and dashes (-). The name must start with a letter and cannot start or
 * end with a dash. The regex is:
 *
 * \code
 * [-a-z0-9]+
 * \endcode
 *
 * The name is mandatory and needs to be at least 2 characters.
 *
 * The \<version> must be a set of digits separated by periods. Note that
 * debian accepts many other characters. We do not here. It will make it
 * a lot easier to parse a version and sort items in order. In most cases,
 * users will have to rename their JavaScript files so they work in Snap!
 * The regex is:
 *
 * \code
 * [0-9]+(\.[0-9]+)*
 * \endcode
 *
 * A version is mandatory and must be at least one digit although we strongly
 * suggest that you use 3 numbers for published versions and a forth number
 * for development purposes:
 *
 * \code
 * <version>.<release>.<patch>.<development>
 * \endcode
 *
 * So, if you published a library with version 3.54.7 and find a small
 * problem, use version 3.54.7.1, 3.54.7.2, etc. until you find the full
 * fix for the problem and then release the fixed version as 3.54.8.
 * This will help you with loading the script because a new version forces
 * the browser to load the new image and refresh its cache. If you do not
 * change the version, the cache will most probably be in the way.
 *
 * \param[in] file  The file attachment information.
 * \param[in,out] secure  Whether the file is considered secure.
 * \param[in] fast  Whether it is the fast check (true) or not (false).
 */
void javascript::on_check_attachment_security(content::attachment_file const & file, content::permission_flag & secure, bool const fast)
{
    // always check the filename, just in case
    QString const cpath(file.get_file().get_filename());
    if(cpath.startsWith("js/") || cpath == "js")
    {
        snap_version::versioned_filename js_filename(".js");
        if(!js_filename.set_filename(file.get_file().get_filename()))
        {
            // not considered valid
            secure.not_permitted(js_filename.get_error());
            return;
        }
    }

    if(!fast)
    {
        // slow check, here we can verify that the script compiles
        // with QScript (TBD -- we'll have to make sure that QScript
        // does indeed support the full spectrum of the JavaScript
        // specification with scripts such as jQuery, Sizzle, etc.)
        // or maybe js (from node.js); we could also have a special
        // parser in our as2js project that verifies that the script
        // is okay (i.e. we could then, for example, forbid the eval()
        // function among others.)
    }
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
