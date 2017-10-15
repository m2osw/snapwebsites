// Snap Websites Server -- generate the data for javascript-unicode.js
// Copyright (C) 2016-2017  Made to Order Software Corp.
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

//#include "not_reached.h"
//#include "not_used.h"

//#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include <QChar>
#include <QString>




/*
QChar::Mark_NonSpacing	        1	Unicode class name Mn
QChar::Mark_SpacingCombining	2	Unicode class name Mc
QChar::Mark_Enclosing	        3	Unicode class name Me
QChar::Number_DecimalDigit	    4	Unicode class name Nd
QChar::Number_Letter	        5	Unicode class name Nl
QChar::Number_Other	            6	Unicode class name No
QChar::Separator_Space	        7	Unicode class name Zs
QChar::Separator_Line	        8	Unicode class name Zl
QChar::Separator_Paragraph	    9	Unicode class name Zp
QChar::Other_Control	        10	Unicode class name Cc
QChar::Other_Format	            11	Unicode class name Cf
QChar::Other_Surrogate	        12	Unicode class name Cs
QChar::Other_PrivateUse	        13	Unicode class name Co
QChar::Other_NotAssigned	    14	Unicode class name Cn
QChar::Letter_Uppercase	        15	Unicode class name Lu
QChar::Letter_Lowercase	        16	Unicode class name Ll
QChar::Letter_Titlecase	        17	Unicode class name Lt
QChar::Letter_Modifier	        18	Unicode class name Lm
QChar::Letter_Other	            19	Unicode class name Lo
QChar::Punctuation_Connector	20	Unicode class name Pc
QChar::Punctuation_Dash	        21	Unicode class name Pd
QChar::Punctuation_Open	        22	Unicode class name Ps
QChar::Punctuation_Close	    23	Unicode class name Pe
QChar::Punctuation_InitialQuote	24	Unicode class name Pi
QChar::Punctuation_FinalQuote	25	Unicode class name Pf
QChar::Punctuation_Other	    26	Unicode class name Po
QChar::Symbol_Math	            27	Unicode class name Sm
QChar::Symbol_Currency	        28	Unicode class name Sc
QChar::Symbol_Modifier	        29	Unicode class name Sk
QChar::Symbol_Other	            30	Unicode class name So
*/
char const * const  g_category_names[] =
{
    // these need to match the "enum QChar::Category" declaration...
    "no-category",
    "Mn", "Mc", "Me", "Nd", "Nl", "No", "Zs", "Zl", "Zp", "Cc", "Cf", "Cs",
    "Co", "Cn", "Lu", "Ll", "Lt", "Lm", "Lo", "Pc", "Pd", "Ps", "Pe", "Pi",
    "Pf", "Po", "Sm", "Sc", "Sk", "So"
};


std::fstream        g_out;


class range_t
{
public:
    typedef std::vector<range_t>        vector_t;

            range_t(uint c)
                : f_start(c)
                , f_end(c)
            {
            }

    bool    extend_range(uint const c)
            {
                if(f_end + 1 == c)
                {
                    f_end = c;
                    return true;
                }

                return false;
            }

    bool    is_one() const
            {
                return f_start == f_end;
            }

    QString to_string(bool as_range) const
            {
                if(as_range)
                {
                    return QString("%1,%2").arg(f_start).arg(f_end);
                }
                else
                {
                    if(!is_one())
                    {
                        QString result(QString("%1").arg(f_start));
                        for(uint c(f_start + 1); c < f_end; ++c)
                        {
                            result = QString("%1,%2").arg(result).arg(c);
                        }
                        return result;
                    }
                    else
                    {
                        return QString("%1").arg(f_start);
                    }
                }
            }

    uint    size() const
            {
                return f_end - f_start + 1;
            }

private:
    uint    f_start;
    uint    f_end;
};


std::vector<range_t::vector_t>    g_arrays;


