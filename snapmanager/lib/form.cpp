//
// File:        form.cpp
// Object:      Helper functions to generate a simple form.
//
// Copyright:   Copyright (c) 2016-2017 Made to Order Software Corp.
//              All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// ourselves
//
#include "snapmanager/form.h"

// our lib
//
#include "snapmanager/manager.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>

// last entry
//
#include <snapwebsites/poison.h>


namespace snap_manager
{



widget::widget(QString const & name)
    : f_name(name)
{
}


widget_description::widget_description(QString const & label, QString const & name, QString const & description)
    : widget(name)
    , f_label(label)
    , f_description(description)
{
}


void widget_description::generate(QDomElement parent)
{
    QDomDocument doc(parent.ownerDocument());

    if(!f_label.isEmpty())
    {
        QDomElement label(doc.createElement("label"));
        label.setAttribute("for", f_name);
        parent.appendChild(label);

        //QDomText label_text(doc.createTextNode(f_label));
        //label.appendChild(label_text);
        snap::snap_dom::insert_html_string_to_xml_doc(label, f_label);
    }

    if(!f_description.isEmpty())
    {
        QDomElement p(doc.createElement("p"));
        p.setAttribute("class", "description");
        parent.appendChild(p);

        //QDomText p_text(doc.createTextNode(f_description));
        //p.appendChild(p_text);
        snap::snap_dom::insert_html_string_to_xml_doc(p, f_description);
    }
}


widget_input::widget_input(QString const & label, QString const & name, QString const & initial_value, QString const & description)
    : widget(name)
    , f_label(label)
    , f_value(initial_value)
    , f_description(description)
{
}


void widget_input::generate(QDomElement parent)
{
    QDomDocument doc(parent.ownerDocument());

    if(!f_label.isEmpty())
    {
        QDomElement label(doc.createElement("label"));
        label.setAttribute("for", f_name);
        parent.appendChild(label);

        //QDomText label_text(doc.createTextNode(f_label));
        //label.appendChild(label_text);
        snap::snap_dom::insert_html_string_to_xml_doc(label, f_label);
    }

    QDomElement input(doc.createElement("input"));
    input.setAttribute("type", "input"); // be explicit
    input.setAttribute("name", f_name);
    input.setAttribute("value", f_value);
    input.setAttribute("id", f_name); // names have to be unique so this is enough for id
    parent.appendChild(input);

    if(!f_description.isEmpty())
    {
        QDomElement p(doc.createElement("p"));
        p.setAttribute("class", "description");
        parent.appendChild(p);

        //QDomText p_text(doc.createTextNode(f_description));
        //p.appendChild(p_text);
        snap::snap_dom::insert_html_string_to_xml_doc(p, f_description);
    }

}


widget_text::widget_text(QString const & label, QString const & name, QString const & initial_value, QString const & description)
    : widget(name)
    , f_label(label)
    , f_value(initial_value)
    , f_description(description)
{
}


void widget_text::generate(QDomElement parent)
{
    QDomDocument doc(parent.ownerDocument());

    if(!f_label.isEmpty())
    {
        QDomElement label(doc.createElement("label"));
        label.setAttribute("for", f_name);
        parent.appendChild(label);

        //QDomText label_text(doc.createTextNode(f_label));
        //label.appendChild(label_text);
        snap::snap_dom::insert_html_string_to_xml_doc(label, f_label);
    }

    QDomElement edit_text(doc.createElement("textarea"));
    edit_text.setAttribute("name", f_name);
    edit_text.setAttribute("autocomplete", "off");
    edit_text.setAttribute("cols", "100");
    edit_text.setAttribute("rows", "10");
    edit_text.setAttribute("wrap", "soft");
    edit_text.setAttribute("style", "white-space: pre; overflow-wrap: normal; overflow: auto;");
    edit_text.setAttribute("id", f_name); // names have to be unique so this is enough for id
    parent.appendChild(edit_text);

    QDomText text(doc.createTextNode(f_value));
    edit_text.appendChild(text);

    if(!f_description.isEmpty())
    {
        QDomElement p(doc.createElement("p"));
        p.setAttribute("class", "description");
        parent.appendChild(p);

        //QDomText p_text(doc.createTextNode(f_description));
        //p.appendChild(p_text);
        snap::snap_dom::insert_html_string_to_xml_doc(p, f_description);
    }

}


widget_select::widget_select(QString const & label
                        , QString const & name
                        , QStringList const & initial_value
                        , QString const & default_value
                        , QString const & description)
    : widget(name)
    , f_label(label)
    , f_valueList(initial_value)
    , f_defaultValue(default_value)
    , f_description(description)
{
}


void widget_select::generate(QDomElement parent)
{
    QDomDocument doc(parent.ownerDocument());

    if(!f_label.isEmpty())
    {
        QDomElement label(doc.createElement("label"));
        label.setAttribute("for", f_name);
        parent.appendChild(label);

        snap::snap_dom::insert_html_string_to_xml_doc(label, f_label);
    }

    QDomElement select(doc.createElement("select"));
    select.setAttribute( "name", f_name                 );
    select.setAttribute( "form", parent.attribute("id") );
    for( QString const& item : f_valueList )
    {
        QDomElement option ( doc.createElement("option")  );
        option.setAttribute( "value", item );
        if( item == f_defaultValue )
        {
            option.setAttribute( "selected", "selected" );
        }
        snap::snap_dom::insert_html_string_to_xml_doc( option, item );
        select.appendChild(option);
    }
    parent.appendChild(select);

    if(!f_description.isEmpty())
    {
        QDomElement p(doc.createElement("p"));
        p.setAttribute("class", "description");
        parent.appendChild(p);
        snap::snap_dom::insert_html_string_to_xml_doc(p, f_description);
    }

}


form::form(QString const & plugin_name, QString const & field_name, button_t buttons)
    : f_plugin_name(plugin_name)
    , f_field_name(field_name)
    , f_buttons(buttons)
{
}


void form::generate(QDomElement parent, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    QString host("-undefined-");
    if(uri.has_query_option("host"))
    {
        host = uri.query_option("host");
    }

    // create the form tag
    //
    QDomElement form_tag(doc.createElement("form"));
    form_tag.setAttribute("class", "manager_form");
    form_tag.setAttribute("accept-charset", "UTF-8");
    form_tag.setAttribute("action", "?" + uri.query_string());
    form_tag.setAttribute("method", "POST");
    form_tag.setAttribute("id", QString("%1::%2").arg(f_plugin_name).arg(f_field_name) );
    parent.appendChild(form_tag);

    // add the host, plugin name, and field name as hidden fields
    //
    QDomElement host_name_tag(doc.createElement("input"));
    host_name_tag.setAttribute("name", "hostname");
    host_name_tag.setAttribute("type", "hidden");
    host_name_tag.setAttribute("value", host);
    form_tag.appendChild(host_name_tag);

    QDomElement plugin_name_tag(doc.createElement("input"));
    plugin_name_tag.setAttribute("name", "plugin_name");
    plugin_name_tag.setAttribute("type", "hidden");
    plugin_name_tag.setAttribute("value", f_plugin_name);
    form_tag.appendChild(plugin_name_tag);

    QDomElement field_name_tag(doc.createElement("input"));
    field_name_tag.setAttribute("name", "field_name");
    field_name_tag.setAttribute("type", "hidden");
    field_name_tag.setAttribute("value", f_field_name);
    form_tag.appendChild(field_name_tag);

    // add the widgets defined by the caller
    //
    std::for_each(f_widgets.begin(), f_widgets.end(),
            [&form_tag](auto const & w) { w->generate(form_tag); });

    // add reset and save buttons
    //
    if((f_buttons & FORM_BUTTON_RESET) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "reset");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Reset"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_SAVE) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "save");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Save"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_SAVE_EVERYWHERE) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "save_everywhere");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Save Everywhere"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_RESTORE_DEFAULT) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "restore_default");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Restore Default"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_INSTALL) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "install");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Install"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_UNINSTALL) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "uninstall");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Uninstall"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_REBOOT) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "reboot");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Reboot"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_UPGRADE) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "upgrade");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Upgrade"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_UPGRADE_EVERYWHERE) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "upgrade_everywhere");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Upgrade Everywhere"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_REFRESH) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "refresh");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Refresh"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_RESTART) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "restart");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Restart Service"));
        button.appendChild(text);
    }
    if((f_buttons & FORM_BUTTON_RESTART_EVERYWHERE) != 0)
    {
        QDomElement button(doc.createElement("button"));
        button.setAttribute("type", "submit");
        button.setAttribute("name", "restart_everywhere");
        form_tag.appendChild(button);

        QDomText text(doc.createTextNode("Restart Service Everywhere"));
        button.appendChild(text);
    }
}


void form::add_widget(widget::pointer_t w)
{
    f_widgets.push_back(w);
}



} // namespace snap_manager
// vim: ts=4 sw=4 et
