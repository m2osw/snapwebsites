// Snap Websites Server -- test against the snap_version classes
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

//
// This test verifies that names, versions, and browsers are properly
// extracted from filenames and dependencies and then that the resulting
// versioned_filename and dependency objects compare against each others
// as expected.
//

#include <snapwebsites/snap_version.h>
#include <snapwebsites/qstring_stream.h>

#include <QStringList>

#include <iostream>


bool g_verbose = false;


struct versions_t
{
    char const *                    f_extension;
    char const *                    f_left;
    char const *                    f_left_canonicalized;
    char const *                    f_right;
    char const *                    f_right_canonicalized;
    bool                            f_left_valid;
    bool                            f_right_valid;
    snap::snap_version::compare_t   f_compare;
};


versions_t g_versions[] =
{
    {
        ".js",
        "name_1.2.3.js",
        "name_1.2.3.js",
        "name_2.5.7.js",
        "name_2.5.7.js",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_SMALLER
    },
    {
        ".js",
        "addr_2.5.7.js",
        "addr_2.5.7.js",
        "name_1.2.3.js",
        "name_1.2.3.js",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_SMALLER
    },
    {
        "css",
        "name_1.2.0.css",
        "name_1.2.css",
        "name_1.2.3.css",
        "name_1.2.3.css",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_SMALLER
    },
    {
        "css",
        "name_1.2.css",
        "name_1.2.css",
        "name_1.2.3.css",
        "name_1.2.3.css",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_SMALLER
    },
    {
        ".js",
        "poo-34_1.2.3.js",
        "poo-34_1.2.3.js",
        "poo-34_1.2.3_ie.js",
        "poo-34_1.2.3_ie.js",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_SMALLER
    },
    {
        ".js",
        "addr_1.2.3_ie.js",
        "addr_1.2.3_ie.js",
        "name_1.2.3.js",
        "name_1.2.3.js",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_SMALLER
    },
    {
        ".js",
        "name_1.2.3_ie.js",
        "name_1.2.3_ie.js",
        "name_1.2.3_mozilla.js",
        "name_1.2.3_mozilla.js",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_SMALLER
    },
    {
        "js",
        "q/name_01.02.03_mozilla.js",
        "name_1.2.3_mozilla.js",
        "name_1.2.3_mozilla.js",
        "name_1.2.3_mozilla.js",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_EQUAL
    },
    {
        "js",
        "name_1.2.3_moz-lla.js",
        "name_1.2.3_moz-lla.js",
        "just/a/path/name_01.02.03_moz-lla.js",
        "name_1.2.3_moz-lla.js",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_EQUAL
    },
    {
        "lla",
        "name_1.02.3.99999_mozi.lla",
        "name_1.2.3.99999_mozi.lla",
        "name_000001.2.03.99998_mozi.lla",
        "name_1.2.3.99998_mozi.lla",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_LARGER
    },
    {
        "lla",
        "zoob_1.02.3.99998_mozi.lla",
        "zoob_1.2.3.99998_mozi.lla",
        "name_000001.2.03.99999_mozi.lla",
        "name_1.2.3.99999_mozi.lla",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_LARGER
    },
    {
        ".js",
        "removed/name_2.5.7_ie.js",
        "name_2.5.7_ie.js",
        "name_1.2.3_ie.js",
        "name_1.2.3_ie.js",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_LARGER
    },
    {
        "jpg",
        "name_2.5.7a_ie.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "a_2.5.7_ie.jpg",
        "",
        "ignored/name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "path/name_3.5_ie.jpg",
        "name_3.5_ie.jpg",
        "super/long/path/name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        true,
        true,
        snap::snap_version::compare_t::COMPARE_LARGER
    },
    {
        "jpg",
        "_2.5.7_ie.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_2.5.7_l.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_2.5.7_.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_2.5.7_LL.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_2.5.7_-p.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_2.5.7_p-.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq__ll.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_._ll.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_3._ll.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_.3_ll.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "q.q_4.3.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_3..jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_.3.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "6q_3.5.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "-q_3.5.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "q-_3.5.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "q--q_3.5.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_3.5:.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::snap_version::compare_t::COMPARE_INVALID
    }
};




