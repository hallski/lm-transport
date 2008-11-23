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

#ifndef __LM_IDUMMY_H__
#define __LM_IDUMMY_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define LM_TYPE_IDUMMY             (lm_idummy_get_type())
#define LM_IDUMMY(o)               (G_TYPE_CHECK_INSTANCE_CAST((o), LM_TYPE_IDUMMY, LmIDummy))
#define LM_IS_IDUMMY(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o), LM_TYPE_IDUMMY))
#define LM_IDUMMY_GET_IFACE(o)     (G_TYPE_INSTANCE_GET_INTERFACE((o), LM_TYPE_IDUMMY, LmIDummyIface))

typedef struct _LmIDummy      LmIDummy;
typedef struct _LmIDummyIface LmIDummyIface;

struct _LmIDummyIface {
    GTypeInterface parent;

    /* <vtable> */
    void     (*connect)      (LmIDummy *idummy);
    gboolean (*write)        (LmIDummy *idummy,
                              gchar    *buf,
                              gsize     len);
    gboolean (*read)         (LmIDummy *idummy,
                              gchar    *buf,
                              gsize     buf_len,
                              gsize    *read_len);
    void     (*disconnect)   (LmIDummy *idummy);
};

typedef void  (*LmIDummyCallback)  (LmIDummy *idummy,
                                    guint     status_code,
                                    gpointer  user_data);

GType          lm_idummy_get_type          (void);

LmIDummy *     lm_idummy_new               (const gchar *host,
                                            guint        port);
/* Use DNS lookup to find the port and the host */
LmIDummy *     lm_idummy_new_to_service    (const gchar *service);

/* All async functions so doesn't make a lot of sense to return anything */
/* Use LmIDummyCallback instead of signal for the connect result */
void           lm_idummy_connect           (LmIDummy    *idummy);
gboolean       lm_idummy_write             (LmIDummy    *idummy,
                                            gchar       *buf,
                                            gsize        len);
gboolean       lm_idummy_read              (LmIDummy    *idummy,
                                            gchar       *buf,
                                            gsize        buf_len,
                                            gsize       *read_len);
void           lm_idummy_disconnect        (LmIDummy    *idummy);

G_END_DECLS

#endif /* __LM_IDUMMY_H__ */

