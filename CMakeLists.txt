cmake_minimum_required(VERSION 2.6)

project(LmTransports C)

find_package(PkgConfig REQUIRED)

include(Warnings.cmake)
include(GenMarshal.cmake)

include(ConfigureChecks.cmake)
configure_file(config.h.cmake config.h)

pkg_check_modules(LM REQUIRED
	glib-2.0
	gobject-2.0)

set(SOURCES
	lm-debug.c
	lm-debug.h
	lm-dummy.c
	lm-dummy.h
	lm-idummy.c
	lm-idummy.h
	lm-marshal.c
	lm-marshal.h
	lm-misc.c
	lm-misc.h
	lm-old-socket.c
	lm-old-socket.h
	lm-sock.c
	lm-sock.h
	lm-socket.c
	lm-socket.h
	lm-transport.c
	lm-transport.h
)

add_executable(test test.c ${SOURCES})
target_link_libraries(test ${LM_LIBRARIES})

include_directories("." ${LM_INCLUDE_DIRS})
link_directories(${LM_LIBRARY_DIRS})

genmarshal("lm")

