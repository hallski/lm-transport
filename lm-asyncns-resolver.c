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
#include <asyncns.h>
#define freeaddrinfo(x) asyncns_freeaddrinfo(x)

/* Needed on Mac OS X */
#if HAVE_ARPA_NAMESER_COMPAT_H
#include <arpa/nameser_compat.h>
#endif

#include <arpa/nameser.h>
#include <resolv.h>

#include "lm-error.h"
#include "lm-marshal.h"
#include "lm-misc.h"
#include "lm-resolver.h"
#include "lm-asyncns-resolver.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_ASYNCNS_RESOLVER, LmAsyncnsResolverPriv))

enum {
    RESOLVE_TYPE_HOST,
    RESOLVE_TYPE_SRV
};

typedef struct LmAsyncnsResolverPriv LmAsyncnsResolverPriv;
struct LmAsyncnsResolverPriv {
    GSource     *watch_resolv;
    asyncns_query_t *resolv_query;
    asyncns_t   *asyncns_ctx;
    GIOChannel  *resolv_channel;

    gint         resolve_type;

    LmSocketAddress *sa;
};

static void     asyncns_resolver_finalize      (GObject          *object);
static void     asyncns_resolver_cleanup       (LmResolver       *resolver);
static void     asyncns_resolver_host_done     (LmResolver       *resolver);
static void     asyncns_resolver_srv_done      (LmResolver       *resolver);
static void     asyncns_resolver_lookup_host   (LmResolver       *resolver,
                                                LmSocketAddress  *sa);
static void     asyncns_resolver_lookup_srv    (LmResolver       *resolver,
                                                const gchar      *srv);
static void     asyncns_resolver_cancel        (LmResolver       *resolver);

G_DEFINE_TYPE (LmAsyncnsResolver, lm_asyncns_resolver, LM_TYPE_RESOLVER)

static void
lm_asyncns_resolver_class_init (LmAsyncnsResolverClass *class)
{
    GObjectClass    *object_class = G_OBJECT_CLASS (class);
    LmResolverClass *resolver_class = LM_RESOLVER_CLASS (class);

    object_class->finalize = asyncns_resolver_finalize;
    
    resolver_class->lookup_host = asyncns_resolver_lookup_host;
    resolver_class->lookup_srv  = asyncns_resolver_lookup_srv;
    resolver_class->cancel      = asyncns_resolver_cancel;

    g_type_class_add_private (object_class, sizeof (LmAsyncnsResolverPriv));
}

static void
lm_asyncns_resolver_init (LmAsyncnsResolver *asyncns_resolver)
{
    LmAsyncnsResolverPriv *priv;

    priv = GET_PRIV (asyncns_resolver);
}

static void
asyncns_resolver_finalize (GObject *object)
{
    LmAsyncnsResolverPriv *priv;

    priv = GET_PRIV (object);

    asyncns_resolver_cleanup (LM_RESOLVER (object));

    (G_OBJECT_CLASS (lm_asyncns_resolver_parent_class)->finalize) (object);
}

static void
asyncns_resolver_cleanup (LmResolver *resolver)
{
    LmAsyncnsResolverPriv *priv = GET_PRIV (resolver);

    if (priv->resolv_channel != NULL) {
        g_io_channel_unref (priv->resolv_channel);
        priv->resolv_channel = NULL;
    }
 
    if (priv->watch_resolv) {
        g_source_destroy (priv->watch_resolv);
        priv->watch_resolv = NULL;
    }

    if (priv->asyncns_ctx) {
        asyncns_free (priv->asyncns_ctx);
        priv->asyncns_ctx = NULL;
    }

    if (priv->sa) {
        lm_socket_address_unref (priv->sa);
    }

    priv->resolv_query = NULL;
}

typedef gboolean  (* LmAsyncnsResolverCallback) (LmResolver *resolver);

static gboolean
asyncns_resolver_io_cb (GSource      *source,
                        GIOCondition  condition,
                        LmResolver   *resolver)
{
    LmAsyncnsResolverPriv     *priv = GET_PRIV (resolver);

    asyncns_wait (priv->asyncns_ctx, FALSE);

    if (!asyncns_isdone (priv->asyncns_ctx, priv->resolv_query)) {
        return TRUE;
    }

    switch (priv->resolve_type) {
        case RESOLVE_TYPE_HOST:
            asyncns_resolver_host_done (resolver);
            break;
        case RESOLVE_TYPE_SRV:
            asyncns_resolver_srv_done (resolver);
            break;
        default:
            g_assert_not_reached ();
    };

    return FALSE;
}

