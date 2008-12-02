option(WITH_GNUTLS "Whether to use GnuTLS for TLS support" ON)
option(WITH_OPENSSL "Whether to use OpenSSL for TLS support" OFF)

if(WITH_GNUTLS AND WITH_OPENSSL)
	message(FATAL_ERROR "Only GnuTLS OR OpenSSL can be selected")
endif(WITH_GNUTLS AND WITH_OPENSSL)

if(WITH_GNUTLS)
	find_package(GnuTLS)
	if(NOT GNUTLS_FOUND)
		message(FATAL_ERROR "GnuTLS was selected but not found")
	endif(NOT GNUTLS_FOUND)
	set(HAVE_GNUTLS 1)
	set(SSL_INCLUDE_DIRS ${GNUTLS_INCLUDE_DIRS})
	set(SSL_LIBRARIES ${GNUTLS_LIBRARIES})
endif(WITH_GNUTLS)

if(WITH_OPENSSL)
	find_package(OpenSSL)
	if(NOT OPENSSL_FOUND)
		message(FATAL_ERROR "OpenSSL was selected but not found")
	endif(NOT OPENSSL_FOUND)
	set(HAVE_OPENSSL 1)
	set(SSL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIRS})
	set(SSL_LIBRARIES ${OPENSSL_LIBRARIES})
endif(WITH_OPENSSL)

if(HAVE_GNUTLS OR HAVE_OPENSSL)
	set(HAVE_SSL 1)
endif(HAVE_GNUTLS OR HAVE_OPENSSL)


