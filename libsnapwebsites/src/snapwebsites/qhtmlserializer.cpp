// Snap Websites Servers -- generate HTML from the output of an XML Query
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

// The importance of having an HTML specific serializer comes from
// the fact that XML defines empty tags as <div/> which are not
// supported by most browsers. This serializer generates <div></div>
// which works in all browsers. It is sad that such things are as
// they are, but browsers have to be compatible with many old websites
// which include such bad syntax.
//
// Also unfortunate, Qt does not provide such a class.

#include "snapwebsites/qhtmlserializer.h"

#include <stdexcept>

#include "snapwebsites/poison.h"


QHtmlSerializer::QHtmlSerializer(QXmlNamePool namepool, QBuffer *output, bool const is_html)
    : f_namepool(namepool)
    , f_output(output)
    , f_status(html_serializer_status_t::HTML_SERIALIZER_STATUS_READY)
    //, f_element_stack() -- auto-init
    , f_is_html(is_html)
{
}


QHtmlSerializer::~QHtmlSerializer()
{
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void QHtmlSerializer::atomicValue(const QVariant& value)
{
}
#pragma GCC diagnostic pop


void QHtmlSerializer::attribute(const QXmlName& name, const QStringRef& value)
{
    f_output->write(" ");
    if(!name.prefix(f_namepool).isEmpty())
    {
        f_output->write(name.prefix(f_namepool).toUtf8());
        f_output->write(":");
    }
    f_output->write(name.localName(f_namepool).toUtf8());
    f_output->write("=\"");
    QString v(value.toString());
    v.replace('&', "&amp;")
     .replace('\"', "&quot;")
     .replace('<', "&lt;")
     .replace('>', "&gt;");
    f_output->write(v.simplified().toUtf8());
    f_output->write("\"");
}


void QHtmlSerializer::characters(const QStringRef& value)
{
    closeElement();

    QString v(value.toString());
    v.replace('&', "&amp;")
     .replace('<', "&lt;")
     .replace('>', "&gt;");
    f_output->write(v.toUtf8());
}


void QHtmlSerializer::comment(const QString& value)
{
    closeElement();

    // TBD -- I would think that value cannot include "--"
    //        because it has to be a valid comment;
    //        also, we want to have a way to remove all
    //        "useless" comments from the output
    f_output->write("<!--");
    f_output->write(value.toUtf8());
    f_output->write("-->");
}


void QHtmlSerializer::endDocument()
{
    // we are done
}


void QHtmlSerializer::endElement()
{
    // here is the magic necessary for proper HTML, all tags
    // are always closed with </name> except it is marked as
    // an empty tag
    QString element(f_element_stack.back());
    f_element_stack.pop_back();

    QString e(element.toLower());
    bool is_empty(false);
    if(f_is_html) switch(e[0].unicode())
    {
    case 'a':
        is_empty = e == "area";
        break;

    case 'b':
        is_empty = e == "br" || e == "base" || e == "basefont";
        break;

    case 'c':
        is_empty = e == "col";
        break;

    case 'f':
        is_empty = e == "frame";
        break;

    case 'h':
        is_empty = e == "hr";
        break;

    case 'i':
        is_empty = e == "img" || e == "input" || e == "isindex";
        break;

    case 'l':
        is_empty = e == "link";
        break;

    case 'm':
        is_empty = e == "meta";
        break;

    case 'p':
        is_empty = e == "param";
        break;

    default:
        is_empty = false;
        break;

    }
    if(is_empty)
    {
        if(f_status != html_serializer_status_t::HTML_SERIALIZER_STATUS_ELEMENT_OPEN)
        {
            throw std::runtime_error(QString("data was written inside empty HTML tag \"%1\"").arg(e).toUtf8().data());
        }
        f_status = html_serializer_status_t::HTML_SERIALIZER_STATUS_READY;

        // close empty tag
        // (note that the / is not required, but we want to keep it
        // XML compatible)
        f_output->write("/>");
    }
    else
    {
        closeElement();

        // close the element
        f_output->write("</");
        f_output->write(element.toUtf8());
        f_output->write(">");
    }
}


void QHtmlSerializer::endOfSequence()
{
    // nothing to do here
}


void QHtmlSerializer::namespaceBinding(const QXmlName& name)
{
    QString uri(name.namespaceUri(f_namepool));
    if(!uri.isEmpty())
    {
        // prefix is saved as a suffix in an attribute name
        f_output->write(" xmlns");
        if(!name.prefix(f_namepool).isEmpty())
        {
            f_output->write(":");
            f_output->write(name.prefix(f_namepool).toUtf8());
        }
        f_output->write("=\"");
        uri.replace('\"', "&quot;")
           .replace('<', "&lt;")
           .replace('>', "&gt;")
           .replace('&', "&amp;");
        f_output->write(uri.toUtf8());
        f_output->write("\"");
    }
}


void QHtmlSerializer::processingInstruction(const QXmlName& target, const QString& value )
{
    closeElement();

    // prefix is saved as a suffix in a processing instruction
    f_output->write("<?");
    f_output->write(target.localName(f_namepool).toUtf8());
    QString prefix(target.prefix(f_namepool));
    if(!prefix.isEmpty())
    {
        f_output->write(":");
        f_output->write(prefix.toUtf8());
    }
    f_output->write(value.toUtf8());
    f_output->write("?>");
}


void QHtmlSerializer::startDocument()
{
    // should we create docs here?
}


void QHtmlSerializer::startElement(const QXmlName& name)
{
    closeElement();

    f_output->write("<");
    QString element(name.prefix(f_namepool));
    if(!element.isEmpty())
    {
        element += ":";
    }
    element += name.localName(f_namepool);
    f_output->write(element.toUtf8());
    f_status = html_serializer_status_t::HTML_SERIALIZER_STATUS_ELEMENT_OPEN;
    f_element_stack.push_back(element);
}


void QHtmlSerializer::startOfSequence()
{
    // nothing to do here
}


void QHtmlSerializer::closeElement()
{
    if(f_status == html_serializer_status_t::HTML_SERIALIZER_STATUS_ELEMENT_OPEN)
    {
        f_status = html_serializer_status_t::HTML_SERIALIZER_STATUS_READY;
        f_output->write(">");
    }
}


// vim: ts=4 sw=4 et