static gboolean
asyncns_resolver_prep (LmResolver  *resolver, 
                       int          resolve_type)
{
    LmAsyncnsResolverPriv *priv = GET_PRIV (resolver);
    GMainContext          *context;

    /* Each LmResolver can only be used once */
    g_return_val_if_fail (priv->asyncns_ctx == NULL, FALSE);

    priv->resolve_type = resolve_type;

    priv->asyncns_ctx = asyncns_new (1);
    if (priv->asyncns_ctx == NULL) {
        g_warning ("can't initialise libasyncns");
        return FALSE;
    }

    priv->resolv_channel =
        g_io_channel_unix_new (asyncns_fd (priv->asyncns_ctx));

    g_object_get (resolver, "context", &context, NULL);
        
    priv->watch_resolv = 
        lm_misc_add_io_watch (context,
                              priv->resolv_channel,
                              G_IO_IN,
                              (GIOFunc) asyncns_resolver_io_cb,
                              resolver);

    return TRUE;
}

static void
asyncns_resolver_finished (LmResolver *resolver, LmResolverResult result) 
{
    LmAsyncnsResolverPriv *priv;

    priv = GET_PRIV (resolver);

    g_signal_emit_by_name (resolver, "finished", result, priv->sa);

    asyncns_resolver_cleanup (resolver);

    /* The initial reference is owned by LmResolver itself */
    g_object_unref (resolver);
}

static void
asyncns_resolver_host_done (LmResolver *resolver)
{
    LmAsyncnsResolverPriv *priv = GET_PRIV (resolver);
    struct addrinfo       *ans;
    int                    err;
    LmResolverResult       result;

    err = asyncns_getaddrinfo_done (priv->asyncns_ctx, 
                                    priv->resolv_query, &ans);
    priv->resolv_query = NULL;

    if (err) {
        result = LM_RESOLVER_RESULT_FAILED;
    } else {
        result = LM_RESOLVER_RESULT_OK;
        lm_socket_address_set_results (priv->sa, ans);
    } 

    asyncns_resolver_finished (resolver, result);
}

static void
asyncns_resolver_srv_done (LmResolver *resolver)
{
    LmAsyncnsResolverPriv *priv = GET_PRIV (resolver);
    unsigned char         *srv_ans;
    int                    srv_len;
    LmResolverResult       result;

    g_print ("srv_done callback\n");

    srv_len = asyncns_res_done (priv->asyncns_ctx, 
                                priv->resolv_query, &srv_ans);

    priv->resolv_query = NULL;

    if (srv_len <= 0) {
        result = LM_RESOLVER_RESULT_FAILED;
        g_warning ("Failed to read srv request results");
    } else {
        gchar    *new_server;
        guint     new_port;
        gboolean  parse_result;

        g_print ("trying to parse srv response\n");

        parse_result = _lm_resolver_parse_srv_response (srv_ans, srv_len,
                                                        &new_server, &new_port);
        if (parse_result == TRUE) {
            result = LM_RESOLVER_RESULT_OK;
            priv->sa = lm_socket_address_new (new_server, new_port);
        } else {
            result = LM_RESOLVER_RESULT_FAILED;
        }

        g_free (new_server);
    }

    asyncns_resolver_finished (resolver, result);
}

static void
asyncns_resolver_lookup_host (LmResolver       *resolver,
                              LmSocketAddress  *sa)
{
    LmAsyncnsResolverPriv *priv;
    struct addrinfo        req;

    g_return_if_fail (LM_IS_ASYNCNS_RESOLVER (resolver));
    g_return_if_fail (sa != NULL);

    if (!asyncns_resolver_prep (resolver, RESOLVE_TYPE_HOST)) {
        g_warning ("Failed to initialize the asyncns library");
        return;
    }

    memset (&req, 0, sizeof(req));
    req.ai_family   = AF_UNSPEC;
    req.ai_socktype = SOCK_STREAM;
    req.ai_protocol = IPPROTO_TCP;

    priv->resolv_query = asyncns_getaddrinfo (priv->asyncns_ctx,
                                              lm_socket_address_get_host (sa),
                                              NULL,
                                              &req);
}

static void
asyncns_resolver_lookup_srv (LmResolver   *resolver, 
                             const gchar  *srv)
{
    LmAsyncnsResolverPriv *priv;
    
    g_return_if_fail (LM_IS_ASYNCNS_RESOLVER (resolver));
    g_return_if_fail (srv != NULL);

    priv = GET_PRIV (resolver);

    if (!asyncns_resolver_prep (resolver, RESOLVE_TYPE_SRV)) {
        g_warning ("Failed to initialize the asyncns library");
        return;
    }

    priv->resolv_query = asyncns_res_query (priv->asyncns_ctx, 
                                            srv, C_IN, T_SRV);
}

static void
asyncns_resolver_cancel (LmResolver *resolver)
{
    LmAsyncnsResolverPriv *priv;

    g_return_if_fail (LM_IS_ASYNCNS_RESOLVER (resolver));

    priv = GET_PRIV (resolver);

    if (priv->asyncns_ctx && priv->resolv_query) {
        asyncns_cancel (priv->asyncns_ctx, priv->resolv_query);
        priv->resolv_query = NULL;
    }

    asyncns_resolver_finished (resolver, LM_RESOLVER_RESULT_CANCELLED);
}
