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

#ifndef __LM_SECURE_SOCKET_H__
#define __LM_SECURE_SOCKET_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define LM_TYPE_SECURE_SOCKET            (lm_secure_socket_get_type ())
#define LM_SECURE_SOCKET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LM_TYPE_SECURE_SOCKET, LmSecureSocket))
#define LM_SECURE_SOCKET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LM_TYPE_SECURE_SOCKET, LmSecureSocketClass))
#define LM_IS_SECURE_SOCKET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LM_TYPE_SECURE_SOCKET))
#define LM_IS_SECURE_SOCKET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LM_TYPE_SECURE_SOCKET))
#define LM_SECURE_SOCKET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LM_TYPE_SECURE_SOCKET, LmSecureSocketClass))

typedef struct LmSecureSocket      LmSecureSocket;
typedef struct LmSecureSocketClass LmSecureSocketClass;

struct LmSecureSocket {
    GObject parent;
};

struct LmSecureSocketClass {
    GObjectClass parent_class;
    
    /* <vtable> */
    void  (*initialize)    (LmSecureSocket     *secure_socket,
                            const char *username,
                            const char *server,
                            const char *password);
    void  (*begin)         (LmSecureSocket     *secure_socket);
    void  (*cancel)        (LmSecureSocket     *secure_socket);
};

GType   lm_secure_socket_get_type  (void);

G_END_DECLS

#endif /* __LM_SECURE_SOCKET_H__ */

