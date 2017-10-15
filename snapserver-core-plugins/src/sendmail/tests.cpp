// Snap Websites Server -- tests for sendmail
// Copyright (C) 2012-2017  Made to Order Software Corp.
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

#include "sendmail.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>

#include <iostream>

#include <snapwebsites/poison.h>

SNAP_PLUGIN_EXTENSION_START(sendmail)


SNAP_TEST_PLUGIN_SUITE(sendmail)
    SNAP_TEST_PLUGIN_TEST(sendmail, test_parse_email_basic)
    SNAP_TEST_PLUGIN_TEST(sendmail, test_parse_email_mixed)
    SNAP_TEST_PLUGIN_TEST(sendmail, test_parse_email_report)
SNAP_TEST_PLUGIN_SUITE_END()




SNAP_TEST_PLUGIN_TEST_IMPL(sendmail, test_parse_email_basic)
{
    email e;
    QString const basic_email(
"From test@snap.website  Tue May 19 17:00:01 2015\n"
"Return-Path: <test@snap.website>\n"
"Received: by mail.snap.website (Postfix, from userid 999)\n"
"	id AABBCCDDEE; Tue, 19 May 2015 17:00:01 -0700 (PDT)\n"
"From: root@snap.website (Snap Daemon)\n"
"To: help@snap.website\n"
"Subject: Basic email for parse_email() test\n"
"Content-Type: text/plain; charset=ANSI_X3.4-1968\n"
"Message-Id: <1122334455@snap.website>\n"
"Date: Tue, 19 May 2015 17:00:01 -0700 (PDT)\n"
"\n"
"This is the email body.\n"
"\n"
        );
    SNAP_TEST_PLUGIN_SUITE_ASSERT(parse_email(basic_email, e, false));

    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Return-Path") == "<test@snap.website>");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Received") == "by mail.snap.website (Postfix, from userid 999) id AABBCCDDEE; Tue, 19 May 2015 17:00:01 -0700 (PDT)");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("From") == "root@snap.website (Snap Daemon)");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("To") == "help@snap.website");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Subject") == "Basic email for parse_email() test");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Content-Type") == "text/plain; charset=ANSI_X3.4-1968");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Message-Id") == "<1122334455@snap.website>");
}


