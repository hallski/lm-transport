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

#ifndef __LM_TRANSPORT_H__
#define __LM_TRANSPORT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define LM_TYPE_TRANSPORT             (lm_transport_get_type())
#define LM_TRANSPORT(o)               (G_TYPE_CHECK_INSTANCE_CAST((o), LM_TYPE_TRANSPORT, LmTransport))
#define LM_IS_TRANSPORT(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o), LM_TYPE_TRANSPORT))
#define LM_TRANSPORT_GET_IFACE(o)     (G_TYPE_INSTANCE_GET_INTERFACE((o), LM_TYPE_TRANSPORT, LmTransportIface))

typedef struct _LmTransport      LmTransport;
typedef struct _LmTransportIface LmTransportIface;

struct _LmTransportIface {
    GTypeInterface parent;

    /* <vtable> */
    int       (*get_handle) (LmTransport *transport);
    GIOError  (*read)       (LmTransport *transport,
                             void        *buf,
                             gsize        len,
                             gsize       *bytes_read);
    void      (*write)      (LmTransport *transport,
                             void        *buf,
                             gsize        len);
    void      (*disconnect) (LmTransport *transport);
};

/* Should remain in lm-connection.h */
typedef enum {
    LM_DISCONNECT_REASON_OK,
    LM_DISCONNECT_REASON_PING_TIME_OUT,
    LM_DISCONNECT_REASON_HUP,
    LM_DISCONNECT_REASON_ERROR,
    LM_DISCONNECT_REASON_RESOURCE_CONFLICT,
    LM_DISCONNECT_REASON_INVALID_XML,
    LM_DISCONNECT_REASON_UNKNOWN
} LmDisconnectReason;

GType          lm_transport_get_type          (void);

int            lm_transport_get_handle        (LmTransport *transport);
GIOError       lm_transport_read              (LmTransport *transport,
                                               void        *buf,
                                               gsize        len,
                                               gsize       *bytes_read);

void           lm_transport_write             (LmTransport *transport,
                                               void        *buf,
                                               gsize        len);

void           lm_transport_disconnect        (LmTransport *transport);

G_END_DECLS

#endif /* __LM_TRANSPORT_H__ */

