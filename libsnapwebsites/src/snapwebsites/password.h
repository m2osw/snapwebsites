// Snap Websites Server -- handle creating / encrypting password
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

// snapwebsites lib
//
#include "snapwebsites/file_content.h"
#include "snapwebsites/snap_exception.h"


namespace snap
{

class password_exception : public snap::snap_exception
{
public:
    password_exception(char const *        what_msg) : snap_exception(what_msg) {}
    password_exception(std::string const & what_msg) : snap_exception(what_msg) {}
    password_exception(QString const &     what_msg) : snap_exception(what_msg) {}
};

class password_exception_function_failure : public password_exception
{
public:
    password_exception_function_failure(char const *        what_msg) : password_exception(what_msg) {}
    password_exception_function_failure(std::string const & what_msg) : password_exception(what_msg) {}
    password_exception_function_failure(QString const &     what_msg) : password_exception(what_msg) {}
};

class password_exception_invalid_parameter : public password_exception
{
public:
    password_exception_invalid_parameter(char const *        what_msg) : password_exception(what_msg) {}
    password_exception_invalid_parameter(std::string const & what_msg) : password_exception(what_msg) {}
    password_exception_invalid_parameter(QString const &     what_msg) : password_exception(what_msg) {}
};

class password_exception_digest_not_available : public password_exception
{
public:
    password_exception_digest_not_available(char const *        what_msg) : password_exception(what_msg) {}
    password_exception_digest_not_available(std::string const & what_msg) : password_exception(what_msg) {}
    password_exception_digest_not_available(QString const &     what_msg) : password_exception(what_msg) {}
};

class password_exception_encryption_failed : public password_exception
{
public:
    password_exception_encryption_failed(char const *        what_msg) : password_exception(what_msg) {}
    password_exception_encryption_failed(std::string const & what_msg) : password_exception(what_msg) {}
    password_exception_encryption_failed(QString const &     what_msg) : password_exception(what_msg) {}
};



class password
{
public:
                        password();
                        ~password();

    void                set_digest(std::string const & digest);
    std::string const & get_digest() const;

    void                generate_password(int min_length = 64);
    void                set_plain_password(std::string const & plain_password, std::string const & salt = std::string());
    std::string const & get_plain_password() const;
    bool                get_password_from_console(std::string const & salt = std::string());

    std::string const & get_salt() const;

    void                set_encrypted_password(std::string const & encrypted_password, std::string const & salt = std::string());
    std::string const & get_encrypted_password() const;

    bool                operator == (password const & rhs) const;
    bool                operator < (password const & rhs) const;

    static void         clear_string(std::string & str);

private:
    void                generate_password_salt();
    void                encrypt_password();

    std::string         f_plain_password;
    std::string         f_encrypted_password;
    std::string         f_salt;
    std::string         f_digest = "sha512";
};


class password_file
{
public:
                            password_file(std::string const & password_filename);
                            ~password_file();

    bool                    find(std::string const & name, password & p);
    bool                    save(std::string const & name, password const & p);
    bool                    remove(std::string const & name);
    std::string             next(password & p);
    void                    rewind();

private:
    bool                    load_passwords();

    bool                    f_file_loaded = false;
    std::string::size_type  f_next = 0;
    file_content            f_passwords;
};

} // snap namespace
// vim: ts=4 sw=4 et
