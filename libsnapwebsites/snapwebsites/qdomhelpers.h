// Snap Websites Servers -- helper functions used against the DOM
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

// libexcept lib
//
#include    <libexcept/exception.h>


// Qt lib
//
#include    <QDomDocument>
#include    <QDomElement>



namespace snap
{
namespace snap_dom
{


DECLARE_MAIN_EXCEPTION(snap_dom_exception);

DECLARE_EXCEPTION(snap_dom_exception, snap_dom_exception_element_not_found);




bool            get_tag(QString const & tag_name, QDomElement & element, QDomElement & tag, bool create = true);
void            append_plain_text_to_node(QDomNode & node, QString const & plain_text);
void            append_integer_to_node(QDomNode & node, int64_t integer);
void            insert_html_string_to_xml_doc(QDomNode & child, QString const & xml);
void            insert_node_to_xml_doc(QDomNode & child, QDomNode const & node);
QString         xml_to_string(QDomNode const & node);
QString         xml_children_to_string(QDomNode const & node);
void            replace_node_with_html_string(QDomNode & child, QString const & html);
void            replace_node_with_elements(QDomNode & replace, QDomNode const & node);
void            remove_all_children(QDomElement & parent);
QDomElement     get_element(QDomDocument & doc, QString const & name, bool must_exist = true);
QDomElement     get_child_element(QDomNode parent, QString const & path);
QDomElement     create_element(QDomNode parent, QString const & path);
QString         remove_tags(QString const & html);
QString         escape(QString const & str);
QString         unescape(QString const & str);


} // namespace snap_dom
} // namespace snap
// vim: ts=4 sw=4 et
