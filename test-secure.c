/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "lm-channel.h"
#include "lm-resolver.h"
#include "lm-secure-channel.h"
#include "lm-socket.h"

static gchar     *host;
static int        port;
static GMainLoop *loop;

#define MESSAGE "Hey there! Whazzup?"

static void
channel_opened (LmChannel *channel) 
{
    lm_secure_channel_start_handshake (LM_SECURE_CHANNEL (channel), host);
    /*lm_channel_write (channel, MESSAGE, (gssize)strlen (MESSAGE), NULL, NULL) */
}

/*
 * static void
write_some (LmChannel *channel)
{
    static int foo = 0;
    gchar *msg;

    msg = g_strdup_printf ("Yay %d!", ++foo);

    lm_channel_write (channel, msg, (gssize)strlen (msg), NULL, NULL);

    g_print ("Returning\n");

    lm_channel_close (channel);
}
*/

static void
channel_readable (LmChannel *channel)
{
    char  buf[1024];
    gsize read_len;

    g_print ("Attempting to read\n");

    lm_channel_read (channel, buf, 1023, &read_len, NULL);
    buf[read_len] = '\0';

    g_print ("Read: %s\n", buf);
    /* write_some (channel); */
}

static void
channel_closed (LmChannel *channel, LmChannelCloseReason reason)
{
    g_print ("Closed due to: ");

    switch (reason) {
        case LM_CHANNEL_CLOSE_REQUESTED:
            g_print ("User requested\n");
            break;
        case LM_CHANNEL_CLOSE_IO_ERROR:
            g_print ("I/O Error\n");
            break;
        case LM_CHANNEL_CLOSE_HUP:
            g_print ("HUP\n");
            break;
    };

    g_free (host);
    g_main_loop_quit (loop);
}

static void
socket_connected (LmSocket              *socket,
                  LmSocketConnectResult  res, 
                  gpointer               user_data)
{
    LmChannel *secure;

    g_print ("Connect callback: %d\n", res);

    if (res != LM_SOCKET_CONNECT_OK) {
        g_print ("Failed to connect\n");
        g_main_loop_quit (loop);
    }

    secure = lm_secure_channel_new (NULL, LM_CHANNEL (socket));

    g_signal_connect (secure, "opened",
                      G_CALLBACK (channel_opened),
                      NULL);
    g_signal_connect (secure, "readable",
                      G_CALLBACK (channel_readable),
                      NULL);
    g_signal_connect (secure, "closed",
                      G_CALLBACK (channel_closed),
                      NULL);

}

int
main (int argc, char **argv)
{
    LmSocketAddress *sa;
    LmChannel       *socket;

    g_type_init ();

    if (argc < 3) {
        g_print ("Give a host and port\n");
        return 1;
    }

    host = g_strdup (argv[1]);
    port = atoi (argv[2]);

    g_print ("Connecting to %s:%d ...\n", host, port);

    sa = lm_socket_address_new (host, port);

    socket = lm_socket_new (NULL, sa);
    g_signal_connect (socket, "connect-result", 
                      G_CALLBACK (socket_connected),
                      NULL);
    lm_socket_connect (LM_SOCKET (socket));

  
    loop = g_main_loop_new (NULL, FALSE);

    g_main_loop_run (loop);

    g_object_unref (socket);

    lm_socket_address_unref (sa);

    return 0;
}

