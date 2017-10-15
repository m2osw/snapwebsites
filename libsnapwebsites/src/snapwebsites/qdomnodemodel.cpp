/*
Copyright (c) 2011, Stanislaw Adaszewski
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Stanislaw Adaszewski nor the
      names of other contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL STANISLAW ADASZEWSKI BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Copyright (c) 2012-2017
Changes made by Alexis Wilke so the model works in Qt 5.x and with Snap.
*/

#include "snapwebsites/qdomnodemodel.h"

#include <QDomNode>
#include <QDomDocument>
#include <QUrl>
#include <QVector>
#include <QSourceLocation>
#include <QVariant>
#include <QDebug>

#include "snapwebsites/poison.h"


class MyDomNode: public QDomNode
{
public:
    MyDomNode(const QDomNode& other):
        QDomNode(other)
    {
    }

    MyDomNode(QDomNodePrivate *otherImpl):
        QDomNode(otherImpl)
    {
    }

    QDomNodePrivate *getImpl()
    {
        return impl;
    }
};

QDomNodeModel::QDomNodeModel(QXmlNamePool pool, QDomDocument doc):
    f_pool(pool), f_doc(doc)
{
qDebug() << "QDomNodeModel::init";
}

QUrl QDomNodeModel::baseUri (const QXmlNodeModelIndex &) const
{
    // TODO: Not implemented.
qDebug() << "Not implemented: QDomNodeModel::baseUri()";
    return QUrl();
}

QXmlNodeModelIndex::DocumentOrder QDomNodeModel::compareOrder (
    const QXmlNodeModelIndex & ni1,
    const QXmlNodeModelIndex & ni2 ) const
{
qDebug() << "compareOrder";
    QDomNode n1 = toDomNode(ni1);
    QDomNode n2 = toDomNode(ni2);

//qDebug() << "Comparing...";
    if (n1 == n2)
        return QXmlNodeModelIndex::Is;

    Path p1 = path(n1);
    Path p2 = path(n2);

    for (int i = 1; i < p1.size(); i++)
        if (p1[i] == n2)
            return QXmlNodeModelIndex::Follows;

    for (int i = 1; i < p2.size(); i++)
        if (p2[i] == n1)
            return QXmlNodeModelIndex::Precedes;

    for (int i = 1; i < p1.size(); i++)
        for (int j = 1; j < p2.size(); j++)
        {
            if (p1[i] == p2[j]) // Common ancestor
            {
                int ci1 = childIndex(p1[i-1]);
                int ci2 = childIndex(p2[j-1]);

                if (ci1 < ci2)
                    return QXmlNodeModelIndex::Precedes;
                else
                    return QXmlNodeModelIndex::Follows;
            }
        }

    return QXmlNodeModelIndex::Precedes; // Should be impossible!
}

QUrl QDomNodeModel::documentUri(const QXmlNodeModelIndex&) const
{
    // TODO: Not implemented.
qDebug() << "Not implemented: QDomNodeModel::documentUri()";
    return QUrl();
}

QXmlNodeModelIndex QDomNodeModel::elementById(const QXmlName& id) const
{
qDebug() << "id to element";
    return fromDomNode(f_doc.elementById(id.toClarkName(f_pool)));
}

QXmlNodeModelIndex::NodeKind QDomNodeModel::kind(const QXmlNodeModelIndex& ni) const
{
qDebug() << "kind";
    QDomNode n = toDomNode(ni);
    if (n.isAttr()) {
//qDebug() << "Kind... attr";
        return QXmlNodeModelIndex::Attribute;
    }
    else if (n.isText()) {
//qDebug() << "Kind... text";
        return QXmlNodeModelIndex::Text;
    }
    else if (n.isComment()) {
//qDebug() << "Kind... comment";
        return QXmlNodeModelIndex::Comment;
    }
    else if (n.isDocument()) {
//qDebug() << "Kind... document";
        return QXmlNodeModelIndex::Document;
    }
    else if (n.isElement()) {
//qDebug() << "Kind... element";
        return QXmlNodeModelIndex::Element;
    }
    else if (n.isProcessingInstruction()) {
//qDebug() << "Kind... processing?!";
        return QXmlNodeModelIndex::ProcessingInstruction;
    }

//qDebug() << "No Kind??";
    return static_cast<QXmlNodeModelIndex::NodeKind>(0);
}

QXmlName QDomNodeModel::name(const QXmlNodeModelIndex& ni) const
{
qDebug() << "name";
    QDomNode n = toDomNode(ni);

    if (n.isAttr() || n.isElement() || n.isProcessingInstruction()) {
//qDebug() << "Get name" << n.nodeName() << "/" << n.namespaceURI() << "/" << n.prefix();
        return QXmlName(f_pool, n.localName(), n.namespaceURI(), n.prefix());
    }

//qDebug() << "No name?!";
    return QXmlName(f_pool, QString(), QString(), QString());
}

