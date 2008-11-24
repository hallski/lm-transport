/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#ifndef __LM_SOCKET_ADDRESS_H__
#define __LM_SOCKET_ADDRESS_H__

#include <glib-object.h>
#include <netdb.h>

G_BEGIN_DECLS

#define LM_TYPE_SOCKET_ADDRESS (lm_socket_address_get_type ())

typedef struct LmSocketAddress     LmSocketAddress;
typedef struct LmSocketAddressIter LmSocketAddressIter;

GType             lm_socket_address_get_type (void);
LmSocketAddress * lm_socket_address_new      (const gchar     *hostname,
                                              guint            port);

const gchar *     lm_socket_address_get_host (LmSocketAddress *sa);
guint             lm_socket_address_get_port (LmSocketAddress *sa);

gboolean          lm_socket_address_is_resolved (LmSocketAddress *sa);

/* The returned iterator is owned by the LmSocketAddress */
LmSocketAddressIter *
lm_socket_address_get_result_iter            (LmSocketAddress *sa);

/* Ref counting */
LmSocketAddress * lm_socket_address_ref      (LmSocketAddress *sa);
void              lm_socket_address_unref    (LmSocketAddress *sa);

/* Only to be used by the resolver */
void              lm_socket_address_set_results (LmSocketAddress *sa,
                                                 struct addrinfo *ai);

/* Result iterator */
struct addrinfo * lm_socket_address_iter_get_next (LmSocketAddressIter *iter);
void              lm_socket_address_iter_reset    (LmSocketAddressIter *iter);

G_END_DECLS

#endif /* __LM_SOCKET_ADDRESS_H__ */


