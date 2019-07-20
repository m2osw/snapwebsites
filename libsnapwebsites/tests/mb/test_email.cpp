// Snap Websites Server -- test_email.cpp/h
// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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

//
// This test works along the test_ssl_server.cpp and implements the
// client's side. It connects to the server, sends a few messages (START,
// PAUSE x 4, STOP) and expects a HUP before quitting.
//
// To run the test, you first have to start the server otherwise the
// client won't be able to connect.
//

// snapwebsites lib
//
#include <snapwebsites/email.h>
#include <snapwebsites/file_content.h>
#include <snapwebsites/log.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snapwebsites.h>


// snapdev lib
//
#include <snapdev/hexadecimal_string.h>
#include <snapdev/not_reached.h>


// Qt Lib
//
#include <QFile>


// C++ lib
//
#include <functional>


// C lib
//
#include <unistd.h>


// last include
//
#include <snapdev/poison.h>




namespace
{

// path as defined on the compiler command line, so the tests here run
// as expected only as long as the source is available at the same place...
//
char const * sendmail_path = SENDMAIL_PATH;

}


// we setup an environment variable to control which sendmail tool
// gets used when we call send()
//
void init_test()
{

    char * p(getenv("PATH"));
    if(p == nullptr)
    {
        // I don't think this should ever happen.
        //
        std::cerr << "error:test_email: could not retrieve your $PATH environment variable." << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
    std::string path(sendmail_path);
    if(*p != '\0')
    {
        path += ":";
        path += p;
    }

    // we use setenv() to avoid inheritent (a.k.a. annoying)
    // problems with putenv()
    //
    setenv("PATH", path.c_str(), 1);

    // now our sendmail script should take over the sending of emails so
    // we can make sure it works as expected (add plain text, include this
    // or that header, etc.)
}


// our sendmail script saves its results in the /tmp/email.eml
// file so we now read the file line by line and verify that
// it matches what we expect to find in that text file.
//
class file_handler
    : public snap::file_content
{
public:
    file_handler()
        : file_content("/tmp/email.eml", false, true)
    {
        if(!read_all())
        {
            std::cerr << "error: could not open \"/tmp/email.eml\" after running our sendmail script." << std::endl;
            exit(1);
        }
    }

    bool eof() const
    {
        return f_pos >= f_content.length();
    }

    std::string read_line()
    {
        // we reached the end?
        //
        if(f_pos >= f_content.length())
        {
            return std::string();
        }
        std::string::size_type start(f_pos);

        // find the end of line
        //
        std::string::size_type end(f_content.find('\n', f_pos));
        if(end == std::string::npos)
        {
            // no \n at the end?
            //
            f_pos = f_content.length();

            return f_content.substr(start);
        }

        f_pos = end + 1; // +1 to skip the '\n'

        return f_content.substr(start, end - start);
    }

    std::string read_to(std::string const & boundary)
    {
        if(f_pos >= f_content.length())
        {
            return std::string();
        }
        std::string::size_type start(f_pos);

        std::string::size_type end(f_content.find(boundary, f_pos));
        if(end == std::string::npos)
        {
            f_pos = f_content.length();

            return f_content.substr(start);
        }

        f_pos = end + boundary.length();
        if(f_pos < f_content.length()
        && f_content[f_pos] == '\n')
        {
            ++f_pos;
        }

        return f_content.substr(start, end - start);
    }

    void match_line(std::string const & expected)
    {
        std::string const line(read_line());
        if(line != expected)
        {
            std::cerr << "error: unexpected first line in \""
                      << f_filename
                      << "\": found \""
                      << line
                      << "\" and expected \""
                      << expected
                      << "\"."
                      << std::endl;
            exit(1);
        }
    }

    bool main_header(snap::email const & e, QCaseInsensitiveString const & name, QString const & value)
    {
        if(name == "Content-Language")
        {
            if(value != "en-us")
            {
                std::cerr << "error: header named \"Content-Language\" from \""
                          << f_filename
                          << "\" does not have the expected value \""
                             "en-us"
                             "\" instead we found \""
                          << value
                          << "\"."
                          << std::endl;
                exit(1);
            }
            return true;
        }

        if(name == "Content-Type")
        {
            // the content-type gets changed because we have
            // attachments
            //
            if(value != "multipart/mixed;")
            {
                std::cerr << "error: header named \"Content-Type\" from \""
                          << f_filename
                          << "\" does not have the expected value \""
                             "multipart/mixed;"
                             "\" instead we found \""
                          << value
                          << "\"."
                          << std::endl;
                exit(1);
            }

            // the content type is followed by a boundary,
            // which we put on the next line
            //
            std::string const boundary(read_line());
            if(boundary.substr(0, 27) != "  boundary=\"=Snap.Websites=")
            {
                std::cerr << "error: header named \"Content-Type\" from \""
                          << f_filename
                          << "\" does not have the boundary starting with \""
                             "  boundary=\"=Snap.Websites="
                             "\" as expected, instead we found ["
                          << value
                          << "]."
                          << std::endl;
                exit(1);
            }
            if(*boundary.rbegin() != '\"')
            {
                std::cerr << "error: header named \"Content-Type\" from \""
                          << f_filename
                          << "\" does not end the boundary with a double quote as expected ["
                          << value
                          << "]."
                          << std::endl;
                exit(1);
            }
            f_boundary = boundary.substr(12, boundary.length() - 13);
            return true;
        }

        if(name == "Date")
        {
            // Date is a moving target, not too sure how to test
            // here right now.
            //if(value != "...")
            //{
            //    std::cerr << "error: header named \"Date\" from \""
            //              << f_filename
            //              << "\" does not have the expected value \""
            //                 "..."
            //                 "\" instead we found \""
            //              << value
            //              << "\"."
            //              << std::endl;
            //    exit(1);
            //}
            return true;
        }

        if(name == "MIME-Version")
        {
            if(value != "1.0")
            {
                std::cerr << "error: header named \"MIME-Version\" from \""
                          << f_filename
                          << "\" does not have the expected value \""
                             "1.0"
                             "\" instead we found \""
                          << value
                          << "\"."
                          << std::endl;
                exit(1);
            }
            return true;
        }

        if(e.get_branding())
        {
            if(name == "X-Generated-By")
            {
                if(value != "Snap! Websites C++ v" SNAPWEBSITES_VERSION_STRING " (https://snapwebsites.org/)")
                {
                    std::cerr << "error: header named \"X-Generated-By\" from \""
                              << f_filename
                              << "\" does not have the expected value \""
                                 "Snap! Websites C++ v" SNAPWEBSITES_VERSION_STRING " (https://snapwebsites.org/)"
                                 "\" instead we found \""
                              << value
                              << "\"."
                              << std::endl;
                    exit(1);
                }
                return true;
            }
            if(name == "X-Mailer")
            {
                if(value != "Snap! Websites C++ v" SNAPWEBSITES_VERSION_STRING " (https://snapwebsites.org/)")
                {
                    std::cerr << "error: header named \"X-Mailer\" from \""
                              << f_filename
                              << "\" does not have the expected value \""
                                 "Snap! Websites C++ v" SNAPWEBSITES_VERSION_STRING " (https://snapwebsites.org/)"
                                 "\" instead we found \""
                              << value
                              << "\"."
                              << std::endl;
                    exit(1);
                }
                return true;
            }
        }

        return false;
    }

    bool attachment_header(snap::email::attachment const & a, QCaseInsensitiveString const & name, QString const & value)
    {
        snap::NOTUSED(a);
        if(name == "Content-Type")
        {
            // the content-type gets changed because we have
            // attachments
            //
            QString type(a.get_header("Content-Type"));
            if(type != value)
            {
                QString const filename(a.get_header("Document-Filename"));
                type += "; name=";
                type += filename;
                if(value != type)
                {
                    std::cerr << "error: header named \"Content-Type\" from \""
                              << f_filename
                              << "\" does not have the expected value \""
                              << type
                              << "\" instead we found \""
                              << value
                              << "\"."
                              << std::endl;
                    exit(1);
                }
            }

            return true;
        }

        return false;
    }

    template<class T>
    bool match_header(snap::email::header_map_t const & h
                    , T t
                    , std::function<bool(T, QCaseInsensitiveString const & name, QString const & value)> f)
    {
        if(eof())
        {
            std::cerr << "error: end of email found before the end of the email headers in \""
                      << f_filename
                      << "\"."
                      << std::endl;
            exit(1);
        }

        std::string const line(read_line());
        if(line.empty())
        {
            // empty line, end of headers, return false
            //
            return false;
        }

        std::string::size_type colon(line.find(':'));
        if(colon == std::string::npos)
        {
            std::cerr << "error: header line \""
                      << line
                      << "\" from \""
                      << f_filename
                      << "\" does not include a colon as expected."
                      << std::endl;
            exit(1);
        }

        QCaseInsensitiveString const name(QString::fromUtf8(line.substr(0, colon).c_str()).trimmed());
        QString const value(QString::fromUtf8(line.substr(colon + 1).c_str()).trimmed());

        if(f(t, name, value))
        {
            return true;
        }

        auto const & it(h.find(name));
        if(it == h.end())
        {
            std::cerr << "error: header named \""
                      << name
                      << "\" from \""
                      << f_filename
                      << "\" is not defined in the email object."
                      << std::endl;
            exit(1);
        }

        if(it->second != value)
        {
            std::cerr << "error: header value for \""
                      << name
                      << "\" from \""
                      << f_filename
                      << "\" does not have the expected value \""
                      << it->second
                      << "\" but has \""
                      << value
                      << "\" instead."
                      << std::endl;
            exit(1);
        }

        return true;
    }

    void find_first_attachment()
    {
        std::string const prelude(read_to("--" + f_boundary));

        if(prelude !=
                "The following are various parts of a multipart email.\n"
                "It is likely to include a text version (first part) that you should\n"
                "be able to read as is.\n"
                "It may be followed by HTML and then various attachments.\n"
                "Please consider installing a MIME capable client to read this email.\n"
                "\n")
        {
            std::cerr << "error: prelude in \""
                      << f_filename
                      << "\" does not match as expected."
                      << std::endl;
            exit(1);
        }
    }

    bool read_attachment(snap::email::attachment & a, int idx)
    {
        // when reaching here the cursor position is already at
        // the beginning of the next attachment header
        //
        while(match_header<snap::email::attachment>(a.get_all_headers(), a, std::bind(&file_handler::attachment_header, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

        std::string const buffer(read_to("--" + f_boundary));
        QByteArray const data(buffer.c_str());
        std::string const da(snap::bin_to_hex(data.trimmed().data()));
        std::string const db(snap::bin_to_hex(a.get_data().trimmed().data()));
        if(da != db)
        {
            // WARNING: first I tested with:
            //          if(data.trimmed() != a.get_data().trimmed())
            //          and it FAILED, not too sure why the da/db
            //          trick works when the trimmed() compared
            //          direct fails...
            //
//std::cerr << "\nGot [\n" << data.trimmed().data() << "\n] expected [\n" << a.get_data().trimmed().data() << "\n]\n";
//if(da == db) std::cerr << "the hex strings are equal???\n";
            std::cerr << "error: data in attachment #"
                      << idx
                      << " does not match the data found in \""
                      << f_filename
                      << "\":" << std::endl
                      << snap::bin_to_hex(data.trimmed().data()) << std::endl
                      << "versus: " << std::endl
                      << snap::bin_to_hex(a.get_data().trimmed().data()) << std::endl
                      << std::endl;
            exit(1);
        }

        if(f_pos + 1 < f_content.length()
        && f_content[f_pos] == '-'
        && f_content[f_pos + 1] == '-')
        {
            // we found the last boundary
            //
            //f_pos += 2;
//std::cerr << "Got the final boundary this time!\n";
            return true;
        }

        return false;
    }

    void empty_lines()
    {
        while(f_pos < f_boundary.length())
        {
            std::string const empty(read_line());
            if(!empty.empty())
            {
                std::cerr << "error: data found after the attachments in \""
                          << f_filename
                          << "\"." << std::endl;
                exit(1);
            }
        }
    }

private:
    std::string::size_type      f_pos = 0;
    std::string                 f_boundary = std::string();
};




// run a simple test: create an in memory email then call send() to
// see whether our sendmail script receives what we expect
//
// this one is the simplest with the least amount of data that we can
// possibly pass to the email object
//
void simple_test()
{
    for(int cnt(0); cnt < 100; ++cnt)
    {
        snap::email e;

        // basics
        //
        int const basic_on_off(rand());
        e.set_branding  ((basic_on_off & 0b00000001) == 0);
        e.set_cumulative((basic_on_off & 0b00000010) == 0 ? "left" : "right");
        e.set_site_key  ((basic_on_off & 0b00000100) == 0 ? "here" : "there");
        e.set_email_path((basic_on_off & 0b00001000) == 0 ? "<>" : "good-path");        // ignored at the low level
        e.set_email_key ((basic_on_off & 0b00010000) == 0 ? "special-key" : "low-key"); // ignored at the low level

        // headers
        //
        int const headers_on_off(rand());
        e.set_from    ((headers_on_off & 0b00000001) == 0 ? "Alex <alexis@example.com>" : "\"R. Doug Barbieri\" <doug@example.com>");
        e.set_to      ((headers_on_off & 0b00000010) == 0 ? "\"Henri VIII\" <henri@mail.example.com>" : "\"Charles Senior\" <charles@mail.example.com>");
        e.set_priority(static_cast<snap::email::priority_t>(rand() % 5 + 1));
        e.set_subject ((headers_on_off & 0b00000100) == 0 ? "This subject is fun" : "Talk about this & that too <hidden>");
        e.add_header  ("Content-Type", (headers_on_off & 0b00001000) == 0 ? "text/plain" : "application/pdf");

        // attachments
        //
        int64_t date((time(nullptr) - 100000) * 1000000);
        int const count_attachments(rand() % 10 + 3);
        int const body_attachment(rand() % count_attachments);
        for(int idx(0); idx < count_attachments; ++idx)
        {
            snap::email::attachment a;

            int const attachment_on_off(rand());

            // data
            //
            {
                QByteArray data;
                int const size(rand() % 1000);
                for(int i(0); i < size; ++i)
                {
                    // at this point, avoid binary codes because the sendmail
                    // script does not handle many control characters the
                    // wrong way
                    //
                    data.append(rand() % 95 + ' ');
                }
                a.set_data(data, "application/octet-stream");
            }

            // basics
            //
            QString const filename((attachment_on_off & 0b00000001) == 0 ? "/tmp/file.txt" : "special.secret");
            a.set_content_disposition(
                    filename,
                    date - rand() * 1000,
                    (attachment_on_off & 0b00000010) == 0 ? "attachment" : "image");
            a.add_header("Content-Type", (attachment_on_off & 0b00000100) == 0 ? "text/plain; charset=utf-8" : "audio/wave");
            QString const basename((attachment_on_off & 0b00000001) == 0 ? "file.txt" : "special.secret");
            a.add_header("Document-Filename", basename);

            // eventually add a related attachment or two
            //
            if((attachment_on_off & 0b00001000) == 0) // related #1
            {
                snap::email::attachment r;

                // data
                //
                {
                    QByteArray data;
                    int const size(rand() % 1000);
                    for(int i(0); i < size; ++i)
                    {
                        data.append(rand() % 95 + ' ');
                    }
                    r.set_data(data, "application/octet-stream");
                }

                // basics
                //
                int const related_on_off(rand());
                QString const rfilename((related_on_off & 0b00000001) == 0 ? "picture.gif" : "photo.jpeg");
                r.set_content_disposition(
                        rfilename,
                        date - rand() * 1000,
                        (related_on_off & 0b00000010) == 0 ? "image" : "picture");
                r.add_header("Content-Type", (related_on_off & 0b00000100) == 0 ? "image/gif" : "image/jpeg");
                r.add_header("Document-Filename", rfilename);

                // add the related
                //
                a.add_related(r);
            }
            if((attachment_on_off & 0b00010000) == 0) // related #2
            {
                snap::email::attachment r;

                // data
                //
                {
                    QByteArray data;
                    int const size(rand() % 1000);
                    for(int i(0); i < size; ++i)
                    {
                        data.append(rand() % 95 + ' ');
                    }
                    r.set_data(data, "application/pdf");
                }

                // basics
                //
                int const related_on_off(rand());
                QString const rfilename((related_on_off & 0b00000001) == 0 ? "/tmp/email.eml" : "/dev/block.device");
                r.set_content_disposition(
                        rfilename,
                        date - rand() * 1000,
                        (related_on_off & 0b00000010) == 0 ? "attachment" : "image");
                r.add_header("Content-Type", (related_on_off & 0b00000100) == 0 ? "text/plain; charset=utf-8" : "audio/wave");
                QString const rbasename((related_on_off & 0b00000001) == 0 ? "email.eml" : "block.device");
                r.add_header("Document-Filename", rbasename);

                // add the related
                //
                a.add_related(r);
            }

            // add the attachment
            //
            if(idx == body_attachment)
            {
                // for the body, make it a valid text/plain email!
                //
                snap::email::attachment b;

                QString const body(
                        "This is the body of the email\n"
                        "It can be really long or really short\n"
                        "And we should test with HTML to see the conversion working\n"
                    );
                b.set_data(body.toUtf8(), "text/plain; charset=\"utf-8\"");
                //b.add_header("Content-Type", "text/plain; charset=\"utf-8\"");

                e.set_body_attachment(b);
            }

            e.add_attachment(a);
        }

        // parameters
        //
        int const count_parameters(rand() % 10 + 3);
        for(int idx(0); idx < count_parameters; ++idx)
        {
            QString name;
            int const name_size(rand() % 20 + 1);
            for(int i(0); i < name_size; ++i)
            {
                int c(0);
                do
                {
                    c = rand() & 255;
                }
                while(c == 0);
                name.append(c);
            }

            QString value;
            int const value_size(rand() % 1000);
            for(int i(0); i < value_size; ++i)
            {
                int c(0);
                do
                {
                    c = rand() & 255;
                }
                while(c == 0);
                value.append(c);
            }

            e.add_parameter(name, value);
        }

        // send that email now
        //
        e.send();


        file_handler eml;

        eml.match_line(std::string("From ") +
                        ((headers_on_off & 0b00000001) == 0
                                ? "alexis@example.com"
                                : "doug@example.com"));

        // now match the main headers until we find that empty line
        //
        while(eml.match_header<snap::email>(e.get_all_headers(), e, std::bind(&file_handler::main_header, &eml, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

        eml.find_first_attachment();

        for(int idx(0); ; ++idx)
        {
            // WARNING: here count_attachment does not include the body
            //          (we add the body as an extra attachment)
            //
            if(idx > count_attachments)
            {
                std::cerr << "error: not enough attachments were found in \""
                          << "/tmp/email.eml"
                          << "\"."
                          << std::endl;
                exit(1);
            }

            snap::email::attachment & a(e.get_attachment(idx));
            if(eml.read_attachment(a, idx))
            {
                if(idx != count_attachments)
                {
                    std::cerr << "error: get last attachment before the last boundary was found in \""
                              << "/tmp/email.eml"
                              << "\"."
                              << std::endl;
                    exit(1);
                }
                break;
            }
        }

        eml.empty_lines();
    }
}





int main(int argc, char * argv[])
{
    try
    {
        snap::logging::set_progname("test_ssl_server");
        snap::logging::configure_console();
        snap::logging::set_log_output_level(snap::logging::log_level_t::LOG_LEVEL_TRACE);

        unsigned int seed(static_cast<unsigned int>(time(nullptr)));
        if(argc >= 2)
        {
            if((strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
            {
                std::cout << "Usage: " << argv[0] << " [-opts]" << std::endl;
                std::cout << "Where -opts is one or more of:" << std::endl;
                std::cout << "  -h | --help    shows this help screen" << std::endl;
                return 0;
            }
            if((strcmp(argv[1], "-s") == 0 || strcmp(argv[1], "--seed") == 0))
            {
                if(argc < 3)
                {
                    std::cerr << "error: --seed must be followed by the seed.\n";
                    exit(1);
                }
                seed = std::atol(argv[2]);
            }
        }
        srand(seed);

        SNAP_LOG_INFO("start email class test with seed ")(seed);

        init_test();

        simple_test();

        SNAP_LOG_INFO("done email class test");

        return 0;
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_FATAL("Caught standard exception [")(e.what())("].");
    }

    return 1;
}

// vim: ts=4 sw=4 et
