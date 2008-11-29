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

#include "lm-channel.h"
#include "lm-marshal.h"
#include "lm-secure-channel.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_SECURE_CHANNEL, LmSecureChannelPriv))

typedef struct LmSecureChannelPriv LmSecureChannelPriv;
struct LmSecureChannelPriv {
    gchar    *expected_fingerprint;
    gchar    *fingerprint;

    gboolean  encrypted;

    gint my_prop;
};

static void       secure_channel_finalize     (GObject           *object);
static void       secure_channel_get_property (GObject           *object,
                                               guint              param_id,
                                               GValue            *value,
                                               GParamSpec        *pspec);
static void       secure_channel_set_property (GObject           *object,
                                               guint              param_id,
                                               const GValue      *value,
                                               GParamSpec        *pspec);

G_DEFINE_TYPE (LmSecureChannel, lm_secure_channel, LM_TYPE_CHANNEL)

enum {
    PROP_0,
    PROP_FINGERPRINT,
    PROP_EXPECTED_FINGERPRINT,
    PROP_IS_SECURE,
    PROP_MY_PROP
};

enum {
    HANDSHAKE_RESULT,
    RESPONSE_REQUIRED, /* Come up with a better name */
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
lm_secure_channel_class_init (LmSecureChannelClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    object_class->finalize     = secure_channel_finalize;
    object_class->get_property = secure_channel_get_property;
    object_class->set_property = secure_channel_set_property;

    g_object_class_install_property (object_class,
                                     PROP_MY_PROP,
                                     g_param_spec_string ("my-prop",
                                                          "My Prop",
                                                          "My Property",
                                                          NULL,
                                                          G_PARAM_READWRITE));
    
    signals[HANDSHAKE_RESULT] = 
        g_signal_new ("handshake-result",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__INT,
                      G_TYPE_NONE, 
                      1, G_TYPE_INT);
    
    g_type_class_add_private (object_class, sizeof (LmSecureChannelPriv));
}

static void
lm_secure_channel_init (LmSecureChannel *secure_channel)
{
    LmSecureChannelPriv *priv;

    priv = GET_PRIV (secure_channel);

}

static void
secure_channel_finalize (GObject *object)
{
    LmSecureChannelPriv *priv;

    priv = GET_PRIV (object);

    (G_OBJECT_CLASS (lm_secure_channel_parent_class)->finalize) (object);
}

static void
secure_channel_get_property (GObject    *object,
                             guint       param_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    LmSecureChannelPriv *priv;

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
secure_channel_set_property (GObject      *object,
                             guint         param_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    LmSecureChannelPriv *priv;

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

LmChannel *
lm_secure_channel_new (GMainContext *context, LmChannel *inner_channel)
{
    LmChannel           *channel;
    LmSecureChannelPriv *priv;

    channel = g_object_new (LM_TYPE_SECURE_CHANNEL, NULL);
    priv    = GET_PRIV (channel);

    lm_channel_set_inner (channel, inner_channel);

    return channel;
}
