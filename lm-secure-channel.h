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
 * The SecureChannel provides an LmChannel for encrypting traffic through
 * SSL or TLS. It will be subclassed by LmGnuTLSChannel and LmOpenSSLChannel
 * to provide implementation specific versions.
 */

#ifndef __LM_SECURE_CHANNEL_H__
#define __LM_SECURE_CHANNEL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define LM_TYPE_SECURE_CHANNEL            (lm_secure_channel_get_type ())
#define LM_SECURE_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LM_TYPE_SECURE_CHANNEL, LmSecureChannel))
#define LM_SECURE_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LM_TYPE_SECURE_CHANNEL, LmSecureChannelClass))
#define LM_IS_SECURE_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LM_TYPE_SECURE_CHANNEL))
#define LM_IS_SECURE_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LM_TYPE_SECURE_CHANNEL))
#define LM_SECURE_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LM_TYPE_SECURE_CHANNEL, LmSecureChannelClass))

typedef struct LmSecureChannel      LmSecureChannel;
typedef struct LmSecureChannelClass LmSecureChannelClass;

struct LmSecureChannel {
    GObject parent;
};

struct LmSecureChannelClass {
    GObjectClass parent_class;
    
    /* <vtable> */
    GIOStatus   (*secure_read)       (LmChannel    *channel,
                                      gchar        *buf,
                                      gsize         count,
                                      gsize        *bytes_read,
                                      GError      **error);
    GIOStatus   (*secure_write)      (LmChannel    *channel,
                                      const gchar  *buf,
                                      gssize        count,
                                      gsize        *bytes_written,
                                      GError      **error);
    void        (*secure_close)      (LmChannel    *channel);
};

GType   lm_secure_channel_get_type        (void);

/* 
 * Need a way to communicate certificate and ssl-callback
 *
 * A set of SSL errors that might occur
 *
 */

void          lm_secure_channel_start_handshake (LmSecureChannel *channel);

const gchar * lm_secure_channel_get_fingerprint (LmSecureChannel *channel);

G_END_DECLS

#endif /* __LM_SECURE_CHANNEL_H__ */