SNAP_TEST_PLUGIN_TEST_IMPL(sendmail, test_parse_email_mixed)
{
    email e;
    QString const mixed_email(
"From test@snap.website  Tue Nov 10 18:15:02 2015\n"
"Return-Path: <test-return@snap.website>\n"
"Received: by mail.snap.website (Postfix, from userid 123)\n"
"	id AABBCCDDEEF; Tue, 10 Nov 2015 18:15:02 -0800 (PST)\n"
"Content-Language: en-us\n"
"Content-Type: multipart/mixed;\n"
"  boundary=\"=Snap.Websites=AABBCCDDEEFF\"\n"
"Date: 11 Nov 2015 02:15:02 +0000\n"
"From: test-user@snap.website\n"
"Importance: High\n"
"List-Unsubscribe: http://snap.website/unsubscribe/01234567890123456789012345678901\n"
"Message-ID: <12345678901234567890123456789012@snapwebsites>\n"
"MIME-Version: 1.0\n"
"Precedence: High\n"
"Subject: Please verify your email address\n"
"To: help@snap.website\n"
"X-MSMail-Priority: High\n"
"X-Priority: 4 (High)\n"
"X-Generated-By: Snap! Websites C++ v0.1.71 (http://snapwebsites.org/)\n"
"X-Mailer: Snap! Websites C++ v0.1.71 (http://snapwebsites.org/)\n"
"\n"
"--=Snap.Websites=AABBCCDDEEFF\n"
"Content-Type: multipart/alternative;\n"
"  boundary=\"=Snap.Websites=AABBCCDDEEFF.msg\"\n"
"\n"
"--=Snap.Websites=AABBCCDDEEFF.msg\n"
"Content-Type: text/plain; charset=\"utf-8\"\n"
"Content-Transfer-Encoding: quoted-printable\n"
"Content-Description: Mail message body\n"
"\n"
"\n"
"\n"
"Please verify your email address\n"
"\n"
"Sent on 2015-11-11\n"
"Welcome to your Snap! C++ website.\n"
"In order to complete the creation of your new account, please click on\n"
"the following link:\n"
"\n"
"     Click_here_to_validate_your_email_address\n"
"\n"
"If you have a problem clicking on this link, your verification code\n"
"is:\n"
"\n"
"     aabbccddeeff0011\n"
"\n"
"Thank you.\n"
"To unsubscribe click here: Unsubscribe_from_Snap!_emails.\n"
"Copyright 2015 (c) by Made to Order Software Corporation -- All Rights\n"
"Reserved\n"
"\n"
"--=Snap.Websites=AABBCCDDEEFF.msg\n"
"Content-Transfer-Encoding: quoted-printable\n"
"Content-Type: text/html; charset=\"utf-8\"\n"
"\n"
"<!DOCTYPE html><html lang=3D\"en\" xml:lang=3D\"en\" prefix=3D\"og: http://ogp.m=\n"
"e/ns#\" class=3D\"sendmail snap sendmail standard visitor\"><head><meta http-e=\n"
"quiv=3D\"Content-Type\" content=3D\"text/html; charset=3Dutf-8\"/><title>Please=\n"
" verify your email address | ExDox</title><meta property=3D\"og:title\" conte=\n"
"nt=3D\"Please verify your email address\"/><meta property=3D\"og:site_name\" co=\n"
"ntent=3D\"ExDox\"/><meta property=3D\"og:type\" content=3D\"website\"/><link rel=\n"
"=3D\"bookmark\" type=3D\"text/html\" title=3D\"Generator\" href=3D\"http://snapweb=\n"
"sites.org/\"/><meta name=3D\"generator\" content=3D\"Snap! Websites\"/><link rel=\n"
"=3D\"canonical\" type=3D\"text/html\" title=3D\"Canonical URI\" href=3D\"http://cs=\n"
"nap.m2osw.com/\"/><meta property=3D\"og:url\" content=3D\"http://csnap.m2osw.co=\n"
"m/\"/><link rel=3D\"schema.dcterms\" type=3D\"text/uri-list\" href=3D\"http://pur=\n"
"l.org/dc/terms/\"/><meta name=3D\"date\" content=3D\"2015-11-04\"/><meta name=3D=\n"
"\"dcterms.date\" content=3D\"2015-11-04\"/><meta name=3D\"dcterms.created\" conte=\n"
"nt=3D\"2015-01-09\"/><link rel=3D\"shortcut icon\" type=3D\"image/x-icon\" href=\n"
"=3D\"http://csnap.m2osw.com/favicon.ico\"/><link rel=3D\"top\" type=3D\"text/htm=\n"
"l\" href=3D\"http://csnap.m2osw.com/\"/><link rel=3D\"up\" type=3D\"text/html\" hr=\n"
"ef=3D\"http://csnap.m2osw.com/admin/email/users\"/><link rel=3D\"search\" type=\n"
"=3D\"text/html\" title=3D\"Search\" href=3D\"http://csnap.m2osw.com/search\"/><me=\n"
"ta name=3D\"user_status\" content=3D\"visitor\"/><meta name=3D\"action\" content=\n"
"=3D\"\"/><meta name=3D\"path\" content=3D\"admin/email/users\"/><meta name=3D\"ful=\n"
"l_path\" content=3D\"\"/><style>\n"
"	body\n"
"					{\n"
"						font-family: sans-serif;\n"
"						background: white;\n"
"					}\n"
"\n"
"					body, div\n"
"					{\n"
"			padding: 0;\n"
"						margin: 0;\n"
"					}\n"
"\n"
"					h1\n"
"			{\n"
"						font-size: 150%;\n"
"					}\n"
"					h2\n"
"			{\n"
"						font-size: 130%;\n"
"					}\n"
"					h3\n"
"			{\n"
"						font-size: 115%;\n"
"					}\n"
"\n"
"					.page\n"
"			{\n"
"						padding: 10px;\n"
"					}\n"
"\n"
"					.header\n"
"		{\n"
"						height: 65px;\n"
"						border-bottom: 1px solid #666666;\n"
"				margin-bottom: 20px;\n"
"					}\n"
"\n"
"					.header h1\n"
"					{\n"
"			text-align: center;\n"
"						font-size: 250%;\n"
"						padding-top: 10px;\n"
"			}\n"
"\n"
"					.left\n"
"					{\n"
"						float: left;\n"
"				padding-right: 10px;\n"
"						width: 239px;\n"
"						min-height: 350px;\n"
"				border-right: 1px solid #666666;\n"
"					}\n"
"\n"
"					.content\n"
"					{\n"
"float: left;\n"
"						width: 730px;\n"
"						padding: 10px;\n"
"					}\n"
"\n"
"					.clear-both\n"
"					{\n"
"						clear: both;\n"
"			}\n"
"\n"
"					.inner-page\n"
"					{\n"
"					}\n"
"\n"
"					.content .body\n"
"					{\n"
"					}\n"
"\n"
"					.footer\n"
"				{\n"
"						margin-top: 20px;\n"
"						padding: 10px;\n"
"					border-top: 1px solid #666666;\n"
"						text-align: center;\n"
"						color: #888888;\n"
"		font-size: 80%;\n"
"					}\n"
"\n"
"					.error input\n"
"					{\n"
"			color: #ff0000;\n"
"					}\n"
"\n"
"					.left .box input.line-edit-input,\n"
"					.left .box input.password-input\n"
"					{\n"
"						display: block;\n"
"				width: 150px;\n"
"					}\n"
"\n"
"					.input-with-background-value\n"
"					{\n"
"		color: #888888;\n"
"					}\n"
"				</style></head><body><div class=3D\"page\"><div class=3D\"header\"><h1 styl=\n"
"e=3D\"font-size: 150%;\">Please verify your email address</h1></div><div><div=\n"
" id=3D\"content\"><div class=3D\"editor-content\"><p class=3D\"email_date\" style=\n"
"=3D\"text-align: right;\">Sent on <date>2015-11-11</date></p><p class=3D\"welc=\n"
"ome\">Welcome to your Snap! C++ website.</p><p class=3D\"process-to-complete\"=\n"
">In order to complete the creation of your new account, please click on the=\n"
" following link:</p><blockquote><a href=3D\"http://csnap.m2osw.com/verify/aa=\n"
"bbccddeeff0011\">Click here to validate your email address</a></blockquote><=\n"
"p>If you have a problem clicking on this link, your verification code is:</=\n"
"p><blockquote><code>aabbccddeeff0011</code></blockquote><p class=3D\"thank-y=\n"
"ou\">Thank you.</p><p>To unsubscribe click here: <a href=3D\"http://csnap.m2o=\n"
"sw.com/unsubscribe/01234567890123456789012345678901\">Unsubscribe from Snap!=\n"
" emails</a>.</p><p style=3D\"text-align: center;\">Copyright 2015 (c) by Made=\n"
" to Order Software Corporation -- All Rights Reserved</p></div></div></div>=\n"
"<div style=3D\"clear: both;\"></div></div></body></html>\n"
"--=Snap.Websites=AABBCCDDEEFF.msg--\n"
"\n"
"--=Snap.Websites=AABBCCDDEEFF--\n"
"\n"
        );
    SNAP_TEST_PLUGIN_SUITE_ASSERT(parse_email(mixed_email, e, false));

    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Return-Path") == "<test-return@snap.website>");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Received") == "by mail.snap.website (Postfix, from userid 123) id AABBCCDDEEF; Tue, 10 Nov 2015 18:15:02 -0800 (PST)");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Content-Language") == "en-us");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Content-Type") == "multipart/mixed; boundary=\"=Snap.Websites=AABBCCDDEEFF\"");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Date") == "11 Nov 2015 02:15:02 +0000");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("From") == "test-user@snap.website");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Importance") == "High");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("List-Unsubscribe") == "http://snap.website/unsubscribe/01234567890123456789012345678901");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Message-ID") == "<12345678901234567890123456789012@snapwebsites>");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("MIME-Version") == "1.0");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Precedence") == "High");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Subject") == "Please verify your email address");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("To") == "help@snap.website");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("X-MSMail-Priority") == "High");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("X-Priority") == "4 (High)");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("X-Generated-By") == "Snap! Websites C++ v0.1.71 (http://snapwebsites.org/)");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("X-Mailer") == "Snap! Websites C++ v0.1.71 (http://snapwebsites.org/)");

    // TODO: test that the HTML is converted back to pure HTML and not quoted data
}


