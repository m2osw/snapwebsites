/* catch_email.cpp
 * Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
 *
 * Project: https://snapwebsites.org/project/snapwebsites
 *
 * Permission is hereby granted, free of charge, to any
 * person obtaining a copy of this software and
 * associated documentation files (the "Software"), to
 * deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/** \file
 * \brief Verify the email class.
 *
 * This file implements tests to verify that the email
 * class functions as expected.
 *
 * \important
 * Note that this test only runs unit tests. It creates emails with
 * attachments and verifies that it works as expected, but it does
 * not call the send() function. For the send() function tests,
 * see the mb/test_email.cpp test instead.
 */

// self
//
#include "catch_tests.h"

// libsnapwebsites
//
#include "snapwebsites/email.h"


TEST_CASE( "email", "[email]" )
{
    // "normal" (canonicalized) URI
    GIVEN("basics")
    {
        time_t now(time(nullptr));
        snap::email e;

        SECTION("serialization version")
        {
            // if the serialization version changes then we probably will
            // need to change the test
            //
            REQUIRE(snap::email::EMAIL_MAJOR_VERSION == 1);
            REQUIRE(snap::email::EMAIL_MINOR_VERSION == 0);
        }

        SECTION("branding flag")
        {
            // default branding is ON
            //
            REQUIRE(e.get_branding());

            // default set keeps branding ON
            //
            e.set_branding();
            REQUIRE(e.get_branding());

            // explicit set branding to ON
            //
            e.set_branding(true);
            REQUIRE(e.get_branding());

            // explicit set branding to OFF
            //
            e.set_branding(false);
            REQUIRE_FALSE(e.get_branding());
        }

        SECTION("cumulative string")
        {
            // default is empty
            //
            REQUIRE(e.get_cumulative() == "");

            // change the value
            //
            e.set_cumulative("testing");
            REQUIRE(e.get_cumulative() == "testing");

            // reset the value
            //
            e.set_cumulative("");
            REQUIRE(e.get_cumulative() == "");
        }

        SECTION("site key")
        {
            // default is empty
            //
            REQUIRE(e.get_site_key() == "");

            // change the value
            //
            e.set_site_key("testing");
            REQUIRE(e.get_site_key() == "testing");

            // reset the value
            //
            e.set_site_key("");
            REQUIRE(e.get_site_key() == "");
        }

        SECTION("email path")
        {
            // default is empty
            //
            REQUIRE(e.get_email_path() == "");

            // change the value
            //
            e.set_email_path("/path/to/email/in/database");
            REQUIRE(e.get_email_path() == "/path/to/email/in/database");

            // reset the value
            //
            e.set_email_path("");
            REQUIRE(e.get_email_path() == "");
        }

        SECTION("email key")
        {
            // default is empty
            //
            REQUIRE(e.get_email_key() == "");

            // change the value
            //
            e.set_email_key("a-key-is-just-a-number-usually");
            REQUIRE(e.get_email_key() == "a-key-is-just-a-number-usually");

            // reset the value
            //
            e.set_email_key("");
            REQUIRE(e.get_email_key() == "");
        }

        SECTION("creation time")
        {
            // verify that the time is as expected around the time the
            // email object was created
            //
            // (we allow up to nearly 4 second which could happen on a
            // process switch or worst a memory swap at the wrong time.)
            //
            REQUIRE(e.get_time() - now <= 3);
        }

        SECTION("no headers by default")
        {
            // we have default headers, but they get set when we call
            // the send() function in case the user did not set them
            // yet by the time get called to send the email
            //
            REQUIRE(e.get_all_headers().size() == 0);
        }

        SECTION("set From: header")
        {
            // we have default headers, but they get set when we call
            // the send() function in case the user did not set them
            // yet by the time get called to send the email
            //
            e.set_from("valid@example.com");

            // test that we can access the name whatever the case
            //
            for(int i(0); i < 0b10000; ++i)
            {
                char from[5];
                from[0] = i & 0b0001 ? 'F' : 'f';
                from[1] = i & 0b0010 ? 'R' : 'r';
                from[2] = i & 0b0100 ? 'O' : 'o';
                from[3] = i & 0b1000 ? 'M' : 'm';
                from[4] = '\0';

                REQUIRE(e.has_header(from));

                REQUIRE(e.get_header(from) == "valid@example.com");
            }


            snap::email::header_map_t const & h(e.get_all_headers());
            REQUIRE(h.size() == 1);

            REQUIRE(h.begin()->first == "From");
            REQUIRE(h.begin()->second == "valid@example.com");

            REQUIRE(++h.begin() == h.end());

            // reset the list of headers
            //
            e.remove_header("From");
            REQUIRE(h.size() == 0);
            REQUIRE(h.begin() == h.end());

            // verify that the set+remove cleaned up the headers 100%
            //
            REQUIRE(e.get_all_headers().size() == 0);
        }

        SECTION("set To: header")
        {
            // we have default headers, but they get set when we call
            // the send() function in case the user did not set them
            // yet by the time get called to send the email
            //
            e.set_to("valid@example.com");

            // case does not matter
            //
            REQUIRE(e.has_header("To"));
            REQUIRE(e.has_header("to"));
            REQUIRE(e.has_header("tO"));
            REQUIRE(e.has_header("TO"));

            REQUIRE(e.get_header("To") == "valid@example.com");
            REQUIRE(e.get_header("to") == "valid@example.com");
            REQUIRE(e.get_header("tO") == "valid@example.com");
            REQUIRE(e.get_header("TO") == "valid@example.com");

            snap::email::header_map_t const & h(e.get_all_headers());
            REQUIRE(h.size() == 1);

            REQUIRE(h.begin()->first == "To");
            REQUIRE(h.begin()->second == "valid@example.com");

            REQUIRE(++h.begin() == h.end());

            // reset the list of headers
            //
            e.remove_header("To");
            REQUIRE(h.size() == 0);
            REQUIRE(h.begin() == h.end());

            // verify that the set+remove cleaned up the headers 100%
            //
            REQUIRE(e.get_all_headers().size() == 0);
        }

        SECTION("set priority")
        {
            // by default there is no priority defined
            //
            REQUIRE_FALSE(e.has_header("X-Priority"));

            // test setting the default priority
            //
            e.set_priority();
            REQUIRE(e.has_header("X-Priority"));
            REQUIRE(e.has_header("X-MSMail-Priority"));
            REQUIRE(e.has_header("Importance"));
            REQUIRE(e.has_header("Precedence"));
            REQUIRE(e.get_header("X-Priority") == "3 (Normal)");
            REQUIRE(e.get_header("X-MSMail-Priority") == "Normal");
            REQUIRE(e.get_header("Importance") == "Normal");
            REQUIRE(e.get_header("Precedence") == "Normal");

            // remove and make sure it is gone
            // (test that case has no effect)
            //
            e.remove_header("x-priority");
            REQUIRE_FALSE(e.has_header("X-PRIORITY"));
            e.remove_header("x-msmail-priority");
            REQUIRE_FALSE(e.has_header("X-MSMAIL-PRIORITY"));
            e.remove_header("importance");
            REQUIRE_FALSE(e.has_header("IMPORTANCE"));
            e.remove_header("precedence");
            REQUIRE_FALSE(e.has_header("PRECEDENCE"));

            // verify that the set+remove cleaned up the headers 100%
            //
            REQUIRE(e.get_all_headers().size() == 0);

            // explicitly setting the default priority
            //
            e.set_priority(snap::email::priority_t::EMAIL_PRIORITY_NORMAL);
            REQUIRE(e.has_header("X-Priority"));
            REQUIRE(e.has_header("X-MSMail-Priority"));
            REQUIRE(e.has_header("Importance"));
            REQUIRE(e.has_header("Precedence"));
            REQUIRE(e.get_header("X-Priority") == "3 (Normal)");
            REQUIRE(e.get_header("X-MSMail-Priority") == "Normal");
            REQUIRE(e.get_header("Importance") == "Normal");
            REQUIRE(e.get_header("Precedence") == "Normal");

            // remove and make sure it is gone
            // (test that case has no effect)
            //
            e.remove_header("X-PRIORITY");
            REQUIRE_FALSE(e.has_header("x-priority"));
            e.remove_header("X-MSMAIL-PRIORITY");
            REQUIRE_FALSE(e.has_header("x-msmail-priority"));
            e.remove_header("IMPORTANCE");
            REQUIRE_FALSE(e.has_header("importance"));
            e.remove_header("PRECEDENCE");
            REQUIRE_FALSE(e.has_header("precedence"));

            // verify that the set+remove cleaned up the headers 100%
            //
            REQUIRE(e.get_all_headers().size() == 0);

            // setting the BULK priority
            //
            e.set_priority(snap::email::priority_t::EMAIL_PRIORITY_BULK);
            REQUIRE(e.has_header("X-Priority"));
            REQUIRE(e.has_header("X-MSMail-Priority"));
            REQUIRE(e.has_header("Importance"));
            REQUIRE(e.has_header("Precedence"));
            REQUIRE(e.get_header("X-Priority") == "1 (Bulk)");
            REQUIRE(e.get_header("X-MSMail-Priority") == "Bulk");
            REQUIRE(e.get_header("Importance") == "Bulk");
            REQUIRE(e.get_header("Precedence") == "Bulk");

            // remove and make sure it is gone
            // (test that case has no effect)
            //
            e.remove_header("X-PrioritY");
            REQUIRE_FALSE(e.has_header("x-pRIORITy"));
            e.remove_header("X-MSMail-PrioritY");
            REQUIRE_FALSE(e.has_header("x-msmail-pRIORITy"));
            e.remove_header("ImportancE");
            REQUIRE_FALSE(e.has_header("iMPORTANCe"));
            e.remove_header("PrecedencE");
            REQUIRE_FALSE(e.has_header("pRECEDENCe"));

            // verify that the set+remove cleaned up the headers 100%
            //
            REQUIRE(e.get_all_headers().size() == 0);

            // setting the LOW priority
            //
            e.set_priority(snap::email::priority_t::EMAIL_PRIORITY_LOW);
            REQUIRE(e.has_header("X-Priority"));
            REQUIRE(e.has_header("X-MSMail-Priority"));
            REQUIRE(e.has_header("Importance"));
            REQUIRE(e.has_header("Precedence"));
            REQUIRE(e.get_header("X-Priority") == "2 (Low)");
            REQUIRE(e.get_header("X-MSMail-Priority") == "Low");
            REQUIRE(e.get_header("Importance") == "Low");
            REQUIRE(e.get_header("Precedence") == "Low");

            // remove and make sure it is gone
            // (test that case has no effect)
            //
            e.remove_header("X-PrIoRiTy");
            REQUIRE_FALSE(e.has_header("x-pRiOrItY"));
            e.remove_header("X-MsMaIl-PrIoRiTy");
            REQUIRE_FALSE(e.has_header("x-mSmAiL-pRiOrItY"));
            e.remove_header("ImPoRtAnCe");
            REQUIRE_FALSE(e.has_header("iMpOrTaNcE"));
            e.remove_header("PrEcEdEnCe");
            REQUIRE_FALSE(e.has_header("pReCeDeNcE"));

            // verify that the set+remove cleaned up the headers 100%
            //
            REQUIRE(e.get_all_headers().size() == 0);

            // setting the HIGH priority
            //
            e.set_priority(snap::email::priority_t::EMAIL_PRIORITY_HIGH);
            REQUIRE(e.has_header("X-Priority"));
            REQUIRE(e.has_header("X-MSMail-Priority"));
            REQUIRE(e.has_header("Importance"));
            REQUIRE(e.has_header("Precedence"));
            REQUIRE(e.get_header("X-Priority") == "4 (High)");
            REQUIRE(e.get_header("X-MSMail-Priority") == "High");
            REQUIRE(e.get_header("Importance") == "High");
            REQUIRE(e.get_header("Precedence") == "High");

            // remove and make sure it is gone
            // (test that case has no effect)
            //
            e.remove_header("X-PRIOrity");
            REQUIRE_FALSE(e.has_header("x-prioRITY"));
            e.remove_header("X-msmAIL-PRIOrity");
            REQUIRE_FALSE(e.has_header("x-MSmail-prioRITY"));
            e.remove_header("imporTANCE");
            REQUIRE_FALSE(e.has_header("imporTANCE"));
            e.remove_header("preceDENCE");
            REQUIRE_FALSE(e.has_header("preceDENCE"));

            // verify that the set+remove cleaned up the headers 100%
            //
            REQUIRE(e.get_all_headers().size() == 0);

            // setting the URGENT priority
            //
            e.set_priority(snap::email::priority_t::EMAIL_PRIORITY_URGENT);
            REQUIRE(e.has_header("X-Priority"));
            REQUIRE(e.has_header("X-MSMail-Priority"));
            REQUIRE(e.has_header("Importance"));
            REQUIRE(e.has_header("Precedence"));
            REQUIRE(e.get_header("X-Priority") == "5 (Urgent)");
            REQUIRE(e.get_header("X-MSMail-Priority") == "Urgent");
            REQUIRE(e.get_header("Importance") == "Urgent");
            REQUIRE(e.get_header("Precedence") == "Urgent");

            // remove and make sure it is gone
            // (test that case has no effect)
            //
            e.remove_header("X-PRioRIty");
            REQUIRE_FALSE(e.has_header("x-prIOriTY"));
            e.remove_header("x-MSmaiL-PriORitY");
            REQUIRE_FALSE(e.has_header("x-msMAIl-pRIorITy"));
            e.remove_header("imPOrtANce");
            REQUIRE_FALSE(e.has_header("IMpoRTanCE"));
            e.remove_header("prECedENce");
            REQUIRE_FALSE(e.has_header("PRecEDEncE"));

            // verify that the set+remove cleaned up the headers 100%
            //
            REQUIRE(e.get_all_headers().size() == 0);
        }

        SECTION("set Subject: header")
        {
            // test the set_subject() function
            //
            static char const * const subject
            {
                "This is my perfect subject matter!"
            };

            e.set_subject(subject);

            // case does not matter
            //
            REQUIRE(e.has_header("subject"));
            REQUIRE(e.has_header("SUBJECT"));
            REQUIRE(e.has_header("subJECT"));
            REQUIRE(e.has_header("SUBject"));

            REQUIRE(e.get_header("subject") == subject);
            REQUIRE(e.get_header("SUBJECT") == subject);
            REQUIRE(e.get_header("subJECT") == subject);
            REQUIRE(e.get_header("SUBject") == subject);

            snap::email::header_map_t const & h(e.get_all_headers());
            REQUIRE(h.size() == 1);

            REQUIRE(h.begin()->first == "Subject");
            REQUIRE(h.begin()->second == subject);

            REQUIRE(++h.begin() == h.end());

            // reset the list of headers
            //
            e.remove_header("Subject");
            REQUIRE(h.size() == 0);
            REQUIRE(h.begin() == h.end());

            // verify that the set+remove cleaned up the headers 100%
            //
            REQUIRE(e.get_all_headers().size() == 0);
        }

        SECTION("set other headers")
        {
            // test the add_header() function with a few headers
            //
            static char const * const headers[] =
            {
                "Date", "Jan 1, 2011 00:00:01",
                "Content-Type", "text/plain",
                "Content-Encoding", "utf-8",
            };

            for(size_t idx(0); idx < sizeof(headers) / sizeof(headers[0]); idx += 2)
            {
                // not yet defined
                //
                REQUIRE_FALSE(e.has_header(headers[idx]));

                // add header
                //
                e.add_header(headers[idx], headers[idx + 1]);

                // verify using the map
                //
                snap::email::header_map_t const & h(e.get_all_headers());
                REQUIRE(h.size() == 1);
                REQUIRE(h.begin()->first == headers[idx]);
                REQUIRE(h.begin()->second == headers[idx + 1]);
                REQUIRE(++h.begin() == h.end());

                // verify as is
                //
                REQUIRE(e.has_header(headers[idx]));
                REQUIRE(e.get_header(headers[idx]) == headers[idx + 1]);

                // reset the list of headers
                //
                e.remove_header(headers[idx]);
                REQUIRE(h.size() == 0);
                REQUIRE(h.begin() == h.end());
                REQUIRE_FALSE(e.has_header(headers[idx]));

                // verify that the set+remove cleaned up the headers 100%
                //
                REQUIRE(e.get_all_headers().size() == 0);
            }
        }
    }

    GIVEN("invalid calls")
    {
        snap::email e;

        // invalid email address
        //
        REQUIRE_THROWS_AS(e.set_from("with@an@invalid@email@address"), snap::snap_exception_invalid_parameter);

        // invalid email address (empty)
        //
        // although these work as expected (they fail with an exception)
        // they do not make use of the exception in case the size() < 1
        //
        REQUIRE_THROWS_AS(e.set_from("(this is a comment)"), snap::snap_exception_invalid_parameter);
        REQUIRE_THROWS_AS(e.set_from(""), snap::snap_exception_invalid_parameter);

        // invalid email address
        //
        REQUIRE_THROWS_AS(e.set_to("with@an@invalid@email@address"), snap::snap_exception_invalid_parameter);

        // invalid email address (empty)
        //
        // although these work as expected (they fail with an exception)
        // they do not make use of the exception in case the size() < 1
        //
        REQUIRE_THROWS_AS(e.set_to("(this is a comment)"), snap::snap_exception_invalid_parameter);
        REQUIRE_THROWS_AS(e.set_to(""), snap::snap_exception_invalid_parameter);

        // invalid field name (includes invalid characters
        //
        REQUIRE_THROWS_AS(e.set_priority(static_cast<snap::email::priority_t>(-10)), snap::snap_exception_invalid_parameter);

        // invalid field name (includes invalid characters
        //
        REQUIRE_THROWS_AS(e.add_header("Invalid Name", "ignored"), snap::snap_exception_invalid_parameter);

        // invalid field name (includes invalid characters
        //
        REQUIRE_THROWS_AS(e.add_header("From", "with@an@invalid@email@address"), snap::snap_exception_invalid_parameter);

        // only one email address for this field
        //
        REQUIRE_THROWS_AS(e.add_header("Sender", "valid@example.com, invalid@example.com"), snap::snap_exception_invalid_parameter);

        // empty name not valid
        //
        REQUIRE_THROWS_AS(e.has_header(QString()), snap::snap_exception_invalid_parameter);
        REQUIRE_THROWS_AS(e.get_header(QString()), snap::snap_exception_invalid_parameter);

        // no attachment, index will be out of bound
        //
        REQUIRE_THROWS_AS(e.get_attachment(1), std::out_of_range);

        // empty name not valid
        //
        REQUIRE_THROWS_AS(e.add_parameter(QString(), "ignored"), snap::snap_exception_invalid_parameter);
        REQUIRE_THROWS_AS(e.get_parameter(QString()), snap::snap_exception_invalid_parameter);
    }
}