int check_version(versions_t *v)
{
    int errcnt(0);

    snap::snap_version::versioned_filename l(v->f_extension);
    snap::snap_version::versioned_filename r(v->f_extension);

    // parse left
    if(l.set_filename(v->f_left) != v->f_left_valid)
    {
        ++errcnt;
        std::cerr << "error: unexpected left validity for " << v->f_left << " / " << v->f_right << " with " << l.get_error() << std::endl;
    }
    else
    {
        if(g_verbose)
        {
            std::cout << "filename " << v->f_left << " became: name [" << l.get_name()
                      << "], version [" << l.get_version_string() << "/" << l.get_version().size()
                      << "], browser [" << l.get_browser() << "]" << std::endl;
            if(!l.is_valid())
            {
                std::cout << "   error: " << l.get_error() << std::endl;
            }
        }
        bool b(static_cast<bool>(l));
        if(b != v->f_left_valid)
        {
            ++errcnt;
            std::cerr << "error: unexpected left bool operator for " << v->f_left << " / " << v->f_right << std::endl;
        }
        b = !l;
        if(b == v->f_left_valid)
        {
            ++errcnt;
            std::cerr << "error: unexpected left ! operator for " << v->f_left << " / " << v->f_right << std::endl;
        }
    }
    if(l.get_filename(true) != v->f_left_canonicalized)
    {
        ++errcnt;
        std::cerr << "error: left canonicalization \"" << l.get_filename(true) << "\" expected \"" << v->f_left_canonicalized << "\" for \"" << v->f_left << " / " << v->f_right << "\"" << std::endl;
    }
    else
    {
        QString name(v->f_left_canonicalized);
        int p(name.lastIndexOf('.'));
        name = name.left(p);
        if(l.get_filename() != name)
        {
            ++errcnt;
            std::cerr << "error: right canonicalization no extension " << l.get_filename() << " expected " << name << " for " << v->f_left << " / " << v->f_right << std::endl;
        }
    }

    // parse right
    if(r.set_filename(v->f_right) != v->f_right_valid)
    {
        ++errcnt;
        std::cerr << "error: unexpected right validity for " << v->f_left << " / " << v->f_right << " with " << r.get_error() << std::endl;
    }
    else
    {
        if(g_verbose)
        {
            std::cout << "filename " << v->f_right << " became: name [" << r.get_name()
                      << "], version [" << r.get_version_string() << "/" << r.get_version().size()
                      << "], browser [" << r.get_browser() << "]" << std::endl;
            if(!r.is_valid())
            {
                std::cout << "   error: " << r.get_error() << std::endl;
            }
        }
        bool b(static_cast<bool>(r));
        if(b != v->f_right_valid)
        {
            ++errcnt;
            std::cerr << "error: unexpected right bool operator for " << v->f_left << " / " << v->f_right << std::endl;
        }
        b = !r;
        if(b == v->f_right_valid)
        {
            ++errcnt;
            std::cerr << "error: unexpected right ! operator for " << v->f_left << " / " << v->f_right << std::endl;
        }
    }
    if(r.get_filename(true) != v->f_right_canonicalized)
    {
        ++errcnt;
        std::cerr << "error: right canonicalization \"" << r.get_filename(true) << "\" expected \"" << v->f_right_canonicalized << "\" for \"" << v->f_left << " / " << v->f_right << "\"" << std::endl;
    }
    else
    {
        QString name(v->f_right_canonicalized);
        int p(name.lastIndexOf('.'));
        name = name.left(p);
        if(r.get_filename() != name)
        {
            ++errcnt;
            std::cerr << "error: right canonicalization no extension " << r.get_filename() << " expected " << name << " for " << v->f_left << " / " << v->f_right << std::endl;
        }
    }

    snap::snap_version::compare_t c(l.compare(r));
    if(c != v->f_compare)
    {
        ++errcnt;
        std::cerr << "error: unexpected compare() result: " << static_cast<int>(c) << ", for " << v->f_left << " / " << v->f_right << std::endl;
    }
    else
    {
        if(g_verbose)
        {
            std::cout << "   compare " << static_cast<int>(c) << std::endl;
        }
        switch(c)
        {
        case snap::snap_version::compare_t::COMPARE_INVALID:
            if(l == r)
            {
                ++errcnt;
                std::cerr << "error: unexpected == result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(l != r)
            {
                ++errcnt;
                std::cerr << "error: unexpected != result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(l < r)
            {
                ++errcnt;
                std::cerr << "error: unexpected < result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(l <= r)
            {
                ++errcnt;
                std::cerr << "error: unexpected <= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(l > r)
            {
                ++errcnt;
                std::cerr << "error: unexpected > result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(l >= r)
            {
                ++errcnt;
                std::cerr << "error: unexpected >= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            break;

        case snap::snap_version::compare_t::COMPARE_SMALLER:
            if((l == r))
            {
                ++errcnt;
                std::cerr << "error: unexpected == result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l != r))
            {
                ++errcnt;
                std::cerr << "error: unexpected != result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l < r))
            {
                ++errcnt;
                std::cerr << "error: unexpected < result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l <= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected <= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l > r))
            {
                ++errcnt;
                std::cerr << "error: unexpected > result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l >= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected >= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            break;

        case snap::snap_version::compare_t::COMPARE_EQUAL:
            if(!(l == r))
            {
                ++errcnt;
                std::cerr << "error: unexpected == result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l != r))
            {
                ++errcnt;
                std::cerr << "error: unexpected != result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l < r))
            {
                ++errcnt;
                std::cerr << "error: unexpected < result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l <= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected <= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l > r))
            {
                ++errcnt;
                std::cerr << "error: unexpected > result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l >= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected >= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            break;

        case snap::snap_version::compare_t::COMPARE_LARGER:
            if((l == r))
            {
                ++errcnt;
                std::cerr << "error: unexpected == result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l != r))
            {
                ++errcnt;
                std::cerr << "error: unexpected != result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l < r))
            {
                ++errcnt;
                std::cerr << "error: unexpected < result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l <= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected <= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l > r))
            {
                ++errcnt;
                std::cerr << "error: unexpected > result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l >= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected >= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            break;

        }
    }

    return errcnt;
}



struct dependencies_t
{
    char const *        f_dependency_string;
    char const *        f_canonicalized;
    char const *        f_name;
    bool                f_valid;
    char const *        f_versions;
    char const *        f_browsers;
};


dependencies_t g_dependencies[] =
{
    {
        "jquery (1.0.0 <= 1.10.9999) [ie,mozilla]",
        "jquery (>= 1, <= 1.10.9999) [ie, mozilla]",
        "jquery",
        true,
        ">= 1,<= 1.10.9999",
        "ie,mozilla"
    },
    {
        "jquery-extensions (1.10.9999 >= 1.7.3) [chrome, ie, mozilla]",
        "jquery-extensions (>= 1.7.3, <= 1.10.9999) [chrome, ie, mozilla]",
        "jquery-extensions",
        true,
        ">= 1.7.3,<= 1.10.9999",
        "chrome,ie,mozilla"
    },
    {
        "jquery-ui (1.0.0<1.10.9999) [ , ie,, mozilla, , chrome,, ,]",
        "jquery-ui (> 1, < 1.10.9999) [ie, mozilla, chrome]",
        "jquery-ui",
        true,
        "> 1,< 1.10.9999",
        "ie,mozilla,chrome"
    },
    {
        "magic-merlin(>= 1.0.0, <> 1.10.9999)[ie,chrome,mozilla]",
        "magic-merlin (>= 1, != 1.10.9999) [ie, chrome, mozilla]",
        "magic-merlin",
        true,
        ">= 1,!= 1.10.9999",
        "ie,chrome,mozilla"
    },
    {
        "extra-commas(  ,  ,  >= 1.0.0,,,, <> 1.10.9999, , ,,)[ie,chrome,mozilla]",
        "extra-commas (>= 1, != 1.10.9999) [ie, chrome, mozilla]",
        "extra-commas",
        true,
        ">= 1,!= 1.10.9999",
        "ie,chrome,mozilla"
    },
    {
        "rooster (1.10.2)",
        "rooster (>= 1.10.2)",
        "rooster",
        true,
        ">= 1.10.2",
        NULL
    },
    {
        "zebra [ , ie,chrome, mozilla, , ,, ,]",
        "zebra [ie, chrome, mozilla]",
        "zebra",
        true,
        NULL,
        "ie,chrome,mozilla"
    },
    {
        "chimp",
        "chimp",
        "chimp",
        true,
        NULL,
        NULL
    },
    {
        "five-versions (= 1.3.2, == 2.2.7, = 6.5.5, == 7.2.01, = 3.4.1.15)",
        "five-versions (= 1.3.2, = 2.2.7, = 6.5.5, = 7.2.1, = 3.4.1.15)",
        "five-versions",
        true,
        "= 1.3.2,= 2.2.7,= 6.5.5,= 7.2.1,= 3.4.1.15",
        NULL
    },
    {
        "bad_name (1.2.3) [ie]",
        "",
        "",
        false,
        NULL,
        NULL
    },
    {
        "bad-version (1.2.3b) [ie]",
        "bad-version",
        "bad-version",
        false,
        NULL,
        NULL
    },
    {
        "version-bad-browser (1.2.3) [ie,45]",
        "version-bad-browser (>= 1.2.3) [ie]",
        "version-bad-browser",
        false,
        ">= 1.2.3",
        "ie"
    },
    {
        "bad-browser[ie,45]",
        "bad-browser [ie]",
        "bad-browser",
        false,
        NULL,
        "ie"
    },
    {
        "bad-browser[ie, 45]",
        "bad-browser [ie]",
        "bad-browser",
        false,
        NULL,
        "ie"
    },
    {
        "bad-location[ie, pq45](1.33.4 ,)",
        "bad-location [ie, pq45]",
        "bad-location",
        false,
        NULL,
        "ie,pq45"
    }
};

int check_dependency(dependencies_t *d)
{
    int errcnt(0);

    snap::snap_version::dependency dep;
    if(dep.set_dependency(d->f_dependency_string) != d->f_valid)
    {
        ++errcnt;
        std::cerr << "error: unexpected validity result for " << d->f_dependency_string << ", expected: " << d->f_valid << std::endl;
        if(!dep.is_valid())
        {
            std::cerr << "   dependency error is \"" << dep.get_error() << "\"" << std::endl;
        }
    }
    if(dep.get_name() != d->f_name)
    {
        ++errcnt;
        std::cerr << "error: unexpected name \"" << dep.get_name() << "\" from \"" << d->f_dependency_string << "\"" << std::endl;
    }

    {
        QStringList expected_versions;
        if(d->f_versions != NULL)
        {
            expected_versions = QString(d->f_versions).split(',');
        }
        snap::snap_version::version::vector_t versions(dep.get_versions());
        int version_max(versions.size());
        if(version_max != expected_versions.size())
        {
            ++errcnt;
            std::cerr << "error: unexpected number of versions, got " << version_max << " instead of " << expected_versions.size() << std::endl;
            // make sure we don't overflow one of the arrays
            version_max = std::min(version_max, expected_versions.size());
        }
        for(int i(0); i < version_max; ++i)
        {
            if(expected_versions[i] != versions[i].get_opversion_string())
            {
                ++errcnt;
                std::cerr << "error: unexpected versions name \"" << versions[i].get_opversion_string() << "\" instead of \"" << expected_versions[i] << "\"" << std::endl;
            }
        }
    }

    {
        QStringList expected_browsers;
        if(d->f_browsers != NULL)
        {
            expected_browsers = QString(d->f_browsers).split(',');
        }
        snap::snap_version::name::vector_t browsers(dep.get_browsers());
        int browser_max(browsers.size());
        if(browser_max != expected_browsers.size())
        {
            ++errcnt;
            std::cerr << "error: unexpected number of browsers, got " << browser_max << " instead of " << expected_browsers.size() << std::endl;
            // make sure we don't overflow one of the arrays
            browser_max = std::min(browser_max, expected_browsers.size());
        }
        for(int i(0); i < browser_max; ++i)
        {
            if(expected_browsers[i] != browsers[i].get_name())
            {
                ++errcnt;
                std::cerr << "error: unexpected browsers name \"" << browsers[i].get_name() << "\" instead of \"" << expected_browsers[i] << "\"" << std::endl;
            }
        }
    }

    if(dep.get_dependency_string() != d->f_canonicalized)
    {
        ++errcnt;
        std::cerr << "error: expected canonicalized version \"" << d->f_canonicalized << "\" instead of \"" << dep.get_dependency_string() << "\"" << std::endl;
    }

    return errcnt;
}



int main(int argc, char *argv[])
{
    int errcnt(0);

    // check command line options (just --verbose for now)
    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "--verbose") == 0
        || strcmp(argv[i], "-v") == 0)
        {
            g_verbose = true;
        }
    }

    // verify that an empty filename generates an exception
    try
    {
        snap::snap_version::versioned_filename l("");
        ++errcnt;
        std::cerr << "error: constructor accepted an empty extension." << std::endl;
    }
    catch(snap::snap_version::snap_version_exception_invalid_extension const& msg)
    {
        // got the exception as expected
    }

    // check a long stack of name / version / browser filenames
    for(size_t i(0); i < sizeof(g_versions) / sizeof(g_versions[0]); ++i)
    {
        if(g_verbose)
        {
            std::cerr << "----- Filename #" << i << " -----" << std::endl;
        }
        try
        {
            errcnt += check_version(g_versions + i);
        }
        catch(snap::snap_logic_exception const & e)
        {
            ++errcnt;
            std::cerr << "error: check_version() failed (" << e.what() << ")." << std::endl;
        }
    }

    // check a long stack of name / versions / browsers dependencies
    for(size_t i(0); i < sizeof(g_dependencies) / sizeof(g_dependencies[0]); ++i)
    {
        if(g_verbose)
        {
            std::cerr << "----- Dependency #" << i << " -----" << std::endl;
        }
        try
        {
            errcnt += check_dependency(g_dependencies + i);
        }
        catch(snap::snap_logic_exception const & e)
        {
            // got an unexpected exception
            ++errcnt;
            std::cerr << "error: check_dependency() failed (" << e.what() << ")." << std::endl;
        }
    }

    // display # of errors discovered (should always be zero)
    if(errcnt != 0)
    {
        std::cerr << std::endl << "*** " << errcnt << " error" << (errcnt == 1 ? "" : "s")
                  << " detected (out of " << sizeof(g_versions) / sizeof(g_versions[0])
                                           + sizeof(g_dependencies) / sizeof(g_dependencies[0])
                  << " tests)" << std::endl;
    }

    return errcnt > 0 ? 1 : 0;
}

// vim: ts=4 sw=4 et
