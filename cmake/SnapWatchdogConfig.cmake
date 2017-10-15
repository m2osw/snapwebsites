# - Try to find SnapWebsites
#
# Once done this will define
#
# SNAPWATCHDOG_FOUND        - System has SnapWebsites
# SNAPWATCHDOG_INCLUDE_DIR  - The SnapWebsites include directories
# SNAPWATCHDOG_LIBRARY      - The libraries needed to use SnapWebsites (none)
# SNAPWATCHDOG_DEFINITIONS  - Compiler switches required for using SnapWebsites (none)

find_path( SNAPWATCHDOG_INCLUDE_DIR snapwatchdog/snapwatchdog.h
		   PATHS $ENV{SNAPWATCHDOG_INCLUDE_DIR}
		   PATH_SUFFIXES snapwatchdog
		 )
find_library( SNAPWATCHDOG_LIBRARY snapwatchdog
			PATHS $ENV{SNAPWATCHDOG_LIBRARY}
		 )
mark_as_advanced( SNAPWATCHDOG_INCLUDE_DIR SNAPWATCHDOG_LIBRARY )

set( SNAPWATCHDOG_INCLUDE_DIRS ${SNAPWATCHDOG_INCLUDE_DIR} ${SNAPWATCHDOG_INCLUDE_DIR}/snapwatchdog )
set( SNAPWATCHDOG_LIBRARIES    ${SNAPWATCHDOG_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set SNAPWATCHDOG_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( SnapWebsites DEFAULT_MSG SNAPWATCHDOG_INCLUDE_DIR SNAPWATCHDOG_LIBRARY )

