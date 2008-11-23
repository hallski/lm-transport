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
#include "lm-socket.h"
#include "lm-transport.h"
#include "lm-old-socket.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_OLD_SOCKET, LmOldSocketPriv))

typedef struct LmOldSocketPriv LmOldSocketPriv;
struct LmOldSocketPriv {
    gint fd;
};

static void     old_socket_iface_init           (LmSocketIface     *iface);
static void     old_socket_transport_iface_init (LmTransportIface  *iface);
static void     old_socket_finalize             (GObject           *object);
static void     old_socket_connect              (LmSocket          *socket,
                                                 struct addrinfo   *addresses,
                                                 int                port);
static int      old_socket_get_handle           (LmTransport       *transport);
static GIOError old_socket_read                 (LmTransport       *transport,
                                                 void              *buf,
                                                 gsize              len,
                                                 gsize             *bytes_read);
static void     old_socket_write                (LmTransport       *transport,
                                                 void              *buf,
                                                 gsize              len);
static void     old_socket_disconnect           (LmTransport       *transport);


G_DEFINE_TYPE_WITH_CODE (LmOldSocket, lm_old_socket, G_TYPE_OBJECT,
                         {
                         G_IMPLEMENT_INTERFACE (LM_TYPE_SOCKET,
                                                old_socket_iface_init);
                         G_IMPLEMENT_INTERFACE (LM_TYPE_TRANSPORT,
                                                old_socket_transport_iface_init);
                         })

static void
lm_old_socket_class_init (LmOldSocketClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    object_class->finalize     = old_socket_finalize;

    g_type_class_add_private (object_class, sizeof (LmOldSocketPriv));
}

static void
lm_old_socket_init (LmOldSocket *old_socket)
{
    LmOldSocketPriv *priv;

    priv = GET_PRIV (old_socket);
}

static void
old_socket_iface_init (LmSocketIface *iface)
{
    iface->connect = old_socket_connect;
}

static void
old_socket_transport_iface_init (LmTransportIface *iface)
{
    iface->get_handle = old_socket_get_handle;
    iface->read       = old_socket_read;
    iface->write      = old_socket_write;
    iface->disconnect = old_socket_disconnect;
}

static void
old_socket_finalize (GObject *object)
{
    LmOldSocketPriv *priv;

    priv = GET_PRIV (object);

    (G_OBJECT_CLASS (lm_old_socket_parent_class)->finalize) (object);
}

static void
old_socket_connect (LmSocket        *socket,
                    struct addrinfo *addresses,
                    int              port)
{
}

static int
old_socket_get_handle (LmTransport *transport)
{
    return 0;
}

static GIOError
old_socket_read (LmTransport *transport,
                 void        *buf,
                 gsize        len,
                 gsize       *bytes_read)
{
    return G_IO_ERROR_UNKNOWN;
}

static void
old_socket_write (LmTransport *transport, void *buf, gsize len)
{
}

static void
old_socket_disconnect (LmTransport *transport)
{
}

