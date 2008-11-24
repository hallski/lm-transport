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
 * About LmSocket:
 * ===============
 * LmSocket is a slightly higher abstraction around the lower level 
 * socket library functions.
 *
 */

#ifndef __LM_SOCKET_H__
#define __LM_SOCKET_H__

#include <glib-object.h>
#include <netdb.h>

G_BEGIN_DECLS

#define LM_TYPE_SOCKET             (lm_socket_get_type())
#define LM_SOCKET(o)               (G_TYPE_CHECK_INSTANCE_CAST((o), LM_TYPE_SOCKET, LmSocket))
#define LM_IS_SOCKET(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o), LM_TYPE_SOCKET))
#define LM_SOCKET_GET_IFACE(o)     (G_TYPE_INSTANCE_GET_INTERFACE((o), LM_TYPE_SOCKET, LmSocketIface))

typedef struct _LmSocket      LmSocket;
typedef struct _LmSocketIface LmSocketIface;

struct _LmSocketIface {
    GTypeInterface parent;

    /* <vtable> */
    void    (*connect)       (LmSocket        *socket,
                              struct addrinfo *addresses,
                              int              port);
};

GType          lm_socket_get_type          (void);

/* Factory method */
LmSocket *     lm_socket_create            (GMainContext    *context);

void           lm_socket_connect           (LmSocket        *socket,
                                            struct addrinfo *addresses,
                                            int              port);
GIOError       lm_socket_read              (LmSocket        *socket);
GIOError       lm_socket_write             (LmSocket        *socket);
void           lm_socket_disconnect        (LmSocket        *socket);

G_END_DECLS

#endif /* __LM_SOCKET_H__ */

