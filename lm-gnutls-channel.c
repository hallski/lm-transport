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
#include <string.h>

#include "lm-marshal.h"
#include "lm-secure-channel.h"
#include "lm-gnutls-channel.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), LM_TYPE_GNUTLS_CHANNEL, LmGnuTLSChannelPriv))

#define CA_PEM_FILE "/etc/ssl/certs/ca-certificates.crt"

typedef struct LmGnuTLSChannelPriv LmGnuTLSChannelPriv;
struct LmGnuTLSChannelPriv {
    gnutls_session                 gnutls_session;
    gnutls_certificate_credentials gnutls_xcred;

    gboolean                       is_encrypted;
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
gnutls_channel_start_handshake                 (LmSecureChannel   *channel,
                                                const gchar       *host);
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

    priv->is_encrypted = FALSE;

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

    if (!priv->is_encrypted) {
        /* Until we are encrypted, use read from inner channel */
        return lm_channel_read (lm_channel_get_inner (channel),
                                buf, count, bytes_read, error);
    }

    *bytes_read = 0;
    do {
        g_print ("RECV\n");
        b_read = gnutls_record_recv (priv->gnutls_session, buf, count);
        g_print ("RECV done\n");
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

    g_print ("Time to return\n");
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

    if (!priv->is_encrypted) {
        /* Until we are encrypted, use write from inner channel */
        return lm_channel_write (lm_channel_get_inner (channel),
                                 buf, count, bytes_written, error);
    }

    do {
        g_print ("SEND\n");
        *bytes_written = gnutls_record_send (priv->gnutls_session, buf, count);
        g_print ("SEND done\n");
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
   
    if (priv->is_encrypted) {
        gnutls_bye (priv->gnutls_session, GNUTLS_SHUT_RDWR);
        gnutls_deinit (priv->gnutls_session);
    }
        
    lm_channel_close (lm_channel_get_inner (channel));
}

static gboolean
gnutls_channel_request_user_cert_feedback (LmGnuTLSChannel *channel,
                                           LmSSLStatus      status)
{
    LmSSLResponse response;

    /* TODO: Implement */
#if 0
    if (rc != 0) {
        if (base->func (ssl,
                        LM_SSL_STATUS_GENERIC_ERROR,
                        base->func_data) != LM_SSL_RESPONSE_CONTINUE) { }
    }
#endif

    response = LM_SSL_RESPONSE_CONTINUE;

    if (response == LM_SSL_RESPONSE_CONTINUE) {
        return TRUE;
    } else {
        return FALSE;
    }

}

static gboolean
gnutls_channel_verify_certificate (LmGnuTLSChannel *channel, 
                                   const gchar     *server)
{
    LmGnuTLSChannelPriv *priv = GET_PRIV (channel);
	unsigned int         status;
	int                  rc;

	/* This verification function uses the trusted CAs in the credentials
	 * structure. So you must have installed one or more CA certificates.
	 */
	rc = gnutls_certificate_verify_peers2 (priv->gnutls_session, &status);

    if (rc == GNUTLS_E_NO_CERTIFICATE_FOUND) {
        if (!gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_NO_CERT_FOUND)) {
            return FALSE;
		}
	}

	if (rc != 0) {
        if (!gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_GENERIC_ERROR)) {
            return FALSE;
        }
	}

	if (rc == GNUTLS_E_NO_CERTIFICATE_FOUND) {
        if (!gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_NO_CERT_FOUND)) {
			return FALSE;
		}
	}
	
	if (status & GNUTLS_CERT_INVALID
        || status & GNUTLS_CERT_REVOKED) {
		if (!gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_UNTRUSTED_CERT)) {
            return FALSE;
		}
	}
	
    if (gnutls_certificate_expiration_time_peers (priv->gnutls_session) < time (0)) {
		if (!gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_CERT_EXPIRED)) {
			return FALSE;
		}
	}
	
	if (gnutls_certificate_activation_time_peers (priv->gnutls_session) > time (0)) {
		if (!gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_CERT_NOT_ACTIVATED)) {
			return FALSE;
		}
	}
	
	if (gnutls_certificate_type_get (priv->gnutls_session) == GNUTLS_CRT_X509) {
		const gnutls_datum *cert_list;
        guint               cert_list_size;
		size_t              digest_size;
		gnutls_x509_crt     cert;
        gchar              *expected_fingerprint;
        gchar               fingerprint[20];
		
		cert_list = gnutls_certificate_get_peers (priv->gnutls_session,
                                                  &cert_list_size);
        if (cert_list == NULL) {
            /* Signal ssl func */
            if (!gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_NO_CERT_FOUND)) {
				return FALSE;
			}
		}

		gnutls_x509_crt_init (&cert);

        if (gnutls_x509_crt_import (cert, &cert_list[0],
                                    GNUTLS_X509_FMT_DER) != 0) {
            if (!gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_NO_CERT_FOUND)) {
				return FALSE;
			}
		}

		if (!gnutls_x509_crt_check_hostname (cert, server)) {
			if (!gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_CERT_HOSTNAME_MISMATCH)) {
				return FALSE;
			}
		}

		gnutls_x509_crt_deinit (cert);

        digest_size = sizeof (fingerprint);
        g_object_get (channel, 
                      "expected_fingerprint", &expected_fingerprint, NULL);

		if (gnutls_fingerprint (GNUTLS_DIG_MD5, &cert_list[0],
                                fingerprint,
                                &digest_size) >= 0) {
            if (expected_fingerprint &&
                memcmp (expected_fingerprint, 
                        fingerprint,
                        digest_size) &&
                !gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_CERT_FINGERPRINT_MISMATCH)) {
                g_free (expected_fingerprint);
                return FALSE;
            }
        } 
        else if (!gnutls_channel_request_user_cert_feedback (channel, LM_SSL_STATUS_GENERIC_ERROR)) {
            g_free (expected_fingerprint);
            return FALSE; 
        } 

        g_free (expected_fingerprint);
        g_object_set (channel, "fingerprint", fingerprint, NULL);
	}

	return TRUE;
}

static void
gnutls_channel_start_handshake (LmSecureChannel *channel, 
                                const gchar     *host)
{
    LmGnuTLSChannelPriv *priv = GET_PRIV (channel);
    int                  ret;
    gboolean             auth_ok = TRUE;
    
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

    /* TODO: Should be done asynchronously, look at async gnutls branch */
    ret = gnutls_handshake (priv->gnutls_session);
    if (ret >= 0) {
        auth_ok = gnutls_channel_verify_certificate (LM_GNUTLS_CHANNEL (channel),
                                                     host);
    }

    if (ret < 0 || !auth_ok) {
        /* TODO: Signal through handshake result signal */
#if 0

        char *errmsg;

        if (!auth_ok) {
            errmsg = "authentication error";
        } else {
            errmsg = "handshake failed";
        }

        g_set_error (error, 
                     LM_ERROR, LM_ERROR_CONNECTION_OPEN,
                     "*** GNUTLS %s: %s",
                     errmsg, gnutls_strerror (ret));                    
#endif
    }

    g_print ("HANDSHAKE\n");
    priv->is_encrypted = TRUE;
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
