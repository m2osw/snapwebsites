// Snap Websites Servers -- handle messages for QXmlQuery
// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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

// libexcept
//
#include "libexcept/exception.h"

// Qt
//
#include <QDomDocument>
#include <QVariant>
#include <QMap>



namespace snap
{

DECLARE_MAIN_EXCEPTION(xslt_exception);

DECLARE_EXCEPTION(xslt_exception, xslt_initialization_error);
DECLARE_EXCEPTION(xslt_exception, xslt_evaluation_error);




class xslt
{
public:
    void                    set_xsl(QString const & xsl);
    void                    set_xsl(QDomDocument const & xsl);
    void                    set_xsl_from_file(QString const & filename);
    void                    set_document(QString const & doc);
    void                    set_document(QDomDocument & doc);
    void                    add_variable(QString const & name, QVariant const & value);
    void                    clear_variables();

    QString                 evaluate_to_string();
    void                    evaluate_to_document(QDomDocument & output);

    static QString          filter_entities_out(QString const & html);
    static QString          convert_entity(QString const & entity_name);

private:
    void                    evaluate(QString * output_string, QDomDocument * output_document);

    QString                 f_xsl = QString();
    QString                 f_input = QString(); // document as a string
    QMap<QString, QVariant> f_variables = {};
    QDomDocument            f_doc = QDomDocument();
};

} // namespace snap
// vim: ts=4 sw=4 et
