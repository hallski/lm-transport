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
    LmChannel *inner_channel;
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
static GIOStatus   secure_channel_read        (LmChannel         *channel,
                                               gchar             *buf,
                                               gsize              len,
                                               gsize             *read_len,
                                               GError           **error);
static GIOStatus   secure_channel_write       (LmChannel         *channel,
                                               const gchar       *buf,
                                               gssize             len,
                                               gsize             *written_len,
                                               GError           **error);
static void        secure_channel_close       (LmChannel         *channel);
static LmChannel * secure_channel_get_inner   (LmChannel         *channel);

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
    GObjectClass   *object_class = G_OBJECT_CLASS (class);
    LmChannelClass *channel_class = LM_CHANNEL_CLASS (class);

    object_class->finalize     = secure_channel_finalize;
    object_class->get_property = secure_channel_get_property;
    object_class->set_property = secure_channel_set_property;

    channel_class->read        = secure_channel_read;
    channel_class->write       = secure_channel_write;
    channel_class->close       = secure_channel_close;
    channel_class->get_inner   = secure_channel_get_inner;

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

static GIOStatus
secure_channel_read (LmChannel  *channel,
                     gchar      *buf,
                     gsize       len,
                     gsize      *read_len,
                     GError    **error)
{
    LmSecureChannelPriv *priv;

    g_return_val_if_fail (LM_IS_SECURE_CHANNEL (channel), 
                          G_IO_STATUS_ERROR);
    
    priv = GET_PRIV (channel);

    g_print ("Secure read\n");
   
    if (!priv->encrypted) {
        return lm_channel_read (priv->inner_channel,
                                buf, len, read_len, error);
    } 

    return LM_SECURE_CHANNEL_GET_CLASS(channel)->secure_read (channel,
                                                              buf, len, 
                                                              read_len, error);
}

static GIOStatus
secure_channel_write (LmChannel    *channel,
                      const gchar  *buf,
                      gssize        len,
                      gsize        *written_len,
                      GError      **error)
{
    LmSecureChannelPriv *priv;

    g_return_val_if_fail (LM_IS_SECURE_CHANNEL (channel), 
                          G_IO_STATUS_ERROR);
    
    priv = GET_PRIV (channel);
   
    if (!priv->encrypted) {
        return lm_channel_write (priv->inner_channel,
                                 buf, len, written_len, error);
    } 

    return LM_SECURE_CHANNEL_GET_CLASS(channel)->secure_write (channel,
                                                               buf, len,
                                                               written_len,
                                                               error);
}

static void
secure_channel_close (LmChannel *channel)
{
    LmSecureChannelPriv *priv;

    g_return_if_fail (LM_IS_SECURE_CHANNEL (channel));
    
    priv = GET_PRIV (channel);
   
    if (!priv->encrypted) {
        return lm_channel_close (priv->inner_channel);
    } 

    LM_SECURE_CHANNEL_GET_CLASS(channel)->secure_close (channel);
    /* FIXME: Read here too? */
}

static LmChannel *
secure_channel_get_inner (LmChannel *channel)
{
    LmSecureChannelPriv *priv;

    g_return_val_if_fail (LM_IS_SECURE_CHANNEL (channel), NULL);

    priv = GET_PRIV (channel);

    return priv->inner_channel;
}

static void
secure_channel_inner_opened_cb (LmChannel *inner, LmChannel *channel)
{
    g_signal_emit_by_name (channel, "opened");
}

static void
secure_channel_inner_readable_cb (LmChannel *inner, LmChannel *channel)
{
    g_signal_emit_by_name (channel, "readable");
}

static void
secure_channel_inner_writeable_cb (LmChannel *inner, LmChannel *channel)
{
    g_signal_emit_by_name (channel, "writeable");
}

static void
secure_channel_inner_closed_cb (LmChannel            *inner, 
                                LmChannelCloseReason  reason,
                                LmChannel            *channel)
{
    g_signal_emit_by_name (channel, "closed", reason);
}

LmChannel *
lm_secure_channel_new (GMainContext *context, LmChannel *inner_channel)
{
    LmChannel           *channel;
    LmSecureChannelPriv *priv;

    channel = g_object_new (LM_TYPE_SECURE_CHANNEL, NULL);
    priv    = GET_PRIV (channel);

    priv->inner_channel = g_object_ref (inner_channel);

    /* Move this to a LmChannel parent class (make it a class instead of an 
     * interface).
     */
    g_signal_connect (priv->inner_channel, "opened",
                      G_CALLBACK (secure_channel_inner_opened_cb),
                      channel);

    g_signal_connect (priv->inner_channel, "readable",
                      G_CALLBACK (secure_channel_inner_readable_cb),
                      channel);

    g_signal_connect (priv->inner_channel, "writeable",
                      G_CALLBACK (secure_channel_inner_writeable_cb),
                      channel);

    g_signal_connect (priv->inner_channel, "closed",
                      G_CALLBACK (secure_channel_inner_closed_cb),
                      channel);

    return channel;
}
