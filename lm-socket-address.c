/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#include "lm-socket-address.h"

struct LmSocketAddress {
    gchar *hostname;
    guint  port;

    guint  ref_count;
};

LmSocketAddress *
lm_socket_address_new (const gchar *hostname, guint port)
{
    LmSocketAddress *sa;

    sa = g_slice_new0 (LmSocketAddress);
    sa->hostname = g_strdup (hostname);
    sa->port = port;

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
        g_slice_free (LmSocketAddress, sa);
    }
}

