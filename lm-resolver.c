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
#include "lm-resolver.h"

static void    resolver_base_init (LmResolverIface *iface);

enum {
    FINISHED_HOST,
    FINISHED_SRV,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

GType
lm_resolver_get_type (void)
{
    static GType iface_type = 0;

    if (!iface_type) {
        static const GTypeInfo iface_info = {
            sizeof (LmResolverIface),
            (GBaseInitFunc)     resolver_base_init,
            (GBaseFinalizeFunc) NULL,
        };

        iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                             "LmResolverIface",
                                             &iface_info,
                                             0);

        g_type_interface_add_prerequisite (iface_type, G_TYPE_OBJECT);
    }

    return iface_type;
}

static void
resolver_base_init (LmResolverIface *iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        signals[FINISHED_HOST] =
            g_signal_new ("finished-host",
                          LM_TYPE_RESOLVER,
                          G_SIGNAL_RUN_LAST,
                          0,
                          NULL, NULL,
                          _lm_marshal_VOID__INT_BOXED,
                          G_TYPE_NONE,
                          2, G_TYPE_INT, LM_TYPE_SOCKET_ADDRESS);
        signals[FINISHED_SRV] =
            g_signal_new ("finished-srv",
                          LM_TYPE_RESOLVER,
                          G_SIGNAL_RUN_LAST,
                          0,
                          NULL, NULL,
                          _lm_marshal_VOID__INT_BOXED,
                          G_TYPE_NONE,
                          2, G_TYPE_INT, LM_TYPE_SOCKET_ADDRESS);
        initialized = TRUE;
    }
}

LmResolver *
lm_resolver_create (GMainContext *context)
{
    return NULL;
}

void
lm_resolver_lookup_host (LmResolver *resolver, LmSocketAddress *sa)
{
    g_return_if_fail (LM_IS_RESOLVER (resolver));

    if (!LM_RESOLVER_GET_IFACE(resolver)->lookup_host) {
        g_assert_not_reached ();
    }

    return LM_RESOLVER_GET_IFACE(resolver)->lookup_host (resolver, sa);
}

void
lm_resolver_lookup_srv (LmResolver *resolver, gchar *srv)
{
    g_return_if_fail (LM_IS_RESOLVER (resolver));

    if (!LM_RESOLVER_GET_IFACE(resolver)->lookup_srv) {
        g_assert_not_reached ();
    }

    return LM_RESOLVER_GET_IFACE(resolver)->lookup_srv (resolver, srv);
}

void
lm_resolver_cancel (LmResolver *resolver)
{
    g_return_if_fail (LM_IS_RESOLVER (resolver));

    if (!LM_RESOLVER_GET_IFACE(resolver)->cancel) {
        g_assert_not_reached ();
    }

    return LM_RESOLVER_GET_IFACE(resolver)->cancel (resolver);
}

