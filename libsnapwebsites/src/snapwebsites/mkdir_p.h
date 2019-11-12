// Snap Websites Server -- like shell `mkdir -p ...`
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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

#ifndef NOQT
// Qt lib
//
#include <QString>
#endif

namespace snap
{

#ifndef NOQT
int mkdir_p(QString const & path
          , bool include_filename = false
          , int mode = 0
          , QString const & owner = QString()
          , QString const & group = QString());
#endif

int mkdir_p(std::string const & path
          , bool include_filename = false
          , int mode = 0
          , std::string const & owner = std::string()
          , std::string const & group = std::string());

int mkdir_p(char const * path
          , bool include_filename = false
          , int mode = 0
          , char const * owner = nullptr
          , char const * group = nullptr);

} // snap namespace
// vim: ts=4 sw=4 et
