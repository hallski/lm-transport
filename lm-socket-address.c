/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#include <config.h>

/* Needed on Mac OS X */
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "lm-resolver.h"
#include "lm-socket-address.h"

struct LmSocketAddress {
    gchar               *hostname;
    guint                port;

    /* Add result iterator here */
    struct addrinfo     *results;
    LmSocketAddressIter *results_iter;

    guint                ref_count;
};

struct LmSocketAddressIter {
    LmSocketAddress *sa;
    struct addrinfo *current;
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

LmSocketAddressIter *
lm_socket_address_get_result_iter (LmSocketAddress *sa)
{
    if (!sa->results_iter) {
        sa->results_iter = g_slice_new0 (LmSocketAddressIter);
        sa->results_iter->sa = sa;
        sa->results_iter->current = sa->results;
    } else {
        /* Just reset it */
        sa->results_iter->current = sa->results;
    }

    return sa->results_iter;
}

LmSocketAddress *
lm_socket_address_ref (LmSocketAddress *sa)
{
    sa->ref_count++;

    /* g_print ("SA_REFCOUNT = %d\n", sa->ref_count); */

    return sa;
}

void
lm_socket_address_unref (LmSocketAddress *sa)
{
    sa->ref_count--;

    /* g_print ("SA_REFCOUNT = %d\n", sa->ref_count); */
    
    if (sa->ref_count == 0) {
        g_free (sa->hostname);
        if (sa->results) {
            lm_resolver_freeaddrinfo (sa->results);
        }

        g_slice_free (LmSocketAddress, sa);
    }
}

void
lm_socket_address_set_results (LmSocketAddress *sa, struct addrinfo *ai)
{
    struct addrinfo *addr;

    g_return_if_fail (sa != NULL);

    sa->results = ai;

    /* Set the lower level sockaddr_in port on all results */
    addr = ai;
    while (addr) {
        int port;
        port = htons (sa->port);
        ((struct sockaddr_in *) addr->ai_addr)->sin_port = port;

        addr = addr->ai_next;
    }
}

/* -- LmSocketAddressIter: Results iterator -- */
struct addrinfo *
lm_socket_address_iter_get_next (LmSocketAddressIter *iter)
{
    struct addrinfo *current = iter->current;
   
    if (iter->current) {
        iter->current = iter->current->ai_next;
    }

    return current;
}

void
lm_socket_address_iter_reset (LmSocketAddressIter *iter)
{
    iter->current = iter->sa->results;
}


