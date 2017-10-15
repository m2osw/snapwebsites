// Snap Websites Server -- images management (transformations)
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

// other plugins
//
#include "../path/path.h"
#include "../versions/versions.h"

// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_backend.h>

// Magick lib
//
#include <Magick++.h>


namespace snap
{
namespace images
{


enum class name_t
{
    SNAP_NAME_IMAGES_ACTION,
    SNAP_NAME_IMAGES_MODIFIED,
    SNAP_NAME_IMAGES_PROCESS_IMAGE,
    SNAP_NAME_IMAGES_ROW,
    SNAP_NAME_IMAGES_SCRIPT
};
char const * get_name(name_t name) __attribute__ ((const));


class images_exception : public snap_exception
{
public:
    explicit images_exception(char const *        what_msg) : snap_exception("images", what_msg) {}
    explicit images_exception(std::string const & what_msg) : snap_exception("images", what_msg) {}
    explicit images_exception(QString const &     what_msg) : snap_exception("images", what_msg) {}
};

class images_exception_no_backend : public images_exception
{
public:
    images_exception_no_backend(char const *        what_msg) : images_exception(what_msg) {}
    images_exception_no_backend(std::string const & what_msg) : images_exception(what_msg) {}
    images_exception_no_backend(QString const &     what_msg) : images_exception(what_msg) {}
};









class images
        : public plugins::plugin
        , public server::backend_action
        , public path::path_execute
{
public:
    enum class virtual_path_t
    {
        VIRTUAL_PATH_READY,
        VIRTUAL_PATH_INVALID,
        VIRTUAL_PATH_NOT_AVAILABLE
    };

                        images();
                        ~images();

    // plugins implementation
    static images *     instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // server::backend_action implementation
    virtual void        on_backend_action(QString const & action);

    // server signals
    void                on_attach_to_session();
    void                on_register_backend_cron(server::backend_action_set & actions);
    void                on_register_backend_action(server::backend_action_set & actions);

    // links signals
    void                on_modified_link(links::link_info const & src);

    // path::path_execute implementation
    virtual bool        on_path_execute(content::path_info_t & ipath);

    // path signals
    void                on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info);

    // versions signals
    void                on_versions_libraries(filter::filter::token_info_t & token);

    // content signals
    void                on_create_content(content::path_info_t & ipath, QString const & owner, QString const & type);
    void                on_modified_content(content::path_info_t & ipath);

    // listener signals
    void                on_listener_check(snap_uri const & uri, content::path_info_t & page_ipath, QDomDocument doc, QDomElement result);

    // filter signals
    void                on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                on_token_help(filter::filter::token_help_t & help);

    Magick::Image       apply_image_script(QString const & script, content::path_info_t::map_path_info_t image_ipaths);

private:
    // we want a stack of images (we can "push" by loading a new image that
    // we want to apply to a previous image in some way)
    typedef std::vector<Magick::Image>      images_t;

    struct parameters_t
    {
        snap_string_list                        f_params;
        images_t                                f_image_stack;
        content::path_info_t::map_path_info_t   f_image_ipaths;
        QString                                 f_command; // mainly for errors
    };

    struct func_t
    {
        char const *    f_command_name = nullptr;
        size_t          f_min_params = 0;
        size_t          f_max_params = 0;
        size_t          f_min_stack = 0;
        bool            (images::*f_command)(parameters_t & params) = nullptr;
    };

    virtual_path_t      check_virtual_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info);
    void                content_update(int64_t variables_timestamp);
    int64_t             transform_images();
    bool                do_image_transformations(QString const & image_key);
    bool                get_color(QString str, Magick::Color & color);

    bool                func_alpha(parameters_t & params);
    bool                func_background_color(parameters_t & params);
    bool                func_blur(parameters_t & params);
    bool                func_border(parameters_t & params);
    bool                func_border_color(parameters_t & params);
    bool                func_charcoal(parameters_t & params);
    bool                func_composite(parameters_t & params);
    bool                func_contrast(parameters_t & params);
    bool                func_create(parameters_t & params);
    bool                func_crop(parameters_t & params);
    bool                func_density(parameters_t & params);
    bool                func_emboss(parameters_t & params);
    bool                func_erase(parameters_t & params);
    bool                func_flip(parameters_t & params);
    bool                func_flop(parameters_t & params);
    bool                func_hash(parameters_t & params);
    bool                func_matte_color(parameters_t & params);
    bool                func_modulate(parameters_t & params);
    bool                func_negate(parameters_t & params);
    bool                func_normalize(parameters_t & params);
    bool                func_oil_paint(parameters_t & params);
    bool                func_on_error(parameters_t & params);
    bool                func_pop(parameters_t & params);
    bool                func_read(parameters_t & params);
    bool                func_reduce_noise(parameters_t & params);
    bool                func_resize(parameters_t & params);
    bool                func_rotate(parameters_t & params);
    bool                func_shade(parameters_t & params);
    bool                func_shadow(parameters_t & params);
    bool                func_sharpen(parameters_t & params);
    bool                func_shear(parameters_t & params);
    bool                func_solarize(parameters_t & params);
    bool                func_swap(parameters_t & params);
    bool                func_trim(parameters_t & params);
    bool                func_write(parameters_t & params);

    snap_child *                    f_snap = nullptr;
    snap_backend *                  f_backend = nullptr;
    bool                            f_ping_backend = false;
    QString                         f_on_error; // execute this script on errors

    static images::func_t const     g_commands[];
    static int const                g_commands_size;
};


} // namespace list
} // namespace snap
// vim: ts=4 sw=4 et
