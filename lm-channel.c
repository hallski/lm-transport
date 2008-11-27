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

static void    channel_base_init (LmChannelIface *iface);

enum {
    OPENED,
    READABLE,
    WRITEABLE,
    CLOSED,
    ERROR,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

GType
lm_channel_get_type (void)
{
    static GType iface_type = 0;

    if (!iface_type) {
        static const GTypeInfo iface_info = {
            sizeof (LmChannelIface),
            (GBaseInitFunc)     channel_base_init,
            (GBaseFinalizeFunc) NULL,
        };

        iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                             "LmChannelIface",
                                             &iface_info,
                                             0);

        g_type_interface_add_prerequisite (iface_type, G_TYPE_OBJECT);
    }

    return iface_type;
}

static void
channel_base_init (LmChannelIface *iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
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

        initialized = TRUE;
    }
}

GIOStatus 
lm_channel_read (LmChannel *channel,
                 gchar     *buf,
                 gsize      count,
                 gsize     *bytes_read,
                 GError    **error)
{
    g_return_val_if_fail (LM_IS_CHANNEL (channel), G_IO_STATUS_ERROR);

    if (!LM_CHANNEL_GET_IFACE(channel)->read) {
        g_assert_not_reached ();
    }

    return LM_CHANNEL_GET_IFACE(channel)->read (channel, buf, count,
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

    if (!LM_CHANNEL_GET_IFACE(channel)->write) {
        g_assert_not_reached ();
    }

    return LM_CHANNEL_GET_IFACE(channel)->write (channel, buf, count,
                                                 bytes_written, error);
}

void
lm_channel_close (LmChannel *channel)
{
    g_return_if_fail (LM_IS_CHANNEL (channel));

    if (!LM_CHANNEL_GET_IFACE(channel)->close) {
        g_assert_not_reached ();
    }

    return LM_CHANNEL_GET_IFACE(channel)->close (channel);
}

LmChannel *
lm_channel_get_inner (LmChannel *channel)
{
    g_return_val_if_fail (LM_IS_CHANNEL (channel), NULL);

    if (!LM_CHANNEL_GET_IFACE(channel)->get_inner) {
        g_assert_not_reached ();
    }

    return LM_CHANNEL_GET_IFACE(channel)->get_inner (channel);
}


