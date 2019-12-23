// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapdatabase
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once


/** \file
 * \brief Very simple XML parser.
 *
 * The definitions of the Snap! Database tables is done in XML files.
 * These classes are used to load and parse those files to use them
 * as the schema definition of the tables.
 */

// C++ lib
//
#include    <deque>
#include    <map>
#include    <memory>
#include    <ostream>
#include    <vector>



namespace snapdatabase
{



class xml_node
    : public std::enable_shared_from_this<xml_node>
{
public:
    typedef std::shared_ptr<xml_node>
                                    pointer_t;
    typedef std::map<std::string, pointer_t>
                                    map_t;
    typedef std::weak_ptr<xml_node> weak_pointer_t;
    typedef std::map<std::string, std::string>  
                                    attribute_map_t;
    typedef std::vector<pointer_t>  vector_t;
    typedef std::deque<pointer_t>   deque_t;

                                    xml_node(std::string const & name);

    std::string const &             tag_name() const;
    std::string                     text() const;
    void                            append_text(std::string const & text);
    attribute_map_t                 all_attributes() const;
    std::string                     attribute(std::string const & name) const;
    void                            set_attribute(std::string const & name, std::string const & value);
    void                            append_child(pointer_t n);

    pointer_t                       parent() const;
    pointer_t                       first_child() const;
    pointer_t                       last_child() const;
    pointer_t                       next() const;
    pointer_t                       previous() const;

private:
    std::string const               f_name;
    std::string                     f_text = std::string();
    attribute_map_t                 f_attributes = attribute_map_t();

    pointer_t                       f_next = pointer_t();
    weak_pointer_t                  f_previous = weak_pointer_t();

    pointer_t                       f_child = pointer_t();
    weak_pointer_t                  f_parent = weak_pointer_t();
};

std::ostream & operator << (std::ostream & out, xml_node const & n);




class xml
{
public:
    typedef std::shared_ptr<xml>    pointer_t;
    typedef std::map<std::string, pointer_t>
                                    map_t;

                                    xml(std::string const & filename);

    xml_node::pointer_t             root();

private:
    xml_node::pointer_t             f_root = xml_node::pointer_t();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
