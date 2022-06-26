// Snap Websites Server -- snap websites serving children
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// self
//
#include    "snapwebsites/mail_exchanger.h"


// cppprocess
//
#include    <cppprocess/process.h>
#include    <cppprocess/io_capture_pipe.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/tokenize_string.h>


// libtld
//
#include    <libtld/tld.h>


// C
//
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{




mail_exchanger::mail_exchanger(int priority, std::string const & domain)
    : f_priority(priority)
    , f_domain(domain)
{
}


int mail_exchanger::get_priority() const
{
    return f_priority;
}


std::string mail_exchanger::get_domain() const
{
    return f_domain;
}


bool mail_exchanger::operator < (mail_exchanger const & rhs) const
{
    return f_priority < rhs.f_priority;
}







mail_exchangers::mail_exchangers(std::string const & domain)
{
    // use plain domain name to query the MX record
    // (i.e. a query with "mail.m2osw.com" fails!)
    //
    tld_object domain_obj(domain);
    if(!domain_obj.is_valid())
    {
        // f_domain_found is false by default... it failed
        SNAP_LOG_DEBUG
            << "mail_exchanger called with an invalid domain name: \""
            << domain
            << "\"."
            << SNAP_LOG_SEND;
        return;
    }

    // got the plain domain name now
    //
    std::string const full_domain(domain_obj.full_domain());

    // generate a command line to execute `dig`
    //
    cppprocess::process dig("dig");
    dig.set_command("/usr/bin/dig");
    dig.add_argument(full_domain);
    dig.add_argument("mx");  // get MX field
    cppprocess::io_capture_pipe::pointer_t out(std::make_shared<cppprocess::io_capture_pipe>());
    dig.set_output_io(out);
    int r(dig.start());
    if(r == 0)
    {
        r = dig.wait();
    }

    // retrieve the dig output
    //
    std::string const output(out->get_trimmed_output());

    if(r != 0)
    {
        // dig command failed
        SNAP_LOG_DEBUG
            << "dig.run() returned "
            << r
            << " and output: ["
            << output
            << "]"
            << SNAP_LOG_SEND;
        return;
    }

    // dig worked, check the results
    //
    std::vector<std::string> lines;
    int const count(snapdev::tokenize_string(lines, output, "\n", true, " "));
    if(count <= 0)
    {
        // no output?
        //
        SNAP_LOG_DEBUG
            << "dig returned no output ["
            << output
            << "]"
            << SNAP_LOG_SEND;
        return;
    }

    for(int l(0); l < count; ++l)
    {
        // if no MX are found, we generally get a line with the authority
        //
        if(strncmp(";; AUTHORITY SECTION:", lines[l].c_str(), 21) == 0)
        {
            ++l;
            if(l >= count)
            {
                break;
            }
            std::vector<std::string> fields;
            /*int const ln_count*/ (snapdev::tokenize_string(fields, lines[l], " \t", true, " ."));
            if(fields.empty()
            || fields[0] != full_domain)
            {
                SNAP_LOG_DEBUG
                    << "authority ("
                    << (fields.empty() ? "<empty>" : fields[0])
                    << ") does not match the domain we used ("
                    << full_domain
                    << ")"
                    << SNAP_LOG_SEND;
                return;
            }
            f_domain_found = true;
        }
        else if(strncmp(";; ANSWER SECTION:", lines[l].c_str(), 18) == 0)
        {
            mail_exchanger::mail_exchange_vector_t exchangers;
            for(++l; l < count; ++l)
            {
                if(lines[l].empty())
                {
                    break;
                }

                // TODO: instead of just taking the priority and MX
                //       priority and domain names, we probably should
                //       take all the fields available even if we are
                //       to not use them in some circumstances

                std::string::size_type pos(lines[l].find("MX"));
                if(pos != std::string::npos)
                {
                    // skip the MX (+2) and then skip spaces
                    //
                    char const * mx(lines[l].c_str() + pos + 2);

                    // skip spaces
                    //
                    for(; *mx != '\0' && std::isspace(*mx); ++mx);

                    // if valid, we now have a decimal number
                    //
                    if(*mx < '0' || *mx > '9')
                    {
                        SNAP_LOG_DEBUG
                            << "priority missing in \""
                            << lines[l]
                            << "\""
                            << SNAP_LOG_SEND;
                        return;
                    }

                    int priority(0);
                    for(; *mx >= '0' && *mx <= '9'; ++mx)
                    {
                        // some random maximum; valid MX priority is generally
                        // under 1,000 anyway
                        //
                        if(priority > 500000000)
                        {
                            SNAP_LOG_DEBUG
                                << "priority too large in \""
                                << lines[l]
                                << "\""
                                << SNAP_LOG_SEND;
                            return;
                        }
                        priority = priority * 10 + *mx - '0';
                    }

                    // skip spaces between priority and domain name
                    //
                    for(; *mx != '\0' && (std::isspace(*mx) || *mx == '.'); ++mx);

                    // now we have a domain name that ends with a period
                    //
                    if(*mx == '\0')
                    {
                        SNAP_LOG_DEBUG
                            << "invalid domain entry in \""
                            << lines[l]
                            << "\""
                            << SNAP_LOG_SEND;
                        return;
                    }
                    std::string mx_domain(mx);
                    if(mx_domain[mx_domain.length() - 1] == '.')
                    {
                        // remove the ending period if present
                        //
                        mx_domain.pop_back();
                    }

                    // it is considered valid, add it and move on
                    //
                    mail_exchanger const mx_item(priority, mx_domain);
                    exchangers.push_back(mx_item);
                }
            }

            // this also means the domain is considered valid
            // even if we do not find any authoritative section...
            // however, we expect at least one entry to be valid
            //
            f_domain_found = !exchangers.empty();

            f_mail_exchangers.swap(exchangers);
            break;
        }
    }
}


bool mail_exchangers::domain_found() const
{
    return f_domain_found;
}


size_t mail_exchangers::size() const
{
    return f_mail_exchangers.size();
}


mail_exchanger::mail_exchange_vector_t mail_exchangers::get_mail_exchangers() const
{
    return f_mail_exchangers;
}





} // namespace snap

// vim: ts=4 sw=4 et
