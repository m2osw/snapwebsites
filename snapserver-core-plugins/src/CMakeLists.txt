################################################################################
# File:
#      plugins/CMakeLists.txt
#
# Description:
#      Set of extensions used by the Snap! Websites server.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2017 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#     
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#     
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#
################################################################################

project( snapwebsites_plugins_src )

set( SNAP_TABLE_DESCRIPTION_PATH lib/snapwebsites/tables )


################################################################################
# Keep the plugins in alphabetical order for ease of management
#
add_subdirectory(antihammering)
add_subdirectory(antivirus)
add_subdirectory(attachment)
add_subdirectory(avatar)
add_subdirectory(bookkeeping)
add_subdirectory(char_chart)
add_subdirectory(content)
add_subdirectory(cookie_consent_silktide)
add_subdirectory(date_widgets)
add_subdirectory(detectadblocker)
add_subdirectory(ecommerce)
add_subdirectory(editor)
add_subdirectory(epayment)
add_subdirectory(epayment_creditcard)
add_subdirectory(epayment_paypal)
add_subdirectory(epayment_stripe)
add_subdirectory(favicon)
add_subdirectory(feed)
add_subdirectory(filter)
add_subdirectory(flash)
add_subdirectory(form)
add_subdirectory(hashtag)
add_subdirectory(header)
add_subdirectory(images)
add_subdirectory(info)
add_subdirectory(javascript)
add_subdirectory(layout)
add_subdirectory(layout_contrib)
add_subdirectory(links)
add_subdirectory(list)
add_subdirectory(listener)
add_subdirectory(locale)
add_subdirectory(locale_settings)
add_subdirectory(locale_widgets)
add_subdirectory(mailinglist)
add_subdirectory(menu)
add_subdirectory(messages)
add_subdirectory(mimetype)
add_subdirectory(no_iframe)
add_subdirectory(oauth2)
add_subdirectory(output)
add_subdirectory(password)
add_subdirectory(path)
add_subdirectory(permissions)
add_subdirectory(qrcode)
add_subdirectory(robotstxt)
add_subdirectory(search)
add_subdirectory(sessions)
add_subdirectory(sendmail)
add_subdirectory(server_access)
add_subdirectory(shorturl)
add_subdirectory(sitemapxml)
add_subdirectory(snap_software_description)
add_subdirectory(sortable)
add_subdirectory(taxonomy)
add_subdirectory(test_plugin_suite)
add_subdirectory(test_plugin)
add_subdirectory(timetracker)
add_subdirectory(tracker)
add_subdirectory(users)
add_subdirectory(users_ui)
add_subdirectory(versions)
add_subdirectory(webform)

# vim: ts=4 sw=4 nocindent
