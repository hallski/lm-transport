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

#include <config.h>

#include <gnutls/x509.h>

#include "lm-marshal.h"
#include "lm-secure-channel.h"
#include "lm-gnutls-channel.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_GNUTLS_CHANNEL, LmGnuTLSChannelPriv))

#define CA_PEM_FILE "/etc/ssl/certs/ca-certificates.crt"

typedef struct LmGnuTLSChannelPriv LmGnuTLSChannelPriv;
struct LmGnuTLSChannelPriv {
    gnutls_session                 gnutls_session;
    gnutls_certificate_credentials gnutls_xcred;
};

static void       gnutls_channel_finalize      (GObject           *object);
static void       gnutls_channel_init_gnutls   (LmGnuTLSChannel   *channel);
static void       gnutls_channel_deinit_gnutls (LmGnuTLSChannel   *channel);
static GIOStatus  gnutls_channel_read          (LmChannel         *channel,
                                                gchar             *buf,
                                                gsize              count,
                                                gsize             *bytes_read,
                                                GError           **error);
static GIOStatus  gnutls_channel_write         (LmChannel         *channel,
                                                const gchar       *buf,
                                                gssize             count,
                                                gsize            *bytes_written,
                                                GError           **error);
static void       gnutls_channel_close         (LmChannel         *channel);
static void
gnutls_channel_start_handshake                 (LmSecureChannel   *channel);
static ssize_t    gnutls_channel_pull_func     (LmGnuTLSChannel   *channel,
                                                void              *buf,
                                                size_t             count);
static ssize_t    gnutls_channel_push_func     (LmGnuTLSChannel   *channel,
                                                const void        *buf,
                                                size_t             count);

G_DEFINE_TYPE (LmGnuTLSChannel, lm_gnutls_channel, LM_TYPE_SECURE_CHANNEL)

static void
lm_gnutls_channel_class_init (LmGnuTLSChannelClass *class)
{
    GObjectClass         *object_class    = G_OBJECT_CLASS (class);
    LmChannelClass       *channel_class   = LM_CHANNEL_CLASS (class);
    LmSecureChannelClass *secure_ch_class = LM_SECURE_CHANNEL_CLASS (class);

    object_class->finalize = gnutls_channel_finalize;

    channel_class->read    = gnutls_channel_read;
    channel_class->write   = gnutls_channel_write;
    channel_class->close   = gnutls_channel_close;

    secure_ch_class->start_handshake = gnutls_channel_start_handshake;

    g_type_class_add_private (object_class, sizeof (LmGnuTLSChannelPriv));
}

static void
lm_gnutls_channel_init (LmGnuTLSChannel *gnutls_channel)
{
    LmGnuTLSChannelPriv *priv;

    priv = GET_PRIV (gnutls_channel);

    gnutls_channel_init_gnutls (gnutls_channel);
}

static void
gnutls_channel_finalize (GObject *object)
{
    LmGnuTLSChannelPriv *priv;

    priv = GET_PRIV (object);

    gnutls_channel_deinit_gnutls (LM_GNUTLS_CHANNEL (object));

    (G_OBJECT_CLASS (lm_gnutls_channel_parent_class)->finalize) (object);
}

static void
gnutls_channel_init_gnutls (LmGnuTLSChannel *channel)
{
    LmGnuTLSChannelPriv *priv = GET_PRIV (channel);
    
    /* TODO: This function is not thread safe    */
    /*       Only call once ensure thread safety */
    gnutls_global_init ();
    gnutls_certificate_allocate_credentials (&priv->gnutls_xcred);
    gnutls_certificate_set_x509_trust_file(priv->gnutls_xcred,
                                           CA_PEM_FILE,
                                           GNUTLS_X509_FMT_PEM);
}

static void
gnutls_channel_deinit_gnutls (LmGnuTLSChannel *channel)
{
    LmGnuTLSChannelPriv *priv = GET_PRIV (channel);

    gnutls_certificate_free_credentials (priv->gnutls_xcred);
    gnutls_global_deinit ();
}

static GIOStatus
gnutls_channel_read (LmChannel  *channel,
                     gchar      *buf,
                     gsize       count,
                     gsize      *bytes_read,
                     GError    **error)
{
    LmGnuTLSChannelPriv *priv;
    GIOStatus            status;
    gint                 b_read;

    g_return_val_if_fail (LM_IS_GNUTLS_CHANNEL (channel), 
                          G_IO_STATUS_ERROR);

    priv = GET_PRIV (channel);

    /* Check if we are setup with encryption, otherwise call parents read */

    *bytes_read = 0;
    do {
        b_read = gnutls_record_recv (priv->gnutls_session, buf, count);
    } while (b_read == GNUTLS_E_INTERRUPTED || b_read == GNUTLS_E_AGAIN);

    if (b_read > 0) {
        *bytes_read = (guint) b_read;
        status = G_IO_STATUS_NORMAL;
    }
    else if (b_read < 0) {
        status = G_IO_STATUS_ERROR;
        *bytes_read = 0;
        if (error != NULL) {
            /* TODO: Set error */
        }
    } else { /* b_read == 0 */
        status = G_IO_STATUS_EOF;
    }

    return status;
}