TEST_CASE( "email_attachments", "[email]" )
{
    // "normal" (canonicalized) URI
    GIVEN("basics")
    {
        time_t now(time(nullptr));
        snap::email::attachment a;

        SECTION("data")
        {
            // default is empty
            //
            REQUIRE(a.get_data().size() == 0);

            // change the value
            //
            for(int count(0); count < 100; ++count)
            {
                QByteArray data;

                // define the size of the data block we'll use next
                //
                int const size(rand() % 1000);

                // put random data in the buffer
                //
                for(int i(0); i < size; ++i)
                {
                    data.append(rand());
                }

                a.set_data(data, "application/octet-stream");

                QByteArray const buf(a.get_data());

                REQUIRE(data.size() == buf.size());
                REQUIRE(data == buf);
            }
        }

        SECTION("no headers by default")
        {
            // we have default headers, but they get set when we call
            // the send() function in case the user did not set them
            // yet by the time get called to send the email
            //
            REQUIRE(a.get_all_headers().size() == 0);
        }

        SECTION("set Content-Disposition: header")
        {
            // Set the content disposition, this generates the correct
            // header so we don't have to guess how to correctly generate
            // this header each time we add an attachment
            //
            time_t const modification_date(time(nullptr));
            struct tm mod;
            gmtime_r(&modification_date, &mod);
            char mod_date[256];
            strftime(mod_date, sizeof(mod_date), "%d %b %Y %T +0000", &mod);
            QString const content_disposition_value(QString("attachment; filename=my-file.pdf; modification-date=\"%1\";").arg(mod_date));

            a.set_content_disposition("my-file.pdf", modification_date * 1000000, "attachment");

            // test that we can access the name whatever the case
            //
            for(int i(0); i < 0b10000; ++i)
            {
                char content_disposition[20];
                content_disposition[ 0] = i & 0b0000000000000000001 ? 'C' : 'c';
                content_disposition[ 1] = i & 0b0000000000000000010 ? 'O' : 'o';
                content_disposition[ 2] = i & 0b0000000000000000100 ? 'N' : 'n';
                content_disposition[ 3] = i & 0b0000000000000001000 ? 'T' : 't';
                content_disposition[ 4] = i & 0b0000000000000010000 ? 'E' : 'e';
                content_disposition[ 5] = i & 0b0000000000000100000 ? 'N' : 'n';
                content_disposition[ 6] = i & 0b0000000000001000000 ? 'T' : 't';
                content_disposition[ 7] = i & 0b0000000000010000000 ? '-' : '-';
                content_disposition[ 8] = i & 0b0000000000100000000 ? 'D' : 'd';
                content_disposition[ 9] = i & 0b0000000001000000000 ? 'I' : 'i';
                content_disposition[10] = i & 0b0000000010000000000 ? 'S' : 's';
                content_disposition[11] = i & 0b0000000100000000000 ? 'P' : 'p';
                content_disposition[12] = i & 0b0000001000000000000 ? 'O' : 'o';
                content_disposition[13] = i & 0b0000010000000000000 ? 'S' : 's';
                content_disposition[14] = i & 0b0000100000000000000 ? 'I' : 'i';
                content_disposition[15] = i & 0b0001000000000000000 ? 'T' : 't';
                content_disposition[16] = i & 0b0010000000000000000 ? 'I' : 'i';
                content_disposition[17] = i & 0b0100000000000000000 ? 'O' : 'o';
                content_disposition[18] = i & 0b1000000000000000000 ? 'N' : 'n';
                content_disposition[19] = '\0';

                REQUIRE(a.has_header(content_disposition));

                REQUIRE(a.get_header(content_disposition) == content_disposition_value);
            }

            // very it is officially empty
            //
            snap::email::header_map_t const & h(a.get_all_headers());
            REQUIRE(h.size() == 1);

            REQUIRE(h.begin()->first == "Content-Disposition");
            REQUIRE(h.begin()->second == content_disposition_value);

            REQUIRE(++h.begin() == h.end());

            // reset the list of headers
            //
            a.remove_header("Content-Disposition");
            REQUIRE(h.size() == 0);
            REQUIRE(h.begin() == h.end());

            // verify that the set+remove cleaned up the headers 100%
            //
            REQUIRE(a.get_all_headers().size() == 0);
        }

        SECTION("set other headers")
        {
            // test the add_header() function with a few headers
            //
            static char const * const headers[] =
            {
                "Date", "Jan 1, 2011 00:00:01",
                "Content-Type", "text/plain",
                "Content-Encoding", "utf-8",
            };

            for(size_t idx(0); idx < sizeof(headers) / sizeof(headers[0]); idx += 2)
            {
                // not yet defined
                //
                REQUIRE_FALSE(a.has_header(headers[idx]));

                // add header
                //
                a.add_header(headers[idx], headers[idx + 1]);

                // verify using the map
                //
                snap::email::header_map_t const & h(a.get_all_headers());
                REQUIRE(h.size() == 1);
                REQUIRE(h.begin()->first == headers[idx]);
                REQUIRE(h.begin()->second == headers[idx + 1]);
                REQUIRE(++h.begin() == h.end());

                // verify as is
                //
                REQUIRE(a.has_header(headers[idx]));
                REQUIRE(a.get_header(headers[idx]) == headers[idx + 1]);

                // reset the list of headers
                //
                a.remove_header(headers[idx]);
                REQUIRE(h.size() == 0);
                REQUIRE(h.begin() == h.end());
                REQUIRE_FALSE(a.has_header(headers[idx]));

                // verify that the set+remove cleaned up the headers 100%
                //
                REQUIRE(a.get_all_headers().size() == 0);
            }
        }
    }

    GIVEN("attachment")
    {
        SECTION("add attachments")
        {
            snap::email e;
            snap::email::attachment a;
            snap::email::attachment b;

            // still empty
            //
            REQUIRE(e.get_attachment_count() == 0);

            // add the first attachment
            //
            e.add_attachment(a);
            REQUIRE(e.get_attachment_count() == 1);

            // add the second attachment
            //
            e.add_attachment(b);
            REQUIRE(e.get_attachment_count() == 2);
        }
    }

    GIVEN("related")
    {
        SECTION("add related attachments")
        {
            snap::email e;
            snap::email::attachment a;
            snap::email::attachment r1;
            snap::email::attachment r2;

            // still empty
            //
            REQUIRE(e.get_attachment_count() == 0);

            // add a related attachment to the attachment
            // 
            a.add_related(r1);
            REQUIRE(a.get_related_count() == 1);
            a.add_related(r2);
            REQUIRE(a.get_related_count() == 2);

            // add the first attachment
            //
            e.add_attachment(a);
            REQUIRE(e.get_attachment_count() == 1);
        }
    }

    GIVEN("invalid related")
    {
        SECTION("attachment with related can't be a related itself (case 1)")
        {
            snap::email::attachment a;
            snap::email::attachment b;
            snap::email::attachment c;

            // this one is fine, adding c as a related attachment of b
            //
            b.add_related(c);

            // now we cannot add b as a related attachment to a
            //
            REQUIRE_THROWS_AS(a.add_related(b), snap::email_exception_too_many_levels);
        }

        SECTION("attachment with related can't be a related itself (case 2)")
        {
            snap::email::attachment a;
            snap::email::attachment b;
            snap::email::attachment c;

            // the other case is where we first add b to a then try
            // to add c to b which is then not possible anymore
            //
            a.add_related(b);

            // this one fails
            //
            // (note that we have to retrieve the copy of 'b' since
            // just adding 'c' to 'c' would work since 'b' was not
            // itself modified)
            //
            snap::email::attachment & d(a.get_related(0));
            REQUIRE_THROWS_AS(d.add_related(c), snap::email_exception_too_many_levels);

            // this one fails too!
            //
            REQUIRE_THROWS_AS(d.add_related(a), snap::email_exception_too_many_levels);
        }
    }

    GIVEN("invalid calls")
    {
        SECTION("verify exceptions")
        {
            snap::email::attachment a;

            // missing name for get_header()
            //
            REQUIRE_THROWS_AS(a.get_header(QString()), snap::snap_exception_invalid_parameter);

            // missing attachment_type for set_content_disposition()
            //
            REQUIRE_THROWS_AS(a.set_content_disposition("filename", -1, QString()), snap::snap_exception_invalid_parameter);

            // missing name for has_header()
            //
            REQUIRE_THROWS_AS(a.has_header(QString()), snap::snap_exception_invalid_parameter);

            // missing name for add_header()
            //
            REQUIRE_THROWS_AS(a.add_header(QString(), "ignored"), snap::snap_exception_invalid_parameter);

            // index out of range
            //
            REQUIRE_THROWS_AS(a.get_related(1), std::out_of_range);
        }
    }
}


