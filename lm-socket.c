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

#include <unistd.h>

#include "lm-marshal.h"
#include "lm-socket.h"

#ifndef G_OS_WIN32
typedef int SocketHandle;
#else  /* G_OS_WIN32 */
typedef SOCKET SocketHandle;
#endif /* G_OS_WIN32 */

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_SOCKET, LmSocketPriv))

typedef struct LmSocketPriv LmSocketPriv;
struct LmSocketPriv {
    GMainContext    *context;
    LmSocketAddress *sa;

    SocketHandle     handle;
    GIOChannel      *io_channel;

    gint             my_prop;
};

static void      socket_finalize            (GObject           *object);
static void      socket_get_property        (GObject           *object,
                                             guint              param_id,
                                             GValue            *value,
                                             GParamSpec        *pspec);
static void      socket_set_property        (GObject           *object,
                                             guint              param_id,
                                             const GValue      *value,
                                             GParamSpec        *pspec);
static void      socket_do_connect          (LmSocket          *socket);
static void      socket_do_close            (LmSocket          *socket);
static GIOStatus socket_do_read             (LmSocket          *socket,
                                             gchar             *buf,
                                             gsize              len,
                                             gsize             *read_len,
                                             GError           **error);
static GIOStatus socket_do_write            (LmSocket          *socket,
                                             gchar             *buf,
                                             gsize              len,
                                             gsize             *written_len,
                                             GError           **error);

G_DEFINE_TYPE (LmSocket, lm_socket, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_CONTEXT,
    PROP_MY_PROP
};

enum {
    CONNECTED,
    READABLE,
    WRITABLE,
    DISCONNECTED,
    ERROR,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
lm_socket_class_init (LmSocketClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    GParamSpec   *pspec;

    class->connect             = socket_do_connect;
    class->close               = socket_do_close;
    class->read                = socket_do_read;
    class->write               = socket_do_write;

    object_class->finalize     = socket_finalize;
    object_class->get_property = socket_get_property;
    object_class->set_property = socket_set_property;

    pspec = g_param_spec_pointer ("context",
                                  "Main context",
                                  "GMainContext to run this socket",
                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_CONTEXT, pspec);

    signals[CONNECTED] = 
        g_signal_new ("connected",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__INT,
                      G_TYPE_NONE, 
                      1, G_TYPE_INT);
     signals[DISCONNECTED] = 
        g_signal_new ("disconnected",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__INT,
                      G_TYPE_NONE, 
                      1, G_TYPE_INT);
     signals[READABLE] = 
        g_signal_new ("readable",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__INT,
                      G_TYPE_NONE, 
                      1, G_TYPE_INT);
    signals[WRITABLE] = 
        g_signal_new ("writable",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__INT,
                      G_TYPE_NONE, 
                      1, G_TYPE_INT);
    signals[ERROR] = 
        g_signal_new ("error",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__INT,
                      G_TYPE_NONE, 
                      1, G_TYPE_INT);
    
    g_type_class_add_private (object_class, sizeof (LmSocketPriv));
}

static void
lm_socket_init (LmSocket *socket)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (socket);
}

static void
socket_finalize (GObject *object)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (object);

    if (priv->sa) {
        lm_socket_address_unref (priv->sa);
    }

    (G_OBJECT_CLASS (lm_socket_parent_class)->finalize) (object);
}

static void
socket_get_property (GObject    *object,
                    guint       param_id,
                    GValue     *value,
                    GParamSpec *pspec)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (object);

    switch (param_id) {
    case PROP_MY_PROP:
        g_value_set_int (value, priv->my_prop);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    };
}

static void
socket_set_property (GObject      *object,
                    guint         param_id,
                    const GValue *value,
                    GParamSpec   *pspec)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (object);

    switch (param_id) {
    case PROP_MY_PROP:
        priv->my_prop = g_value_get_int (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    };
}

static void
socket_do_connect (LmSocket *socket)
{
    /* Do all the cruft involved */
}

static void
socket_do_close (LmSocket *socket)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (socket);

#ifndef G_OS_WIN32
    close (priv->handle);
#else  /* G_OS_WIN32 */
    closesocket (priv->handle);
#endif /* G_OS_WIN32 */
}

static GIOStatus
socket_do_read (LmSocket  *socket,
                gchar     *buf,
                gsize      len,
                gsize     *read_len,
                GError   **error)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (socket);

    return g_io_channel_read_chars (priv->io_channel, 
                                    buf, len, read_len, error);
}

static GIOStatus
socket_do_write (LmSocket  *socket,
                 gchar     *buf,
                 gsize      len,
                 gsize     *written_len,
                 GError   **error)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (socket);

    return g_io_channel_write_chars (priv->io_channel,
                                     buf, len, written_len, error);
}

LmSocket * 
lm_socket_new (LmSocketAddress *address, GMainContext *context)
{
    LmSocket     *socket;
    LmSocketPriv *priv;

    socket = g_object_new (LM_TYPE_SOCKET, "context", context, NULL);
    priv   = GET_PRIV (socket);

    priv->sa = lm_socket_address_ref (address);

    return socket;
}

/* Support proxy here
 * LmSocket * lm_socket_new_with_proxy (LmSocketAddress *address); 
 */

void
lm_socket_connect (LmSocket *socket)
{
    if (!LM_SOCKET_GET_CLASS(socket)->connect) {
        g_assert_not_reached ();
    }

    LM_SOCKET_GET_CLASS(socket)->connect (socket);
}

void
lm_socket_close (LmSocket *socket)
{
    if (!LM_SOCKET_GET_CLASS(socket)->close) {
        g_assert_not_reached ();
    }

    LM_SOCKET_GET_CLASS(socket)->close (socket);
}

GIOStatus
lm_socket_read (LmSocket  *socket,
                gchar     *buf,
                gsize      len,
                gsize     *read_len,
                GError   **error)
{
     if (!LM_SOCKET_GET_CLASS(socket)->read) {
         g_assert_not_reached ();
     }

     LM_SOCKET_GET_CLASS(socket)->read (socket, buf, len, read_len, error);

     return G_IO_STATUS_NORMAL;
}

GIOStatus
lm_socket_write (LmSocket  *socket,
                 gchar     *buf,
                 gsize      len,
                 gsize     *written_len,
                 GError   **error)
{
    if (!LM_SOCKET_GET_CLASS(socket)->write) {
        g_assert_not_reached ();
    }

    LM_SOCKET_GET_CLASS(socket)->write (socket, buf, 
                                        len, written_len, error);

    return G_IO_STATUS_NORMAL;
}

