#
# Text:
#      CMakeLists.txt
#
# Description:
#      Defines the source files.
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
#      Permission is hereby granted, free of charge, to any person obtaining a
#      copy of this software and associated documentation files (the
#      "Software"), to deal in the Software without restriction, including
#      without limitation the rights to use, copy, modify, merge, publish,
#      distribute, sublicense, and/or sell copies of the Software, and to
#      permit persons to whom the Software is furnished to do so, subject to
#      the following conditions:
#
#      The above copyright notice and this permission notice shall be included
#      in all copies or substantial portions of the Software.
#
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

project( libdbproxy )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AS2JS_INCLUDE_DIR}
    ${CASSWRAPPER_INCLUDE_DIR}
    ${snapwebsites_src}
)

configure_file(
		version.h.in
		${PROJECT_BINARY_DIR}/version.h
		)

set( HEADER_FILES
    libdbproxy.h
    cell.h
    consistency_level.h
    context.h
    exception.h
    order.h
    order_result.h
    predicate.h
    proxy.h
    row.h
    table.h
    value.h
    version.h.in
    qstring_stream.h
    ${PROJECT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    ${HEADER_FILES}
    libdbproxy.cpp
    cell.cpp
    context.cpp
    exception.cpp
    order.cpp
    order_result.cpp
    predicate.cpp
    proxy.cpp
    row.cpp
    table.cpp
    value.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${AS2JS_LIBRARIES}
    ${CASSWRAPPER_LIBRARIES}
    ${CASSVALUE_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    Qt5::Core
    Qt5::Xml
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${SNAPWEBSITES_VERSION_MAJOR}.${SNAPWEBSITES_VERSION_MINOR}
    SOVERSION ${SNAPWEBSITES_VERSION_MAJOR}
)

install(
    TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib
)

install(
	FILES ${HEADER_FILES}
    DESTINATION include/libdbproxy
)

install(
	FILES ${PROJECT_BINARY_DIR}/version.h
    DESTINATION include/libdbproxy
)

# vim: ts=4 sw=4 et
