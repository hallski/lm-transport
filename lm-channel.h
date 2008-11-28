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

/* 
 * A Channel is a input/output component that can be connected into a chain to
 * provide functionality in the transport stack of Loudmouth.
 *
 * As an example you can connect:
 * LmBufferedChannel -> LmCompressionChannel -> LmSecureChannel  -> LmSocket
 *  < Buffered I/O >     <ZLib compression>     <TLS encryption>    <Network>
 */

#ifndef __LM_CHANNEL_H__
#define __LM_CHANNEL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define LM_TYPE_CHANNEL            (lm_channel_get_type ())
#define LM_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LM_TYPE_CHANNEL, LmChannel))
#define LM_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LM_TYPE_CHANNEL, LmChannelClass))
#define LM_IS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LM_TYPE_CHANNEL))
#define LM_IS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LM_TYPE_CHANNEL))
#define LM_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LM_TYPE_CHANNEL, LmChannelClass))

typedef enum {
    LM_CHANNEL_CLOSE_REQUESTED,
    LM_CHANNEL_CLOSE_HUP,
    LM_CHANNEL_CLOSE_IO_ERROR,
} LmChannelCloseReason;

typedef struct LmChannel      LmChannel;
typedef struct LmChannelClass LmChannelClass;

struct LmChannel {
    GObject parent;
};

struct LmChannelClass {
    GObjectClass parent_class;

    /* <signals> */
    /* TODO: Connect these to the signals in .c-file */
    void        (*opened)     (LmChannel            *channel);
    void        (*readable)   (LmChannel            *channel);
    void        (*writeable)  (LmChannel            *channel);
    void        (*closed)     (LmChannel            *channel,
                               LmChannelCloseReason  reason);
    
    /* <vtable> */
    GIOStatus   (*read)       (LmChannel    *channel,
                               gchar        *buf,
                               gsize         count,
                               gsize        *bytes_read,
                               GError      **error);
    GIOStatus   (*write)      (LmChannel    *channel,
                               const gchar  *buf,
                               gssize        count,
                               gsize        *bytes_written,
                               GError      **error);

    void        (*close)      (LmChannel    *channel);

    LmChannel * (*get_inner)  (LmChannel   *channel);
};

GType          lm_channel_get_type          (void);

GIOStatus      lm_channel_read              (LmChannel *channel,
                                             gchar     *buf,
                                             gsize      count,
                                             gsize     *bytes_read,
                                             GError    **error);
GIOStatus      lm_channel_write             (LmChannel *channel,
                                             const gchar *buf,
                                             gssize       count,
                                             gsize       *bytes_written,
                                             GError      **error);

void           lm_channel_close             (LmChannel *channel);

LmChannel *    lm_channel_get_inner         (LmChannel *channel);

G_END_DECLS

#endif /* __LM_CHANNEL_H__ */

