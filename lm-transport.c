/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2008 Imendio AB
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include "lm-marshal.h"
#include "lm-transport.h"

static void    transport_base_init (LmTransportIface *iface);

enum {
    READABLE,
    DISCONNECTED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

GType
lm_transport_get_type (void)
{
    static GType iface_type = 0;

    if (!iface_type) {
        static const GTypeInfo iface_info = {
            sizeof (LmTransportIface),
            (GBaseInitFunc)     transport_base_init,
            (GBaseFinalizeFunc) NULL,
        };

        iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                             "LmTransportIface",
                                             &iface_info,
                                             0);

        g_type_interface_add_prerequisite (iface_type, G_TYPE_OBJECT);
    }

    return iface_type;
}

static void
transport_base_init (LmTransportIface *iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        signals[READABLE] =
            g_signal_new ("readable",
                          LM_TYPE_TRANSPORT,
                          G_SIGNAL_RUN_LAST,
                          0,
                          NULL, NULL,
                          _lm_marshal_VOID__VOID,
                          G_TYPE_NONE,
                          0);
        signals[DISCONNECTED] =
            g_signal_new ("disconnected",
                          LM_TYPE_TRANSPORT,
                          G_SIGNAL_RUN_LAST,
                          0,
                          NULL, NULL,
                          _lm_marshal_VOID__INT,
                          G_TYPE_NONE,
                          1, G_TYPE_INT);
        initialized = TRUE;
    }
}

int
lm_transport_get_handle (LmTransport *transport)
{
    g_return_val_if_fail (LM_IS_TRANSPORT (transport), -1);

    if (!LM_TRANSPORT_GET_IFACE(transport)->get_handle) {
        g_assert_not_reached ();
    }

    return LM_TRANSPORT_GET_IFACE(transport)->get_handle (transport);
}

GIOError
lm_transport_read (LmTransport *transport,
                   void        *buf,
                   gsize        len,
                   gsize       *bytes_read)
{
    g_return_val_if_fail (LM_IS_TRANSPORT (transport), G_IO_ERROR_UNKNOWN);

    if (!LM_TRANSPORT_GET_IFACE(transport)->read) {
        g_assert_not_reached ();
    }

    return LM_TRANSPORT_GET_IFACE(transport)->read (transport, buf, len, 
                                                    bytes_read);
}

void
lm_transport_write (LmTransport *transport,
                    void        *buf,
                    gsize        len)
{
    g_return_if_fail (LM_IS_TRANSPORT (transport));

    if (!LM_TRANSPORT_GET_IFACE(transport)->write) {
        g_assert_not_reached ();
    }

    return LM_TRANSPORT_GET_IFACE(transport)->write (transport, buf, len);
}

void
lm_transport_disconnect (LmTransport *transport)
{
    g_return_if_fail (LM_IS_TRANSPORT (transport));

    if (!LM_TRANSPORT_GET_IFACE(transport)->disconnect) {
        g_assert_not_reached ();
    }

    return LM_TRANSPORT_GET_IFACE(transport)->disconnect (transport);
}
