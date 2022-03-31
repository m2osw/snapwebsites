// Snap Websites Server -- manage debug, info, warning, error messages
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

// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>


namespace snap
{
namespace messages
{

enum class name_t
{
    SNAP_NAME_MESSAGES_MESSAGES,
    SNAP_NAME_MESSAGES_WARNING_HEADER
};
char const * get_name(name_t name) __attribute__ ((const));


DECLARE_MAIN_EXCEPTION(messages_exception);

DECLARE_EXCEPTION(messages_exception, messages_exception_invalid_field_name);
DECLARE_EXCEPTION(messages_exception, messages_exception_already_defined);



class messages
    : public cppthread::plugin
    , public QtSerialization::QSerializationObject
{
public:
    // version used in the message class (for serialization)
    static int const MESSAGES_MAJOR_VERSION = 1;
    static int const MESSAGES_MINOR_VERSION = 0;

    class message
        : public QtSerialization::QSerializationObject
    {
    public:
        enum class message_type_t
        {
            MESSAGE_TYPE_ERROR,
            MESSAGE_TYPE_WARNING,
            MESSAGE_TYPE_INFO,
            MESSAGE_TYPE_DEBUG
        };

                            message();
                            message(message_type_t t, QString const & title, QString const & body);
                            message(message const & rhs);

        snap_child::user_status_t
                            get_user_status() const;
        snap_child::user_identifier_t
                            get_user_id() const;
        message_type_t      get_type() const;
        int                 get_id() const;
        QString const &     get_title() const;
        QString const &     get_body() const;

        QString const &     get_widget_name() const;
        void                set_widget_name(QString const & widget_name);

        // internal functions used to save the data serialized
        void                unserialize(QtSerialization::QReader & r);
        virtual void        readTag(QString const & name, QtSerialization::QReader & r);
        void                serialize(QtSerialization::QWriter & w) const;

    private:
        void                set_user_status();

        message_type_t              f_type = message_type_t::MESSAGE_TYPE_DEBUG;
        int32_t                     f_id = -1;
        QString                     f_title = QString();
        QString                     f_body = QString();
        QString                     f_widget_name = QString();
        time_t                      f_date = time(nullptr);
        snap_child::user_status_t   f_user_status = snap_child::user_status_t::USER_STATUS_UNKNOWN;
        snap_child::user_identifier_t
                                    f_user_id = 0;  // we do not have access to users::IDENTIFIER_ANONYMOUS
    };

                        messages();
                        messages(messages const & rhs) = delete;
    virtual             ~messages() override;

    messages &          operator = (messages const & rhs) = delete;

    static messages *   instance();

    // plugins::plugin implementation
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    // server signals
    void                on_user_status(snap_child::user_status_t status, snap_child::user_identifier_t id);

    message &           set_http_error(snap_child::http_code_t err_code, QString err_name, QString const & err_description, QString const & err_details, bool err_security);
    message &           set_error(QString err_name, QString const & err_description, QString const & err_details, bool err_security);
    message &           set_warning(QString warning_name, QString const & warning_description, QString const & warning_details);
    message &           set_info(QString info_name, QString const & info_description);
    message &           set_debug(QString debug_name, QString const & debug_description);

    void                clear_messages();
    message const &     get_message(int idx) const;
    void                remove_message(int idx);
    message const &     get_last_message() const;
    int                 get_message_count() const;
    int                 get_error_count() const;
    int                 get_warning_count() const;
    snap_child::user_status_t
                        get_current_user_status() const;
    snap_child::user_identifier_t
                        get_current_user_id() const;

    // "internal" functions used to save the data serialized
    void                unserialize(QString const & data);
    virtual void        readTag(QString const & name, QtSerialization::QReader & r) override;
    QString             serialize() const;

private:
    snap_child *                f_snap = nullptr;
    QVector<message>            f_messages = QVector<message>();
    int32_t                     f_error_count = 0;
    int32_t                     f_warning_count = 0;
    snap_child::user_status_t   f_current_user_status = snap_child::user_status_t::USER_STATUS_UNKNOWN;
    snap_child::user_identifier_t
                                f_current_user_id = 0;  // we do not have access to users::IDENTIFIER_ANONYMOUS
};

} // namespace messages
} // namespace snap
// vim: ts=4 sw=4 et
