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

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef G_OS_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif /* G_OS_WIN32 */

#include "lm-channel.h"
#include "lm-marshal.h"
#include "lm-misc.h"
#include "lm-resolver.h"
#include "lm-sock.h"
#include "lm-socket.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_SOCKET, LmSocketPriv))

typedef struct LmSocketPriv LmSocketPriv;
struct LmSocketPriv {
    GMainContext        *context;
    LmSocketAddress     *sa;

    LmSocketHandle       handle;
    GIOChannel          *io_channel;
    GSource             *io_watch;

    gboolean             connected;

    /* DNS Lookup */
    LmResolver          *resolver;

    /* Connect */
    LmSocketAddressIter *sa_iter;
};

static void      socket_channel_iface_init  (LmChannelIface    *iface);
static void      socket_finalize            (GObject           *object);
static void      socket_get_property        (GObject           *object,
                                             guint              param_id,
                                             GValue            *value,
                                             GParamSpec        *pspec);
static void      socket_set_property        (GObject           *object,
                                             guint              param_id,
                                             const GValue      *value,
                                             GParamSpec        *pspec);
static GIOStatus socket_read                (LmChannel         *channel,
                                             gchar             *buf,
                                             gsize              len,
                                             gsize             *read_len,
                                             GError           **error);
static GIOStatus socket_write               (LmChannel         *channel,
                                             const gchar       *buf,
                                             gssize             len,
                                             gsize             *written_len,
                                             GError           **error);
static void      socket_close               (LmChannel         *channel);
static void      socket_attempt_connect_next(LmSocket          *socket);
static gboolean  socket_attempt_connect     (LmSocket          *socket,
                                             struct addrinfo   *addr);
static gboolean  socket_io_cb               (GIOChannel        *channel,
                                             GIOCondition       condition,
                                             LmSocket          *socket);

G_DEFINE_TYPE_WITH_CODE (LmSocket, lm_socket, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LM_TYPE_CHANNEL,
                                                socket_channel_iface_init))

enum {
    PROP_0,
    PROP_CONTEXT,
    PROP_ADDRESS
};

enum {
    CONNECT_RESULT,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
lm_socket_class_init (LmSocketClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    GParamSpec   *pspec;

    object_class->finalize     = socket_finalize;
    object_class->get_property = socket_get_property;
    object_class->set_property = socket_set_property;

    pspec = g_param_spec_pointer ("context",
                                  "Main context",
                                  "GMainContext to run this socket",
                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_CONTEXT, pspec);

    pspec = g_param_spec_boxed ("address",
                                "Socket address",
                                "The socket address of the remote host",
                                LM_TYPE_SOCKET_ADDRESS,
                                G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_ADDRESS, pspec);

    signals[CONNECT_RESULT] = 
        g_signal_new ("connect_result",
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

    priv->connected = FALSE;
}

static void
socket_channel_iface_init (LmChannelIface *iface)
{
    iface->read  = socket_read;
    iface->write = socket_write;
    iface->close = socket_close;
}

static void
socket_finalize (GObject *object)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (object);

    if (priv->sa) {
        lm_socket_address_unref (priv->sa);
    }

    if (priv->connected) {
        /* TODO: disconnect and what not */
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
        case PROP_CONTEXT:
            g_value_set_pointer (value, priv->context);
            break;
        case PROP_ADDRESS:
            g_value_set_boxed (value, priv->sa);
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
    GMainContext *context;

    priv = GET_PRIV (object);

    switch (param_id) {
        case PROP_CONTEXT:
            context = g_value_get_pointer (value);
            if (context) {
                priv->context = g_main_context_ref (context);
            }
            break;
        case PROP_ADDRESS:
            priv->sa = g_value_get_boxed (value);
            if (priv->sa) {
                lm_socket_address_ref (priv->sa);
            }
            break;
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    };
}

static void
socket_close (LmChannel *channel)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (channel);

    _lm_sock_close (priv->handle);
}

static GIOStatus
socket_read (LmChannel *channel,
             gchar     *buf,
             gsize      len,
             gsize     *read_len,
             GError   **error)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (channel);

    return g_io_channel_read_chars (priv->io_channel, 
                                    buf, len, read_len, error);
}

static GIOStatus
socket_write (LmChannel    *channel,
              const gchar  *buf,
              gssize        len,
              gsize        *written_len,
              GError      **error)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (channel);

    return g_io_channel_write_chars (priv->io_channel,
                                     buf, len, written_len, error);
}

static void
socket_resolver_finished_cb (LmResolver       *resolver,
                             LmResolverResult  result,
                             LmSocketAddress  *address,
                             LmSocket         *socket)
{
    if (result != LM_RESOLVER_RESULT_OK) {
        g_warning ("Failed to lookup host: %s\n",
                   lm_socket_address_get_host (address));

        return;
    }

    socket_attempt_connect_next (socket);
}

