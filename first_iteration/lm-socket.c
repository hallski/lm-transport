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
#include "lm-old-socket.h"
#include "lm-socket.h"

static void    socket_base_init (LmSocketIface *iface);

enum {
    CONNECTED,
    CONNECT_ERROR,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

GType
lm_socket_get_type (void)
{
    static GType iface_type = 0;

    if (!iface_type) {
        static const GTypeInfo iface_info = {
            sizeof (LmSocketIface),
            (GBaseInitFunc)     socket_base_init,
            (GBaseFinalizeFunc) NULL,
        };

        iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                             "LmSocketIface",
                                             &iface_info,
                                             0);

        g_type_interface_add_prerequisite (iface_type, G_TYPE_OBJECT);
    }

    return iface_type;
}

static void
socket_base_init (LmSocketIface *iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        signals[CONNECTED] =
            g_signal_new ("connected",
                          LM_TYPE_SOCKET,
                          G_SIGNAL_RUN_LAST,
                          0,
                          NULL, NULL,
                          _lm_marshal_VOID__VOID,
                          G_TYPE_NONE,
                          0);
        signals[CONNECT_ERROR] =
            g_signal_new ("connect-error",
                          LM_TYPE_SOCKET,
                          G_SIGNAL_RUN_LAST,
                          0,
                          NULL, NULL,
                          _lm_marshal_VOID__VOID,
                          G_TYPE_NONE,
                          1, G_TYPE_INT);
        initialized = TRUE;
    }
}

LmSocket *
lm_socket_create (GMainContext *context)
{
    return lm_old_socket_new (context);
}

void
lm_socket_connect (LmSocket        *socket,
                   struct addrinfo *addresses,
                   int              port)
{
    g_return_if_fail (LM_IS_SOCKET (socket));

    if (!LM_SOCKET_GET_IFACE(socket)->connect) {
        g_assert_not_reached ();
    }

    return LM_SOCKET_GET_IFACE(socket)->connect (socket, addresses, port);
}


