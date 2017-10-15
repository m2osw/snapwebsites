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

#include "AboutDialog.h"
#include <QCoreApplication>
#include <QMessageBox>

AboutDialog::AboutDialog(QWidget *p)
    : QDialog(p)
{
    setupUi(this);

    textBrowser->setHtml(
        QCoreApplication::translate
        (
            "AboutDialog"
            , "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
             "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
             "p, li { white-space: pre-wrap; }\n"
             "</style></head><body style=\" font-family:'Ubuntu'; font-size:11pt; font-weight:400; font-style:normal;\">\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:22pt; font-weight:600;\">CassView</span></p>\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-style:italic;\">v"
             CASSVIEW_VERSION
             "</span></p>\n"
             "<p align=\"center\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p>\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:"
             "0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">To view your</span></p>\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Cassandra Cluster</p>\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Optimized for</p>\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Snap! Server<br /><br />Made to Order Software Corporation</span></p></body></html>"
            , 0
            //, QApplication::UnicodeUTF8
        )
    );
}

AboutDialog::~AboutDialog()
{
}

// vim: ts=4 sw=4 et
