// Snap Manager -- snap database manager Decode UTF-8 dialog
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

#include "snap-manager-decode-utf8.h"

#include <snapwebsites/snapwebsites.h>

#include <stdio.h>

snap_manager_decode_utf8::snap_manager_decode_utf8(QWidget *snap_parent)
    : QDialog(snap_parent)
{
    setupUi(this);

    f_data = findChild<QTextEdit *>("data");
}

snap_manager_decode_utf8::~snap_manager_decode_utf8()
{
}

void snap_manager_decode_utf8::on_clear_clicked()
{
    f_data->setPlainText("");
}


int hex(char c)
{
    if(c >= '0' && c <= '9') {
        return c - '0';
    }
    if(c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if(c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

void snap_manager_decode_utf8::on_decode_clicked()
{
    QString str(f_data->toPlainText());
    QByteArray s(str.toUtf8());
    QByteArray output;
    for(const char *u = s.data(); u[0] != '\0' && u[1] != '\0'; u += 2) {
        int a = hex(u[0]);
        int b = hex(u[1]);
        if(a == -1 || b == -1) {
            // if there is an error, leave those characters alone
            // (it would be good to add color if possible)
            output += u[0];
            output += u[1];
        }
        else {
            // append the byte as is
            char c = static_cast<char>((a << 4) | b);
            if(c == 0) {
                // null terminator is saved as \0
                output += "\\0";
            }
            else if(c < 0x20 && c != '\n' && c != '\r' && c != '\t') {
                output += "^";
                output += '@' + c;
            }
            else {
                output += c;
            }
        }
    }
    f_data->setPlainText(QString::fromUtf8(output.data()));
}

char to_char(int c)
{
    if(c < 10) {
        return c + '0';
    }
    return c + 'a' - 10;
}

void snap_manager_decode_utf8::on_encode_clicked()
{
    QString str(f_data->toPlainText());
    QByteArray s(str.toUtf8());
    QByteArray output;
    for(const char *u = s.data(); u[0] != '\0'; ++u) {
        output += to_char((u[0] >> 4) & 0x0F);
        output += to_char(u[0] & 0x0F);
    }
    f_data->setPlainText(QString::fromUtf8(output.data()));
}

void snap_manager_decode_utf8::on_close_clicked()
{
    hide();
}


// vim: ts=4 sw=4 et