static void
socket_emit_connect_result (LmSocket *socket, gboolean result)
{
    g_signal_emit (socket, signals[CONNECT_RESULT], 0, result);
}

static void
socket_attempt_connect_next (LmSocket *socket)
{
    LmSocketPriv    *priv = GET_PRIV (socket);
    struct addrinfo *addr;

    if (!priv->sa_iter) {
        priv->sa_iter = lm_socket_address_get_result_iter (priv->sa);
    }

    while (TRUE) {
        addr = lm_socket_address_iter_get_next (priv->sa_iter);
        if (!addr) {
            g_warning ("Failed to connect, phase 0");
            socket_emit_connect_result (socket, FALSE);
            break;
        }

        if (!socket_attempt_connect (socket, addr)) {
            g_warning ("Failed to connect, %s", G_STRFUNC);
            /* Try next */
        } else {
            break;
        }
    }
}

#define IO_CONDITION_ALL (G_IO_OUT | G_IO_ERR | G_IO_IN | G_IO_PRI | G_IO_HUP)

static gboolean
socket_attempt_connect (LmSocket *lm_socket, struct addrinfo *addr)
{
    LmSocketPriv    *priv;
    int              res;

    priv = GET_PRIV (lm_socket);

    priv->handle = (LmSocketHandle)socket (addr->ai_family, 
                                           addr->ai_socktype, 
                                           addr->ai_protocol);

    if (!_LM_SOCK_VALID (priv->handle)) {
        g_warning ("Failed to connect, phase 1");
        return FALSE;
    }

    priv->io_channel = g_io_channel_unix_new (priv->handle);

    g_io_channel_set_encoding (priv->io_channel, NULL, NULL);
    g_io_channel_set_buffered (priv->io_channel, FALSE);

    _lm_sock_set_blocking (priv->handle, FALSE);

    priv->io_watch = lm_misc_add_io_watch (priv->context,
                                           priv->io_channel,
                                           IO_CONDITION_ALL,
                                           (GIOFunc) socket_io_cb,
                                           lm_socket);

    res = connect (priv->handle, addr->ai_addr, (int)addr->ai_addrlen);
    if (res < 0) {
        int err;
        err = _lm_sock_get_last_error ();
        if (!_lm_sock_is_blocking_error (err)) {
            g_warning ("Failed to connect, phase 2 (lm-old-socket.c:662)");
            _lm_sock_close (priv->handle);
            priv->handle = 0;
            return FALSE;
        }
    }

    /* TODO: Add a timeout here? */
    return TRUE;
}

static gboolean
socket_handle_connect_reply (LmSocket *socket, gboolean out_event)
{
    LmSocketPriv *priv = GET_PRIV (socket);
    
    if (out_event) {
        priv->connected = TRUE;
        socket_emit_connect_result (socket, TRUE);
        return TRUE;
    } else {
        socklen_t len;
        int       err;
        len = sizeof (err);
        _lm_sock_get_error (priv->handle, &err, &len);
        if (!_lm_sock_is_blocking_error (err)) {
            g_warning ("Connection failed\n");
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean
socket_io_cb (GIOChannel   *channel,
              GIOCondition  condition,
              LmSocket     *socket)
{
    LmSocketPriv *priv = GET_PRIV (socket);
    gboolean      ret_val = TRUE;

    switch (condition) {
        case G_IO_IN:
        case G_IO_PRI:
            g_signal_emit_by_name (socket, "readable");
            break;
        case G_IO_OUT:
            if (priv->connected) {
                g_signal_emit_by_name (socket, "writeable");
            } else {
                ret_val = !socket_handle_connect_reply (socket, TRUE);
            }
            break;
        case G_IO_ERR:
        case G_IO_NVAL:
            if (priv->connected) {
                g_signal_emit_by_name (socket, "disconnected", /* TODO: ERROR */ 0);
                ret_val = FALSE;
            } else {
                ret_val = !socket_handle_connect_reply (socket, FALSE);
            }
            break;
        case G_IO_HUP:
            g_signal_emit_by_name (socket, "disconnected", /* TODO: HUP */ 0);
            ret_val = FALSE;
            /* TODO: Clean up */
            break;
    };

    return ret_val;
}

/* -- Public API -- */
LmSocket * 
lm_socket_new (GMainContext *context, LmSocketAddress *address)
{
    LmSocket *socket;

    socket = g_object_new (LM_TYPE_SOCKET, 
                           "context", context,
                           "address", address, 
                           NULL);
    return socket;
}

void
lm_socket_connect (LmSocket *socket)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (socket);

    if (!lm_socket_address_is_resolved (priv->sa)) {
        priv->resolver = lm_resolver_lookup_host (priv->context, priv->sa);
        g_signal_connect (priv->resolver, "finished", 
                          G_CALLBACK (socket_resolver_finished_cb),
                          socket);
    } else {
        socket_attempt_connect_next (socket);
    }
}

