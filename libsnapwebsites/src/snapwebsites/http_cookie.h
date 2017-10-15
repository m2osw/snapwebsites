// Snap Websites Servers -- HTTP cookie handling (outgoing)
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

#include "snapwebsites/snap_exception.h"

#include <QDateTime>

namespace snap
{

class http_cookie_exception : public snap_exception
{
public:
    http_cookie_exception(char const *        what_msg) : snap_exception("http_cookie", what_msg) {}
    http_cookie_exception(std::string const & what_msg) : snap_exception("http_cookie", what_msg) {}
    http_cookie_exception(QString const &     what_msg) : snap_exception("http_cookie", what_msg) {}
};


class http_cookie_parse_exception : public http_cookie_exception
{
public:
    http_cookie_parse_exception(char const *        what_msg) : http_cookie_exception(what_msg) {}
    http_cookie_parse_exception(std::string const & what_msg) : http_cookie_exception(what_msg) {}
    http_cookie_parse_exception(QString const &     what_msg) : http_cookie_exception(what_msg) {}
};



class snap_child;

class http_cookie
{
public:
    enum class http_cookie_type_t
    {
        HTTP_COOKIE_TYPE_PERMANENT,
        HTTP_COOKIE_TYPE_SESSION,
        HTTP_COOKIE_TYPE_DELETE
    };

                        http_cookie(); // for QMap to work; DO NOT USE!
                        http_cookie(snap_child * snap, QString const & name, QString const & value = "");

    void                set_value(QString const & value);
    void                set_value(QByteArray const & value);
    void                set_domain(QString const & domain);
    void                set_path(QString const & path);
    void                set_delete();
    void                set_session();
    void                set_expire(QDateTime const & date_time);
    void                set_expire_in(int64_t seconds);
    void                set_secure(bool secure = true);
    void                set_http_only(bool http_only = true);
    void                set_comment(QString const & comment);
    void                set_comment_url(QString const & comment_url);

    QString const &     get_name() const;
    QByteArray const &  get_value() const;
    http_cookie_type_t  get_type() const;
    QString const &     get_domain() const;
    QString const &     get_path() const;
    QDateTime const &   get_expire() const;
    bool                get_secure() const;
    bool                get_http_only() const;
    QString const &     get_comment() const;
    QString const &     get_comment_url() const;

    QString             to_http_header() const;

private:
    snap_child *                f_snap = nullptr;   // the snap child that created this cookie
    QString                     f_name;             // name of the cookie
    QByteArray                  f_value;            // the cookie value (binary buffer)
    QString                     f_domain;           // domain for which the cookie is valid
    QString                     f_path;             // path under which the cookie is valid
    QDateTime                   f_expire;           // when to expire the cookie (if null, session, if past delete)
    bool                        f_secure = false;   // only valid on HTTPS
    bool                        f_http_only = false;// JavaScript cannot access this cookie
    QString                     f_comment;          // verbatim comment
    QString                     f_comment_url;      // verbatim comment
};



} // namespace snap
// vim: ts=4 sw=4 et
