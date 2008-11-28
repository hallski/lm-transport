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
#include "lm-channel.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_CHANNEL, LmChannelPriv))

typedef struct LmChannelPriv LmChannelPriv;
struct LmChannelPriv {
    GMainContext *context;
    gint my_prop;
};

static void     channel_finalize            (GObject           *object);
static void     channel_get_property        (GObject           *object,
                                             guint              param_id,
                                             GValue            *value,
                                             GParamSpec        *pspec);
static void     channel_set_property        (GObject           *object,
                                             guint              param_id,
                                             const GValue      *value,
                                             GParamSpec        *pspec);

G_DEFINE_ABSTRACT_TYPE (LmChannel, lm_channel, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_CONTEXT,
    PROP_MY_PROP
};

enum {
    OPENED,
    READABLE,
    WRITEABLE,
    CLOSED,
    ERROR,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
lm_channel_class_init (LmChannelClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    GParamSpec   *pspec;

    object_class->finalize     = channel_finalize;
    object_class->get_property = channel_get_property;
    object_class->set_property = channel_set_property;

    pspec = g_param_spec_pointer ("context",
                                  "Main context",
                                  "GMainContext to run this socket",
                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    
    g_object_class_install_property (object_class, PROP_CONTEXT, pspec);

 
    signals[OPENED] =
        g_signal_new ("opened",
                      LM_TYPE_CHANNEL,
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);
    signals[READABLE] =
        g_signal_new ("readable",
                      LM_TYPE_CHANNEL,
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    signals[WRITEABLE] =
        g_signal_new ("writeable",
                      LM_TYPE_CHANNEL,
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    signals[CLOSED] =
        g_signal_new ("closed",
                      LM_TYPE_CHANNEL,
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__INT,
                      G_TYPE_NONE,
                      1, G_TYPE_INT);

    signals[ERROR] =
        g_signal_new ("error",
                      LM_TYPE_CHANNEL,
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      _lm_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    g_type_class_add_private (object_class, sizeof (LmChannelPriv));
}

static void
lm_channel_init (LmChannel *channel)
{
    LmChannelPriv *priv;

    priv = GET_PRIV (channel);

}

static void
channel_finalize (GObject *object)
{
    LmChannelPriv *priv;

    priv = GET_PRIV (object);

    (G_OBJECT_CLASS (lm_channel_parent_class)->finalize) (object);
}

static void
channel_get_property (GObject    *object,
                    guint       param_id,
                    GValue     *value,
                    GParamSpec *pspec)
{
    LmChannelPriv *priv;

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
channel_set_property (GObject      *object,
                      guint         param_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
    LmChannelPriv *priv;
    GMainContext *context;

    priv = GET_PRIV (object);

    switch (param_id) {
        case PROP_CONTEXT:
            context = g_value_get_pointer (value);
            if (context) {
                priv->context = g_main_context_ref (context);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    };
}

GIOStatus 
lm_channel_read (LmChannel *channel,
                 gchar     *buf,
                 gsize      count,
                 gsize     *bytes_read,
                 GError    **error)
{
    g_return_val_if_fail (LM_IS_CHANNEL (channel), G_IO_STATUS_ERROR);

    if (!LM_CHANNEL_GET_CLASS(channel)->read) {
        g_assert_not_reached ();
    }

    return LM_CHANNEL_GET_CLASS(channel)->read (channel, buf, count,
                                                bytes_read, error);
}

GIOStatus
lm_channel_write (LmChannel *channel,
                  const gchar *buf,
                  gssize       count,
                  gsize       *bytes_written,
                  GError      **error)
{
    g_return_val_if_fail (LM_IS_CHANNEL (channel), G_IO_STATUS_ERROR);

    if (!LM_CHANNEL_GET_CLASS(channel)->write) {
        g_assert_not_reached ();
    }

    return LM_CHANNEL_GET_CLASS(channel)->write (channel, buf, count,
                                                 bytes_written, error);
}

void
lm_channel_close (LmChannel *channel)
{
    g_return_if_fail (LM_IS_CHANNEL (channel));

    if (!LM_CHANNEL_GET_CLASS(channel)->close) {
        g_assert_not_reached ();
    }

    return LM_CHANNEL_GET_CLASS(channel)->close (channel);
}

LmChannel *
lm_channel_get_inner (LmChannel *channel)
{
    g_return_val_if_fail (LM_IS_CHANNEL (channel), NULL);

    if (!LM_CHANNEL_GET_CLASS(channel)->get_inner) {
        g_assert_not_reached ();
    }

    return LM_CHANNEL_GET_CLASS(channel)->get_inner (channel);
}



