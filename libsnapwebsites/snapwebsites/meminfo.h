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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

// C++ lib
//
#include <cstdint>


namespace snap
{

// WARNING: we use uint64_t for all the members so that way we simplify
//          the internal parsing, some values are smaller and some may
//          even be boolean
//
struct meminfo_t
{
    bool            is_valid() const;

    uint64_t        f_mem_total = 0;
    uint64_t        f_mem_free = 0;
    uint64_t        f_mem_available = 0;
    uint64_t        f_buffers = 0;
    uint64_t        f_cached = 0;
    uint64_t        f_swap_cached = 0;
    uint64_t        f_active = 0;
    uint64_t        f_inactive = 0;
    uint64_t        f_active_anon = 0;
    uint64_t        f_inactive_anon = 0;
    uint64_t        f_active_file = 0;
    uint64_t        f_inactive_file = 0;
    uint64_t        f_unevictable = 0;
    uint64_t        f_mlocked = 0;
    uint64_t        f_swap_total = 0;
    uint64_t        f_swap_free = 0;
    uint64_t        f_dirty = 0;
    uint64_t        f_writeback = 0;
    uint64_t        f_anon_pages = 0;
    uint64_t        f_mapped = 0;
    uint64_t        f_shmem = 0;
    uint64_t        f_slab = 0;
    uint64_t        f_sreclaimable = 0;
    uint64_t        f_sunreclaim = 0;
    uint64_t        f_kernel_stack = 0;
    uint64_t        f_page_tables = 0;
    uint64_t        f_nfs_unstable = 0;
    uint64_t        f_bounce = 0;
    uint64_t        f_writeback_tmp = 0;
    uint64_t        f_commit_limit = 0;
    uint64_t        f_committed_as = 0;
    uint64_t        f_vmalloc_total = 0;
    uint64_t        f_vmalloc_used = 0;
    uint64_t        f_vmalloc_chunk = 0;
    uint64_t        f_hardware_corrupted = 0;
    uint64_t        f_anon_huge_pages = 0;
    uint64_t        f_cma_total = 0;
    uint64_t        f_cma_free = 0;
    uint64_t        f_huge_pages_total = 0;
    uint64_t        f_huge_pages_free = 0;
    uint64_t        f_huge_pages_rsvd = 0;
    uint64_t        f_huge_pages_surp = 0;
    uint64_t        f_huge_page_size = 0;
    uint64_t        f_direct_map4k = 0;
    uint64_t        f_direct_map2m = 0;
    uint64_t        f_direct_map1g = 0;
};

meminfo_t get_meminfo();


} // snap namespace
// vim: ts=4 sw=4 et
