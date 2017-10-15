// Snap Manager -- snap database manager Help Window
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

#include "snap-manager-help.h"
#include <stdio.h>

snap_manager_help::snap_manager_help(QWidget *snap_parent)
    : QMainWindow(snap_parent)
{
    setupUi(this);
}

snap_manager_help::~snap_manager_help()
{
}



// vim: ts=4 sw=4 et
