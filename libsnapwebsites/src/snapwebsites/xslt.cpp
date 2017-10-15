// Snap Websites Servers -- handle calls to QXmlQuery
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

#include "snapwebsites/xslt.h"

#include "snapwebsites/snapwebsites.h"
#include "snapwebsites/log.h"
#include "snapwebsites/qdomreceiver.h"
#include "snapwebsites/qhtmlserializer.h"
#include "snapwebsites/qxmlmessagehandler.h"

#include <QXmlQuery>

#include "snapwebsites/poison.h"


namespace snap
{


/** \brief Save the XSLT parser.
 *
 * This function receives a copy of the XSLT parser in the form of a string.
 * It gets saved in the class and reused by the evaluation functions
 * until changed.
 *
 * \param[in] xsl  The XSLT parser document.
 */
void xslt::set_xsl(QString const & xsl)
{
    f_xsl = xsl;
}


/** \brief Save the XSLT parser.
 *
 * This function receives a copy of the XSLT parser in the form of a string.
 * It gets saved in the class and reused by the evaluation functions
 * until changed.
 *
 * \note
 * At this the XML document is immediately transformed to a string. At
 * some point we may be able to use the XML document as such with
 * QXmlQuery...
 *
 * \param[in] xsl  The XSLT parser document.
 */
void xslt::set_xsl(QDomDocument const & xsl)
{
    f_xsl = xsl.toString(-1);
}


/** \brief Set the XSLT parser from the content of a file.
 *
 * When you have a filename you may bypass loading the file yourselves
 * and call this function.
 *
 * The loading is done using the load_file() signal.
 *
 * \warning
 * This function assumes that you have a valid running server with
 * all the plugins loaded (i.e. running in snap_child or snap_backend
 * or any plugin.)
 *
 * \exception xslt_initialization_error
 * This exception is raised if the named file cannot be loaded since
 * that means we will not be able to transform the input.
 *
 * \param[in] filename  The name of the file to load.
 */
void xslt::set_xsl_from_file(QString const & filename)
{
    // make sure we have access to a valid server
    server::pointer_t server(server::instance());
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }

    // whether he file was found
    bool found(false);

    // setup the file
    snap::snap_child::post_file_t file;
    file.set_filename(filename);

    // try to load the data
    server->load_file(file, found);

    // if not found, we have a problem
    if(!found)
    {
        throw xslt_initialization_error(QString("xslt::set_xsl_from_file() could not load file \"%1\".").arg(filename));
    }

    // okay, it got loaded, save the resulting file in here
    f_xsl = QString::fromUtf8(file.get_data(), file.get_size());
}


/** \brief Set the document to be transformed.
 *
 * This function saves a copy of the specified input document in
 * this object. This is the document that is going to be transformed
 * using the XSLT script.
 *
 * In most cases this document is expected to be HTML or XHTML.
 *
 * \note
 * If you have a DOM document, you may call the other
 * set_document(QDomDocument &) to get the same effect.
 *
 * \param[in] input  The document to be parsed.
 *
 * \sa set_document(QDomDocument & doc);
 */
void xslt::set_document(QString const & input)
{
    f_doc.clear();
    f_input = input;
}


/** \brief Set the document to be transformed.
 *
 * This function saves a copy of the specified input document in
 * this object. This is the document that is going to be transformed
 * using the XSLT script.
 *
 * \note
 * If you have a string, you may call the other
 * set_document(QString &) function which is going
 * to be must faster than generating a document
 * and saving it back to a string.
 *
 * \param[in] doc  The document to be parsed.
 *
 * \sa set_document(QDomDocument & doc);
 */
void xslt::set_document(QDomDocument & doc)
{
    f_input.clear();
    f_doc = doc;
}


/** \brief Add a variable which will be used with the parser.
 *
 * This function can be used to add variables to the parser.
 * These are added in the evaluation function just before
 * the evaluation takes place.
 *
 * In most cases, though, you add your data to the input
 * XML document and not here as variables.
 *
 * \param[in] name  The name of the variable.
 * \param[in] value  The value of the variable.
 *
 * \sa clear_variables()
 */
void xslt::add_variable(QString const & name, QVariant const & value)
{
    f_variables[name] = value;
}


