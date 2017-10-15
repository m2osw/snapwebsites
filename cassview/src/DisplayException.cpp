//===============================================================================
// Copyright (c) 2011-2017 by Made to Order Software Corporation
// 
// http://snapwebsites.org/
// contact@m2osw.com
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
//===============================================================================

#include "DisplayException.h"

#include <iostream>

#include <QApplication>
#include <QMessageBox>

#include <snapwebsites/qstring_stream.h>

DisplayException::DisplayException( const QString& what, const QString& caption, const QString& message )
    : f_what(what)
    , f_caption(caption)
    , f_message(message)
{
    genMessage();
}


void DisplayException::genMessage()
{
    f_fullMessage = QObject::tr("Exception caught: [%1]\n%2").arg(f_what).arg(f_message);
}


void DisplayException::outputStdError()
{
    std::cerr << f_fullMessage << std::endl;
}


void DisplayException::showMessageBox()
{
    QMessageBox::critical( QApplication::activeWindow()
            , f_caption
            , f_fullMessage
            );
}


void DisplayException::displayError()
{
    outputStdError();
    showMessageBox();
}


// vim: ts=4 sw=4 et syntax=cpp.doxygen
