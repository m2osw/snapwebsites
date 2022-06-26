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
Changes made by Alexis Wilke so the model works in Qt 4.8 and with Snap.
*/
#ifndef _QDOMNODEMODEL_H_
#define _QDOMNODEMODEL_H_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QAbstractXmlNodeModel>
#include <QXmlNamePool>
#include <QDomDocument>
#pragma GCC diagnostic pop

class QDomNodeModel: public QAbstractXmlNodeModel
{
public:
    typedef QVector<QDomNode> Path;

                                            QDomNodeModel(QXmlNamePool, QDomDocument);
    virtual                                 ~QDomNodeModel() {}

    // QAbstractXmlNodeModel implementation
    virtual QUrl                            baseUri(QXmlNodeModelIndex const & n) const;
    virtual QXmlNodeModelIndex::DocumentOrder compareOrder(QXmlNodeModelIndex const & ni1, QXmlNodeModelIndex const & ni2) const;
    virtual QUrl                            documentUri(QXmlNodeModelIndex const & n) const;
    virtual QXmlNodeModelIndex              elementById(QXmlName const & id) const;
    virtual QXmlNodeModelIndex::NodeKind    kind(QXmlNodeModelIndex const & ni) const;
    virtual QXmlName                        name(QXmlNodeModelIndex const & ni) const;
    virtual QVector<QXmlName>               namespaceBindings(QXmlNodeModelIndex const & n) const;
    virtual QVector<QXmlNodeModelIndex>     nodesByIdref(QXmlName const & idref) const;
    virtual QXmlNodeModelIndex              root(QXmlNodeModelIndex const & n) const;
    virtual QString                         stringValue(QXmlNodeModelIndex const & n) const;
    virtual QVariant                        typedValue(QXmlNodeModelIndex const & node) const;

    //virtual QSourceLocation                 sourceLocation(QXmlNodeModelIndex const & index) const; -- this is not yet virtual

    QXmlNodeModelIndex                      fromDomNode(QDomNode const & n) const;
    QDomNode                                toDomNode(QXmlNodeModelIndex const & ni) const;

protected:
    // QAbstractXmlNodeModel implementation
    virtual QVector<QXmlNodeModelIndex>     attributes(QXmlNodeModelIndex const & element) const;
    virtual QXmlNodeModelIndex              nextFromSimpleAxis(SimpleAxis axis, QXmlNodeModelIndex const & origin) const;

private:
    Path path(QDomNode const & n) const;
    int childIndex(QDomNode const & n) const;

    mutable QXmlNamePool f_pool;
    mutable QDomDocument f_doc;
};

#endif // _QDOMNODEMODEL_H_
// vim: ts=4 sw=4 et
