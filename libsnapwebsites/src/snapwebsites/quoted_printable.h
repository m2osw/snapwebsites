// Snap Websites Servers -- quote a string so it is only ASCII (for emails)
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

#include    <string>

namespace quoted_printable
{
int const QUOTED_PRINTABLE_FLAG_BINARY         = 0x0001;
int const QUOTED_PRINTABLE_FLAG_EDBIC          = 0x0002;
int const QUOTED_PRINTABLE_FLAG_LFONLY         = 0x0004; // many sendmail(1) do not like \r\n somehow
int const QUOTED_PRINTABLE_FLAG_NO_LONE_PERIOD = 0x0008;

std::string encode(std::string const & text, int flags = 0);
std::string decode(std::string const & text);
}

// vim: ts=4 sw=4 et
