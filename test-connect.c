/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "lm-channel.h"
#include "lm-resolver.h"
#include "lm-socket.h"

static gchar     *host;
static int        port;
static GMainLoop *loop;

#define MESSAGE "Hey there! Whazzup?"

static void
write_some (LmSocket *socket)
{
        static int foo = 0;
        gchar *msg;

        msg = g_strdup_printf ("Yay %d!", ++foo);

        lm_channel_write (LM_CHANNEL (socket), 
                          msg, (gssize)strlen (msg), NULL, NULL);

        g_print ("Returning\n");

        lm_channel_close (LM_CHANNEL (socket));
}

static void
socket_readable (LmSocket *socket)
{
        char  buf[1024];
        gsize read_len;

        g_print ("Attempting to read\n");

        lm_channel_read (LM_CHANNEL (socket),
                         buf, 1023, &read_len, NULL);
        buf[read_len] = '\0';

        g_print ("Read: %s\n", buf);
        write_some (socket);
}

static void
socket_connected (LmSocket *socket, int res, gpointer user_data)
{
        g_print ("Connect callback: %d\n", res);

        lm_channel_write (LM_CHANNEL (socket), 
                          MESSAGE, (gssize)strlen (MESSAGE), NULL, NULL);

        g_print ("Returning\n");
}

static void
socket_closed (LmSocket *socket, LmChannelCloseReason reason)
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
        g_signal_connect (socket, "readable",
                          G_CALLBACK (socket_readable),
                          NULL);
        g_signal_connect (socket, "closed",
                          G_CALLBACK (socket_closed),
                          NULL);

        lm_socket_connect (LM_SOCKET (socket));

        loop = g_main_loop_new (NULL, FALSE);

        g_main_loop_run (loop);

        g_object_unref (socket);

        lm_socket_address_unref (sa);

        return 0;
}




