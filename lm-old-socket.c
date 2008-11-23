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

#include "lm-debug.h"
#include "lm-marshal.h"
#include "lm-misc.h"
#include "lm-sock.h"
#include "lm-socket.h"
#include "lm-transport.h"
#include "lm-old-socket.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_OLD_SOCKET, LmOldSocketPriv))

typedef struct LmOldSocketPriv LmOldSocketPriv;
struct LmOldSocketPriv {
    GMainContext      *context;

    struct addrinfo   *addresses;
    guint              port;

    LmOldSocketT       fd;

    GIOChannel        *io_channel;
    GSource           *watch_in;
    GSource           *watch_err;
    GSource           *watch_hup;
    GSource           *watch_out;

    GString           *out_buf;
    
    GSource           *watch_connect;
    gboolean           cancel_open;
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
static gint     old_socket_do_write             (LmOldSocket       *socket,
                                                 const gchar       *buf, 
                                                 guint              len);


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

    if (priv->context) {
        g_main_context_unref (priv->context);
    }

    if (priv->addresses) {
        freeaddrinfo (priv->addresses);
    }

    if (priv->out_buf) {
        g_string_free (priv->out_buf, TRUE);
    }

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

static gboolean
old_socket_output_is_buffered (LmOldSocket     *socket,
                               const gchar  *buffer,
                               gint          len)
{
    LmOldSocketPriv *priv;
    
    priv = GET_PRIV (socket);

    if (priv->out_buf) {
        lm_verbose ("Appending %d bytes to output buffer\n", len);
        g_string_append_len (priv->out_buf, buffer, len);
        return TRUE;
    }

    return FALSE;
}

static gboolean
socket_buffered_write_cb (GIOChannel   *source, 
                          GIOCondition  condition,
                          LmOldSocket     *socket)
{
    LmOldSocketPriv *priv;
    gint             b_written;
    GString         *out_buf;

    priv = GET_PRIV (socket);

    out_buf = priv->out_buf;
    if (!out_buf) {
        /* Should not be possible */
        return FALSE;
    }

    b_written = old_socket_do_write (socket, out_buf->str, out_buf->len);

    if (b_written < 0) {
        g_signal_emit_by_name (socket, "disconnected", 
                               LM_DISCONNECT_REASON_ERROR);
        return FALSE;
    }

    g_string_erase (out_buf, 0, (gsize) b_written);
    if (out_buf->len == 0) {
        lm_verbose ("Output buffer is empty, going back to normal output\n");

        if (priv->watch_out) {
            g_source_destroy (priv->watch_out);
            priv->watch_out = NULL;
        }

        g_string_free (out_buf, TRUE);
        priv->out_buf = NULL;
        return FALSE;
    }

    return TRUE;
}

static void
old_socket_setup_output_buffer (LmOldSocket *socket, 
                                const gchar *buffer, 
                                gint         len)
{
    LmOldSocketPriv *priv;

    priv = GET_PRIV (socket);

    lm_verbose ("OUTPUT BUFFER ENABLED\n");

    priv->out_buf = g_string_new_len (buffer, len);

    priv->watch_out =
        lm_misc_add_io_watch (priv->context,
                              priv->io_channel,
                              G_IO_OUT,
                              (GIOFunc) socket_buffered_write_cb,
                              socket);
}

static gint
old_socket_do_write (LmOldSocket *socket, const gchar *buf, guint len)
{
    LmOldSocketPriv *priv;
    gint      b_written;
    GIOStatus io_status = G_IO_STATUS_AGAIN;
    gsize     bytes_written;

    priv = GET_PRIV (socket);

    while (io_status == G_IO_STATUS_AGAIN) {
        io_status = g_io_channel_write_chars (priv->io_channel, 
                                              buf, len, 
                                              &bytes_written,
                                              NULL);
    }

    b_written = bytes_written;

    if (io_status != G_IO_STATUS_NORMAL) {
        b_written = -1;
    }

    return b_written;
}

static void
old_socket_write (LmTransport *transport, void *buf, gsize len)
{
    LmOldSocket *socket = LM_OLD_SOCKET (transport);
    gint         b_written;

    if (old_socket_output_is_buffered (socket, buf, len)) {
        return;
    }

    b_written = old_socket_do_write (socket, buf, len);

    if (b_written < len && b_written != -1) {
        old_socket_setup_output_buffer (socket,
                                        (char*)buf + b_written,
                                        len - b_written);
        return;
    }
        
    return;
}

static void
old_socket_disconnect (LmTransport *transport)
{
}

LmSocket *
lm_old_socket_new (GMainContext *context)
{
    LmOldSocket     *socket;
    LmOldSocketPriv *priv;
    
    socket = g_object_new (LM_TYPE_OLD_SOCKET, NULL);
    priv = GET_PRIV (socket);

    priv->context = g_main_context_ref (context);

    return LM_SOCKET (socket);
}

