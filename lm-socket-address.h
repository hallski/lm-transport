/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#ifndef __LM_SOCKET_ADDRESS_H__
#define __LM_SOCKET_ADDRESS_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct LmSocketAddress LmSocketAddress;

GType              lm_socket_address_get_type (void);
LmSocketAddress *  lm_socket_address_new      (const gchar     *hostname,
                                               guint            port);

const gchar *      lm_socket_address_get_host (LmSocketAddress *sa);
guint              lm_socket_address_get_port (LmSocketAddress *sa);

LmSocketAddress *  lm_socket_address_ref      (LmSocketAddress *sa);
void               lm_socket_address_unref    (LmSocketAddress *sa);

G_END_DECLS

#endif /* __LM_SOCKET_ADDRESS_H__ */


