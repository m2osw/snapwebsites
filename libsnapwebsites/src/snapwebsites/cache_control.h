// Snap Websites Servers -- parse and memorize cache control settings
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

#include <QString>

namespace snap
{


class cache_control_settings
{
public:
    // From spec: "HTTP/1.1 servers SHOULD NOT send Expires dates
    //            more than one year in the future."
    //
    static int64_t const            AGE_MAXIMUM = 365 * 24 * 60 * 60;
    static int64_t const            IGNORE_VALUE = -1;

                                    cache_control_settings();
                                    cache_control_settings(QString const & info, bool const internal_setup);

    // general handling
    void                            reset_cache_info();
    void                            set_cache_info(QString const & info, bool const internal_setup);

    // response only (server)
    void                            set_must_revalidate(bool const must_revalidate);
    bool                            get_must_revalidate() const;

    void                            set_private(bool const private_cache);
    bool                            get_private() const;

    void                            set_proxy_revalidate(bool const proxy_revalidate);
    bool                            get_proxy_revalidate() const;

    void                            set_public(bool const public_cache);
    bool                            get_public() const;

    // request and response (client and server)
    void                            set_max_age(int64_t const max_age);
    void                            set_max_age(QString const & max_age);
    void                            update_max_age(int64_t max_age);
    int64_t                         get_max_age() const;

    void                            set_no_cache(bool const no_cache);
    bool                            get_no_cache() const;

    void                            set_no_store(bool const no_store);
    bool                            get_no_store() const;

    void                            set_no_transform(bool const no_transform);
    bool                            get_no_transform() const;

    void                            set_s_maxage(int64_t const s_maxage);
    void                            set_s_maxage(QString const & s_maxage);
    void                            update_s_maxage(int64_t s_maxage);
    int64_t                         get_s_maxage() const;

    // request only (client)
    void                            set_max_stale(int64_t const max_stale);
    void                            set_max_stale(QString const & max_stale);
    int64_t                         get_max_stale() const;

    void                            set_min_fresh(int64_t const min_fresh);
    void                            set_min_fresh(QString const & min_fresh);
    int64_t                         get_min_fresh() const;

    void                            set_only_if_cached(bool const only_if_cached);
    bool                            get_only_if_cached() const;

    static int64_t                  string_to_seconds(QString const & max_age);
    static int64_t                  minimum(int64_t const a, int64_t const b);

private:
    // in alphabetical order
    int64_t                         f_max_age = 0;
    int64_t                         f_max_stale = -1;
    int64_t                         f_min_fresh = -1;
    bool                            f_must_revalidate = true;
    bool                            f_no_cache = false;
    bool                            f_no_store = false;
    bool                            f_no_transform = false;
    bool                            f_only_if_cached = false;
    bool                            f_private = false;
    bool                            f_proxy_revalidate = false;
    bool                            f_public = false;
    int64_t                         f_s_maxage = -1;
};


} // namespace snap
// vim: ts=4 sw=4 et