/** \brief Clear all the variables.
 *
 * This function can be used to clear all the variables of an XSLT
 * object. Any variable that were added with the add_variable()
 * function are removed.
 *
 * \sa add_variable()
 */
void xslt::clear_variables()
{
    f_variables.clear();
}


/** \brief Run parser and return the result as a string.
 *
 * This function runs the parser to apply the XSLT 2.0 transformations
 * against the input document and returns the result as a string.
 *
 * \return The transformation result as a string.
 */
QString xslt::evaluate_to_string()
{
    QString result;
    evaluate(&result, nullptr);
    return result;
}


/** \brief Run parser and return the result in \p output.
 *
 * This function runs the parser to apply the XSLT 2.0 transformations
 * against the input document and returns the result as an XML document
 * in the specified output object. Whether the output DOM is currently
 * empty has no bearing to the process although things may not work
 * as expected if the output is not just one document root element,
 * though.
 *
 * \param[in,out] output  The output QDomDocument where the parsing gets saved.
 *
 * \return The transformation result as a string.
 */
void xslt::evaluate_to_document(QDomDocument & output)
{
    evaluate(nullptr, &output);
}


/** \brief Evaluate the transformation.
 *
 * This function is the internal function that is used to evaluate the
 * output. The functions calling parameters is "complicated" and hence
 * the function was made internal and two public functions created
 * so the outside world can make easy calls instead.
 *
 * \todo
 * Look into getting the XML sitemap version to here? Actually, the
 * best would be to look into making use of XML data as input since
 * that would allow us to avoid some (many?) conversions to and from
 * string.
 *
 * \param[out] output_string  The output string if evaluate_to_string()
 *                            was called.
 * \param[in,out] output_document  The output document if
 *                                 evaluate_to_document() was called.
 */
