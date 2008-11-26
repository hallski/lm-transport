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
	asyncns.c
	asyncns.h
	lm-channel.c
	lm-channel.h
	lm-dummy.c
	lm-dummy.h
	lm-error.c
	lm-error.h
	lm-idummy.c
	lm-idummy.h
	lm-marshal.c
	lm-marshal.h
	lm-misc.c
	lm-misc.h
	lm-secure-channel.c
	lm-secure-channel.h
	lm-sock.c
	lm-sock.h
	lm-socket.c
	lm-socket.h
	lm-socket-address.c
	lm-socket-address.h
	#lm-gnutls-socket.c
	#lm-openssl-socket.c
	#lm-openssl-socket.h
	lm-resolver.c
	lm-resolver.h
	lm-asyncns-resolver.c
	lm-asyncns-resolver.h
	lm-blocking-resolver.c
	lm-blocking-resolver.h
)

add_executable(test test.c ${SOURCES})
target_link_libraries(test ${LM_LIBRARIES} 'resolv')

add_executable(test-connect test-connect.c ${SOURCES})
target_link_libraries(test-connect ${LM_LIBRARIES} 'resolv')

include_directories("." ${LM_INCLUDE_DIRS})
link_directories(${LM_LIBRARY_DIRS})

genmarshal("lm")

