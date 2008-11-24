/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#include "lm-socket-address.h"

struct LmSocketAddress {
    gchar *hostname;
    guint  port;

    /* Add result iterator here */

    guint  ref_count;
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

