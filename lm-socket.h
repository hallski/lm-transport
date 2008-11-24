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

#ifndef __LM_SOCKET_H__
#define __LM_SOCKET_H__

#include <glib-object.h>

#include "lm-socket-address.h"

G_BEGIN_DECLS

#define LM_TYPE_SOCKET            (lm_socket_get_type ())
#define LM_SOCKET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LM_TYPE_SOCKET, LmSocket))
#define LM_SOCKET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LM_TYPE_SOCKET, LmSocketClass))
#define LM_IS_SOCKET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LM_TYPE_SOCKET))
#define LM_IS_SOCKET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LM_TYPE_SOCKET))
#define LM_SOCKET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LM_TYPE_SOCKET, LmSocketClass))

typedef struct LmSocket      LmSocket;
typedef struct LmSocketClass LmSocketClass;

struct LmSocket {
    GObject parent;
};

struct LmSocketClass {
    GObjectClass parent_class;
    
    /* <vtable> */
    void       (*connect)        (LmSocket        *socket);
    void       (*close)          (LmSocket        *socket);
    GIOStatus  (*read)           (LmSocket        *socket,
                                  gchar           *buf,
                                  gsize            len,
                                  gsize           *read_len,
                                  GError         **error);
    GIOStatus  (*write)          (LmSocket        *socket,
                                  gchar           *buf,
                                  gsize            len,
                                  gsize           *written_len,
                                  GError         **error);
};

GType      lm_socket_get_type  (void);

LmSocket * lm_socket_new            (LmSocketAddress *address,
                                     GMainContext    *context);
/* Support proxy here
 * LmSocket * lm_socket_new_with_proxy (LmSocketAddress *address); 
 */

void       lm_socket_connect        (LmSocket        *socket);
void       lm_socket_close          (LmSocket        *socket);
GIOStatus  lm_socket_read           (LmSocket        *socket,
                                     gchar           *buf,
                                     gsize            len,
                                     gsize           *read_len,
                                     GError         **error);
GIOStatus  lm_socket_write          (LmSocket        *socket,
                                     gchar           *buf,
                                     gsize            len,
                                     gsize           *written_len,
                                     GError         **error);

G_END_DECLS

#endif /* __LM_SOCKET_H__ */

