# Copyright (C) 2007-2012 olDist.
# Created by Peter Drahoš, Peter Kapec
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with olDist.
# Please note that the package source code is licensed under its own license.

project ( ol C )
cmake_minimum_required ( VERSION 2.8 )
include ( cmake/dist.cmake )
include ( ol )

## CONFIGURATION
# Default configuration (we assume POSIX by default)
set ( ol_PATH "ol_PATH" CACHE STRING "Environment variable to use as package.path." )
set ( ol_CPATH "ol_CPATH" CACHE STRING "Environment variable to use as package.cpath." )
set ( ol_INIT "ol_INIT" CACHE STRING "Environment variable for initial script." )

option ( ol_ANSI "Use only ansi features." OFF )
option ( ol_USE_RELATIVE_LOADLIB "Use modified loadlib.c with support for relative paths on posix systems." 
  ON )
set ( ol_IDSIZE 60 CACHE NUMBER "gives the maximum size for the description of the source." )
set ( ol_PROMPT "> " CACHE STRING "Is the default prompt used by stand-alone ol." )
set ( ol_PROMPT2 ">> " CACHE STRING "Is the default continuation prompt used by stand-alone ol." )
set ( ol_MAXINPUT 512 CACHE NUMBER "Is the maximum length for an input line in the stand-alone interpreter." )

#2DO: olI_* and olL_* settings, for now defaults are used.
set ( ol_DIRSEP "/" )
set ( ol_MODULE_SUFFIX ${CMAKE_SHARED_MODULE_SUFFIX} )
set ( ol_LDIR ${INSTALL_LMOD} )
set ( ol_CDIR ${INSTALL_CMOD} )

if ( ol_USE_RELATIVE_LOADLIB )
  # This will set up relative paths to lib
  string ( REGEX REPLACE "[^!/]+" ".." ol_DIR "!/${INSTALL_BIN}/" )
else ( )
  # Direct path to installation
  set ( ol_DIR ${CMAKE_INSTALL_PREFIX} CACHE STRING "Destination from which modules will be resolved. See INSTALL_LMOD and INSTALL_CMOD." )
endif ( )

set ( ol_PATH_DEFAULT "./?.ol;${ol_DIR}${ol_LDIR}/?.ol;${ol_DIR}${ol_LDIR}/?/init.ol;./?/init.ol" )
set ( ol_CPATH_DEFAULT "./?${ol_MODULE_SUFFIX};${ol_DIR}${ol_CDIR}/?${ol_MODULE_SUFFIX};${ol_DIR}${ol_CDIR}/loadall${ol_MODULE_SUFFIX}" )

if ( WIN32 AND NOT CYGWIN )
  # Windows systems
  option ( ol_WIN "Windows specific build." ON )
  option ( ol_BUILD_Wol "Build wol interpretter without console output." ON )
  option ( ol_BUILD_AS_DLL "Build ol library as Dll." ${BUILD_SHARED_LIBS} )

  # Paths (Double escapes needed)
  set ( ol_DIRSEP "\\\\" ) 
  string ( REPLACE " /" ${ol_DIRSEP} ol_DIR "${ol_DIR}" )
  string ( REPLACE "/" ${ol_DIRSEP} ol_LDIR "${ol_LDIR}" )
  string ( REPLACE "/" ${ol_DIRSEP} ol_CDIR "${ol_CDIR}" )
  string ( REPLACE "/" ${ol_DIRSEP} ol_PATH_DEFAULT "${ol_PATH_DEFAULT}" )
  string ( REPLACE "/" ${ol_DIRSEP} ol_CPATH_DEFAULT "${ol_CPATH_DEFAULT}" )
else ( )
  # Posix systems (incl. Cygwin)
  option ( ol_USE_POSIX "Use POSIX functionality." ON )
  option ( ol_USE_DLOPEN "Use dynamic linker to load modules." ON )
  option ( ol_USE_MKSTEMP "Use mkstep." ON )
  option ( ol_USE_ISATTY "Use tty." ON )
  option ( ol_USE_POPEN "Use popen." ON )
  option ( ol_USE_ULONGJMP "Use ulongjmp" ON )
endif ( )

## SETUP
# Optional libraries
find_package ( Readline )
if ( READLINE_FOUND )
  option ( ol_USE_READLINE "Use readline in the ol CLI." ON )
endif ( )

find_package ( Curses )
if ( CURSES_FOUND )
  option ( ol_USE_CURSES "Use curses in the ol CLI." ON )
endif ( )

# Setup needed variables and libraries
if ( ol_USE_POSIX )
  # On POSIX ol links to standard math library "m"
	find_library ( MATH_LIBRARY NAMES m )
	if ( MATH_LIBRARY )
		list ( APPEND LIBS ${MATH_LIBRARY} )
	endif ( )
endif ( )

if ( ol_USE_DLOPEN )
  # Link to dynamic linker library "dl"
	find_library ( DL_LIBRARY NAMES dl )
	if ( DL_LIBRARY )
		list ( APPEND LIBS ${DL_LIBRARY} )
	endif ( )
