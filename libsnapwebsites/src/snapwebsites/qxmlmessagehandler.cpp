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


// self
//
#include    "snapwebsites/qxmlmessagehandler.h"

#include    "snapwebsites/snap_exception.h"


// snapdev
//
#include    "snapdev/qstring_extensions.h"


// snaplogger
//
#include    <snaplogger/message.h>


// Qt
//
#include    <QDomDocument>
#include    <QFile>


// included last
//
#include    <snapdev/poison.h>




namespace snap
{


QMessageHandler::QMessageHandler(QObject *parent_object)
    : QAbstractMessageHandler(parent_object)
    //, f_had_msg(false) -- auto-init
{
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void QMessageHandler::handleMessage(QtMsgType type, QString const & description, QUrl const & identifier, QSourceLocation const & sourceLocation)
{
    QDomDocument doc("description");
    doc.setContent(description, true, nullptr, nullptr, nullptr);
    QDomElement root(doc.documentElement());
    // TODO: note that the description may include <span>, <b>, <i>, ...
    f_error_description = root.text();
    f_error_type = type;

    if(f_error_type == QtFatalMsg
    && f_error_description.startsWith("Entity")
    && f_error_description.endsWith("not declared."))
    {
        f_has_entities = true;
        return;
    }

//std::cerr << "URI A = [" << identifier.toString() << "]\n";
//std::cerr << "URI B = [" << sourceLocation.uri().toString() << "]\n";
//std::cerr << "MESSAGE = [" << f_error_description << "]\n";

    // avoid "variable unused" warnings
    if(type != QtWarningMsg
    || !f_error_description.startsWith("The variable")
    || !f_error_description.endsWith("is unused"))
    {
        // TODO: determine whether QtDebugMsg should not turn this flag
        //       on; although I'm not too sure how you get debug messages
        //       in the first place...
        f_had_msg = true;

        snaplogger::severity_t level(snaplogger::severity_t::SEVERITY_OFF);
        switch(type)
        {
        case QtDebugMsg:
            level = snaplogger::severity_t::SEVERITY_DEBUG;
            break;

        case QtWarningMsg:
            level = snaplogger::severity_t::SEVERITY_WARNING;
            break;

        case QtCriticalMsg:
            level = snaplogger::severity_t::SEVERITY_ERROR;
            break;

        //case QtFatalMsg:
        default:
            level = snaplogger::severity_t::SEVERITY_FATAL;
            break;

        }

        {
            snaplogger::message::pointer_t l(snaplogger::create_message(level, __FILE__, __func__, __LINE__));
            QString const location(sourceLocation.uri().toString());
            if(!location.isEmpty())
            {
                *l << location << ":";
            }
            if(sourceLocation.line() != 0)
            {
                *l << "line #" << sourceLocation.line() << ":";
            }
            if(sourceLocation.column() != 0)
            {
                *l << "column #" << sourceLocation.column() << ":";
            }
            *l << ' ' << f_error_description;
            if(!f_xsl.isEmpty())
            {
#ifdef DEBUG
                *l << " XSLT Script:\n[" << f_xsl << "]\n";
                static int count(0);
                QFile file_xsl(QString("/tmp/error%1-query.xsl").arg(count));
                file_xsl.open(QIODevice::WriteOnly);
                file_xsl.write(f_xsl.toUtf8());
                file_xsl.close();

                *l << " in memory XML document:\n[" << f_doc << "]\n";
                QFile file_xml(QString("/tmp/error%1-document.xml").arg(count));
                file_xml.open(QIODevice::WriteOnly);
                file_xml.write(f_doc.toUtf8());
                file_xml.close();
                ++count;

                // to actually know who called the QXmlQuery function
                snap_exception_base::output_stack_trace(100);
#else
                *l << " Beginning of the XSLT script involved:\n"
                   << f_xsl.left(200)
                   << "\nBeginning of the XML script involved:\n"
                   << f_doc.left(200);
#endif
            }
            ::snaplogger::send_message(*l);
        } // print log
    }
}
#pragma GCC diagnostic pop


} // namespace snap
// vim: ts=4 sw=4 et
