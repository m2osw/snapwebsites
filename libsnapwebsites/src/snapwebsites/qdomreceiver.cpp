// Snap Websites Servers -- generate a DOM from the output of an XML Query
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

#include "snapwebsites/qdomreceiver.h"

#include "snapwebsites/poison.h"


QDomReceiver::QDomReceiver(QXmlNamePool namepool, QDomDocument doc)
    : f_namepool(namepool)
    , f_doc(doc)
{
    f_element = doc.documentElement();
}

QDomReceiver::~QDomReceiver()
{
}

void QDomReceiver::atomicValue(const QVariant& )
{
}

void QDomReceiver::attribute(const QXmlName& name, const QStringRef& value)
{
    if(name.prefix(f_namepool).isEmpty())
    {
        f_element.setAttribute(name.localName(f_namepool), value.toString());
    }
    else
    {
        f_element.setAttributeNS(name.prefix(f_namepool), name.localName(f_namepool), value.toString());
    }
}

void QDomReceiver::characters(const QStringRef& value)
{
    f_element.appendChild(f_doc.createTextNode(value.toString()));
}

void QDomReceiver::comment(const QString& value)
{
    f_element.appendChild(f_doc.createComment(value));
}

void QDomReceiver::endDocument()
{
    // we're done
}

void QDomReceiver::endElement()
{
    // elements are automatically closed, but we want to move up in the tree
    f_element = f_element.parentNode().toElement();
}

void QDomReceiver::endOfSequence()
{
    // nothing to do here
}

void QDomReceiver::namespaceBinding(const QXmlName& name)
{
    QString uri(name.namespaceUri(f_namepool));
    if(!uri.isEmpty())
    {
        // prefix is saved as a suffix in an attribute name
        QString prefix(name.prefix(f_namepool));
        if(!prefix.isEmpty())
        {
            prefix = ":" + prefix;
        }
        f_element.setAttribute("xmlns" + prefix, uri);
    }
}

void QDomReceiver::processingInstruction(const QXmlName& target, const QString& value )
{
    // prefix is saved as a suffix in a processing instruction
    QString prefix(target.prefix(f_namepool));
    if(!prefix.isEmpty())
    {
        prefix = ":" + prefix;
    }
    f_element.appendChild(f_doc.createProcessingInstruction(target.localName(f_namepool) + prefix, value));
}

void QDomReceiver::startDocument()
{
    // should we create docs here?
}

void QDomReceiver::startElement(const QXmlName& name)
{
    QString prefix(name.prefix(f_namepool));
    if(!prefix.isEmpty())
    {
        prefix += ":";
    }
    QDomElement element(f_doc.createElement(prefix + name.localName(f_namepool)));
    if(f_element.isNull())
    {
        f_doc.appendChild(element);
    }
    else
    {
        f_element.appendChild(element);
    }
    f_element = element;
}

void QDomReceiver::startOfSequence()
{
    // nothing to do here
}

// vim: ts=4 sw=4 et