TEST_CASE( "email_parameters", "[email]" )
{
    // "normal" (canonicalized) URI
    GIVEN("basics")
    {
        snap::email e;

        SECTION("no parameters by default")
        {
            // no default parameters
            //
            REQUIRE(e.get_all_parameters().size() == 0);
        }

        SECTION("set path parameter")
        {
            // set a path as a parameter
            //
            e.add_parameter("path", "/this/path/here");

            // make sure it worked
            //
            //REQUIRE(e.has_parameter("path")); -- not implemented (yet)
            REQUIRE(e.get_parameter("path") == "/this/path/here");

            // test that we can access the parameter only in lowercase
            // (start at 1 prevents the special case of all lowercase)
            //
            for(int i(1); i < 0b10000; ++i)
            {
                char path[5];
                path[0] = i & 0b0001 ? 'P' : 'p';
                path[1] = i & 0b0010 ? 'A' : 'a';
                path[2] = i & 0b0100 ? 'T' : 't';
                path[3] = i & 0b1000 ? 'H' : 'h';
                path[4] = '\0';

                REQUIRE_FALSE(e.has_header(path));
                REQUIRE_FALSE(e.get_header(path) == "/this/path/here");
            }


            // test with the map directly
            //
            snap::email::parameter_map_t const & p(e.get_all_parameters());
            REQUIRE(p.size() == 1);

            REQUIRE(p.begin()->first == "path");
            REQUIRE(p.begin()->second == "/this/path/here");

            REQUIRE(++p.begin() == p.end());

            REQUIRE(e.get_all_parameters().size() == 1);
        }
    }
}


