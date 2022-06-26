// Snap Websites Servers -- HTTP cookie handling (outgoing)
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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

// libexcept lib
//
#include "libexcept/exception.h"


// Qt lib
//
#include <QDateTime>



namespace snap
{

DECLARE_MAIN_EXCEPTION(http_cookie_exception);

DECLARE_EXCEPTION(http_cookie_exception, http_cookie_parse_exception);





class snap_child;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
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
    snap_child *        f_snap = nullptr;           // the snap child that created this cookie
    QString             f_name = QString();         // name of the cookie
    QByteArray          f_value = QByteArray();     // the cookie value (binary buffer)
    QString             f_domain = QString();       // domain for which the cookie is valid
    QString             f_path = QString();         // path under which the cookie is valid
    QDateTime           f_expire = QDateTime();     // when to expire the cookie (if null, session, if past delete)
    bool                f_secure = false;           // only valid on HTTPS
    bool                f_http_only = false;        // JavaScript cannot access this cookie
    QString             f_comment = QString();      // verbatim comment
    QString             f_comment_url = QString();  // verbatim comment
};
#pragma GCC diagnostic pop



} // namespace snap
// vim: ts=4 sw=4 et