QVector<QXmlName> QDomNodeModel::namespaceBindings(const QXmlNodeModelIndex&) const
{
    // TODO: Not implemented.
qDebug() << "Not implemented: QDomNodeModel::namespaceBindings()";
    return QVector<QXmlName>();
}

QVector<QXmlNodeModelIndex> QDomNodeModel::nodesByIdref(const QXmlName&) const
{
    // TODO: Not implemented.
qDebug() << "Not implemented: QDomNodeModel::nodesByIdref()";
    return QVector<QXmlNodeModelIndex>();
}

QXmlNodeModelIndex QDomNodeModel::root ( const QXmlNodeModelIndex & ni ) const
{
qDebug() << "Get root";
    QDomNode n = toDomNode(ni);
    while (!n.parentNode().isNull()) {
        n = n.parentNode();
    }

    return fromDomNode(n);
}

// This is not a virtual function, we cannot overload it
//QSourceLocation QDomNodeModel::sourceLocation(const QXmlNodeModelIndex&) const
//{
//    // TODO: Not implemented.
//qDebug() << "Not implemented: QDomNodeModel::sourceLocation()";
//    return QSourceLocation();
//}

QString    QDomNodeModel::stringValue(const QXmlNodeModelIndex & ni) const
{
qDebug() << "stringValue";
    QDomNode n = toDomNode(ni);

    if (n.isProcessingInstruction()) {
        return n.toProcessingInstruction().data();
    }
    else if (n.isText()) {
//qDebug() << "Text:" << n.toText().data();
        return n.toText().data();
    }
    else if (n.isComment()) {
        return n.toComment().data();
    }
    else if (n.isElement()) {
//qDebug() << "Element:" << n.toElement().text();
        return n.toElement().text();
    }
    else if (n.isDocument()) {
        return n.toDocument().documentElement().text();
    }
    else if (n.isAttr()) {
//qDebug() << "Attribute:" << n.toElement().text();
        return n.toAttr().value();
    }

    return QString();
}

QVariant QDomNodeModel::typedValue(const QXmlNodeModelIndex& ni) const
{
qDebug() << "Typed (hmmm?) value";
    return qVariantFromValue(stringValue(ni));
}

QXmlNodeModelIndex QDomNodeModel::fromDomNode(const QDomNode& n) const
{
qDebug() << "QDomNodeModel::fromDomNode()...";
    if(n.isNull()) {
qDebug() << "QDomNodeModel::fromDomNode() -- NULL!?";
        return QXmlNodeModelIndex();
    }

qDebug() << "QDomNodeModel::fromDomNode() -- createIndex() called..." << MyDomNode(n).getImpl();
    return createIndex(MyDomNode(n).getImpl(), 0);
}

QDomNode QDomNodeModel::toDomNode(const QXmlNodeModelIndex& ni) const
{
qDebug() << "toDomNode";
    return MyDomNode(reinterpret_cast<QDomNodePrivate *>(ni.data()));
}

QDomNodeModel::Path QDomNodeModel::path(const QDomNode& n) const
{
qDebug() << "path";
    Path res;
    QDomNode cur = n;
    while (!cur.isNull())
    {
        res.push_back(cur);
        cur = cur.parentNode();
    }
    return res;
}

int QDomNodeModel::childIndex(const QDomNode& n) const
{
qDebug() << "childIndex";
    QDomNodeList children = n.parentNode().childNodes();
    for (int i = 0; i < children.size(); i++)
        if (children.at(i) == n)
            return i;

    return -1;
}

QVector<QXmlNodeModelIndex> QDomNodeModel::attributes ( const QXmlNodeModelIndex & ni ) const
{
qDebug() << "attributes";
    QDomElement n = toDomNode(ni).toElement();
    QDomNamedNodeMap attrs = n.attributes();
    QVector<QXmlNodeModelIndex> res;
    for (int i = 0; i < attrs.size(); i++)
    {
        res.push_back(fromDomNode(attrs.item(i)));
    }
    return res;
}

QXmlNodeModelIndex QDomNodeModel::nextFromSimpleAxis ( SimpleAxis axis, const QXmlNodeModelIndex & ni ) const
{
qDebug() << "nextFromSimpleAxis";
    QDomNode n(toDomNode(ni));
    switch(axis) {
    case Parent:
        return fromDomNode(n.parentNode());

    case FirstChild:
        return fromDomNode(n.firstChild());

    case PreviousSibling:
        return fromDomNode(n.previousSibling());

    case NextSibling:
        return fromDomNode(n.nextSibling());

    default:
        return QXmlNodeModelIndex();

    }
}

// vim: ts=4 sw=4
