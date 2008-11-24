/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#ifndef __LM_SOCKET_ADDRESS_H__
#define __LM_SOCKET_ADDRESS_H__

#include <glib-object.h>
#include <netdb.h>

G_BEGIN_DECLS

#define LM_TYPE_INET_ADDRESS   (lm_inet_address_get_type ())
#define LM_TYPE_SOCKET_ADDRESS (lm_socket_address_get_type ())

typedef struct LmInetAddress   LmInetAddress;
typedef struct LmSocketAddress LmSocketAddress;

GType             lm_socket_address_get_type (void);
LmSocketAddress * lm_socket_address_new      (const gchar     *hostname,
                                              guint            port);

const gchar *     lm_socket_address_get_host (LmSocketAddress *sa);
guint             lm_socket_address_get_port (LmSocketAddress *sa);

gboolean          lm_socket_is_resolved      (LmSocketAddress *sa);

LmInetAddress *   lm_socket_address_get_inet_address   (LmSocketAddress *sa);
GSList *          lm_socket_address_get_inet_addresses (LmSocketAddress *sa);

LmSocketAddress * lm_socket_address_ref      (LmSocketAddress *sa);
void              lm_socket_address_unref    (LmSocketAddress *sa);

GType             lm_inet_address_get_type     (void);
struct addrinfo * lm_inet_address_get_addrinfo (LmInetAddress *ia);
LmInetAddress *   lm_inet_address_ref          (LmInetAddress *ia);
void              lm_inet_address_unref        (LmInetAddress *ia);


/* Only to be used by the resolver */
void              lm_socket_address_set_results (LmSocketAddress *sa,
                                                 struct addrinfo *ai);
LmInetAddress *   lm_inet_address_new           (struct addrinfo *ai);


G_END_DECLS

#endif /* __LM_SOCKET_ADDRESS_H__ */