static GIOStatus
gnutls_channel_write (LmChannel    *channel,
                      const gchar  *buf,
                      gssize        count,
                      gsize        *bytes_written,
                      GError      **error)
{
    LmGnuTLSChannelPriv *priv;

    g_return_val_if_fail (LM_IS_GNUTLS_CHANNEL (channel),
                          G_IO_STATUS_ERROR);

    priv = GET_PRIV (channel);

    /* Check if we are setup with encryption, otherwise call parents write */

    do {
        *bytes_written = gnutls_record_send (priv->gnutls_session, buf, count);
    } while (*bytes_written == GNUTLS_E_INTERRUPTED ||
             *bytes_written == GNUTLS_E_AGAIN);

    if (*bytes_written >= 0) {
        return G_IO_STATUS_NORMAL;
    } else { /* Error */
        /* TODO: Set error */
        return G_IO_STATUS_ERROR;
    } 
}

static void
gnutls_channel_close (LmChannel *channel)
{
    LmGnuTLSChannelPriv *priv;

    g_return_if_fail (LM_IS_GNUTLS_CHANNEL (channel));

    priv = GET_PRIV (channel);
    
    /* Check if we have encryption going and deinit if we do */
    gnutls_bye (priv->gnutls_session, GNUTLS_SHUT_RDWR);

    gnutls_deinit (priv->gnutls_session);
}

static void
gnutls_channel_start_handshake (LmSecureChannel *channel)
{
    LmGnuTLSChannelPriv *priv = GET_PRIV (channel);

    const int cert_type_priority[] =
        { GNUTLS_CRT_X509, GNUTLS_CRT_OPENPGP, 0 };
    const int compression_priority[] =
    { GNUTLS_COMP_DEFLATE, GNUTLS_COMP_NULL, 0 };

    gnutls_init (&priv->gnutls_session, GNUTLS_CLIENT);
    gnutls_set_default_priority (priv->gnutls_session);
    gnutls_certificate_type_set_priority (priv->gnutls_session,
                                          cert_type_priority);
    gnutls_compression_set_priority (priv->gnutls_session,
                                     compression_priority);
    gnutls_credentials_set (priv->gnutls_session,
                            GNUTLS_CRD_CERTIFICATE,
                            priv->gnutls_xcred);

    gnutls_transport_set_ptr (priv->gnutls_session,
                              (gnutls_transport_ptr_t)(glong) channel);

    gnutls_transport_set_push_function (priv->gnutls_session,
                                        (gnutls_push_func) gnutls_channel_push_func);
    gnutls_transport_set_pull_function (priv->gnutls_session,
                                        (gnutls_pull_func) gnutls_channel_pull_func);
}

static ssize_t
gnutls_channel_pull_func (LmGnuTLSChannel *channel,
                          void            *buf,
                          size_t           count)
{
    GIOStatus  status;
    gsize      bytes_read;
    ssize_t    ret_val;

    status = lm_channel_read (lm_channel_get_inner (LM_CHANNEL (channel)), 
                              buf, count, &bytes_read, NULL);

    /* errno should be set by underlying LmSocket so no need to reset it here */
    switch (status) {
        case G_IO_STATUS_NORMAL:
            ret_val = bytes_read;
            break;

        case G_IO_STATUS_EOF:
            ret_val = 0;
            break;
        case G_IO_STATUS_AGAIN:
        case G_IO_STATUS_ERROR:
            ret_val = -1;
            break;
    } 

    /* Use this? gnutls_transport_set_errno (priv->gnutls_session, errno)? */
    /* Should possibly be used to signal back error from G_IO_STATUS to the
     * GnuTLS layer
     */
    return ret_val;
}

static ssize_t 
gnutls_channel_push_func (LmGnuTLSChannel *channel,
                          const void      *buf,
                          size_t           count)
{
    GIOStatus  status;
    gsize      bytes_written;
    ssize_t    ret_val;

    status = lm_channel_write (lm_channel_get_inner (LM_CHANNEL (channel)),
                               buf, count, &bytes_written, NULL);
    
    /* errno should be set by underlying LmSocket so no need to reset it here */
    switch (status) {
        case G_IO_STATUS_NORMAL:
            ret_val = bytes_written;
            break;
        case G_IO_STATUS_EOF:
            ret_val = 0;
            break;
        case G_IO_STATUS_AGAIN:
        case G_IO_STATUS_ERROR:
            ret_val = -1;
            break;
    }

    return ret_val;
}
