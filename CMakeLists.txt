cmake_minimum_required(VERSION 2.6)

project(LmTransports C)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(PkgConfig REQUIRED)

include(Warnings.cmake)
include(GenMarshal.cmake)
include(SelectSSL.cmake)

include(ConfigureChecks.cmake)
configure_file(config.h.cmake config.h)

pkg_check_modules(LM REQUIRED
	glib-2.0
	gobject-2.0)

set(LM_LIBRARIES ${LM_LIBRARIES} ${SSL_LIBRARIES})
set(LM_INCLUDE_DIRS ${LM_INCLUDE_DIRS} ${SSL_INCLUDE_DIRS})
set(LM_LIBRARY_DIRS ${LM_LIBRARY_DIRS} ${SSL_LIBRARY_DIRS})

set(SOURCES
	asyncns.c
	asyncns.h
	lm-channel.c
	lm-channel.h
	lm-dummy.c
	lm-dummy.h
	lm-error.c
	lm-error.h
	lm-gnutls-channel.c
	lm-gnutls-channel.h
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

add_executable(test-secure test-secure.c ${SOURCES})
target_link_libraries(test-secure ${LM_LIBRARIES} 'resolv')

include_directories("." ${LM_INCLUDE_DIRS})
link_directories(${LM_LIBRARY_DIRS})

genmarshal("lm")

