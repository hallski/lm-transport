/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#include <config.h>

/* Needed on Mac OS X */
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "asyncns.h"
#include "lm-socket-address.h"

struct LmSocketAddress {
    gchar           *hostname;
    guint            port;

    /* Add result iterator here */
    struct addrinfo *results;
    struct addrinfo *current;

    guint            ref_count;
};

GType
lm_socket_address_get_type (void)
{
    static GType  type;

    if (type == 0) {
        const gchar *str;

        str = g_intern_static_string ("LmSocketAddress");

        type = g_boxed_type_register_static (str, 
                                             (GBoxedCopyFunc) lm_socket_address_ref,
                                             (GBoxedFreeFunc) lm_socket_address_unref);
    }

    return type;
}

LmSocketAddress *
lm_socket_address_new (const gchar *hostname, guint port)
{
    LmSocketAddress *sa;

    sa = g_slice_new0 (LmSocketAddress);
    sa->hostname = g_strdup (hostname);
    sa->port = port;

    sa->ref_count = 1;

    return sa;
}

const gchar *
lm_socket_address_get_host (LmSocketAddress *sa)
{
    return sa->hostname;
}

guint
lm_socket_address_get_port (LmSocketAddress *sa)
{
    return sa->port;
}

gboolean 
lm_socket_address_is_resolved (LmSocketAddress *sa)
{
    return (sa->results != NULL);
}

LmSocketAddress *
lm_socket_address_ref (LmSocketAddress *sa)
{
    sa->ref_count++;

    return sa;
}

void
lm_socket_address_unref (LmSocketAddress *sa)
{
    sa->ref_count--;

    if (sa->ref_count == 0) {
        g_free (sa->hostname);
        if (sa->results) {
            asyncns_freeaddrinfo (sa->results);
        }

        g_slice_free (LmSocketAddress, sa);
    }
}

GType
lm_inet_address_get_type     (void)
{
    static GType  type;

    if (type == 0) {
        const gchar *str;

        str = g_intern_static_string ("LmInetAddress");

        type = g_boxed_type_register_static (str, 
                                             (GBoxedCopyFunc) lm_inet_address_ref,
                                             (GBoxedFreeFunc) lm_inet_address_unref);
    }

    return type;
}

struct LmInetAddress {
    struct addrinfo *ai;

    guint            ref_count;
};

struct addrinfo *
lm_inet_address_get_addrinfo (LmInetAddress *ia)
{
    return ia->ai;
}

LmInetAddress *
lm_inet_address_ref (LmInetAddress *ia)
{
    ia->ref_count++;

    return ia;
}

void
lm_inet_address_unref (LmInetAddress *ia)
{
    ia->ref_count--;

    if (ia->ref_count == 0) {
        /* Free addrinfo */
        g_slice_free (LmInetAddress, ia);
    }
}

void
lm_socket_address_set_results (LmSocketAddress *sa, struct addrinfo *ai)
{
    struct addrinfo *addr;

    sa->results = ai;
    sa->current = sa->results;

    /* Set the lower level sockaddr_in port on all results */
    addr = ai;
    while (addr) {
        int port;
        port = htons (sa->port);
        ((struct sockaddr_in *) addr->ai_addr)->sin_port = port;
        addr = addr->ai_next;
    }

}

LmInetAddress *
lm_inet_address_new (struct addrinfo *ai)
{
    LmInetAddress *ia;
    
    ia = g_slice_new0 (LmInetAddress);
    ia->ai = ai; /* Copy it */
    ia->ref_count = 1;

    return ia;
}


