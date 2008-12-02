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

#include <gnutls/x509.h>

#include "lm-marshal.h"
#include "lm-secure-channel.h"
#include "lm-gnutls-channel.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_GNUTLS_CHANNEL, LmGnuTLSChannelPriv))

typedef struct LmGnuTLSChannelPriv LmGnuTLSChannelPriv;
struct LmGnuTLSChannelPriv {
    gnutls_session session;

    gint my_prop;
};

static void       gnutls_channel_finalize      (GObject           *object);
static void       gnutls_channel_get_property  (GObject           *object,
                                                guint              param_id,
                                                GValue            *value,
                                                GParamSpec        *pspec);
static void       gnutls_channel_set_property  (GObject           *object,
                                                guint              param_id,
                                                const GValue      *value,
                                                GParamSpec        *pspec);
static GIOStatus  gnutls_channel_read          (LmChannel         *channel,
                                                gchar             *buf,
                                                gsize              count,
                                                gsize             *bytes_read,
                                                GError           **error);
static GIOStatus  gnutls_channel_write         (LmChannel         *channel,
                                                const gchar       *buf,
                                                gssize             count,
                                                gsize            *bytes_written,
                                                GError           **error);
static void       gnutls_channel_close         (LmChannel         *channel);
static void
gnutls_channel_start_handshake                 (LmSecureChannel   *channel);

G_DEFINE_TYPE (LmGnuTLSChannel, lm_gnutls_channel, LM_TYPE_SECURE_CHANNEL)

enum {
    PROP_0,
    PROP_MY_PROP
};

enum {
    SIGNAL_NAME,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
lm_gnutls_channel_class_init (LmGnuTLSChannelClass *class)
{
    GObjectClass         *object_class    = G_OBJECT_CLASS (class);
    LmChannelClass       *channel_class   = LM_CHANNEL_CLASS (class);
    LmSecureChannelClass *secure_ch_class = LM_SECURE_CHANNEL_CLASS (class);

    object_class->finalize           = gnutls_channel_finalize;
    object_class->get_property       = gnutls_channel_get_property;
    object_class->set_property       = gnutls_channel_set_property;

    channel_class->read              = gnutls_channel_read;
    channel_class->write             = gnutls_channel_write;
    channel_class->close             = gnutls_channel_close;

    secure_ch_class->start_handshake = gnutls_channel_start_handshake;

    g_object_class_install_property (object_class,
                                     PROP_MY_PROP,
                                     g_param_spec_string ("my-prop",
                                                          "My Prop",
                                                          "My Property",
                                                          NULL,
                                                          G_PARAM_READWRITE));
    
    signals[SIGNAL_NAME] = 
        g_signal_new ("signal-name",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__INT,
                      G_TYPE_NONE, 
                      1, G_TYPE_INT);
    
    g_type_class_add_private (object_class, sizeof (LmGnuTLSChannelPriv));
}

static void
lm_gnutls_channel_init (LmGnuTLSChannel *gnutls_channel)
{
    LmGnuTLSChannelPriv *priv;

    priv = GET_PRIV (gnutls_channel);

}

static void
gnutls_channel_finalize (GObject *object)
{
    LmGnuTLSChannelPriv *priv;

    priv = GET_PRIV (object);

    (G_OBJECT_CLASS (lm_gnutls_channel_parent_class)->finalize) (object);
}

static void
gnutls_channel_get_property (GObject    *object,
                    guint       param_id,
                    GValue     *value,
                    GParamSpec *pspec)
{
    LmGnuTLSChannelPriv *priv;

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
gnutls_channel_set_property (GObject      *object,
                    guint         param_id,
                    const GValue *value,
                    GParamSpec   *pspec)
{
    LmGnuTLSChannelPriv *priv;

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

static GIOStatus
gnutls_channel_read (LmChannel  *channel,
                     gchar      *buf,
                     gsize       count,
                     gsize      *bytes_read,
                     GError    **error)
{
    return G_IO_STATUS_ERROR;
}

static GIOStatus
gnutls_channel_write (LmChannel    *channel,
                      const gchar  *buf,
                      gssize        count,
                      gsize        *bytes_written,
                      GError      **error)
{
    return G_IO_STATUS_ERROR;
}

static void
gnutls_channel_close (LmChannel *channel)
{

}

static void
gnutls_channel_start_handshake (LmSecureChannel *channel)
{
}


