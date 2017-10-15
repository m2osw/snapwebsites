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

project( QtCassandra )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AS2JS_INCLUDE_DIR}
    ${CASSWRAPPER_INCLUDE_DIR}
    ${snapwebsites_src}
)

configure_file(
		QCassandraVersion.h.in
		${PROJECT_BINARY_DIR}/QCassandraVersion.h
		)

set( HEADER_FILES
    QCassandra.h
    QCassandraCell.h
    QCassandraConsistencyLevel.h
    QCassandraContext.h
    QCassandraException.h
    QCassandraOrder.h
    QCassandraOrderResult.h
    QCassandraPredicate.h
    QCassandraProxy.h
    QCassandraRow.h
    QCassandraTable.h
    QCassandraValue.h
    QCassandraVersion.h.in
    QStringStream.h
    ${PROJECT_BINARY_DIR}/QCassandraVersion.h
)

add_library(${PROJECT_NAME} SHARED
    ${HEADER_FILES}
    QCassandra.cpp
    QCassandraCell.cpp
    QCassandraContext.cpp
    QCassandraException.cpp
    QCassandraOrder.cpp
    QCassandraOrderResult.cpp
    QCassandraPredicate.cpp
    QCassandraProxy.cpp
    QCassandraRow.cpp
    QCassandraTable.cpp
    QCassandraValue.cpp
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
    DESTINATION include/QtCassandra
)

install(
	FILES ${PROJECT_BINARY_DIR}/QCassandraVersion.h
    DESTINATION include/QtCassandra
)

# vim: ts=4 sw=4 et
