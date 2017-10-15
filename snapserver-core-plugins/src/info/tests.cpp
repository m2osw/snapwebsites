// Snap Websites Server -- tests for the info plugin and core elements
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

#include "info.h"

#include "../messages/messages.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(info)


SNAP_TEST_PLUGIN_SUITE(info)
    SNAP_TEST_PLUGIN_TEST(info, verify_core_dependencies)
    SNAP_TEST_PLUGIN_TEST(info, verify_all_dependencies)
SNAP_TEST_PLUGIN_SUITE_END()


SNAP_TEST_PLUGIN_TEST_IMPL(info, verify_core_dependencies)
{
    // get a copy of the normal set of plugins
    // (our test system runs one test at a time)
    //
    plugins::plugin_map_t const list(plugins::get_plugin_list());

    // go through the list of core plugins
    //
    for(auto const & c : list)
    {
        QString const name(c->get_plugin_name());
        if(f_snap->is_core_plugin(name))
        {
            // the test does not need to be recursive because we are
            // testing all the plugins anyway; whether the plugins
            // are all in a well defined tree is a different test
            //
            plugins::plugin * p(plugins::get_plugin(name));
            snap_string_list const deps(p->dependencies().split('|', QString::SkipEmptyParts));
            for(auto const & d : deps)
            {
                if(d == "server")
                {
                    continue;
                }

                // Without these messages an error is really hard to fix
                // if you have no clue where it is... (i.e. did not just
                // add a new plugin...)
                //
                if(!list.contains(d))
                {
                    messages::messages::instance()->set_info(
                        "Check Dependency",
                        QString("Core Plugin \"%1\" has dependency \"%2\" which is not currently in the list of installed plugins.").arg(name).arg(d)
                    );
                }

                // the core plugin must exist
                SNAP_TEST_PLUGIN_SUITE_ASSERT(list.contains(d));

                if(!f_snap->is_core_plugin(d))
                {
                    messages::messages::instance()->set_info(
                        "Check Dependency",
                        QString("Core Plugin \"%1\" has dependency \"%2\" which is not itself a Core Plugin.").arg(name).arg(d)
                    );
                }

                // and also all core plugin dependencies have to be core
                // plugins themselves
                SNAP_TEST_PLUGIN_SUITE_ASSERT(f_snap->is_core_plugin(d));
            }
        }
    }
}


SNAP_TEST_PLUGIN_TEST_IMPL(info, verify_all_dependencies)
{
    struct recursive_test
    {
        static void recursive(QString const plugins_paths, QString const & name, snap_string_list & parents)
        {
            // the server is a special case and we consider that it works
            // each and every time without having to do anything more.
            //
            if(name == "server")
            {
                return;
            }

            // Without this message an error is really hard to fix
            // if you have no clue where it is... (i.e. did not just
            // add a new plugin...)
            if(parents.contains(name))
            {
                messages::messages::instance()->set_info(
                    "Check Dependency Tree",
                    QString("Plugin \"%1\" is part of its parents \"%2\", meaning that it depends on itself.").arg(name).arg(parents.join(", "))
                );
            }

            // if present in the list of parents, then we have a loop
            //
            SNAP_TEST_PLUGIN_SUITE_ASSERT(!parents.contains(name));

            // we become a parent
            //
            parents << name;

            // retrieve our list of children (dependencies) and check
            // each one of them
            //
            plugins::plugin_info const information(plugins_paths, name);
            snap_string_list const deps(information.get_dependencies().split('|', QString::SkipEmptyParts));
            for(auto const & d : deps)
            {
                recursive(plugins_paths, d, parents);
            }

            // we are done as a parent
            //
            parents.removeAll(name);
        }
    };

    QString const plugins_paths( f_snap->get_server_parameter("plugins_path") );

    // get a copy of all possible plugins, even those not currently installed
    //
    snap_string_list const list(plugins::list_all(plugins_paths));

    // for each plugin, check all of the dependencies and dependencies of
    // dependencies, recursively making sure we never have a dependency
    // loop
    //
    for(auto const & name : list)
    {
        snap_string_list parents;
        recursive_test::recursive(plugins_paths, name, parents);
    }
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
