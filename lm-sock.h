/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2006 Imendio AB 
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

#ifndef __LM_SOCK_H__
#define __LM_SOCK_H__

G_BEGIN_DECLS

#include <glib.h>

#ifndef G_OS_WIN32
typedef int LmSocketHandle;
#else  /* G_OS_WIN32 */
typedef SOCKET LmSocketHandle;
#endif /* G_OS_WIN32 */

#ifndef G_OS_WIN32

#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#define _LM_SOCK_EINPROGRESS EINPROGRESS
#define _LM_SOCK_EWOULDBLOCK EWOULDBLOOK
#define _LM_SOCK_EALREADY    EALREADY
#define _LM_SOCK_EISCONN     EISCONN
#define _LM_SOCK_EINVAL      EINVAL
#define _LM_SOCK_VALID(S)    ((S) >= 0)

#else  /* G_OS_WIN32 */

/* This means that we require Windows XP or above to build on
 * Windows, the reason for this, is that getaddrinfo() and
 * freeaddrinfo() require ws2tcpip.h functions that are only available
 * on these platforms. 
 *
 * This MUST be defined before windows.h is included.
 */


#if (_WIN32_WINNT < 0x0501)
#undef  WINVER
#define WINVER 0x0501
#undef  _WIN32_WINNT
#define _WIN32_WINNT WINVER
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#define _LM_SOCK_EINPROGRESS WSAEINPROGRESS
#define _LM_SOCK_EWOULDBLOCK WSAEWOULDBLOCK
#define _LM_SOCK_EALREADY    WSAEALREADY
#define _LM_SOCK_EISCONN     WSAEISCONN
#define _LM_SOCK_EINVAL      WSAEINVAL
#define _LM_SOCK_VALID(S)    ((S) != INVALID_SOCKET)

#endif /* G_OS_WIN32 */

gboolean       _lm_sock_library_init        (void);
void           _lm_sock_library_shutdown    (void);
void           _lm_sock_set_blocking        (LmSocketHandle    sock,
                                             gboolean          block);
void           _lm_sock_shutdown            (LmSocketHandle    sock);
void           _lm_sock_close               (LmSocketHandle    sock);
LmSocketHandle _lm_sock_makesocket          (int               af,
                                             int               type,
                                             int               protocol);
int            _lm_sock_connect             (LmSocketHandle    sock,
                                             const struct sockaddr *name,
                                             int               namelen);
gboolean       _lm_sock_is_blocking_error   (int               err);
gboolean       _lm_sock_is_blocking_success (int               err);
int            _lm_sock_get_last_error      (void);
void           _lm_sock_get_error           (LmSocketHandle    sock, 
                                             void             *error, 
                                             socklen_t        *len);
const gchar *  _lm_sock_get_error_str       (int               err);
const gchar * 
_lm_sock_addrinfo_get_error_str             (int               err);
gchar       *  _lm_sock_get_local_host      (LmSocketHandle    sock);

gboolean       _lm_sock_set_keepalive       (LmSocketHandle    sock,
                                             int               delay);
G_END_DECLS

#endif /* __LM_SOCK__ */