TEST_CASE( "email_serialization", "[email]" )
{
    // "normal" (canonicalized) URI
    GIVEN("loop")
    {
        SECTION("simple")
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
                e.set_email_path((basic_on_off & 0b00001000) == 0 ? "<>" : "good-path");
                e.set_email_key ((basic_on_off & 0b00010000) == 0 ? "special-key" : "low-key");

                // headers
                //
                int const headers_on_off(rand());
                e.set_from    ((headers_on_off & 0b00000001) == 0 ? "alexis@example.com" : "doug@example.com");
                e.set_to      ((headers_on_off & 0b00000010) == 0 ? "henri@mail.example.com" : "charles@mail.example.com");
                e.set_priority(static_cast<snap::email::priority_t>(rand() % 5 + 1));
                e.set_subject ((headers_on_off & 0b00000100) == 0 ? "This subject is fun" : "Talk about this & that too <hidden>");
                e.add_header  ("Content-Type", (headers_on_off & 0b00001000) == 0 ? "text/plain" : "application/pdf");

                // attachments
                //
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
                            data.append(rand());
                        }
                        a.set_data(data, "application/octet-stream");
                    }

                    // basics
                    //
                    a.set_content_disposition(
                            (attachment_on_off & 0b00000001) == 0 ? "/tmp/file.txt" : "special.secret",
                            rand(),
                            (attachment_on_off & 0b00000010) == 0 ? "attachment" : "image");
                    a.add_header("Content-Type", (attachment_on_off & 0b00000100) == 0 ? "text/plain; charset=utf-8" : "audio/wave");

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
                                data.append(rand());
                            }
                            r.set_data(data, "application/octet-stream");
                        }

                        // basics
                        //
                        int const related_on_off(rand());
                        r.set_content_disposition(
                                (related_on_off & 0b00000001) == 0 ? "picture.gif" : "photo.jpeg",
                                rand(),
                                (related_on_off & 0b00000010) == 0 ? "image" : "picture");
                        a.add_header("Content-Type", (related_on_off & 0b00000100) == 0 ? "image/gif" : "image/jpeg");

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
                                data.append(rand());
                            }
                            r.set_data(data, "application/pdf");
                        }

                        // basics
                        //
                        int const related_on_off(rand());
                        r.set_content_disposition(
                                (related_on_off & 0b00000001) == 0 ? "/tmp/file.txt" : "special.secret",
                                rand(),
                                (related_on_off & 0b00000010) == 0 ? "attachment" : "image");
                        a.add_header("Content-Type", (related_on_off & 0b00000100) == 0 ? "text/plain; charset=utf-8" : "audio/wave");

                        // add the related
                        //
                        a.add_related(r);
                    }

                    // add the attachment
                    //
                    if(idx == body_attachment)
                    {
                        e.set_body_attachment(a);
                    }
                    else
                    {
                        e.add_attachment(a);
                    }
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

                // now we serialize and unserialize and make sure it worked
                //
                QString const xml(e.serialize());

                snap::email n;
                n.unserialize(xml);

                QString const xml_verify(n.serialize());
//std::cout << "XML 1: " << xml        << std::endl;
//std::cout << "XML 2: " << xml_verify << std::endl;

                REQUIRE(xml == xml_verify);

                REQUIRE(e == n);
            }
        }
    }
}


// vim: ts=4 sw=4 et
