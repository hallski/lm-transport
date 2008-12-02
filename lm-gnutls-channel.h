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

#ifndef __LM_GNUTLS_CHANNEL_H__
#define __LM_GNUTLS_CHANNEL_H__

#include <glib-object.h>

#include "lm-secure-channel.h"

G_BEGIN_DECLS

#define LM_TYPE_GNUTLS_CHANNEL            (lm_gnutls_channel_get_type ())
#define LM_GNUTLS_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LM_TYPE_GNUTLS_CHANNEL, LmGnuTLSChannel))
#define LM_GNUTLS_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LM_TYPE_GNUTLS_CHANNEL, LmGnuTLSChannelClass))
#define LM_IS_GNUTLS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LM_TYPE_GNUTLS_CHANNEL))
#define LM_IS_GNUTLS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LM_TYPE_GNUTLS_CHANNEL))
#define LM_GNUTLS_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LM_TYPE_GNUTLS_CHANNEL, LmGnuTLSChannelClass))

typedef struct LmGnuTLSChannel      LmGnuTLSChannel;
typedef struct LmGnuTLSChannelClass LmGnuTLSChannelClass;

struct LmGnuTLSChannel {
    LmSecureChannel parent;
};

struct LmGnuTLSChannelClass {
    LmSecureChannelClass parent_class;
    
    /* <vtable> */
    void  (*initialize)    (LmGnuTLSChannel     *gnutls_channel,
                            const char *username,
                            const char *server,
                            const char *password);
    void  (*begin)         (LmGnuTLSChannel     *gnutls_channel);
    void  (*cancel)        (LmGnuTLSChannel     *gnutls_channel);
};

GType   lm_gnutls_channel_get_type  (void);

G_END_DECLS

#endif /* __LM_GNUTLS_CHANNEL_H__ */

