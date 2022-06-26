// Snap Websites Server -- read /proc/meminfo
// Copyright (c) 2018-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
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
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// self
//
#include "snapwebsites/meminfo.h"


// snaplogger lib
//
#include <snaplogger/message.h>


// snapdev lib
//
#include <snapdev/tokenize_string.h>


// C++ lib
//
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>


// last include
//
#include <snapdev/poison.h>




namespace snap
{


/** \brief Test whether this meminfo_t structure is considered valid.
 *
 * A valid meminfo_t structure is one that was returned by get_meminfo()
 * after reading the /proc/meminfo file without encountering any errors.
 *
 * An invalid meminfo_t structure is one that was just initialized.
 *
 * \return true if the structure is considered valid.
 */
bool meminfo_t::is_valid() const
{
    // we could test more but just that one should be enough
    return f_mem_total != 0;
}


meminfo_t get_meminfo()
{
    // IMPORTANT NOTE:
    // We keep this as a static inside the get_meminfo() because
    // it is a slow initialization that we can avoid 99.9% of the
    // time (i.e. only functions that need to call get_meminfo()
    // will ever initialize that map)
    //
    // note that all the values are uint64_t which is why we can
    // use just one single map
    //
    typedef uint64_t meminfo_t::* offset_t;
    static std::map<std::string const, offset_t const> const name_to_offset =
    {
        { "memtotal",           &meminfo_t::f_mem_total },
        { "memfree",            &meminfo_t::f_mem_free },
        { "memavailable",       &meminfo_t::f_mem_available },
        { "buffers",            &meminfo_t::f_buffers },
        { "cached",             &meminfo_t::f_cached },
        { "swapcached",         &meminfo_t::f_swap_cached },
        { "active",             &meminfo_t::f_active },
        { "inactive",           &meminfo_t::f_inactive },
        { "active(anon)",       &meminfo_t::f_active_anon },
        { "inactive(anon)",     &meminfo_t::f_inactive_anon },
        { "active(file)",       &meminfo_t::f_active_file },
        { "inactive(file)",     &meminfo_t::f_inactive_file },
        { "unevictable",        &meminfo_t::f_unevictable },
        { "mlocked",            &meminfo_t::f_mlocked },
        { "swaptotal",          &meminfo_t::f_swap_total },
        { "swapfree",           &meminfo_t::f_swap_free },
        { "dirty",              &meminfo_t::f_dirty },
        { "writeback",          &meminfo_t::f_writeback },
        { "anonpages",          &meminfo_t::f_anon_pages },
        { "mapped",             &meminfo_t::f_mapped },
        { "shmem",              &meminfo_t::f_shmem },
        { "slab",               &meminfo_t::f_slab },
        { "sreclaimable",       &meminfo_t::f_sreclaimable },
        { "sunreclaim",         &meminfo_t::f_sunreclaim },
        { "kernelstack",        &meminfo_t::f_kernel_stack },
        { "pagetables",         &meminfo_t::f_page_tables },
        { "nfs_unstable",       &meminfo_t::f_nfs_unstable },
        { "bounce",             &meminfo_t::f_bounce },
        { "writebacktmp",       &meminfo_t::f_writeback_tmp },
        { "commitlimit",        &meminfo_t::f_commit_limit },
        { "committed_as",       &meminfo_t::f_committed_as },
        { "vmalloctotal",       &meminfo_t::f_vmalloc_total },
        { "vmallocused",        &meminfo_t::f_vmalloc_used },
        { "vmallocchunk",       &meminfo_t::f_vmalloc_chunk },
        { "hardwarecorrupted",  &meminfo_t::f_hardware_corrupted },
        { "anonhugepages",      &meminfo_t::f_anon_huge_pages },
        { "cmatotal",           &meminfo_t::f_cma_total },
        { "cmafree",            &meminfo_t::f_cma_free },
        { "hugepages_total",    &meminfo_t::f_huge_pages_total },
        { "hugepages_free",     &meminfo_t::f_huge_pages_free },
        { "hugepages_rsvd",     &meminfo_t::f_huge_pages_rsvd },
        { "hugepages_surp",     &meminfo_t::f_huge_pages_surp },
        { "hugepagesize",       &meminfo_t::f_huge_page_size },
        { "directmap4k",        &meminfo_t::f_direct_map4k },
        { "directmap2m",        &meminfo_t::f_direct_map2m },
        { "directmap1g",        &meminfo_t::f_direct_map1g }
    };

    std::ifstream in;
    in.open("/proc/meminfo");
    if(!in.is_open())
    {
        return meminfo_t();
    }

    meminfo_t   info;
    for(;;)
    {
        char buf[256];
        in.getline(buf, sizeof(buf));
        if(!in)
        {
            if(in.eof())
            {
                break;
            }

            // other errors and we return an "invalid" structure
            //
            return meminfo_t();
        }
        std::string line(buf);
        std::vector<std::string> tokens;
        size_t const size(snapdev::tokenize_string(tokens, line, " \t", true, ":"));
        if(size >= 2)
        {
            // name / value [ / "kB"]
            //
            std::transform(tokens[0].begin(), tokens[0].end(), tokens[0].begin(), ::tolower);
            auto it(name_to_offset.find(tokens[0]));
            if(it != name_to_offset.end())
            {
                // it exists, save it in our structure
                //
                offset_t const o(it->second);
                info.*o = std::stoll(tokens[1]);
                if(size >= 3)
                {
                    if(tokens[2] == "kB")
                    {
                        // assuming that "kB" means 1,000 bytes
                        // transform to byte size so we don't have to guess
                        // what it could possibly be outside of here
                        //
                        info.*o *= 1000;
                    }
                }
            }
            else
            {
                SNAP_LOG_TRACE
                    << "unknown value in /proc/meminfo: \""
                    << line
                    << "\", meminfo.cpp() may need some updating."
                    << SNAP_LOG_SEND;
            }
        }
    }

    return info;
}


} // snap namespace
// vim: ts=4 sw=4 et