void add_char(uint const c)
{
    // get category of this UCS-4 character
    QChar::Category const category(QChar::category(c));
    if(category >= g_arrays.size())
    {
        throw std::runtime_error("category is larger than the number of array elements, see generate_arrays() and enlarge the number in the resize.");
    }
    range_t::vector_t & v(g_arrays[category]);

    if(!v.empty())
    {
        if(v.rbegin()->extend_range(c))
        {
            // it was extended, we are all good
            return;
        }
    }

    range_t const r(c);
    v.push_back(r);
}


void generate_arrays()
{
    // TODO: See whether we have a waht to define this 31 using a QChar
    //       definition (31 is the largest category + 1)
    //
    g_arrays.resize(31);

    for(uint c(0); c < 0x110000; ++c)
    {
        // skip the surrogates; they are just surrogates and we do not
        // need to get their category, really
        if(c >= 0xD800 && c <= 0xDFFF)
        {
            c = 0xDFFF;
            continue;
        }
        add_char(c);
    }
}


void output_header()
{
    // first we create and open the file
    //
    g_out.open("javascript-unicode.js", std::ios_base::out | std::ios_base::trunc);
    if(!g_out.is_open())
    {
        throw std::runtime_error("could not open output file \"javascript-unicode.js\" for writing");
    }

    g_out << "/** @preserve" << std::endl
          << " * Name: javascript-unicode" << std::endl
          << " * Version: 0.0.1" << std::endl
          << " * Browsers: all" << std::endl
          << " * Dependencies: output (>= 0.1.5)" << std::endl
          << " * Copyright: Copyright 2016 (c) Made to Order Software Corporation  All right reserved." << std::endl
          << " * License: GPL 2.0" << std::endl
          << " * Description: WARNING -- this code is generated by javascript_unicode.cpp" << std::endl
          << " *                         it is part of Snap! Websites (http://snapwebsites.org/)" << std::endl
          << " */" << std::endl;

}


QString array_to_js(size_t cat, uint level)
{
    QString ones;
    QString ranges;

    range_t::vector_t const & v(g_arrays[cat]);
    size_t const max_ranges(v.size());
    for(size_t idx(0); idx < max_ranges; ++idx)
    {
        if(v[idx].size() > level)
        {
            if(!ranges.isEmpty())
            {
                ranges += ",";
            }
            ranges += v[idx].to_string(true);
        }
        else
        {
            if(!ones.isEmpty())
            {
                ones += ",";
            }
            ones += v[idx].to_string(false);
        }
    }

    if(ranges.isEmpty())
    {
        if(ones.isEmpty())
        {
            // this does happen with "no-category" (i.e. all characters have
            // a valid category!)
            //throw std::runtime_error(QString("ranges and ones are both empty for category \"%1\"?!").arg(g_category_names[cat]).toUtf8().data());
            return QString();
        }
        return QString("o%1:[%2]").arg(g_category_names[cat]).arg(ones);
    }
    else
    {
        if(ones.isEmpty())
        {
            return QString("r%1:[%2]").arg(g_category_names[cat]).arg(ranges);
        }
        return QString("m%1:[[%2],[%3]]").arg(g_category_names[cat]).arg(ones).arg(ranges);
    }
}


void output_arrays()
{
    // each index of 'arrays' is a category
    //
    g_out << "snapwebsites.UnicodeCategories_={" << std::endl; // this is a 'static const' declaration
    for(size_t cat(0); cat < g_arrays.size(); ++cat)
    {
        QString range;
        for(uint level(0); level < 4; ++level)
        {
            QString n(array_to_js(cat, level));
            if(!n.isEmpty())
            {
                if(range.isEmpty()
                || n.length() < range.length())
                {
                    // keep the smallest
                    range = n;
                }
            }
        }
        if(!range.isEmpty())
        {
            g_out << range.toUtf8().data() << (cat + 1 == g_arrays.size() ? "" : ",") << std::endl;
        }
    }
    g_out << "};" << std::endl;
}


