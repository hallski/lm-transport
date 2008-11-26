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

#include <resolv.h>
#include <string.h>

#include "lm-blocking-resolver.h"
#include "lm-asyncns-resolver.h"
#include "lm-marshal.h"
#include "lm-resolver.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_RESOLVER, LmResolverPriv))

typedef struct LmResolverPriv LmResolverPriv;
struct LmResolverPriv {
    GMainContext *context;
};

static void     resolver_finalize            (GObject           *object);
static void     resolver_get_property        (GObject           *object,
                                              guint              param_id,
                                              GValue            *value,
                                              GParamSpec        *pspec);
static void     resolver_set_property        (GObject           *object,
                                              guint              param_id,
                                              const GValue      *value,
                                              GParamSpec        *pspec);

G_DEFINE_ABSTRACT_TYPE (LmResolver, lm_resolver, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_CONTEXT
};

enum {
    FINISHED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
lm_resolver_class_init (LmResolverClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    object_class->finalize     = resolver_finalize;
    object_class->get_property = resolver_get_property;
    object_class->set_property = resolver_set_property;

    g_object_class_install_property (object_class,
                                     PROP_CONTEXT,
                                     g_param_spec_pointer ("context",
                                                           "Context",
                                                           "GMainContext powering this resolver",
                                                           G_PARAM_READWRITE));
    
    signals[FINISHED] =
        g_signal_new ("finished",
                      LM_TYPE_RESOLVER,
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__INT_BOXED,
                      G_TYPE_NONE,
                      2, G_TYPE_INT, LM_TYPE_SOCKET_ADDRESS);

    g_type_class_add_private (object_class, sizeof (LmResolverPriv));
}

static void
lm_resolver_init (LmResolver *resolver)
{
    LmResolverPriv *priv;

    priv = GET_PRIV (resolver);

}

static void
resolver_finalize (GObject *object)
{
    LmResolverPriv *priv;

    priv = GET_PRIV (object);

    if (priv->context) {
        g_main_context_unref (priv->context);
    }

    (G_OBJECT_CLASS (lm_resolver_parent_class)->finalize) (object);
}

static void
resolver_get_property (GObject    *object,
                       guint       param_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
    LmResolverPriv *priv;

    priv = GET_PRIV (object);

    switch (param_id) {
        case PROP_CONTEXT:
            g_value_set_pointer (value, priv->context);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    };
}

static void
resolver_set_property (GObject      *object,
                       guint         param_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
    LmResolverPriv *priv;
    GMainContext   *context;

    priv = GET_PRIV (object);

    switch (param_id) {
        case PROP_CONTEXT:
            context = g_value_get_pointer (value);
            if (context) {
                priv->context = g_object_ref (context);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    };
}

static LmResolver *
resolver_create (GMainContext *context)
{
    GType type;

#if 0
    type = LM_TYPE_ASYNCNS_RESOLVER;
#else
    type = LM_TYPE_BLOCKING_RESOLVER;
#endif /* HAVE_ASYNCNS */

    return g_object_new (type, "context", context, NULL);
}

LmResolver *
lm_resolver_lookup_host (GMainContext     *context,
                         LmSocketAddress  *sa)
{
    LmResolver *resolver;

    resolver = resolver_create (context);

    if (!LM_RESOLVER_GET_CLASS(resolver)->lookup_host) {
        g_assert_not_reached ();
    }

    LM_RESOLVER_GET_CLASS(resolver)->lookup_host (resolver, sa);
    
    return resolver;
}

static gchar *
resolver_create_srv_string (const gchar *domain, 
                            const gchar *service,
                            const gchar *protocol)
{
    g_return_val_if_fail (domain != NULL, NULL);
    g_return_val_if_fail (service != NULL, NULL);
    g_return_val_if_fail (protocol != NULL, NULL);

    return g_strdup_printf ("_%s._%s.%s", service, protocol, domain);
}

LmResolver *
lm_resolver_lookup_service (GMainContext  *context,
                            const gchar   *domain,
                            const gchar   *srv)
{
    LmResolver *resolver;
    gchar      *srv_str;

    g_return_val_if_fail (domain != NULL, FALSE);
    g_return_val_if_fail (srv != NULL, FALSE);

    resolver = resolver_create (context);

    if (!LM_RESOLVER_GET_CLASS(resolver)->lookup_srv) {
        g_assert_not_reached ();
    }

    srv_str = resolver_create_srv_string (domain, srv, "tcp");
    
    LM_RESOLVER_GET_CLASS(resolver)->lookup_srv (resolver, srv_str);
    
    g_free (srv_str);
   
    return resolver;
}

void
lm_resolver_cancel (LmResolver *resolver)
{
    g_return_if_fail (LM_IS_RESOLVER (resolver));

    if (!LM_RESOLVER_GET_CLASS(resolver)->cancel) {
        g_assert_not_reached ();
    }

    return LM_RESOLVER_GET_CLASS(resolver)->cancel (resolver);
}

gboolean
_lm_resolver_parse_srv_response (unsigned char  *srv, 
                                 int             srv_len, 
                                 gchar         **out_server, 
                                 guint          *out_port)
{
    int                  qdcount;
    int                  ancount;
    int                  len;
    const unsigned char *pos;
    unsigned char       *end;
    HEADER              *head;
    char                 name[256];
    char                 pref_name[256];
    guint                pref_port = 0;
    guint                pref_prio = 9999;

    pref_name[0] = 0;

    pos = srv + sizeof (HEADER);
    end = srv + srv_len;
    head = (HEADER *) srv;

    qdcount = ntohs (head->qdcount);
    ancount = ntohs (head->ancount);

    /* Ignore the questions */
    while (qdcount-- > 0 && (len = dn_expand (srv, end, pos, name, 255)) >= 0) {
        g_assert (len >= 0);
        pos += len + QFIXEDSZ;
    }

    /* Parse the answers */
    while (ancount-- > 0 && (len = dn_expand (srv, end, pos, name, 255)) >= 0) {
        /* Ignore the initial string */
        uint16_t pref, weight, port;

        g_assert (len >= 0);
        pos += len;
        /* Ignore type, ttl, class and dlen */
        pos += 10;
        GETSHORT (pref, pos);
        GETSHORT (weight, pos);
        GETSHORT (port, pos);

        len = dn_expand (srv, end, pos, name, 255);
        if (pref < pref_prio) {
            pref_prio = pref;
            strcpy (pref_name, name);
            pref_port = port;
        }
        pos += len;
    }

    if (pref_name[0]) {
        *out_server = g_strdup (pref_name);
        *out_port = pref_port;
        return TRUE;
    } 
    return FALSE;
}