void xslt::evaluate(QString * output_string, QDomDocument * output_document)
{
    bool first_attempt(true);

    for(;;)
    {
        QXmlQuery q(QXmlQuery::XSLT20);

        QString doc_str(f_input);
        if(doc_str.isEmpty())
        {
            if(f_doc.isNull())
            {
                throw snap_logic_exception("xslt::evaluate_to_string(): f_input and f_doc are both empty.");
            }

            doc_str = f_doc.toString(-1);
        }

        // if it is our second attempt, then we need to transform the
        // entities to Unicode characters
        if(!first_attempt)
        {
            // this function is rather slow so we try to not run it
            // here since we are likely live when running this...
            // a filter will be created to run this function on
            // all fields in the database that may include "incorrect"
            // (as per QXmlQuery) HTML code.
            //
            doc_str = filter_entities_out(doc_str);
        }

        // setup our message handler
        QMessageHandler msg;
        msg.set_xsl(f_xsl);
        msg.set_doc(doc_str);

#ifdef _DEBUG
//SNAP_LOG_DEBUG("!!!! f_xsl="  )(f_xsl);
//SNAP_LOG_DEBUG("!!!! doc_str=")(doc_str);
#endif

        // setup the XML query object
        q.setMessageHandler(&msg);
        q.setFocus(doc_str);

        // set variables
        // WARNING: variables MUST be set before you call the setQuery()
        //          function since it may start making use of them
        //
        // TBD: I think variables can cause problems, although it is used by
        //      the form and editor implementations...
        //
        for(auto p(f_variables.begin());
                 p != f_variables.end();
                 ++p)
        {
            q.bindVariable(p.key(), p.value());
        }

        // setup the transformation data
        q.setQuery(f_xsl);
        if(!q.isValid())
        {
            if(first_attempt)
            {
                goto try_again;
            }
            throw xslt_evaluation_error("Invalid XSLT query detected by Qt.");
        }

        if(output_document != nullptr)
        {
            // this should be faster since we keep the data in a DOM
            QDomReceiver receiver(q.namePool(), *output_document);
            q.evaluateTo(&receiver);

            if(!msg.has_entities())
            {
                if(msg.had_msg())
                {
                    // do something? but what then?
                }
                return;
            }
        }
        else if(output_string != nullptr)
        {
            // request the evalutation in a QBuffer
            QBuffer output;
            output.open(QBuffer::ReadWrite);
            QHtmlSerializer html(q.namePool(), &output);
            q.evaluateTo(&html);

            if(!msg.has_entities())
            {
                if(!msg.had_msg())
                {
                    *output_string = QString::fromUtf8(output.data());
                }
                else
                {
                    // return a default so we know something went wrong
                    *output_string = "[QXmlParser failed to transform your data]";
                }
                return;
            }
        }
        else
        {
            throw xslt_evaluation_error("incorrect use of evaluate() with both output_document and output_string");
        }

        // if we reach here then the input data includes entities
        if(!first_attempt)
        {
            throw xslt_evaluation_error("your input document could not be converted by QXmlQuery.");
        }
try_again:
        first_attempt = false;
    }

// The following is for history record.
// I first tried to implement the transformation from XML to XML so avoid
// wasting time converting to a string first and then back to XML
// 
//     QString const doc_str(doc.toString(-1));
//     if(doc_str.isEmpty())
//     {
//         throw snap_logic_exception("somehow the memory XML document for the body XSLT is empty");
//     }
//     QXmlQuery q(QXmlQuery::XSLT20);
//     QMessageHandler msg;
//     msg.set_xsl(xsl);
//     msg.set_doc(doc_str);
//     q.setMessageHandler(&msg);
// #if 0
//     QDomNodeModel m(q.namePool(), doc);
//     QXmlNodeModelIndex x(m.fromDomNode(doc.documentElement()));
//     QXmlItem i(x);
//     q.setFocus(i);
// #else
//     q.setFocus(doc_str);
// #endif
//     q.setQuery(xsl);
//     if(!q.isValid())
//     {
//         throw layout_exception_invalid_xslt_data(QString("invalid XSLT query for BODY \"%1\" detected by Qt").arg(ipath.get_key()));
//     }
// #if 0
//     QXmlResultItems results;
//     q.evaluateTo(&results);
//     
//     QXmlItem item(results.next());
//     while(!item.isNull())
//     {
//         if(item.isNode())
//         {
//             //printf("Got a node!\n");
//             QXmlNodeModelIndex node_index(item.toNodeModelIndex());
//             QDomNode node(m.toDomNode(node_index));
//             printf("Got a node! [%s]\n", node.localName()/*ownerDocument().toString(-1)*/.toUtf8().data());
//         }
//         item = results.next();
//     }
// #elif 1
//     // this should be faster since we keep the data in a DOM
//     QDomDocument doc_output("body");
//     QDomReceiver receiver(q.namePool(), doc_output);
//     q.evaluateTo(&receiver);
// 
//     extract_js_and_css(doc, doc_output);
//     body.appendChild(doc.importNode(doc_output.documentElement(), true));
// //std::cout << "Body HTML is [" << doc_output.toString(-1) << "]\n";
// #else
//     //QDomDocument doc_body("body");
//     //doc_body.setContent(get_content_parameter(path, get_name(name_t::SNAP_NAME_CONTENT_BODY) <<-- that would be wrong now).stringValue(), true, nullptr, nullptr, nullptr);
//     //QDomElement content_tag(doc.createElement("content"));
//     //body.appendChild(content_tag);
//     //content_tag.appendChild(doc.importNode(doc_body.documentElement(), true));
// 
//     // TODO: look into getting XML as output
//     QString out;
//     q.evaluateTo(&out);
//     //QDomElement output(doc.createElement("output"));
//     //body.appendChild(output);
//     //QDomText text(doc.createTextNode(out));
//     //output.appendChild(text);
//     QDomDocument doc_output("body");
//     doc_output.setContent(out, true, nullptr, nullptr, nullptr);
//     body.appendChild(doc.importNode(doc_output.documentElement(), true));
// #endif
}


/** \brief Filter an HTML document and replace entities by their characters.
 *
 * This function goes through the entire HTML document and makes sure that
 * all entities get transformed from the HTML syntax (\&name;)
 * to the corresponding Unicode character.
 *
 * This is important because Browsers may send us HTML with entities and
 * unfortunately the QXmlQuery system does not support them. (Actually
 * the Qt XML loaders are expected to transform entities to Unicode
 * characters on the fly, but in our case only the &lt;, &gt;, and &amp;
 * are recognized.)
 *
 * \param[in] html  The HTML to be transformed.
 *
 * \return The transformed HTML.
 */
