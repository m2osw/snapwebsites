// Snap Websites Server -- verify that converting HTML entities works
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

#include <snapwebsites/not_reached.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/xslt.h>

#include <map>
#include <string>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <QString>



void usage()
{
    fprintf(stderr, "Usage: test_entity_filter [-opt] URL\n");
    fprintf(stderr, "  where -opt is one of (each flag must appear separately):\n");
    fprintf(stderr, "    -h                   Print out this help screen\n");
    exit(1);
}


int main(int argc, char * argv[])
{
    int errcnt(0);

    // parse user options
    bool help(false);
    for(int i(1); i < argc; ++i)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 'h':
                usage();
                snap::NOTREACHED();

            default:
                fprintf(stderr, "error: unknown option '%c'.\n", argv[i][1]);
                help = true;
                break;

            }
        }
    }
    if(help)
    {
        usage();
        snap::NOTREACHED();
    }

    {
        QString const t("<test>absolutely no entities</test>");
        QString const r(snap::xslt::filter_entities_out(t));
        if(r != t)
        {
            std::cerr << "String without entity modified." << std::endl;
            std::cerr << "  " << t << std::endl;
            std::cerr << "  " << r << std::endl;
            ++errcnt;
        }
    }

    {
        QString const t("<test>no entities but a lone & in the middle of there</test>");
        QString const r(snap::xslt::filter_entities_out(t));
        if(r != t)
        {
            std::cerr << "String with lone & character changed." << std::endl;
            std::cerr << "  " << t << std::endl;
            std::cerr << "  " << r << std::endl;
            ++errcnt;
        }
    }

    {
        QString const t("<test>the &amp; entity needs to stay as is</test>");
        QString const r(snap::xslt::filter_entities_out(t));
        if(r != t)
        {
            std::cerr << "&amp; needs to be left alone." << std::endl;
            std::cerr << "  " << t << std::endl;
            std::cerr << "  " << r << std::endl;
            ++errcnt;
        }
    }

    {
        QString const t("<test>the &lt; entity needs to stay as is</test>");
        QString const r(snap::xslt::filter_entities_out(t));
        if(r != t)
        {
            std::cerr << "&lt; needs to be left alone." << std::endl;
            std::cerr << "  " << t << std::endl;
            std::cerr << "  " << r << std::endl;
            ++errcnt;
        }
    }

    {
        QString const t("<test>the &gt; entity needs to stay as is</test>");
        QString const r(snap::xslt::filter_entities_out(t));
        if(r != t)
        {
            std::cerr << "&gt; needs to be left alone." << std::endl;
            std::cerr << "  " << t << std::endl;
            std::cerr << "  " << r << std::endl;
            ++errcnt;
        }
    }

    {
        QString const t("<test>the &nbsp; entity must be replaced with 0xA0 character</test>");
        QString const r(snap::xslt::filter_entities_out(t));
        QString e(t);
        e.replace("&nbsp;", "\xA0");
        if(r != e)
        {
            std::cerr << "&gt; needs to be left alone." << std::endl;
            std::cerr << "  " << t << std::endl;
            std::cerr << "  " << r << std::endl;
            std::cerr << "  " << e << std::endl;
            ++errcnt;
        }
    }

    {
        QString const t("all lat1 entities "
                "&nbsp;"
                "&iexcl;"
                "&cent;"
                "&pound;"
                "&curren;"
                "&yen;"
                "&brvbar;"
                "&sect;"
                "&uml;"
                "&copy;"
                "&ordf;"
                "&laquo;"
                "&not;"
                "&shy;"
                "&reg;"
                "&macr;"

                "&deg;"
                "&plusmn;"
                "&sup2;"
                "&sup3;"
                "&acute;"
                "&micro;"
                "&para;"
                "&middot;"
                "&cedil;"
                "&sup1;"
                "&ordm;"
                "&raquo;"
                "&frac14;"
                "&frac12;"
                "&frac34;"
                "&iquest;"

                "&Agrave;"
                "&Aacute;"
                "&Acirc;"
                "&Atilde;"
                "&Auml;"
                "&Aring;"
                "&AElig;"
                "&Ccedil;"
                "&Egrave;"
                "&Eacute;"
                "&Ecirc;"
                "&Euml;"
                "&Igrave;"
                "&Iacute;"
                "&Icirc;"
                "&Iuml;"

                "&ETH;"
                "&Ntilde;"
                "&Ograve;"
                "&Oacute;"
                "&Ocirc;"
                "&Otilde;"
                "&Ouml;"
                "&times;"
                "&Oslash;"
                "&Ugrave;"
                "&Uacute;"
                "&Ucirc;"
                "&Uuml;"
                "&Yacute;"
                "&THORN;"
                "&szlig;"

                "&agrave;"
                "&aacute;"
                "&acirc;"
                "&atilde;"
                "&auml;"
                "&aring;"
                "&aelig;"
                "&ccedil;"
                "&egrave;"
                "&eacute;"
                "&ecirc;"
                "&euml;"
                "&igrave;"
                "&iacute;"
                "&icirc;"
                "&iuml;"

                "&eth;"
                "&ntilde;"
                "&ograve;"
                "&oacute;"
                "&ocirc;"
                "&otilde;"
                "&ouml;"
                "&divide;"
                "&oslash;"
                "&ugrave;"
                "&uacute;"
                "&ucirc;"
                "&uuml;"
                "&yacute;"
                "&thorn;"
                "&yuml;"
            );
        QString const r(snap::xslt::filter_entities_out(t));
        QString e("all lat1 entities ");
        for(int c(0xA0); c < 256; ++c)
        {
            e += QChar(c);
        }
        if(r != e)
        {
            std::cerr << "&gt; needs to be left alone." << std::endl;
            std::cerr << "  " << t << std::endl;
            std::cerr << "  " << r << std::endl;
            std::cerr << "  " << e << std::endl;
            ++errcnt;
        }
    }

    return errcnt > 0 ? 1 : 0;
}

// vim: ts=4 sw=4 et