SNAP_TEST_PLUGIN_TEST_IMPL(sendmail, test_parse_email_report)
{
    email e;
    QString const report_email(
"sender: double-bounce@snap.website\n"
"recipient: bounces@snap.website\n"
"\n"
"From double-bounce@halk.m2osw.com  Wed Nov 11 00:16:52 2015\n"
"Return-Path: <double-bounce@halk.m2osw.com>\n"
"Received: by halk.m2osw.com (Postfix)\n"
"    id 86C5D4C03B8; Wed, 11 Nov 2015 00:16:52 -0800 (PST)\n"
"Date: Wed, 11 Nov 2015 00:16:52 -0800 (PST)\n"
"From: MAILER-DAEMON@snap.website (Mail Delivery System)\n"
"Subject: Postmaster Copy: Undelivered Mail\n"
"To: bounces@snap.website\n"
"Auto-Submitted: auto-generated\n"
"MIME-Version: 1.0\n"
"Content-Type: multipart/report; report-type=delivery-status;\n"
"  boundary=\"E4CA14C03B6.1447229812/halk.m2osw.com\"\n"
"Message-Id: <20151111081652.86C5D4C03B8@halk.m2osw.com>\n"
"\n"
"This is a MIME-encapsulated message.\n"
"\n"
"--E4CA14C03B6.1447229812/halk.m2osw.com\n"
"Content-Description: Notification\n"
"Content-Type: text/plain; charset=us-ascii\n"
"\n"
"\n"
"<invalid@m2osw.com>: host mail.m2osw.com[69.55.231.156] said: 554 5.7.1\n"
"    <invalid@m2osw.com>: Recipient address rejected: Access denied (in reply to\n"
"    RCPT TO command)\n"
"\n"
"--E4CA14C03B6.1447229812/halk.m2osw.com\n"
"Content-Description: Delivery report\n"
"Content-Type: message/delivery-status\n"
"\n"
"Reporting-MTA: dns; snap.website\n"
"X-Postfix-Queue-ID: AABBCCDDEEF\n"
"X-Postfix-Sender: rfc822; alexis@snap.website\n"
"Arrival-Date: Wed, 11 Nov 2015 00:16:39 -0800 (PST)\n"
"\n"
"Final-Recipient: rfc822; invalid@snap.website\n"
"Action: failed\n"
"Status: 5.7.1\n"
"Remote-MTA: dns; snap.website\n"
"Diagnostic-Code: smtp; 554 5.7.1 <invalid@m2osw.com>: Recipient address\n"
"    rejected: Access denied\n"
"\n"
"--E4CA14C03B6.1447229812/halk.m2osw.com\n"
"Content-Description: Undelivered Message Headers\n"
"Content-Type: text/rfc822-headers\n"
"\n"
"Return-Path: <help@snap.website>\n"
"Received: by snap.website (Postfix, from userid 1000)\n"
"       id AABBCCDDEEF; Wed, 11 Nov 2015 00:16:39 -0800 (PST)\n"
"Content-Language: en-us\n"
"Content-Type: multipart/mixed;\n"
"  boundary=\"=Snap.Websites=00112233445566778899\"\n"
"Date: 11 Nov 2015 08:16:39 +0000\n"
"From: snap@snap.website\n"
"Importance: High\n"
"List-Unsubscribe: http://csnap.m2osw.com/unsubscribe/00112233445566778899aabbccddeeff\n"
"Message-ID: <1705d651600627abf20cbc663567dbf9@snapwebsites>\n"
"MIME-Version: 1.0\n"
"Precedence: High\n"
"Subject: Please verify your email address\n"
"To: invalid@snap.website\n"
"X-MSMail-Priority: High\n"
"X-Priority: 4 (High)\n"
"X-Generated-By: Snap! Websites C++ v0.1.71 (http://snapwebsites.org/)\n"
"X-Mailer: Snap! Websites C++ v0.1.71 (http://snapwebsites.org/)\n"
"\n"
"--E4CA14C03B6.1447229812/halk.m2osw.com--\n"
"\n"
        );
    SNAP_TEST_PLUGIN_SUITE_ASSERT(parse_email(report_email, e, true));

    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_parameter("sender") == "double-bounce@snap.website");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_parameter("recipient") == "bounces@snap.website");

    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Return-Path") == "<double-bounce@halk.m2osw.com>");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Received") == "by halk.m2osw.com (Postfix) id 86C5D4C03B8; Wed, 11 Nov 2015 00:16:52 -0800 (PST)");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Date") == "Wed, 11 Nov 2015 00:16:52 -0800 (PST)");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("From") == "MAILER-DAEMON@snap.website (Mail Delivery System)");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Subject") == "Postmaster Copy: Undelivered Mail");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("To") == "bounces@snap.website");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Auto-Submitted") == "auto-generated");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("MIME-Version") == "1.0");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Content-Type") == "multipart/report; report-type=delivery-status; boundary=\"E4CA14C03B6.1447229812/halk.m2osw.com\"");
    SNAP_TEST_PLUGIN_SUITE_ASSERT(e.get_header("Message-Id") == "<20151111081652.86C5D4C03B8@halk.m2osw.com>");

    int const max_attachment_count(e.get_attachment_count());
    SNAP_TEST_PLUGIN_SUITE_ASSERT(max_attachment_count == 3);

    // the attachment will be in order
    // (note that they are in order in the test, they may not stay that
    // way in real life)
    //
    // notification
    //
    {
        snap::sendmail::sendmail::email::email_attachment & notification(e.get_attachment(0));
        SNAP_TEST_PLUGIN_SUITE_ASSERT(notification.get_header("Content-Description") == "Notification");
        SNAP_TEST_PLUGIN_SUITE_ASSERT(notification.get_header("Content-Type") == "text/plain; charset=us-ascii");
        QByteArray const data(notification.get_data());
        SNAP_TEST_PLUGIN_SUITE_ASSERT(QString::fromUtf8(data.data()) ==
"\n"
"<invalid@m2osw.com>: host mail.m2osw.com[69.55.231.156] said: 554 5.7.1\n"
"    <invalid@m2osw.com>: Recipient address rejected: Access denied (in reply to\n"
"    RCPT TO command)"
        );
    }

    // delivery report
    //
    {
        snap::sendmail::sendmail::email::email_attachment & delivery_report(e.get_attachment(1));
        SNAP_TEST_PLUGIN_SUITE_ASSERT(delivery_report.get_header("Content-Description") == "Delivery report");
        SNAP_TEST_PLUGIN_SUITE_ASSERT(delivery_report.get_header("Content-Type") == "message/delivery-status");
        int const max_related(delivery_report.get_related_count());
        SNAP_TEST_PLUGIN_SUITE_ASSERT(max_related == 2);

        {
            snap::sendmail::sendmail::email::email_attachment related(delivery_report.get_related(0));
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Reporting-MTA") == "dns; snap.website");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("X-Postfix-Queue-ID") == "AABBCCDDEEF");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("X-Postfix-Sender") == "rfc822; alexis@snap.website");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Arrival-Date") == "Wed, 11 Nov 2015 00:16:39 -0800 (PST)");
        }

        {
            snap::sendmail::sendmail::email::email_attachment related(delivery_report.get_related(1));
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Final-Recipient") == "rfc822; invalid@snap.website");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Action") == "failed");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Status") == "5.7.1");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Remote-MTA") == "dns; snap.website");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Diagnostic-Code") == "smtp; 554 5.7.1 <invalid@m2osw.com>: Recipient address rejected: Access denied");
        }
     }

    // undelivered message
    //
    {
        snap::sendmail::sendmail::email::email_attachment & undelivered_message_headers(e.get_attachment(2));
        SNAP_TEST_PLUGIN_SUITE_ASSERT(undelivered_message_headers.get_header("Content-Description") == "Undelivered Message Headers");
        SNAP_TEST_PLUGIN_SUITE_ASSERT(undelivered_message_headers.get_header("Content-Type") == "text/rfc822-headers");
        int const max_related(undelivered_message_headers.get_related_count());
        SNAP_TEST_PLUGIN_SUITE_ASSERT(max_related == 1);

        {
            snap::sendmail::sendmail::email::email_attachment related(undelivered_message_headers.get_related(0));
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Return-Path") == "<help@snap.website>");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Received") == "by snap.website (Postfix, from userid 1000) id AABBCCDDEEF; Wed, 11 Nov 2015 00:16:39 -0800 (PST)");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Content-Language") == "en-us");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Content-Type") == "multipart/mixed; boundary=\"=Snap.Websites=00112233445566778899\"");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Date") == "11 Nov 2015 08:16:39 +0000");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("From") == "snap@snap.website");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Importance") == "High");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("List-Unsubscribe") == "http://csnap.m2osw.com/unsubscribe/00112233445566778899aabbccddeeff");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Message-ID") == "<1705d651600627abf20cbc663567dbf9@snapwebsites>");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("MIME-Version") == "1.0");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Precedence") == "High");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("Subject") == "Please verify your email address");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("To") == "invalid@snap.website");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("X-MSMail-Priority") == "High");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("X-Priority") == "4 (High)");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("X-Generated-By") == "Snap! Websites C++ v0.1.71 (http://snapwebsites.org/)");
            SNAP_TEST_PLUGIN_SUITE_ASSERT(related.get_header("X-Mailer") == "Snap! Websites C++ v0.1.71 (http://snapwebsites.org/)");
        }
    }
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