QString xslt::filter_entities_out(QString const & html)
{
    QString result;

    int pos(0);
    for(;;)
    {
        int amp(html.indexOf('&', pos));
        if(amp == -1)
        {
            // no more '&', return immediately
            result += html.mid(pos);
            return result;
        }
        ++amp;

        // TODO: check whether we find a '<', '>', or '&' before the ';'
        int const semi_colon(html.indexOf(';', amp));
        if(semi_colon == -1)
        {
            // found '&' without ';', return it as is
            result += html.mid(pos);
            return result;
        }

        if(amp >= html.length())
        {
            // this is not possible since we found a ';' after the '&'
            throw snap_logic_exception("xslt::filter_entities_out(): amp >= html.length() is just not possible here.");
        }

        // make sure that the first character represents a possible
        // entity otherwise we ignore it altogether
        QChar const c(html[amp]);
        if(c == '#')
        {
            // no need to convert numeric entities
            result += html.mid(pos, semi_colon + 1 - pos);
        }
        else if(semi_colon - amp < 100   // if the name is more than 100 characters, we probably got a problem...
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z'))
        {
            // keep whatever happened before the ampersand
            result += html.mid(pos, amp - 1 - pos);

            // retrieve the entity reference name
            QString const entity(html.mid(amp, semi_colon - amp));
            result += convert_entity(entity);
        }
        else
        {
            // in this case we replace the entity with a starting &amp;
            result += "&amp;";
            result += html.mid(pos, semi_colon - pos);
        }

        pos = semi_colon + 1;
    }
}


/** \brief Convert the named entity to a character or even a few characters.
 *
 * This function "manually" (so very quickly) converts the named entity
 * to a name.
 *
 * If the entity is not known, then we return "&amp;<name>;" so that way
 * we can continue to parse the data and make the problematic character
 * known.
 *
 * \note
 * The total number of entities is enormous and found on this page:
 * http://www.w3.org/TR/xml-entity-names/
 *
 * \todo
 * The idea, at some point, will be to write a parser of source entity
 * files and come up with a function that checks characters one by one
 * very quickly (similar to having a switch at all levels.)
 *
 * \param[in] entity_name  The name of the entity to be converted.
 *
 * \return The corresponding Unicode character.
 */
QString xslt::convert_entity(QString const & entity_name)
{
    if(entity_name.isEmpty())
    {
        return "";
    }

    switch(entity_name[0].unicode())
    {
    case 'A':
        if(entity_name == "Aacute")
        {
            return QString("%1").arg(QChar(193));
        }
        if(entity_name == "Acirc")
        {
            return QString("%1").arg(QChar(194));
        }
        if(entity_name == "AElig")
        {
            return QString("%1").arg(QChar(198));
        }
        if(entity_name == "Agrave")
        {
            return QString("%1").arg(QChar(192));
        }
        if(entity_name == "Aring")
        {
            return QString("%1").arg(QChar(197));
        }
        if(entity_name == "Atilde")
        {
            return QString("%1").arg(QChar(195));
        }
        if(entity_name == "Auml")
        {
            return QString("%1").arg(QChar(196));
        }
        break;

    case 'a':
        if(entity_name == "aacute")
        {
            return QString("%1").arg(QChar(225));
        }
        if(entity_name == "acute")
        {
            return QString("%1").arg(QChar(180));
        }
        if(entity_name == "acirc")
        {
            return QString("%1").arg(QChar(226));
        }
        if(entity_name == "aelig")
        {
            return QString("%1").arg(QChar(230));
        }
        if(entity_name == "agrave")
        {
            return QString("%1").arg(QChar(224));
        }
        if(entity_name == "amp")
        {
            return "&amp;";
        }
        if(entity_name == "aring")
        {
            return QString("%1").arg(QChar(229));
        }
        if(entity_name == "atilde")
        {
            return QString("%1").arg(QChar(227));
        }
        if(entity_name == "auml")
        {
            return QString("%1").arg(QChar(228));
        }
        break;

    case 'b':
        if(entity_name == "brvbar")
        {
            return QString("%1").arg(QChar(166));
        }
        break;

    case 'C':
        if(entity_name == "Ccedil")
        {
            return QString("%1").arg(QChar(199));
        }
        break;

    case 'c':
        if(entity_name == "ccedil")
        {
            return QString("%1").arg(QChar(231));
        }
        if(entity_name == "cedil")
        {
            return QString("%1").arg(QChar(184));
        }
        if(entity_name == "cent")
        {
            return QString("%1").arg(QChar(162));
        }
        if(entity_name == "copy")
        {
            return QString("%1").arg(QChar(169));
        }
        if(entity_name == "curren")
        {
            return QString("%1").arg(QChar(164));
        }
        break;

    case 'd':
        if(entity_name == "deg")
        {
            return QString("%1").arg(QChar(176));
        }
        if(entity_name == "divide")
        {
            return QString("%1").arg(QChar(247));
        }
        break;

    case 'E':
        if(entity_name == "Eacute")
        {
            return QString("%1").arg(QChar(201));
        }
        if(entity_name == "Ecirc")
        {
            return QString("%1").arg(QChar(202));
        }
        if(entity_name == "Egrave")
        {
            return QString("%1").arg(QChar(200));
        }
        if(entity_name == "ETH")
        {
            return QString("%1").arg(QChar(208));
        }
        if(entity_name == "Euml")
        {
            return QString("%1").arg(QChar(203));
        }
        break;

    case 'e':
        if(entity_name == "eacute")
        {
            return QString("%1").arg(QChar(233));
        }
        if(entity_name == "ecirc")
        {
            return QString("%1").arg(QChar(234));
        }
        if(entity_name == "egrave")
        {
            return QString("%1").arg(QChar(232));
        }
        if(entity_name == "eth")
        {
            return QString("%1").arg(QChar(240));
        }
        if(entity_name == "euml")
        {
            return QString("%1").arg(QChar(235));
        }
        break;

    case 'f':
        if(entity_name == "frac12")
        {
            return QString("%1").arg(QChar(189));
        }
        if(entity_name == "frac14")
        {
            return QString("%1").arg(QChar(188));
        }
        if(entity_name == "frac34")
        {
            return QString("%1").arg(QChar(190));
        }
        break;

    case 'g':
        if(entity_name == "gt")
        {
            return "&gt;";
        }
        break;

    case 'I':
        if(entity_name == "Iacute")
        {
            return QString("%1").arg(QChar(205));
        }
        if(entity_name == "Icirc")
        {
            return QString("%1").arg(QChar(206));
        }
        if(entity_name == "Igrave")
        {
            return QString("%1").arg(QChar(204));
        }
        if(entity_name == "Iuml")
        {
            return QString("%1").arg(QChar(207));
        }
        break;

    case 'i':
        if(entity_name == "iacute")
        {
            return QString("%1").arg(QChar(237));
        }
        if(entity_name == "icirc")
        {
            return QString("%1").arg(QChar(238));
        }
        if(entity_name == "iexcl")
        {
            return QString("%1").arg(QChar(161));
        }
        if(entity_name == "igrave")
        {
            return QString("%1").arg(QChar(236));
        }
        if(entity_name == "iquest")
        {
            return QString("%1").arg(QChar(191));
        }
        if(entity_name == "iuml")
        {
            return QString("%1").arg(QChar(239));
        }
        break;

    case 'l':
        if(entity_name == "laquo")
        {
            return QString("%1").arg(QChar(171));
        }
        if(entity_name == "lt")
        {
            return "&lt;";
        }
        break;

    case 'm':
        if(entity_name == "macr")
        {
            return QString("%1").arg(QChar(175));
        }
        if(entity_name == "micro")
        {
            return QString("%1").arg(QChar(181));
        }
        if(entity_name == "middot")
        {
            return QString("%1").arg(QChar(183));
        }
        break;

    case 'N':
        if(entity_name == "Ntilde")
        {
            return QString("%1").arg(QChar(209));
        }
        break;

    case 'n':
        if(entity_name == "nbsp")
        {
            return QString("%1").arg(QChar(160));
        }
        if(entity_name == "not")
        {
            return QString("%1").arg(QChar(172));
        }
        if(entity_name == "ntilde")
        {
            return QString("%1").arg(QChar(241));
        }
        break;

    case 'O':
        if(entity_name == "Oacute")
        {
            return QString("%1").arg(QChar(211));
        }
        if(entity_name == "Ocirc")
        {
            return QString("%1").arg(QChar(212));
        }
        if(entity_name == "Ograve")
        {
            return QString("%1").arg(QChar(210));
        }
        if(entity_name == "Oslash")
        {
            return QString("%1").arg(QChar(216));
        }
        if(entity_name == "Otilde")
        {
            return QString("%1").arg(QChar(213));
        }
        if(entity_name == "Ouml")
        {
            return QString("%1").arg(QChar(214));
        }
        break;

    case 'o':
        if(entity_name == "oacute")
        {
            return QString("%1").arg(QChar(243));
        }
        if(entity_name == "ocirc")
        {
            return QString("%1").arg(QChar(244));
        }
        if(entity_name == "ograve")
        {
            return QString("%1").arg(QChar(242));
        }
        if(entity_name == "ordf")
        {
            return QString("%1").arg(QChar(170));
        }
        if(entity_name == "ordm")
        {
            return QString("%1").arg(QChar(186));
        }
        if(entity_name == "oslash")
        {
            return QString("%1").arg(QChar(248));
        }
        if(entity_name == "otilde")
        {
            return QString("%1").arg(QChar(245));
        }
        if(entity_name == "ouml")
        {
            return QString("%1").arg(QChar(246));
        }
        break;

    case 'p':
        if(entity_name == "para")
        {
            return QString("%1").arg(QChar(182));
        }
        if(entity_name == "plusmn")
        {
            return QString("%1").arg(QChar(177));
        }
        if(entity_name == "pound")
        {
            return QString("%1").arg(QChar(163));
        }
        break;

    case 'r':
        if(entity_name == "raquo")
        {
            return QString("%1").arg(QChar(187));
        }
        if(entity_name == "reg")
        {
            return QString("%1").arg(QChar(174));
        }
        break;

    case 's':
        if(entity_name == "sect")
        {
            return QString("%1").arg(QChar(167));
        }
        if(entity_name == "shy")
        {
            return QString("%1").arg(QChar(173));
        }
        if(entity_name == "sup1")
        {
            return QString("%1").arg(QChar(185));
        }
        if(entity_name == "sup2")
        {
            return QString("%1").arg(QChar(178));
        }
        if(entity_name == "sup3")
        {
            return QString("%1").arg(QChar(179));
        }
        if(entity_name == "szlig")
        {
            return QString("%1").arg(QChar(223));
        }
        break;

    case 'T':
        if(entity_name == "THORN")
        {
            return QString("%1").arg(QChar(222));
        }
        break;

    case 't':
        if(entity_name == "thorn")
        {
            return QString("%1").arg(QChar(254));
        }
        if(entity_name == "times")
        {
            return QString("%1").arg(QChar(215));
        }
        break;

    case 'U':
        if(entity_name == "Uacute")
        {
            return QString("%1").arg(QChar(218));
        }
        if(entity_name == "Ucirc")
        {
            return QString("%1").arg(QChar(219));
        }
        if(entity_name == "Ugrave")
        {
            return QString("%1").arg(QChar(217));
        }
        if(entity_name == "Uuml")
        {
            return QString("%1").arg(QChar(220));
        }
        break;

    case 'u':
        if(entity_name == "uacute")
        {
            return QString("%1").arg(QChar(250));
        }
        if(entity_name == "ucirc")
        {
            return QString("%1").arg(QChar(251));
        }
        if(entity_name == "ugrave")
        {
            return QString("%1").arg(QChar(249));
        }
        if(entity_name == "uml")
        {
            return QString("%1").arg(QChar(168));
        }
        if(entity_name == "uuml")
        {
            return QString("%1").arg(QChar(252));
        }
        break;

    case 'Y':
        if(entity_name == "Yacute")
        {
            return QString("%1").arg(QChar(221));
        }
        break;

    case 'y':
        if(entity_name == "yacute")
        {
            return QString("%1").arg(QChar(253));
        }
        if(entity_name == "yen")
        {
            return QString("%1").arg(QChar(165));
        }
        if(entity_name == "yuml")
        {
            return QString("%1").arg(QChar(255));
        }
        break;

    }

    // if we reach here then it was not found...
    return QString("&amp;%1;").arg(entity_name);
}


} // namespace snap
// vim: ts=4 sw=4 et
