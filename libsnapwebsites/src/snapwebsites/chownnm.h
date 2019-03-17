// Snap Websites Server -- like shell `chown <user>:<group> <path>`
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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

#include <QString>

namespace snap
{

int chownnm(std::string const & path
          , std::string const & user_name
          , std::string const & group_name = std::string());

int chownnm(QString const & path
          , QString const & user_name
          , QString const & group_name = QString());

int chownnm(char const * path
          , char const * user_name
          , char const * group_name = nullptr);

} // snap namespace
// vim: ts=4 sw=4 et
