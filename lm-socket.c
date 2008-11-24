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

#include "asyncns.h"
#include "lm-marshal.h"
#include "lm-misc.h"
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

    asyncns_t       *asyncns_ctx;
    asyncns_query_t *asyncns_query;
    GSource         *resolve_watch;
    GIOChannel      *resolve_channel;

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
static void 
socket_resolve_address_and_connect          (LmSocket          *socket);
static gboolean  socket_resolver_io_cb      (GSource           *source,
                                             GIOCondition       condition,
                                             LmSocket          *socket);
static void      socket_resolved_connect    (LmSocket          *socket);

G_DEFINE_TYPE (LmSocket, lm_socket, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_CONTEXT,
    PROP_ADDRESS
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

    pspec = g_param_spec_boxed ("address",
                                "Socket address",
                                "The socket address of the remote host",
                                LM_TYPE_SOCKET_ADDRESS,
                                G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_ADDRESS, pspec);

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
            break;
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    };
}

static void
socket_do_connect (LmSocket *socket)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (socket);

    if (!lm_socket_address_is_resolved (priv->sa)) {
        socket_resolve_address_and_connect (socket);
    }

    socket_resolved_connect (socket);
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

static void
socket_resolve_address_and_connect (LmSocket *socket)
{
    LmSocketPriv    *priv;
    struct           addrinfo req;
    asyncns_t       *asyncns_ctx;
    asyncns_query_t *asyncns_query;
    GSource         *resolve_watch;
    GIOChannel      *resolve_channel;

    priv = GET_PRIV (socket);

    memset (&req, 0, sizeof(req));
    req.ai_family   = AF_UNSPEC;
    req.ai_socktype = SOCK_STREAM;
    req.ai_protocol = IPPROTO_TCP;
    
    asyncns_ctx = asyncns_new (1);

    resolve_channel = g_io_channel_unix_new (asyncns_fd (asyncns_ctx));

    resolve_watch = lm_misc_add_io_watch (priv->context,
                                          resolve_channel,
                                          G_IO_IN,
                                          (GIOFunc) socket_resolver_io_cb,
                                          socket);
    
    asyncns_query = asyncns_getaddrinfo (asyncns_ctx, 
                                         lm_socket_address_get_host (priv->sa),
                                         NULL, &req);

    priv->asyncns_ctx     = asyncns_ctx;
    priv->resolve_channel = resolve_channel;
    priv->resolve_watch   = resolve_watch;
    priv->asyncns_query   = asyncns_query;
}

static void
socket_cleanup_resolver (LmSocket *socket)
{
    LmSocketPriv *priv;

    priv = GET_PRIV (socket);

    asyncns_free (priv->asyncns_ctx);
    g_source_destroy (priv->resolve_watch);
    g_io_channel_unref (priv->resolve_channel);
    priv->asyncns_query = NULL;
}

static gboolean
socket_resolver_io_cb (GSource      *source,
                       GIOCondition  condition,
                       LmSocket     *socket)
{
    LmSocketPriv    *priv;
    struct addrinfo *ans;
    int              err;
    gboolean         ret_val = FALSE;

    priv = GET_PRIV (socket);

    asyncns_wait (priv->asyncns_ctx, FALSE);

    if (!asyncns_isdone (priv->asyncns_ctx, priv->asyncns_query)) {
        return TRUE;
    }

    err = asyncns_getaddrinfo_done (priv->asyncns_ctx, 
                                    priv->asyncns_query,
                                    &ans);

    if (err) {
        g_warning ("Error occurred during DNS lookup of %s. Should be signalled back to the user.",
                   lm_socket_address_get_host (priv->sa));

        ret_val = FALSE;
    } else {
        g_print ("Successful lookup of hostname: %s\n", 
                 lm_socket_address_get_host (priv->sa));
        lm_socket_address_set_results (priv->sa, ans);
        ret_val = TRUE;
    }

    socket_cleanup_resolver (socket);

    if (ret_val) {
        socket_resolved_connect (socket);
    }

    return ret_val;
}

static void
socket_resolved_connect (LmSocket *socket)
{
    /* */
}

/* -- Public API -- */
LmSocket * 
lm_socket_new (LmSocketAddress *address, GMainContext *context)
{
    LmSocket *socket;

    socket = g_object_new (LM_TYPE_SOCKET, 
                           "context", context,
                           "address", address, 
                           NULL);
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

