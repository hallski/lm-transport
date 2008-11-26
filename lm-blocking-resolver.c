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
#include <sys/types.h>
#include <netdb.h>

/* Needed on Mac OS X */
#if HAVE_ARPA_NAMESER_COMPAT_H
#include <arpa/nameser_compat.h>
#endif

#include <arpa/nameser.h>
#include <resolv.h>

#include "lm-marshal.h"
#include "lm-misc.h"

#include "lm-blocking-resolver.h"

#define SRV_LEN 8192

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_BLOCKING_RESOLVER, LmBlockingResolverPriv))

typedef struct LmBlockingResolverPriv LmBlockingResolverPriv;
struct LmBlockingResolverPriv {
    GSource         *idle_source;

    LmSocketAddress *sa;
    gchar           *srv;
};

static void     blocking_resolver_finalize    (GObject          *object);
static void     blocking_resolver_lookup_host (LmResolver       *resolver,
                                               LmSocketAddress  *sa);
static void     blocking_resolver_lookup_srv  (LmResolver       *resolver,
                                               const gchar      *srv);
static void     blocking_resolver_cancel      (LmResolver       *resolver);


G_DEFINE_TYPE (LmBlockingResolver, lm_blocking_resolver, LM_TYPE_RESOLVER)

static void
lm_blocking_resolver_class_init (LmBlockingResolverClass *class)
{
    GObjectClass    *object_class   = G_OBJECT_CLASS (class);
    LmResolverClass *resolver_class = LM_RESOLVER_CLASS (class);

    object_class->finalize = blocking_resolver_finalize;

    resolver_class->lookup_host = blocking_resolver_lookup_host;
    resolver_class->lookup_srv  = blocking_resolver_lookup_srv;
    resolver_class->cancel      = blocking_resolver_cancel;
    
    g_type_class_add_private (object_class, 
                              sizeof (LmBlockingResolverPriv));
}

static void
lm_blocking_resolver_init (LmBlockingResolver *blocking_resolver)
{
    LmBlockingResolverPriv *priv;

    priv = GET_PRIV (blocking_resolver);
}

static void
blocking_resolver_finalize (GObject *object)
{
    LmBlockingResolverPriv *priv;

    priv = GET_PRIV (object);

    if (priv->idle_source) {
        g_source_destroy (priv->idle_source);
    }

    if (priv->sa) {
        lm_socket_address_unref (priv->sa);
    }

    g_free (priv->srv);

    (G_OBJECT_CLASS (lm_blocking_resolver_parent_class)->finalize) (object);
}

static void
blocking_resolver_finished (LmBlockingResolver *resolver, LmResolverResult result)
{
    LmBlockingResolverPriv *priv = GET_PRIV (resolver);
    
    g_signal_emit_by_name (resolver, "finished", result, priv->sa);

    priv->idle_source = NULL;

    /* The resolver itself owns the initial reference */
    g_object_unref (resolver);
}

static gboolean
blocking_resolver_idle_lookup_host (LmBlockingResolver *resolver)
{
    LmBlockingResolverPriv *priv;
    struct addrinfo         req;
    struct addrinfo        *ans;
    int                     err;
    LmResolverResult        result;

    priv = GET_PRIV (resolver);

    /* Lookup */
    memset (&req, 0, sizeof(req));
    req.ai_family   = AF_UNSPEC;
    req.ai_socktype = SOCK_STREAM;
    req.ai_protocol = IPPROTO_TCP;

    err = getaddrinfo (lm_socket_address_get_host (priv->sa), NULL, &req, &ans);

    if (err != 0 || ans == NULL) {
        g_warning ("Error while looking up '%s': %d\n", 
                   lm_socket_address_get_host (priv->sa), err);
       
        result = LM_RESOLVER_RESULT_FAILED;
    } else {
        g_print ("Found result for %s\n", lm_socket_address_get_host (priv->sa));

        lm_socket_address_set_results (priv->sa, ans);
        result = LM_RESOLVER_RESULT_OK;
    }

    blocking_resolver_finished (resolver, result);

    return FALSE;
}

static gboolean 
blocking_resolver_idle_lookup_srv (LmBlockingResolver *resolver)
{
    LmBlockingResolverPriv *priv;
    gchar                  *new_server = NULL;
    guint                   new_port = 0;
    gboolean                parse_result;
    unsigned char           srv_ans[SRV_LEN];
    int                     len;
    LmResolverResult        result;

    priv = GET_PRIV (resolver);

    res_init ();

    len = res_query (priv->srv, C_IN, T_SRV, srv_ans, SRV_LEN);

    parse_result = _lm_resolver_parse_srv_response (srv_ans, len, 
                                              &new_server, &new_port);
    if (parse_result == FALSE) {
        g_print ("Error while parsing srv response in %s\n", 
                 G_STRFUNC);
        result = LM_RESOLVER_RESULT_FAILED;
    } else {
        result = LM_RESOLVER_RESULT_OK;
        priv->sa = lm_socket_address_new (new_server, new_port);
    }

    blocking_resolver_finished (resolver, result);

    /* End idle source */
    return FALSE;
}

static void
blocking_resolver_lookup_host (LmResolver *resolver, LmSocketAddress *sa)
{
    LmBlockingResolverPriv *priv;
    GMainContext           *context;

    g_return_if_fail (LM_IS_BLOCKING_RESOLVER (resolver));
    g_return_if_fail (sa != NULL);

    priv = GET_PRIV (resolver);

    priv->sa = lm_socket_address_ref (sa);

    g_object_get (resolver, "context", &context, NULL);

    priv->idle_source = lm_misc_add_idle (context, 
                                          (GSourceFunc) blocking_resolver_idle_lookup_host,
                                          resolver);
}

static void
blocking_resolver_lookup_srv (LmResolver   *resolver,
                              const gchar  *srv)
{
    LmBlockingResolverPriv *priv;
    GMainContext           *context;

    g_return_if_fail (LM_IS_BLOCKING_RESOLVER (resolver));
    g_return_if_fail (srv != NULL);

    priv = GET_PRIV (resolver);

    priv->srv = g_strdup (srv);

    g_object_get (resolver, "context", &context, NULL);

    priv->idle_source = lm_misc_add_idle (context, 
                                          (GSourceFunc) blocking_resolver_idle_lookup_srv,
                                          resolver);
}

static void
blocking_resolver_cancel (LmResolver *resolver)
{
    LmBlockingResolverPriv *priv;

    g_return_if_fail (LM_IS_BLOCKING_RESOLVER (resolver));

    priv = GET_PRIV (resolver);

    if (priv->idle_source) {
        g_source_destroy (priv->idle_source);
        priv->idle_source = NULL;
    }

    blocking_resolver_finished (LM_BLOCKING_RESOLVER (resolver),
                                LM_RESOLVER_RESULT_CANCELLED);
}
