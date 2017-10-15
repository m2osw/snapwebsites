// Snap Manager -- Cassandra manager for Snap! Servers
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
#pragma once

#include <QMessageBox>
#include <QString>
#include <QWidget>

// get a child that MUST exist
template<class T>
T *getChild(QWidget *parent, const char *name)
{
    T *w = parent->findChild<T *>(name);
    if(w == nullptr)
    {
            QString const error(QString("Can't find the widget: %1.").arg(name));
            QMessageBox msg(QMessageBox::Critical, "Internal Error", error, QMessageBox::Ok, parent);
            msg.exec();
            exit(1);
            /*NOTREACHED*/
    }

    return w;
}

// vim: ts=4 sw=4 et
