# - Try to find SnapManager[CGI]
#
# Once done this will define
#
# SNAPMANAGER_FOUND        - System has SnapManager[CGI]
# SNAPMANAGER_INCLUDE_DIR  - The SnapWebsites include directories
# SNAPMANAGER_LIBRARY      - The libraries needed to use SnapWebsites (none)

# Search include directory
find_path( SNAPMANAGER_INCLUDE_DIR snapmanager/manager.h
    PATHS $ENV{SNAPMANAGER_INCLUDE_DIR}
    PATH_SUFFIXES snapmanager
)

# Search library
find_library( SNAPMANAGER_LIBRARY snapmanager
    PATHS $ENV{SNAPMANAGER_LIBRARY}
)

# Mark as important
mark_as_advanced( SNAPMANAGER_INCLUDE_DIR SNAPMANAGER_LIBRARY )

# Define the plurial versions
set( SNAPMANAGER_INCLUDE_DIRS ${SNAPMANAGER_INCLUDE_DIR} )
set( SNAPMANAGER_LIBRARIES    ${SNAPMANAGER_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set SNAPWEBSITES_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( SnapManager DEFAULT_MSG SNAPMANAGER_INCLUDE_DIR SNAPMANAGER_LIBRARY )

# vim: ts=4 sw=4 et
