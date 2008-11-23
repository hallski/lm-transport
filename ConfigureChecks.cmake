include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)

# TODO: Handle;
# HAVE_ASYNCNS
# HAVE_GNUTLS
# HAVE_GSSAPI
# HAVE_GSSAPI_GSSAPI_H
# HAVE_OPENSSL
# HAVE_OPENSSL_SSL_H
# HAVE_SSL
# HAVE_TIMEZONE
# HAVE_TM_GMTOFF
# USE_TCP_KEEPALIVES

# Currently only looks for headers that set a define that is actually use.
check_include_files(arpa/inet.h HAVE_ARPA_INET_H)
check_include_files(arpa/nameser_compat.h HAVE_ARPA_NAMESER_COMPAT_H)
check_include_files(netinet/in.h HAVE_NETINET_IN_H)
check_include_files(netinet/in_systm.h HAVE_NETINET_IN_SYSTM_H)

check_library_exists(nsl gethostbyname 
	"/lib;/usr/lib;/usr/local/lib" HAVE_NSLLIB)
check_library_exists(socket socket
	"/lib;/usr/lib;/usr/local/lib" HAVE_SOCKETLIB)
check_library_exists(resolv res_query
	"/lib;/usr/lib;/usr/local/lib" HAVE_RESOLVLIB)
	
