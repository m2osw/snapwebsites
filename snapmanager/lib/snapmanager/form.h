// Snap Websites Server -- handle simple forms
// Copyright (C) 2016  Made to Order Software Corp.
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

#include <snapwebsites/snap_uri.h>

#include <QDomElement>
#include <QString>

#include <memory>
#include <vector>

namespace snap_manager
{



class widget
{
public:
    typedef std::shared_ptr<widget> pointer_t;
    typedef std::vector<pointer_t>  vector_t;

                        widget(QString const & name);

    virtual void        generate(QDomElement parent) = 0;

protected:
    QString             f_name;
};


class widget_description
        : public widget
{
public:
    typedef std::shared_ptr<widget_description> pointer_t;

                        widget_description(QString const & label, QString const & name, QString const & description);

    virtual void        generate(QDomElement parent) override;

private:
    QString             f_label;
    QString             f_description;
};


class widget_input
        : public widget
{
public:
    typedef std::shared_ptr<widget_input> pointer_t;

                        widget_input(QString const & label, QString const & name, QString const & initial_value, QString const & description);

    virtual void        generate(QDomElement parent) override;

private:
    QString             f_label;
    QString             f_value;
    QString             f_description;
};


class widget_text
        : public widget
{
public:
    typedef std::shared_ptr<widget_text> pointer_t;

                        widget_text(QString const & label, QString const & name, QString const & initial_value, QString const & description);

    virtual void        generate(QDomElement parent) override;

private:
    QString             f_label;
    QString             f_value;
    QString             f_description;
};


class widget_select
        : public widget
{
public:
    typedef std::shared_ptr<widget_select> pointer_t;

                        widget_select(QString const & label
                                   , QString const & name
                                   , QStringList const & initial_value
                                   , QString const & default_value
                                   , QString const & description);

    virtual void        generate(QDomElement parent) override;

private:
    QString             f_label;
    QStringList         f_valueList;
    QString             f_defaultValue;
    QString             f_description;
};


class form
{
public:
    typedef uint32_t    button_t;

    static button_t const       FORM_BUTTON_NONE               = 0x00000000;
    static button_t const       FORM_BUTTON_RESET              = 0x00000001;
    static button_t const       FORM_BUTTON_SAVE               = 0x00000002;
    static button_t const       FORM_BUTTON_SAVE_EVERYWHERE    = 0x00000004;
    static button_t const       FORM_BUTTON_RESTORE_DEFAULT    = 0x00000008;
    static button_t const       FORM_BUTTON_INSTALL            = 0x00000010;
    static button_t const       FORM_BUTTON_UNINSTALL          = 0x00000020;
    static button_t const       FORM_BUTTON_REBOOT             = 0x00000040;
    static button_t const       FORM_BUTTON_UPGRADE            = 0x00000080;
    static button_t const       FORM_BUTTON_UPGRADE_EVERYWHERE = 0x00000100;
    static button_t const       FORM_BUTTON_REFRESH            = 0x00000200;
    static button_t const       FORM_BUTTON_RESTART            = 0x00000400;
    static button_t const       FORM_BUTTON_RESTART_EVERYWHERE = 0x00000800;

                        form(QString const & plugin_name, QString const & field_name, button_t button = FORM_BUTTON_NONE);

    void                generate(QDomElement parent, snap::snap_uri const & uri);
    void                add_widget(widget::pointer_t w);

private:
    QString             f_plugin_name;
    QString             f_field_name;
    button_t            f_buttons = FORM_BUTTON_RESET | FORM_BUTTON_SAVE;
    widget::vector_t    f_widgets;
};



} // snap_manager namespace
// vim: ts=4 sw=4 et