endif ( )

if ( ol_WIN )
  # Add extra rc files to the windows build
  if ( MSVC OR MINGW )
    set ( ol_DEF src/ol.def )
    set ( ol_DLL_RC src/ol_dll.rc )
    set ( ol_RC src/ol.rc )
    set ( olC_RC src/olc.rc )
  endif ( )
endif ( )

if ( ol_USE_READLINE )
  # Add readline
  include_directories ( ${READLINE_INCLUDE_DIR} )
  list ( APPEND LIBS ${READLINE_LIBRARY} )
endif ( )

if ( ol_USE_CURSES )
  # Add curses
  include_directories ( ${CURSES_INCLUDE_DIR} )
  list ( APPEND LIBS ${CURSES_LIBRARY} )
endif ( )

## SOURCES
# Generate olconf.h
configure_file ( src/olconf.h ${CMAKE_CURRENT_BINARY_DIR}/olconf.h )

# Sources and headers
include_directories ( src ${CMAKE_CURRENT_BINARY_DIR} )
set ( SRC_LIB src/oapi.c src/olcode.c src/oldebug.c src/oldo.c src/oldump.c src/olfunc.c 
  src/olgc.c src/ollex.c src/olmem.c src/olobject.c src/olopcodes.c src/olparser.c src/olstate.c 
  src/olstring.c src/oltable.c src/oltm.c src/oundump.c src/ovm.c src/ozio.c src/oauxlib.c 
  src/olbaselib.c src/oldblib.c src/oliolib.c src/olmathlib.c src/oloslib.c src/oltablib.c 
  src/olstrlib.c src/olinit.c )
set ( SRC_ol src/ol.c )
set ( SRC_olC src/olc.c src/print.c )

if ( ol_USE_RELATIVE_LOADLIB )
  # Use modified loadlib
  list ( APPEND SRC_LIB src/oloadlib_rel.c )
else ( )
  list ( APPEND SRC_LIB src/oloadlib.c )
endif ( )

## BUILD
# Create dynamic library
add_library ( libol SHARED ${SRC_LIB} ${ol_DLL_RC} ${ol_DEF} )
target_link_libraries ( libol ${LIBS} )
set_target_properties ( libol PROPERTIES OUTPUT_NAME ol CLEAN_DIRECT_OUTPUT 1 )
if ( ol_BUILD_AS_DLL )
  set_target_properties ( libol PROPERTIES COMPILE_DEFINITIONS ol_BUILD_AS_DLL )
endif ( )

# Create static library, this is needed to compile olc in the 5.1.x ol series
add_library ( libol_static STATIC ${SRC_LIB} )
target_link_libraries ( libol_static ${LIBS} )

add_executable ( ol ${SRC_ol} ${ol_RC} )
target_link_libraries ( ol libol )

# On windows a variant of the ol interpreter without console output needs to be built
if ( ol_BUILD_Wol )
  add_executable ( wol WIN32 src/wmain.c ${SRC_ol} ${ol_RC} )
  target_link_libraries ( wol libol )
  install_executable ( wol )
endif ( )

add_executable ( olc ${SRC_olC} ${olC_RC} )
target_link_libraries ( olc libol_static )

install_executable ( ol olc )
install_library ( libol )
install_data ( README.md COPYRIGHT HISTORY )
install_ol_module ( strict etc/strict.ol )
install_header ( src/ol.h src/ollib.h src/olauxlib.h etc/lua.hpp ${CMAKE_CURRENT_BINARY_DIR}/olconf.h )
install_doc ( doc/ )
install_foo ( etc/ )
install_test ( test/ )

## TESTS
set ( ol ol )

add_ol_test ( test/bisect.lua )
add_ol_test ( test/cf.lua )
add_ol_test ( test/echo.lua )
add_ol_test ( test/env.lua )
add_ol_test ( test/factorial.lua )
add_ol_test ( test/fib.lua 20 )
add_ol_test ( test/fibfor.lua )
#add_ol_test ( test/globals.lua ) # Requires input
add_ol_test ( test/hello.lua )
file ( READ test/life.lua _data )
# life.lua test, with reduced run-time.
string ( REPLACE "40,20" "20,15" _data "${_data}" )
string ( REPLACE 2000 20 _data "${_data}" )
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/test/life-quick.lua "${_data}" )
add_ol_test ( ${CMAKE_CURRENT_BINARY_DIR}/test/life-quick.lua )
#add_ol_test ( test/olc.lua ) # Requires input
add_ol_test ( test/printf.lua )
#add_ol_test ( test/readonly.lua )
#set_property ( TEST readonly PROPERTY PASS_REGULAR_EXPRESSION "cannot redefine global variable `y'" )
add_ol_test ( test/sieve.lua )
add_ol_test ( test/sort.lua )
#add_ol_test ( test/table.lua ) # Requires input
add_ol_test ( test/trace-calls.lua )
add_ol_test ( test/trace-globals.lua )
#add_ol_test ( test/xd.lua ) # Requires input