void output_footer()
{
    g_out << "snapwebsites.unicodeCategoryInRanges_=function(ucs4,ranges)"
            "{"
                "var i;"
                "if(ucs4>=ranges[0]&&ucs4<=ranges[ranges.length-1])"
                "{"
                    "for(i=0;i<ranges.length;i+=2)"
                    "{"
                        "if(ucs4>=ranges[i]&&ucs4<=ranges[i+1])"
                        "{"
                            "return true;"
                        "}"
                    "}"
                "}"
                "return false;"
            "};" << std::endl;
    g_out << "snapwebsites.unicodeCategory=function(ucs4)"
            "{"
                "var cat, t;"
                "for(cat in snapwebsites.UnicodeCategories_)"
                "{"
                    "if(snapwebsites.UnicodeCategories_.hasOwnProperty(cat))"
                    "{"
                        "switch(cat[0])"
                        "{"
                        "case 'm':"
                            "if(snapwebsites.UnicodeCategories_[cat][0].indexOf(ucs4)>=0)"
                            "{"
                                "return cat.substr(1);"
                            "}"
                            "if(snapwebsites.unicodeCategoryInRanges_(ucs4,snapwebsites.UnicodeCategories_[cat][1]))"
                            "{"
                                "return cat.substr(1);"
                            "}"
                            "break;"
                        "case 'r':"
                            "if(snapwebsites.unicodeCategoryInRanges_(ucs4,snapwebsites.UnicodeCategories_[cat]))"
                            "{"
                                "return cat.substr(1);"
                            "}"
                            "break;"
                        "case 'm':"
                            "if(snapwebsites.UnicodeCategories_[cat].indexOf(ucs4) != -1)"
                            "{"
                                "return cat.substr(1);"
                            "}"
                            "break;"
                        "}"
                    "}"
                "}"
                "return\"XX\";"
            "};" << std::endl;
    g_out << "snapwebsites.stringToUnicodeCodePoints=function(s)"
            "{"
                "var i=0,c,l,r=[];"
                "while(i<s.length)"
                "{"
                    "c=s.charCodeAt(i);"
                    "if(c>=55296&&c<=56319)" // hi surrogate (0xD800 <= c <= 0xDBFF)
                    "{"
                        "++i;"
                        "if(i<s.length)"
                        "{"
                            "l=s.charCodeAt(i);"
                            "if(l>=56320&&l<=57343)" // lo surrogate (0xDC00 <= l <= 0xDFFF)
                            "{"
                                "r.push(65536+(c&1023)*1024+(l&1023));"
                                "++i;"
                            "}"
                        "}"
                    "}"
                    "else if(c<55296||c>57343)" // NOT surrogate (c < 0xD800 || 0xDFFF < c)
                    "{"
                        "r.push(c);"
                        "++i;"
                    "}"
                "}"
                "return r;"
            "};" << std::endl;
    g_out << "snapwebsites.stringToUnicodeCategories=function(s)"
            "{"
                "var c=snapwebsites.stringToUnicodeCodePoints(s),r=[];"
                "for(p in c)"
                "{"
                    "if(c.hasOwnProperty(p))"
                    "{"
                        "r.push(snapwebsites.unicodeCategory(c[p]));"
                    "}"
                "}"
                "return r;"
            "};" << std::endl;

    //g_out << "// vim: ts=4 sw=4 et" << std::endl; -- probably not necessary...

    // done
    g_out.close();
}


int main(int argc, char * argv[])
{
    try
    {
        generate_arrays();
        output_header();
        output_arrays();
        output_footer();
    }
    catch(std::runtime_error const & e)
    {
        std::cerr << "error: an error occurred while generating the javascript-unicode.js file: " << e.what() << std::endl;
    }

    return 0;
}


// vim: ts=4 sw=4 et
